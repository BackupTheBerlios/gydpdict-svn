/*
 * Copyright (C) 2008 Michał Kurgan <michal.kurgan@moloh.net>
 *
 * This file is part of gydpdict.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gydpdict.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gydp_conf.h"
#include "gydp_util.h"
#include <glib/gstdio.h>

struct _GydpConf {
	GKeyFile *cfg;
};

GydpConf *gydp_conf_new() {
	GydpConf *self = g_slice_alloc( sizeof(GydpConf) );

	/* load configuration */
	self->cfg = g_key_file_new();
	gydp_conf_load(self);

	return self;
}

void gydp_conf_free(GydpConf *self) {
	if( self != NULL ) {
		g_key_file_free(self->cfg);
		g_slice_free(GydpConf, self);
	}
}

gboolean gydp_conf_load(GydpConf *self) {
	/* extract config */
	GKeyFile *cfg = self->cfg;

	GError *error = NULL;
	gchar *filename = NULL;
	gboolean load_default = FALSE;

	/* get configuration file name */
	filename = g_build_filename(g_get_user_config_dir(), GYDP_FILE_RC, NULL);

	/* load configuration */
	if( !g_key_file_load_from_file(cfg, filename, G_KEY_FILE_NONE, &error) ) {
		g_print("Error loading configuration: %s\n", error->message);
		g_print("Loading default configuration...\n");
		g_error_free(error);
	}

	/* load default configuration if needed */
	gchar *path = g_build_filename(g_get_user_data_dir(), GYDP_FILE_DIR, NULL);

	/* default engine */
	if( !g_key_file_has_key(cfg, "general", "engine", NULL) ) {
		g_key_file_set_string(cfg, "general", "engine", gydp_engine_value_to_name(GYDP_ENGINE_SAP));
		load_default = TRUE;
	}

	{ /* engine SAP */
		const gchar *sap = gydp_engine_value_to_nick(GYDP_ENGINE_SAP);
		if( !g_key_file_has_key(cfg, sap, "path", NULL) ) {
			g_key_file_set_string(cfg, sap, "path", path);
			load_default = TRUE;
		}
		if( !g_key_file_has_key(cfg, sap, "lang", NULL) ) {
			g_key_file_set_string(cfg, sap, "lang", gydp_lang_value_to_name(GYDP_LANG_ENG_FROM_POL));
			load_default = TRUE;
		}
	}

	{ /* engine YDP */
		const gchar *ydp = gydp_engine_value_to_nick(GYDP_ENGINE_YDP);
		if( !g_key_file_has_key(cfg, ydp, "path", NULL) ) {
			g_key_file_set_string(cfg, ydp, "path", path);
			load_default = TRUE;
		}
		if( !g_key_file_has_key(cfg, ydp, "lang", NULL) ) {
			g_key_file_set_string(cfg, ydp, "lang", gydp_lang_value_to_name(GYDP_LANG_ENG_FROM_POL));
			load_default = TRUE;
		}
	}

	/* window geometry */
	if( !g_key_file_has_key(cfg, "window", "geometry", NULL) ) {
		g_key_file_set_string(cfg, "window", "geometry", "220x150");
		load_default = TRUE;
	}

	/* window split */
	if( !g_key_file_has_key(cfg, "window", "split", NULL) ) {
		g_key_file_set_string(cfg, "window", "split", "100");
		load_default = TRUE;
	}

	/* free temporary data */
	g_free(path);

	if( load_default )
		g_print("Missing configuration options, using defaults...\n");

	g_free(filename);
	return TRUE;
}

gboolean gydp_conf_save(GydpConf *self) {
	/* extract config */
	GKeyFile *cfg = self->cfg;

	GError *error = NULL;
	gchar *filename = NULL;
	gchar *contents = NULL;
	gsize size;

	/* save configuration */
	if( !(contents = g_key_file_to_data(cfg, &size, &error)) ) {
		g_print("Error saving configuration: %s\n", error->message);
		g_error_free(error);
		return FALSE;
	}

	/* get configuration file name */
	filename = gydp_config_file();

	/* save configuration to file */
	FILE *file = g_fopen(filename, "w");
	fwrite(contents, sizeof(char), size, file);
	fclose(file);

	g_free(filename);
	g_free(contents);
	return TRUE;
}

char *gydp_conf_get_string(GydpConf *self, const char *group, const char *key) {
	GError *error = NULL;
	gchar *value;

	value = g_key_file_get_string(self->cfg, group, key, &error);
	if( error != NULL ) {
		g_print("Error loading key '%s' in group '%s': %s.\n", key, group, error->message);
		g_error_free(error);
		return NULL;
	}

	return value;
}

void gydp_conf_set_string(GydpConf *self, const char *group, const char *key, const char *value) {
	g_key_file_set_string(self->cfg, group, key, value);
}

gint gydp_conf_get_integer(GydpConf *self, const char *group, const char *key) {
	GError *error = NULL;
	gint value;

	value = g_key_file_get_integer(self->cfg, group, key, &error);
	if( error != NULL ) {
		g_print("Error loading key '%s' in group '%s': %s.\n", key, group, error->message);
		g_error_free(error);
		return 0;
	}

	return value;
}

void gydp_conf_set_integer(GydpConf *self, const char *group, const char *key, int value) {
	g_key_file_set_integer(self->cfg, group, key, value);
}


