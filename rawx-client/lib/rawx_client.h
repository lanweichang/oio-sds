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

#ifndef __RAWX_CLIENT_H__
# define __RAWX_CLIENT_H__

#include <metatypes.h>
#include <glib.h>

#define RAWX_REQ_GET_DIRINFO "/rawx/chunk/get"

#define RAWXATTR_NAME_CHUNK_ID "chunk.id"
#define RAWXATTR_NAME_CHUNK_SIZE "chunk.size"
#define RAWXATTR_NAME_CHUNK_HASH "chunk.hash"
#define RAWXATTR_NAME_CHUNK_POS "chunk.position"
#define RAWXATTR_NAME_CHUNK_METADATA "chunk.metadata"

#define RAWXATTR_NAME_CONTENT_PATH "content.path"
#define RAWXATTR_NAME_CONTENT_SIZE "content.size"
#define RAWXATTR_NAME_CONTENT_NBCHUNK "content.nbchunk"
#define RAWXATTR_NAME_CONTENT_METADATA "content.metadata"
#define RAWXATTR_NAME_CONTENT_METADATA_SYS "content.metadatasys"
#define RAWXATTR_NAME_CONTENT_CONTAINER "content.container"

typedef struct rawx_session_s rawx_session_t;

rawx_session_t *rawx_client_create_session(addr_info_t * ai, GError ** err);

void rawx_client_session_set_timeout(rawx_session_t * session, gint cnx, gint req);

void rawx_client_free_session(rawx_session_t * session);

/* STATISTICS */

#define RAWX_STATKEY_REQ_PREFIX "rawx.req"
#define RAWX_STATKEY_REP_PREFIX "rawx.rep"

#define RAWX_STATKEY_REQ_ALL   RAWX_STATKEY_REQ_PREFIX".all"
#define RAWX_STATKEY_REQ_GET   RAWX_STATKEY_REQ_PREFIX".get"
#define RAWX_STATKEY_REQ_PUT   RAWX_STATKEY_REQ_PREFIX".put"
#define RAWX_STATKEY_REQ_DEL   RAWX_STATKEY_REQ_PREFIX".del"
#define RAWX_STATKEY_REQ_INFO  RAWX_STATKEY_REQ_PREFIX".info"
#define RAWX_STATKEY_REQ_STAT  RAWX_STATKEY_REQ_PREFIX".stat"
#define RAWX_STATKEY_REQ_RAW   RAWX_STATKEY_REQ_PREFIX".raw"
#define RAWX_STATKEY_REQ_OTHER RAWX_STATKEY_REQ_PREFIX".other"

#define RAWX_STATKEY_REP_2XX   RAWX_STATKEY_REP_PREFIX".2xx"
#define RAWX_STATKEY_REP_4XX   RAWX_STATKEY_REP_PREFIX".4xx"
#define RAWX_STATKEY_REP_5XX   RAWX_STATKEY_REP_PREFIX".5xx"
#define RAWX_STATKEY_REP_403   RAWX_STATKEY_REP_PREFIX".403"
#define RAWX_STATKEY_REP_404   RAWX_STATKEY_REP_PREFIX".404"
#define RAWX_STATKEY_REP_OTHER RAWX_STATKEY_REP_PREFIX".other"

#define RAWX_STATKEY_BYTES_READ RAWX_STATKEY_REP_PREFIX".bread"
#define RAWX_STATKEY_BYTES_WRITTEN RAWX_STATKEY_REP_PREFIX".bwritten"

GHashTable *rawx_client_get_statistics(rawx_session_t * session, GError ** err);

/* Directory data replicate */

gboolean rawx_client_get_directory_data(rawx_session_t * session, hash_sha256_t chunk_id,
    struct content_textinfo_s *content, struct chunk_textinfo_s *chunk, GError ** error);

gboolean rawx_client_set_directory_data(rawx_session_t * session, hash_sha256_t chunk_id,
    struct content_textinfo_s *content, struct chunk_textinfo_s *chunk, GError ** error);

#endif /*__RAWX_CLIENT_H__*/
