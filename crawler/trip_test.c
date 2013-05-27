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

/* THIS FILE IS NO MORE MAINTAINED */

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "lib_trip.h"
#include "crawler_constants.h"
#include "crawler_common_tools.h"

static gchar* trip_name = "TRIP_TEST";
static gchar* source_cmd_opt_name = "s";
static gchar* extension_cmd_opt_name = "e";

static gchar* source_directory_path = NULL;
static GDir* source_directory_pointer = NULL;
static gchar** extensions = NULL;

/*
 * This function tests if the extension of a particular file name is contained into an array of extension values (TRUE on NULL parameters)
 **/
static gboolean
extension_test(gchar** array, gchar* file_name) {
        const gchar* entry = NULL;
        gchar* src_extension = NULL;
        int i = 0;

        if (NULL == array || NULL == file_name || 0 == g_strv_length(array))
                return TRUE;

        while ((entry = array[i])) {
                src_extension = g_substr(file_name, strlen(file_name) - strlen(entry), strlen(file_name));

                if (!g_strcmp0(src_extension, entry)) {
                        g_free(src_extension);

                        return TRUE;
                }

                g_free(src_extension);

                i++;
        }

        return FALSE;
}

int
trip_start(int argc, char** argv) {
	gchar* temp = NULL;

	/* Source directory path extraction */
	if (NULL == (source_directory_path = get_argv_value(argc, argv, trip_name, source_cmd_opt_name)))
		return EXIT_FAILURE;

	if (NULL == (source_directory_pointer = g_dir_open(source_directory_path, 0, NULL))) {
		g_free(source_directory_path);

		return EXIT_FAILURE;
	}
	/* ------- */

	/* Allowed extensions extraction */
	if (NULL == (temp = get_argv_value(argc, argv, trip_name, extension_cmd_opt_name)))
		return EXIT_SUCCESS;

	extensions = g_strsplit(temp, opt_value_list_separator, -1);
	/* ------- */

	return EXIT_SUCCESS;
}

GVariant*
trip_next(void) {
        gchar* file_name = NULL;

        if (NULL == source_directory_path || NULL == source_directory_pointer)
                return NULL;

        while ((file_name = (gchar*)g_dir_read_name(source_directory_pointer))) {
                if (TRUE == extension_test(extensions, file_name))
                        return g_variant_new_string(file_name);
        }

        return NULL;
}

void
trip_end(void) {
	if (NULL != source_directory_path)
		g_free(source_directory_path);

	if (NULL != source_directory_pointer)
		g_dir_close(source_directory_pointer);

	if (NULL != extensions)
		g_strfreev(extensions);
}
