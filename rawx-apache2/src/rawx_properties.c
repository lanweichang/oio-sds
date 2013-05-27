/*
 * Copyright (C) 2013 AtoS Worldline
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include "../config.h"
#endif
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include <apr_strings.h>
#include <apr_file_io.h>
#include <apr_dbm.h>
#define APR_WANT_BYTEFUNC
#include <apr_want.h>       /* for ntohs and htons */

#include <mod_dav.h>

#ifdef HAVE_COMPAT
# include <metautils_compat.h>
#endif

#include <rawx.h>

#include "./mod_dav_rawx.h"

struct dav_db {
	apr_pool_t *pool;
	const dav_resource *resource;
	int ro;
};

struct dav_deadprop_rollback {
	const char *name;
	const char *value;
};

struct dav_namespace_map {
	int *ns_map;
};

#if 0
static void dav_append_prop(apr_pool_t *pool,
		const char *name, const char *value,
		apr_text_header *phdr)
{
	const char *s;
	const char *lang = value;

	/* skip past the xml:lang value */
	value += strlen(lang) + 1;

	if (*value == '\0') {
		/* the property is an empty value */
		if (*name == ':') {
			/* "no namespace" case */
			s = apr_psprintf(pool, "<%s/>" DEBUG_CR, name+1);
		}
		else {
			s = apr_psprintf(pool, "<ns%s/>" DEBUG_CR, name);
		}
	}
	else if (*lang != '\0') {
		if (*name == ':') {
			/* "no namespace" case */
			s = apr_psprintf(pool, "<%s xml:lang=\"%s\">%s</%s>" DEBUG_CR,
					name+1, lang, value, name+1);
		}
		else {
			s = apr_psprintf(pool, "<ns%s xml:lang=\"%s\">%s</ns%s>" DEBUG_CR,
					name, lang, value, name);
		}
	}
	else if (*name == ':') {
		/* "no namespace" case */
		s = apr_psprintf(pool, "<%s>%s</%s>" DEBUG_CR, name+1, value, name+1);
	}
	else {
		s = apr_psprintf(pool, "<ns%s>%s</ns%s>" DEBUG_CR, name, value, name);
	}

	apr_text_append(pool, phdr, s);
}
#endif

static dav_error *
dav_propdb_open(apr_pool_t *pool, const dav_resource *resource, int ro, dav_db **pdb)
{
	dav_db *db;

	db = apr_pcalloc(pool, sizeof(*db));
	db->pool = pool;
	db->ro = ro;
	db->resource = resource;

	*pdb = db;
	return NULL;
}

static void
dav_propdb_close(dav_db *db)
{
	(void) db;
	/* nothin to do */
}

static dav_error *
dav_propdb_define_namespaces(dav_db *db, dav_xmlns_info *xi)
{
	(void) xi;
	/* XXX JFS : TODO */
	return dav_new_error(db->pool, HTTP_INTERNAL_SERVER_ERROR, 0, "not yet implemented");
}

static dav_error *
dav_propdb_output_value(dav_db *db, const dav_prop_name *name, dav_xmlns_info *xi, apr_text_header *phdr, int *found)
{
	(void) db;
	(void) name;
	(void) xi;
	(void) phdr;
	/* XXX JFS : TODO */
	*found = 0;
	return dav_new_error(db->pool, HTTP_INTERNAL_SERVER_ERROR, 0, "not yet implemented");
}

static dav_error *
dav_propdb_map_namespaces( dav_db *db, const apr_array_header_t *namespaces, dav_namespace_map **mapping)
{
	(void) db;
	(void) namespaces;
	(void) mapping;
	/* XXX JFS : TODO */
	return dav_new_error(db->pool, HTTP_INTERNAL_SERVER_ERROR, 0, "not yet implemented");
}

static dav_error *
dav_propdb_store(dav_db *db, const dav_prop_name *name, const apr_xml_elem *elem, dav_namespace_map *mapping)
{
	(void) db;
	(void) name;
	(void) elem;
	(void) mapping;
	/* XXX JFS : TODO */
	return dav_new_error(db->pool, HTTP_INTERNAL_SERVER_ERROR, 0, "not yet implemented");
}

static dav_error *
dav_propdb_remove(dav_db *db, const dav_prop_name *name)
{
	(void) db;
	(void) name;
	/* XXX JFS : TODO */
	return dav_new_error(db->pool, HTTP_INTERNAL_SERVER_ERROR, 0, "not yet implemented");
}

static int
dav_propdb_exists(dav_db *db, const dav_prop_name *name)
{
	(void) db;
	(void) name;
	/* XXX JFS : TODO */
	return 0;
}

#if 0
static const char *
dav_get_ns_table_uri(dav_db *db, int ns_id)
{
	(void) db;
	(void) ns_id;
	/* XXX JFS : TODO */
	return NULL;
}

static void
dav_set_name(dav_db *db, dav_prop_name *pname)
{
	(void) pname;
	/* XXX JFS : TODO */
}
#endif

static dav_error *
dav_propdb_next_name(dav_db *db, dav_prop_name *pname)
{
	(void) pname;
	/* XXX JFS : TODO */
	return dav_new_error(db->pool, HTTP_INTERNAL_SERVER_ERROR, 0, "not yet implemented");
}

static dav_error *
dav_propdb_first_name(dav_db *db, dav_prop_name *pname)
{
	(void) pname;
	/* XXX JFS : TODO */
	return dav_new_error(db->pool, HTTP_INTERNAL_SERVER_ERROR, 0, "not yet implemented");
}

static dav_error *
dav_propdb_get_rollback(dav_db *db, const dav_prop_name *name, dav_deadprop_rollback **prollback)
{
	(void) name;
	(void) prollback;
	/* XXX JFS : TODO */
	return dav_new_error(db->pool, HTTP_INTERNAL_SERVER_ERROR, 0, "not yet implemented");
}

static dav_error *
dav_propdb_apply_rollback(dav_db *db, dav_deadprop_rollback *rollback)
{
	(void) rollback;
	/* XXX JFS : TODO */
	return dav_new_error(db->pool, HTTP_INTERNAL_SERVER_ERROR, 0, "not yet implemented");
}

const dav_hooks_db dav_hooks_db_dbm =
{
	dav_propdb_open,
	dav_propdb_close,
	dav_propdb_define_namespaces,
	dav_propdb_output_value,
	dav_propdb_map_namespaces,
	dav_propdb_store,
	dav_propdb_remove,
	dav_propdb_exists,
	dav_propdb_first_name,
	dav_propdb_next_name,
	dav_propdb_get_rollback,
	dav_propdb_apply_rollback,

	NULL /* ctx */
};
