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

#ifndef LOG_DOMAIN
#define LOG_DOMAIN "integrity.lib.volume_scanner"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <attr/xattr.h>
#include <sys/stat.h>
#include <unistd.h>

#include <metautils.h>
#include <rawx.h>

#include <motor.h>
#include "volume_scanner.h"
#include "check.h"

/**
 * Free a namelist created by scandir call
 * 
 * @param namelist the struct dirent array returned by scandir
 * @param nb_names the number of entry in array
 */
static void
_free_namelist(struct dirent **namelist, int nb_names)
{
	int i;

	if (namelist == NULL)
		return;

	if (nb_names > 0) {

		for (i = 0; i < nb_names; i++) {
			free(namelist[i]);
		}
	}

	free(namelist);
}

static const gchar*
scanner_traversal_code_to_string(enum scanner_traversal_e rc)
{
	switch (rc) {
		case SCAN_ABORT: return "SCAN_ABORT";
		case SCAN_STOP_BRANCH: return "SCAN_STOP_BRANCH";
		case SCAN_CONTINUE: return "SCAN_CONTINUE";
		case SCAN_STOP_ALL: return "SCAN_STOP_ALL";
	}
	return "*** unexpected ***";
}

static int
is_not_special_dir(const struct dirent *file)
{
	register const char *s = file->d_name;
	return !(s[0]=='.' && (s[1]=='\0' || (s[1]=='.' && s[2]=='\0')));
}

/*
 * Do the job of scan_volume() with the ability to be called recursively
 *
 * @param str_dir
 * @param matching_glob
 * @param callback
 * @param callback_data
 * @param sleep_time
 * @param error
 *
 * @return TRUE or FALSE if an error occured (error is set)
 */
static enum scanner_traversal_e
_recursive_scan_dir(const gchar * str_dir, guint depth, struct volume_scanning_info_s *scanning_info, struct rules_motor_env_s** motor)
{
	enum scanner_traversal_e rc = SCAN_CONTINUE;
	struct stat dir_stat;
	struct dirent **namelist;
	int i, nb_names;

	enum scanner_traversal_e notify_error(void) {
		enum scanner_traversal_e local_rc;
		GError *err;

		if (!scanning_info->error)
			return SCAN_CONTINUE;

		err = NULL;
		GSETCODE(&err, errno, "Failed to scan dir [%s] : %s", str_dir, strerror(errno));
		local_rc = scanning_info->error(str_dir, err, scanning_info->callback_data);
		g_error_free(err);

		if (local_rc == SCAN_ABORT)
			return SCAN_ABORT;
		return SCAN_STOP_BRANCH;
	}

	/*
	   Define select func here because we need the matching_glob inside
	   and have no way to pass it through the scandir
	 */
	int select_file(const struct dirent *file)
	{
		int rc_stat;
		struct stat path_stat;

		if (!is_not_special_dir(file))
			return 0;

		do {
			gchar *fullpath = g_strconcat(str_dir, G_DIR_SEPARATOR_S, file->d_name, NULL);
			rc_stat = stat(fullpath, &path_stat);
			g_free(fullpath);
		} while (0);

		if (-1 == rc_stat)
			return 0;

		if (S_ISDIR(path_stat.st_mode))
			return 1;

		return scanning_info->file_match(str_dir, file->d_name,
				scanning_info->callback_data);
	}

	TRACE("_recursive_scan_dir launched with dir [%s]", str_dir);

	memset(&dir_stat, 0, sizeof(struct stat));

	/* Stat str_dir arg to check if it's really a dir */
	if (-1 == stat(str_dir, &dir_stat))
		return notify_error();
	if (!S_ISDIR(dir_stat.st_mode)) {
		errno = ENOTDIR;
		return notify_error();
	}

	/* Applicative test on the path */
	switch (scanning_info->dir_enter(str_dir, depth, scanning_info->callback_data)) {
		case SCAN_ABORT:
			return SCAN_ABORT;
		case SCAN_STOP_BRANCH:
			return SCAN_CONTINUE;
		case SCAN_CONTINUE:
			break;
		case SCAN_STOP_ALL:
			return SCAN_STOP_ALL;
	}

	/* Scan directory to extract all files matching glob */
	nb_names = scandir(str_dir, &namelist,
		scanning_info->file_match ? select_file : is_not_special_dir,
		alphasort);
	if (nb_names < 0)
		return notify_error();

	/* Run the directories' entries */
	for (i = 0; i < nb_names; i++) {
		struct stat path_stat;
		enum scanner_traversal_e local_rc;
		gchar *path;
		int rc_stat;

		path = g_strconcat(str_dir, G_DIR_SEPARATOR_S, namelist[i]->d_name, NULL);

		rc_stat = stat(path, &path_stat);
		if (-1 == rc_stat) {
			g_free(path);
			continue;
		}

		if (S_ISDIR(path_stat.st_mode)) {
			local_rc = _recursive_scan_dir(path, depth+1, scanning_info, motor);
		}
		else {
			DEBUG("Lauching callback on file [%s]", path);
			local_rc = scanning_info->file_action(path, scanning_info->callback_data, motor);
		}
		g_free(path);

		if (local_rc == SCAN_ABORT) {
			rc = SCAN_ABORT;
			break;
		}
		if (local_rc == SCAN_STOP_BRANCH) {
			rc = SCAN_CONTINUE;
			break;
		}
		if (local_rc == SCAN_STOP_ALL) {
			rc = SCAN_STOP_ALL;
			break;
		}
	}

	_free_namelist(namelist, nb_names);

	/* Applicative */
	if (scanning_info->dir_exit) {
		if (SCAN_ABORT == scanning_info->dir_exit(str_dir, depth, scanning_info->callback_data))
			rc = SCAN_ABORT;
	}

	return rc;
}

void
scan_volume(struct volume_scanning_info_s* scanning_info, struct rules_motor_env_s** motor)
{
	enum scanner_traversal_e rc;

	if (scanning_info == NULL) {
		ERROR("Argument data is NULL");
		return;
	}
	if (!scanning_info->volume_path || !scanning_info->volume_path[0]) {
		ERROR("Volume to be scanned is NULL or empty");
		return;
	}
	if (!scanning_info->file_action) {
		ERROR("No action callback specified for file matching the filter");
		return;
	}

	DEBUG("Starting scan of volume [%s]", scanning_info->volume_path);

	rc = _recursive_scan_dir(scanning_info->volume_path, 0, scanning_info, motor);

	DEBUG("Scan of volume [%s] ended with code %s", scanning_info->volume_path,
		scanner_traversal_code_to_string(rc));
}


/* stamp a chunk */
void
stamp_a_chunk(const char *chunk_path){
	char attr_name_buf[ATTR_NAME_MAX_LENGTH];
	int real_time;
	char real_time_buf[REAL_TIME_BUF_SIZE];
	/* Concatenate the ATTR_DOMAIN and ATTR_NAME_CHUNK_LAST_SCANNED_TIME */
	memset(attr_name_buf, '\0', sizeof(attr_name_buf));
	snprintf(attr_name_buf, sizeof(attr_name_buf), "%s.%s", ATTR_DOMAIN, ATTR_NAME_CHUNK_LAST_SCANNED_TIME);
	/* get time and stamp it g_get_real_time is available since 2.28 */
	real_time = time((time_t *)NULL);
	snprintf(real_time_buf, REAL_TIME_BUF_SIZE, "%d", real_time);
	setxattr(chunk_path, attr_name_buf, real_time_buf, strlen(real_time_buf), 0);
}
