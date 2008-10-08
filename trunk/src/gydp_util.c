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

#include "gydp_util.h"
#include "gydp_conf.h"
#include "gydp_app.h"
#include "gydp_dict_ydp.h"
#include "gydp_dict_sap.h"

static gchar gydp_license[] =
 " This program is free software: you can redistribute it and/or modify\n"
 " it under the terms of the GNU General Public License as published by\n"
 " the Free Software Foundation, either version 3 of the License, or\n"
 " (at your option) any later version.\n"
 "\n"
 " This program is distributed in the hope that it will be useful,\n"
 " but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
 " MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
 " GNU General Public License for more details.\n"
 "\n"
 " You should have received a copy of the GNU General Public License\n"
 " along with gydpdict.  If not, see <http://www.gnu.org/licenses/>.";

GObject *gydp_engine_new(GydpEngine engine) {
	/* extract default engine from configuration */
	if( engine == GYDP_ENGINE_DEFAULT ) {
		/* extract configuration from global app singleton */
		
		GydpConf *config = g_object_get_data(G_OBJECT(gydp_app()), GYDP_APP_CONF);

		/* get default engine */
		gchar *engine_default = gydp_conf_get_string(config, "general", "engine");
		engine = gydp_engine_name_to_value(engine_default);
		g_free(engine_default);
	}

	/* create dictionary */
	switch( engine ) {
	case GYDP_ENGINE_YDP: return gydp_dict_ydp_new();
	case GYDP_ENGINE_SAP: return gydp_dict_sap_new();
	default:              g_return_val_if_reached(NULL);
	}
}

const gchar *gydp_license_text() {
	return gydp_license;
}

GInputStream *gydp_file_open(const gchar *dirname, const gchar *filename) {
	GFileInputStream *stream;
	GFile *file;
	gchar *path;

	/* create file path and open file object */
	path = g_build_filename(dirname, filename, NULL);
	file = g_file_new_for_path(path);

	/* try to open stream */
	stream = g_file_read(file, NULL, NULL);

	/* free temporary objects */
	g_object_unref(file);
	g_free(path);

	return G_INPUT_STREAM(stream);
}

gchar *gydp_config_file() {
	/* get configuration file name */
	const gchar *local = g_get_user_config_dir();
	return g_build_filename(local, GYDP_FILE_RC, NULL);
}

gchar **gydp_data_dirs(GydpEngine engine) {

	/* extract global path */
	const gchar *const *global = g_get_system_data_dirs();
	const guint size = g_strv_length((gchar **)global);

	/* extract local path */
	GydpConf *config = g_object_get_data(G_OBJECT(gydp_app()), GYDP_APP_CONF);
	const gchar *nick = gydp_engine_value_to_nick(engine);
	gchar *local = gydp_conf_get_string(config, nick, "path");

	/* allocate space for data dirs */
	gchar **dirs = g_malloc0((size + 2)*sizeof(gchar *));

	/* local dirs have preference */
	dirs[0] = local;

	/* copy global dirs */
	for(guint i = 0; i < size; ++i)
		dirs[i+1] = g_build_filename(global[i], GYDP_FILE_DIR, NULL);

	return dirs;
}

