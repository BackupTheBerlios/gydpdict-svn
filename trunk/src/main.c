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
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gydp_global.h"
#include "gydp_window.h"
#include "gydp_util.h"
#include "gydp_conf.h"
#include "gydp_dict.h"
#include "gydp_app.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
	GObject *app = gydp_app_new(&argc, &argv);

	/* add configuration to app object */
	g_object_set_data_full(app, GYDP_APP_CONF,
			gydp_conf_new(),
			(GDestroyNotify)gydp_conf_free);

	/* add dictionary to app object */
	g_object_set_data_full(app, GYDP_APP_DICT,
			gydp_engine_new(GYDP_ENGINE_DEFAULT),
			(GDestroyNotify)g_object_unref);

	/* create main widget */
	gydp_app_set_widget(app, gydp_window_new());
	gtk_widget_show_all(gydp_app_get_widget(app));

	/* run application */
	gydp_app_run(app);

	/* experimentally save configuration at exit */
	gydp_conf_save( g_object_get_data(app, GYDP_APP_CONF) );

	/* free application object */
	g_object_unref(app);

	return EXIT_SUCCESS;
}

