/*
 * keyfile.c
 *
 *  Created on: 2014年9月3日
 *      Author: work
 */

#include <string.h>
#include "key_file.h"
#include <stdio.h>
#define DEBUG
static GKeyFile* keyfile = NULL;

gint key_file_open(const gchar *filename)
{
	GError *error = NULL;
	GKeyFileFlags flags;

	flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;

	keyfile = g_key_file_new();
	g_key_file_set_list_separator(keyfile, ',');
	if (!g_key_file_load_from_file(keyfile, filename, flags, &error))
	{
#ifdef DEBUG
		g_error(error->message);
#endif
		g_error_free(error);
		g_key_file_free(keyfile);
		return -1;
	}
	return 0;
}

gint key_file_save(const gchar *filename)
{
	GError* error = NULL;
	gsize len;
	gboolean ret;
	gchar* out = g_key_file_to_data(keyfile, &len, NULL);
	ret = g_file_set_contents(filename, out, len, &error);
	printf("%s\n",filename);

	if (error)
	{
#ifdef DEBUG
		g_error(error->message);
#endif
		g_error_free(error);
	}
	g_free(out);

	if(ret == FALSE)
		return -1;

	return 0;
}

gint key_file_get_int(gchar *group_name, gchar *key, gint default_val)
{
	GError *error = NULL;
	int val = g_key_file_get_integer(keyfile, group_name, key, &error);
	if (error)
	{
#ifdef DEBUG
		g_error(error->message);
#endif
		g_error_free(error);
		return default_val;
	}
	return val;
}

void key_file_set_int(gchar *group_name, gchar *key, gint val)
{
	g_key_file_set_integer(keyfile, group_name, key, val);
}

void key_file_get_string(char *ret, gchar *group_name, gchar *key,
		gchar *default_val)
{
	GError *error = NULL;
	gchar* val = g_key_file_get_string(keyfile, group_name, key, &error);
	if (error)
	{
		strcpy(ret,default_val);
#ifdef DEBUG
		g_error(error->message);
#endif
		g_error_free(error);
		return;
	}
	strcpy(ret,val);
	g_free(val);
}

void key_file_set_string(gchar *group_name, gchar *key, gchar *string)
{
	g_key_file_set_string(keyfile, group_name, key, string);
}
