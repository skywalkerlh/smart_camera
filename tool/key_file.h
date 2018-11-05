/*
 * keyfile.h
 *
 *  Created on: 2014年9月3日
 *      Author: work
 */

#ifndef KEYFILE_H_
#define KEYFILE_H_

#include <glib.h>

gint key_file_open(const gchar *filename);
gint key_file_save(const gchar *filename);

gint key_file_get_int(gchar *group_name, gchar *key, gint default_val);
void key_file_set_int(gchar *group_name, gchar *key, gint val);
void key_file_get_string(char *ret, gchar *group_name, gchar *key,
		gchar *default_val);
void key_file_set_string(gchar *group_name, gchar *key, gchar *string);

#endif /* KEYFILE_H_ */
