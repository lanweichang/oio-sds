#!/usr/bin/env python

# zk-bootstrap.py, a script initating a Zookeeper instance for OpenIO SDS.
# Copyright (C) 2014 Worldine, original work as part of Redcurrant
# Copyright (C) 2015 OpenIO, modified as part of OpenIO Software Defined Storage
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
# 
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import sys, logging, itertools, threading
from time import time as now
import zookeeper
from oio.common.utils import load_namespace_conf

PREFIX='/hc'
PREFIX_NS=PREFIX+'/ns'
hexa = '0123456789ABCDEF'
acl_openbar = [{'perms':zookeeper.PERM_ALL, 'scheme':'world', 'id':'anyone'}]
SRVTYPES = ( ('meta0',0,0), ('meta1',1,3), ('meta2',2,2), ('sqlx',2,2) )

def batch_split (nodes, N):
	"""Creates batches with a common prefixes, and a maximal size of N items"""
	last = 0
	batch = list()
	for x in nodes:
		current = x[0].count('/')
		batch.append(x)
		if len(batch) >= N or last != current:
			yield batch
			batch = list()
		last = current
	yield batch

def batch_create (zh, batch):
	sem = threading.Semaphore(0)
	started = 0
	def create_ignore_errors (zh, path, data):
		def completion (*args, **kwargs):
			rc, zrc, ignored = args
			if rc != 0:
				print "zookeeper.acreate() error"
			else:
				if zrc == 0:
					#print 'create/set('+path+') : OK'
					pass
				elif zrc == zookeeper.NODEEXISTS:
					#print 'create/set('+path+') : ALREADY'
					pass
				else:
					print 'create/set('+path+') : FAILED'
			sem.release()
		zookeeper.acreate(zh, path, data, acl_openbar, 0, completion)
	for path, data in batch:
		create_ignore_errors(zh, path, data)
		started += 1
	for i in range(started):
		sem.acquire()
	return started, 0

def create_tree (zh, nodes, options):
	N, ok, ko = 2048, 0, 0
	if options.SLOW is not None and options.SLOW:
		N = 256
	for batch in batch_split(nodes, N):
		pre = now()
		o, k = batch_create(zh, batch)
		post = now()
		print " > batch({0},{1}) in {2}s".format(o,k,post-pre)
		ok, ko = ok+o, ko+k
	print "Created nodes : ok", ok,"ko",ko

###--------------------------------------------------------------------------###

def hash_tokens (w):
	if w == 1:
		return itertools.product(hexa)
	elif w == 2:
		return itertools.product(hexa,hexa)
	elif w == 3:
		return itertools.product(hexa,hexa,hexa)
	else:
		return []

def hash_tree (d0, w0):
	tokens = [''.join(x) for x in hash_tokens(w0)]
	def depth (d):
		if d == 1:
			return itertools.product(tokens)
		elif d == 2:
			return itertools.product(tokens, tokens)
		elif d == 3:
			return itertools.product(tokens, tokens, tokens)
		else:
			return []
	for d in range(d0+1):
		for x in depth(d):
			yield '/'.join(x) 

def namespace_tree (ns, options):
	yield (PREFIX_NS, '')
	yield (PREFIX_NS+'/'+ns, str(now()))
	yield (PREFIX_NS+'/'+ns+'/srv', '')
	yield (PREFIX_NS+'/'+ns+'/srv/meta0', '')
	yield (PREFIX_NS+'/'+ns+'/el', '')
	for srvtype,d,w in SRVTYPES:
		if options.AVOID_TYPES is not None and srvtype in options.AVOID_TYPES:
			continue
		basedir = PREFIX_NS+'/'+ns+'/el/'+srvtype
		yield (basedir, '')
		for x in hash_tree(d,w):
			yield (basedir+'/'+x, '')

#-------------------------------------------------------------------------------

def main():
	from optparse import OptionParser as OptionParser

	parser = OptionParser()
	parser.add_option('-v', '--verbose', action="store_true", dest="flag_verbose",
		help='Triggers debugging traces')
	parser.add_option('--slow', action="store_true", dest="SLOW",
		help='Only play with small batches to avoid timeouts on slow hosts.')
	parser.add_option('--avoid', action="append", type="string", dest="AVOID_TYPES",
		help='Do not populate entries for the specified service types')

	(options, args) = parser.parse_args(sys.argv)

	# Logging configuration
	if options.flag_verbose:
		logging.basicConfig(
			format='%(asctime)s %(message)s',
			datefmt='%m/%d/%Y %I:%M:%S',
			level=logging.DEBUG)
	else:
		logging.basicConfig(
			format='%(asctime)s %(message)s',
			datefmt='%m/%d/%Y %I:%M:%S',
			level=logging.INFO)

	if len(args) < 2:
		raise ValueError("not enough CLI arguments")

	ns = args[1]
	cnxstr = load_namespace_conf(ns)['zookeeper']
	zookeeper.set_debug_level(zookeeper.LOG_LEVEL_INFO)
	zh = zookeeper.init(cnxstr)

	# synchronous creation of the root
	try:
		zookeeper.create(zh, PREFIX, '', acl_openbar, 0)
	except zookeeper.NodeExistsException:
		pass
	create_tree(zh, namespace_tree(ns, options), options)
	zookeeper.close(zh)

if __name__ == '__main__':
	main()

