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
#ifndef G_LOG_DOMAIN
# define G_LOG_DOMAIN "metacomm.message"
#endif
#ifndef LOG_DOMAIN
# define LOG_DOMAIN G_LOG_DOMAIN
#endif

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#include "./metautils.h"
#include "./metacomm.h"
#include "./Parameter.h"
#include "./Message.h"

static GQuark gquark_log = 0;

struct message_s
{
	Message_t *asnMsg;
};

static void
__octetString_array_free(Parameter_t * p)
{
	if (p) {
		OCTET_STRING_free(&asn_DEF_OCTET_STRING, &(p->name), 1);
		p->name.buf = NULL;
		p->name.size = 0;

		OCTET_STRING_free(&asn_DEF_OCTET_STRING, &(p->value), 1);
		p->value.buf = NULL;
		p->value.size = 0;

		g_free(p);
	}
}

static gint
_clean_asn_message(MESSAGE m, GError ** err)
{
	if (!m) {
		GSETERROR(err, "Invalid parameter");
		return 0;
	}

	if (!(m->asnMsg)) {
		return 1;
	}

	/*quick parameters */
	if (m->asnMsg->id) {
		ASN_STRUCT_FREE(asn_DEF_OCTET_STRING, m->asnMsg->id);
		m->asnMsg->id = NULL;
	}

	if (m->asnMsg->body) {
		ASN_STRUCT_FREE(asn_DEF_OCTET_STRING, m->asnMsg->body);
		m->asnMsg->body = NULL;
	}

	if (m->asnMsg->version) {
		ASN_STRUCT_FREE(asn_DEF_OCTET_STRING, m->asnMsg->version);
		m->asnMsg->version = NULL;
	}

	if (m->asnMsg->name) {
		ASN_STRUCT_FREE(asn_DEF_OCTET_STRING, m->asnMsg->name);
		m->asnMsg->name = NULL;
	}

#if 0
	/*fields list */
	if (m->asnMsg->content.list.array) {
		int i;

		for (i = 0; i < m->asnMsg->content.list.count; i++) {
			Parameter_t *p = m->asnMsg->content.list.array[i];

			if (p) {
				OCTET_STRING_free(&asn_DEF_OCTET_STRING, &(p->name), 1);
				p->name.buf = NULL;
				p->name.size = 0;

				OCTET_STRING_free(&asn_DEF_OCTET_STRING, &(p->value), 1);
				p->value.buf = NULL;
				p->value.size = 0;

				g_free(p);
			}
		}
		g_free(m->asnMsg->content.list.array);
		m->asnMsg->content.list.array = NULL;
		m->asnMsg->content.list.count = 0;
		m->asnMsg->content.list.size = 0;
	}
#endif

	m->asnMsg->content.list.free = __octetString_array_free;
	asn_set_empty(&(m->asnMsg->content.list));

	memset(m->asnMsg, 0x00, sizeof(Message_t));
	g_free(m->asnMsg);

	m->asnMsg = NULL;

	return 1;
}

static gint
_alloc_asn_message(MESSAGE m, GError ** err)
{
	if (!m) {
		GSETERROR(err, "Invalid parameter");
		return 0;
	}

	m->asnMsg = g_try_new0(Message_t, 1);
	if (!m->asnMsg) {
		GSETERROR(err, "Memory allocation error");
		return 0;
	}

	return 1;
}

static gint
__getParameter(MESSAGE m, enum message_param_e mp, OCTET_STRING_t ** os)
{
	if (!m || !(m->asnMsg) || !os)
		return 0;

	*os = NULL;

	switch (mp) {
	case MP_ID:
		*os = m->asnMsg->id;
		return 1;
	case MP_NAME:
		*os = m->asnMsg->name;
		return 1;
	case MP_VERSION:
		*os = m->asnMsg->version;
		return 1;
	case MP_BODY:
		*os = m->asnMsg->body;
		return 1;
	}

	return 0;
}



gint
message_create(MESSAGE * m, GError ** error)
{
	if (!m) {
		GSETERROR(error, "Invalid parameter");
		return 0;
	}

	/*allocates the capsule */
	*m = g_try_new0(struct message_s, 1);
	if (!(*m)) {
		GSETERROR(error, "Memory allocation error");
		return 0;
	}

	return 1;
}

static void
trace_message(MESSAGE m, GByteArray *encoded)
{
	void *ptr = NULL;
	size_t ptrSize = 0;
	gchar idStr[512], nameStr[512];

	if (!TRACE_ENABLED())
		return;

	memset(nameStr, 0, sizeof(nameStr));
	memset(idStr, 0, sizeof(idStr));

	if (0 < message_get_ID(m, &ptr, &ptrSize, NULL)) {
		if (ptr && ptrSize < 256)
			buffer2str(ptr, ptrSize, idStr, sizeof(idStr)-1);
	}

	if (0 < message_get_NAME(m, &ptr, &ptrSize, NULL)) {
		g_strlcpy(nameStr, ptr, sizeof(nameStr)-1);
	}

	TRACE("Message NAME=%.*s ID=%s encoded=%u",
			(int)sizeof(nameStr), nameStr, idStr, encoded->len);
}

gint
message_destroy(MESSAGE m, GError ** error)
{
	if (m) {
		_clean_asn_message(m, error);
		g_free(m);
	}

	return 1;
}

#include <sys/types.h>
#include <unistd.h>

static void
message_add_random_id(MESSAGE m, gsize s)
{
	unsigned int i;
	pid_t pid;
	guint8 idBuf[256 + sizeof(int)];

	if (!m || !m->asnMsg)
		return;
	if (s >= sizeof(idBuf) - sizeof(int))
		s = sizeof(idBuf) - sizeof(int);
	if (s < sizeof(int) * 2)
		s = sizeof(int) * 2;

	memset(idBuf, '\0', sizeof(idBuf));
	pid = getpid();
	memcpy(idBuf, (guint8 *) (&pid), sizeof(pid));
	for (i = sizeof(int); i < s; i += sizeof(int)) {
		int r = random();
		memcpy(idBuf + i, (guint8 *) (&r), sizeof(int));
	}

	if (!message_set_ID(m, idBuf, s, NULL))
		WARN("cannot set the random id");
}

GByteArray*
message_marshall_gba(MESSAGE m, GError **err)
{
	static guint32 u32 = 0;

	int write_f(const void *b, gsize bSize, void *key) {
		if (b && bSize > 0)
			g_byte_array_append((GByteArray*)key, b, bSize);
		return 0;
	}

	GByteArray *result = NULL;
	asn_enc_rval_t encRet;

	/*sanity check */
	if (!m) {
		GSETERROR(err, "Invalid parameter");
		return NULL;
	}

	if (!m->asnMsg)
		_alloc_asn_message(m, err);

	/*set an ID if it is not present */
	if (0 == message_has_ID(m, NULL))
		message_add_random_id(m, 16);

	/*try to encode */
	result = g_byte_array_sized_new(256);
	g_byte_array_append(result, (guint8*)&u32, sizeof(u32));
	encRet = der_encode(&asn_DEF_Message, m->asnMsg, write_f, result);

	if (encRet.encoded < 0) {
		g_byte_array_free(result, TRUE);
		GSETERROR(err, "Encoding error (Message)");
		return NULL;
	}

	/*finalize the l4v encapsulation */
	l4v_prepend_size(result->data, result->len, NULL);
	trace_message(m, result);
	return result;
}

GByteArray*
message_marshall_gba_and_clean(MESSAGE m)
{
	GByteArray *result;

	UTILS_ASSERT(m != NULL);
	result = message_marshall_gba(m, NULL);
	message_destroy(m, NULL);
	return result;	
}

gint
message_marshall(MESSAGE m, void **s, gsize * sSize, GError ** error)
{
	GByteArray *encoded;

	if (!s || !sSize) {
		GSETERROR(error, "Invalid parameter");
		return 0;
	}

	if (!(encoded = message_marshall_gba(m, error)))
		return 0;
	*sSize = encoded->len;
	*s = g_byte_array_free(encoded, FALSE);
	return 1;
}

gint
message_unmarshall(MESSAGE m, void *s, gsize * sSize, GError ** error)
{
	asn_dec_rval_t decRet;
	asn_codec_ctx_t codecCtx;

	guint8 *rawS = NULL;
	gsize rawSSize = 0;

	if (!m || !s || !sSize || *sSize < 4) {
		GSETERROR(error, "Invalid parameter (m=%p s=%p sSize=%p/%i)", m, s, sSize, sSize ? *sSize : ~0U);
		return 0;
	}

	_clean_asn_message(m, error);

	/*extract the real buffer from the L4V capsule */
	switch (l4v_is_complete(s, *sSize, error)) {
	case 0:
		GSETERROR(error, "the buffer does not contain a full message");
	case -1:
		/*error is already set */
		return 0;
	}

	if (!l4v_extract(s, *sSize, (void *) &rawS, &rawSSize, error)) {
		GSETERROR(error, "Cannot extract the L4V encapsulated data");
		return 0;
	}

	/*tries the real unmarshalling work */
	codecCtx.max_stack_size = 0;

	do {/*appease gcc with its ugly strict aliasing rules*/
		void *ptr = NULL;
		decRet = ber_decode(&codecCtx, &asn_DEF_Message, &ptr, rawS, rawSSize);
		m->asnMsg = ptr;
	} while (0);

	switch (decRet.code) {
	case RC_OK:
		break;

	case RC_FAIL:
		GSETERROR(error, "Cannot deserialize: %s", "invalid content");
		goto errorLABEL;

	case RC_WMORE:
		GSETERROR(error, "Cannot deserialize: %s", "uncomplete content");
		goto errorLABEL;
	}

	*sSize = decRet.consumed + 4;

	return 1;
      errorLABEL:
	_clean_asn_message(m, error);
	return 0;
}



gint
message_has_param(MESSAGE m, enum message_param_e mp, GError ** error)
{
	OCTET_STRING_t *os;

	if (!m) {
		GSETERROR(error, "Invalid parameter");
		return -1;
	}

	if (!m->asnMsg)
		return 0;

	return (__getParameter(m, mp, &os) && os && os->buf) ? 1 : 0;
}



gint
message_get_param(MESSAGE m, enum message_param_e mp, void **s, gsize * sSize, GError ** error)
{
	const char *name_mp;
	OCTET_STRING_t *os;

	if (!m || !s || !sSize) {
		GSETERROR(error, "Invalid parameter");
		return -1;
	}

	if (!m->asnMsg)
		_alloc_asn_message(m, error);

	switch (mp) {
		case MP_ID : name_mp = "MP_ID"; break;
		case MP_NAME : name_mp = "MP_NAME"; break;
		case MP_VERSION : name_mp = "MP_VERSION"; break;
		case MP_BODY : name_mp = "MP_BODY"; break;
		default : name_mp = "***invalid***"; break;
	}

	switch (message_has_param(m, mp, error)) {
	case -1:
		/*error is set */
		return -1;

	case 0:
		GSETERROR(error, "Type not set (%d/%s)", mp, name_mp);
		return 0;
	}

	if (__getParameter(m, mp, &os) && os) {
		*sSize = os->size;
		*s = os->buf;
		return 1;
	}

	return 0;
}



gint
message_set_param(MESSAGE m, enum message_param_e mp, const void *s, gsize sSize, GError ** error)
{
	OCTET_STRING_t *os;

	if (!m || !s || sSize < 1) {
		GSETERROR(error, "Invalid parameter (%p %p %d)", m, s, sSize);
		return 0;
	}

	if (!m->asnMsg) {
		_alloc_asn_message(m, error);
	}

	if (!__getParameter(m, mp, &os)) {
		GSETERROR(error, "Invalid message parameter type");
		return 0;
	}

	if (os)
		OCTET_STRING_fromBuf(os, s, sSize);
	else {
		switch (mp) {
		case MP_ID:
			m->asnMsg->id = OCTET_STRING_new_fromBuf(&asn_DEF_OCTET_STRING, s, sSize);
			break;
		case MP_NAME:
			m->asnMsg->name = OCTET_STRING_new_fromBuf(&asn_DEF_OCTET_STRING, s, sSize);
			break;
		case MP_VERSION:
			m->asnMsg->version = OCTET_STRING_new_fromBuf(&asn_DEF_OCTET_STRING, s, sSize);
			break;
		case MP_BODY:
			m->asnMsg->body = OCTET_STRING_new_fromBuf(&asn_DEF_OCTET_STRING, s, sSize);
			break;
		}
	}

	if (0 >= message_has_param(m, mp, NULL)) {
		GSETERROR(error, "Message parameter not set");
		return 0;
	}

	return 1;
}



gint
message_add_field(MESSAGE m, const void *name, gsize nameSize, const void *value, gsize valueSize, GError ** error)
{
	void *pList;
	Parameter_t *pMember = NULL;

	/* Some sanity checks */
	if (!m || !name || nameSize < 1 || !value || valueSize < 1) {
		GSETERROR(error, "Invalid parameter (%p %p %p)", m, name, value);
		return 0;
	}

	if (!m->asnMsg) {
		_alloc_asn_message(m, error);
	}

	pList = &(m->asnMsg->content.list);

	/*allocates enough memory for a new field */
	pMember = g_try_new0(Parameter_t, 1);
	if (!pMember) {
		GSETERROR(error, "Memory allocation error");
		goto errorLABEL;
	}

	/*fill the field */
	if (0 != OCTET_STRING_fromBuf(&(pMember->name), name, strlen_len(name, nameSize))) {
		GSETERROR(error, "Cannot copy the parameter name");
		goto errorLABEL;
	}

	if (0 != OCTET_STRING_fromBuf(&(pMember->value), value, valueSize)) {
		GSETERROR(error, "Cannot copy the parameter value");
		goto errorLABEL;
	}

	/*and then add the field into the message's list */
	if (0 != asn_set_add(pList, pMember)) {
		GSETERROR(error, "Cannot add the parameter to the message");
		goto errorLABEL;
	}

	return 1;
      errorLABEL:
	if (pMember) {
		ASN_STRUCT_FREE(asn_DEF_OCTET_STRING, &(pMember->name));
		ASN_STRUCT_FREE(asn_DEF_OCTET_STRING, &(pMember->value));
		g_free(pMember);
	}
	return 0;
}



gint
message_get_field(MESSAGE m, const void *name, gsize name_size, void **value, gsize * valueSize, GError ** error)
{
	int i, max, name_real_size;

	/*sanity checks */
	if (!m || !name || name_size < 1 || !value || !valueSize) {
		GSETERROR(error, "Invalid parameter");
		return -1;
	}

	if (!m->asnMsg) {
		_alloc_asn_message(m, error);
	}

	if (!m->asnMsg->content.list.array || m->asnMsg->content.list.count<=0) {
		return 0;
	}

	name_real_size = strlen_len(name, name_size);

	/*run the list and find a matching field */
	max = m->asnMsg->content.list.count;
	for (i = 0; i < max ; i++) {
		Parameter_t *p;

		p = m->asnMsg->content.list.array[i];
		if (!p)
			continue;
		if (p->name.size != name_real_size)
			continue;

		if (!memcmp(p->name.buf, name, name_real_size)) {
			*value = p->value.buf;
			*valueSize = p->value.size;
			return 1;
		}
	}

	return 0;
}



gint
message_del_field(MESSAGE m, const void *name, gsize nameSize, GError ** error)
{
	gint32 i, l;

	/*sanity checks */
	if (!m || !name || nameSize < 1) {
		GSETERROR(error, "Invalid parameter");
		return 0;
	}

	if (!m->asnMsg) {
		_alloc_asn_message(m, error);
	}

	if (!m->asnMsg->content.list.array) {
		GSETERROR(error, "Invalid parameter (message not initialized)");
		return -1;
	}

	/*run the list and find the right field */
	for (i = 0; i < m->asnMsg->content.list.count; i++) {
		OCTET_STRING_t *pStr;
		Parameter_t *p = m->asnMsg->content.list.array[i];

		if (p) {
			pStr = &(p->name);
			l = pStr->size;

			if (0 == strncmp((const char*)pStr->buf, name, MIN((gsize) l, nameSize))) {
				asn_set_del(&(m->asnMsg->content.list), i, 1);
				return 1;
			}
		}
	}

	return 0;
}



gint
message_print(MESSAGE m, const gchar * dom, gchar * entete, GError ** err)
{
	gint i;
	guchar *k, *v;
	gint32 kL, vL;

	if (!m || !dom || !entete || !m->asnMsg) {
		GSETERROR(err, "Invalid parameter");
		return 0;
	}

	/*print the quick parameters */
	if (m->asnMsg->version) {
		k = (guint8 *) "VERSION";
		kL = sizeof("VERSION") - 1;
		v = m->asnMsg->version->buf;
		vL = m->asnMsg->version->size;
		TRACE_DOMAIN(dom, "%s%.*s (%i) -> %.*s (%i)", entete, kL, k, kL, vL, v, vL);
	}

	if (m->asnMsg->id) {
		k = (guint8 *) "ID";
		kL = sizeof("ID") - 1;
		v = m->asnMsg->id->buf;
		vL = m->asnMsg->id->size;
		TRACE_DOMAIN(dom, "%s%.*s (%i) -> %.*s (%i)", entete, kL, k, kL, vL, v, vL);
	}

	if (m->asnMsg->name) {
		k = (guint8 *) "NAME";
		kL = sizeof("NAME") - 1;
		v = m->asnMsg->name->buf;
		vL = m->asnMsg->name->size;
		TRACE_DOMAIN(dom, "%s%.*s (%i) -> %.*s (%i)", entete, kL, k, kL, vL, v, vL);
	}

	/*print the fields */
	for (i = 0; i < m->asnMsg->content.list.count; i++) {
		Parameter_t *p = m->asnMsg->content.list.array[i];

		if (p) {
			v = p->value.buf;
			vL = p->value.size;

			k = p->name.buf;
			kL = p->name.size;

			TRACE_DOMAIN(dom, "%s\t%.*s (%i) -> %.*s (%i)", entete, kL, k, kL, vL, v, vL);
		}
	}


	if (m->asnMsg->body) {
		k = (guint8 *) "BODY";
		kL = sizeof("BODY") - 1;
		v = m->asnMsg->body->buf;
		vL = m->asnMsg->body->size;
		TRACE_DOMAIN(dom, "%s%.*s (%i) -> %.*s (%i)", entete, kL, k, kL, vL, v, vL);
	}

	return 1;
}


gchar **
message_get_field_names(MESSAGE m, GError ** error)
{
	gchar **array;
	gint max, nb, i;

	if (!m || !m->asnMsg) {
		GSETERROR(error, "invalid message parameter");
		return NULL;
	}

	max = m->asnMsg->content.list.count;
	if (max < 0) {
		GSETERROR(error, "invalid message field count : %d", max);
		return NULL;
	}

	array = g_try_malloc0(sizeof(gchar *) * (max + 1));
	if (!array) {
		GSETERROR(error, "memory allocation failure");
		return NULL;
	}

	TRACE("found %d fields", max);

	for (nb = 0, i = 0; i < max; i++) {
		Parameter_t *p = m->asnMsg->content.list.array[i];

		if (p && p->name.buf) {
			array[nb] = g_strndup((const gchar*)p->name.buf, p->name.size);
			nb++;
		}
	}

	return array;
}

gint
message_get_fields(MESSAGE m, GHashTable ** hash, GError ** error)
{
	gchar **field_names, **fn_ptr;
	GByteArray *field_value = NULL;

	if (!m || !m->asnMsg) {
		GSETERROR(error, "invalid message parameter");
		return (0);
	}

	field_names = message_get_field_names(m, error);
	if (field_names == NULL) {
		GSETERROR(error, "Failed to get fiels names");
		return (0);
	}

	*hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, metautils_gba_clean);

	for (fn_ptr = field_names; *fn_ptr; fn_ptr++) {
		void *value;
		gsize value_size;

		if (message_get_field(m, *fn_ptr, strlen(*fn_ptr), &value, &value_size, error) > 0) {
			field_value = g_byte_array_new();
			field_value = g_byte_array_append(field_value, value, value_size);
			g_hash_table_insert(*hash, *fn_ptr, field_value);
		}
	}

	g_free(field_names);

	return (1);
}

void
message_add_fieldv_str(MESSAGE m, va_list args)
{
	if (!m)
		return;

	for (;;) {
		char *k, *v;

		k = va_arg(args, char *);
		if (!k)
			break;

		v = va_arg(args, char *);
		if (!v)
			break;
		message_add_field(m, k, strlen(k), v, strlen(v), NULL);
	}
}

void
message_add_fields_str(MESSAGE m, ...)
{
	va_list args;

	if (!m)
		return;

	va_start(args, m);
	message_add_fieldv_str(m, args);
	va_end(args);
}

void
message_add_fieldv_gba(MESSAGE m, va_list args)
{
	if (!m)
		return;

	for (;;) {
		char *k;
		GByteArray *v;
		k = va_arg(args, char *);

		if (!k)
			break;
		v = va_arg(args, GByteArray *);
		if (!v)
			break;
		message_add_field(m, k, strlen(k), v->data, v->len, NULL);
	}
}

void
message_add_fields_gba(MESSAGE m, ...)
{
	va_list args;

	if (!m)
		return;

	va_start(args, m);
	message_add_fieldv_gba(m, args);
	va_end(args);
}

MESSAGE
message_create_request(GError ** err, GByteArray * id, const char *name,
		GByteArray * body, ...)
{
	va_list args;
	MESSAGE msg = NULL;

	message_create(&msg, err);

	if (name)
		message_set_NAME(msg, name, strlen(name), err);

	if (id)
		message_set_ID(msg, id->data, id->len, NULL);

	if (body)
		message_set_BODY(msg, body->data, body->len, NULL);

	va_start(args, body);
	message_add_fieldv_gba(msg, args);
	va_end(args);

	return msg;
}

GError *
message_extract_body_strv(struct message_s *msg, gchar ***result)
{
	int rc;
	void *b = NULL;
	gsize bsize = 0;

	if (!gquark_log)
		gquark_log = g_quark_from_static_string(G_LOG_DOMAIN);

	if (0 >= (rc = message_get_BODY(msg, &b, &bsize, NULL))) {
		GRID_TRACE2("Empty/Invalid body");
		*result = g_malloc0(sizeof(gchar*));
		return NULL;
	}

	*result = metautils_decode_lines(b, ((gchar*)b)+bsize);
	return NULL;
}

GError *
message_extract_prefix(struct message_s *msg, const gchar *n,
		guint8 *d, gsize *dsize)
{
	void *f;
	gsize fsize;

	if (!gquark_log)
		gquark_log = g_quark_from_static_string(G_LOG_DOMAIN);

	if (0 >= message_get_field(msg, n, strlen(n), &f, &fsize, NULL))
		return g_error_new(gquark_log, 400, "Missing ID prefix at '%s'", n);
	if (fsize > *dsize)
		return g_error_new(gquark_log, 400, "Invalid ID prefix at '%s'", n);

	memset(d, 0, *dsize);
	memcpy(d, f, fsize);
	*dsize = fsize;
	return NULL;
}

GError *
message_extract_cid(struct message_s *msg, const gchar *n, container_id_t *cid)
{
	void *f;
	gsize f_size;

	if (!gquark_log)
		gquark_log = g_quark_from_static_string(G_LOG_DOMAIN);

	if (0 >= message_get_field(msg, n, strlen(n), &f, &f_size, NULL))
		return g_error_new(gquark_log, 400, "Missing container ID at '%s'", n);
	if (f_size != sizeof(container_id_t))
		return g_error_new(gquark_log, 400, "Invalid container ID at '%s'", n);

	if (GRID_TRACE_ENABLED()) {
		gchar strf[1 + sizeof(container_id_t)*2];

		memset(strf, 0, sizeof(strf));
		buffer2str(f, f_size, strf, sizeof(strf));

		GRID_TRACE("Got header [%s] <- [%s]", n, strf);
	}

	memcpy(cid, f, sizeof(container_id_t));
	return NULL;
}

GError *
message_extract_string(struct message_s *msg, const gchar *n, gchar *dst,
		gsize dst_size)
{
	void *f;
	gsize f_size;

	if (!gquark_log)
		gquark_log = g_quark_from_static_string(G_LOG_DOMAIN);

	if (0 >= message_get_field(msg, n, strlen(n), &f, &f_size, NULL))
		return g_error_new(gquark_log, 400, "Missing field '%s'", n);
	if (f_size >= dst_size)
		return g_error_new(gquark_log, 400, "Invalid field '%s'", n);

	memcpy(dst, f, f_size);
	memset(dst+f_size, 0, dst_size-f_size);
	GRID_TRACE("Got header [%s] <- [%s]", n, dst);
	return NULL;
}

GError *
message_extract_flag(struct message_s *msg, const gchar *n,
		gboolean mandatory, gboolean *flag)
{
	guint8 *b, _flag = 0;
	void *f;
	gsize f_size;

	if (!gquark_log)
		gquark_log = g_quark_from_static_string(G_LOG_DOMAIN);

	if (0 >= message_get_field(msg, n, strlen(n), &f, &f_size, NULL)) {
		if (mandatory)
			return g_error_new(gquark_log, 400, "Missing field '%s'", n);
		return NULL;
	}

	for (b=(guint8*)f + f_size; b > (guint8*)f;)
		_flag |= *(--b);

	GRID_TRACE("Got header [%s] <- [%d]", n, _flag);
	*flag = _flag;
	return NULL;
}

GError*
message_extract_flags32(struct message_s *msg, const gchar *n,
		gboolean mandatory, guint32 *flags)
{
	void *f = NULL;
	gsize f_size = 0;

	UTILS_ASSERT(flags != NULL);
	*flags = 0;

	if (0 >= message_get_field(msg, n, strlen(n), &f, &f_size, NULL)) {
		if (mandatory)
			return NEWERROR(400, "Missing field '%s'", n);
		return NULL;
	}

	if (f_size != 4)
		return NEWERROR(400, "Invalid 32bit flag set");

	*flags = *((guint32*)f);
	GRID_TRACE("Got header [%s] <- [%04X]", n, *flags);
	return NULL;
}

GError *
message_extract_body_gba(struct message_s *msg, GByteArray **result)
{
	int rc;
	void *b = NULL;
	gsize bsize = 0;

	UTILS_ASSERT(result != NULL);

	if (!gquark_log)
		gquark_log = g_quark_from_static_string(G_LOG_DOMAIN);

	if (0 >= (rc = message_get_BODY(msg, &b, &bsize, NULL)))
		return g_error_new(gquark_log, 400, "Missing body");

	*result = g_byte_array_append(g_byte_array_new(), b, bsize);
	return NULL;
}

GError *
message_extract_body_string(struct message_s *msg, gchar **result)
{
	int rc;
	void *b = NULL;
	gsize bsize = 0;

	if (!gquark_log)
		gquark_log = g_quark_from_static_string(G_LOG_DOMAIN);

	if (0 >= (rc = message_get_BODY(msg, &b, &bsize, NULL)))
		return g_error_new(gquark_log, 400, "Missing body");
	else {
		register gchar *c, *last;
		for (c=b,last=b+bsize; c < last ;c++) {
			if (!g_ascii_isprint(*c))
				return g_error_new(gquark_log, 400,
						"Body contains non printable characters");
		}
	}

	*result = g_strndup((gchar*)b, bsize);
	return NULL;
}

GError *
message_extract_body_encoded(struct message_s *msg, GSList **result,
		gint (decoder)(GSList **, const void*, gsize*, GError**))
{
	int rc;
	void *b = NULL;
	gsize bsize = 0;
	GError *err = NULL;

	if (!gquark_log)
		gquark_log = g_quark_from_static_string(G_LOG_DOMAIN);

	UTILS_ASSERT(result != NULL);
	UTILS_ASSERT(decoder != NULL);

	if (0 >= (rc = message_get_BODY(msg, &b, &bsize, NULL)))
		return g_error_new(gquark_log, 400, "Missing body");

	rc = decoder(result, b, &bsize, &err);

	if (rc <= 0) {
		UTILS_ASSERT(err != NULL);
		err->code = 400;
		g_prefix_error(&err, "Invalid body: ");
		return err;
	}

	return NULL;
}

GError *
message_extract_strint64(struct message_s *msg, const gchar *n, gint64 *i64)
{
	gchar *end, dst[32];
	GError *err;

	if (!gquark_log)
		gquark_log = g_quark_from_static_string(G_LOG_DOMAIN);

	err = message_extract_string(msg, n, dst, sizeof(dst));
	if (err != NULL)
		return err;
	end = NULL;
	*i64 = g_ascii_strtoll(dst, &end, 10);

	switch (*i64) {
		case G_MININT64:
		case G_MAXINT64:
			return (errno == ERANGE)
				? g_error_new(gquark_log, 400, "Invalid number") : NULL;
		case 0:
			return (end == dst)
				? g_error_new(gquark_log, 400, "Invalid number") : NULL;
		default:
			return NULL;
	}
}

GError*
message_extract_struint(struct message_s *msg, const gchar *n, guint *u)
{
	gint64 i64;
	GError *err;

	err = message_extract_strint64(msg, n, &i64);
	if (err != NULL)
		return err;
	*u = i64;
	return NULL;
}

GError*
message_extract_header_gba(struct message_s *msg, const gchar *n,
		gboolean mandatory, GByteArray **result)
{
	int rc;
	void *f;
	gsize fsize;

	g_assert(msg != NULL);
	g_assert(n != NULL);
	g_assert(result != NULL);

	rc = message_get_field(msg, n, strlen(n), &f, &fsize, NULL);

	if (rc < 0)
		return g_error_new(GQ(), 400, "Missing ID prefix at '%s'", n);
	if (!rc) {
		if (mandatory)
			return g_error_new(GQ(), 400, "Missing ID prefix at '%s'", n);
		*result = NULL;
		return NULL;
	}

	*result = g_byte_array_append(g_byte_array_new(), f, fsize);
	return NULL;
}

