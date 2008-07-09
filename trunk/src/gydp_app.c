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

#include "gydp_app.h"
#include <gtk/gtk.h>

struct _GydpAppClass {
	GObjectClass __parent__;
};

struct _GydpApp {
	GObject __parent__;
	GtkWidget *widget;   /* main widget */
};

/* parent class holder */
static GObjectClass *gydp_app_parent_class = NULL;

/* global singleton */
static GydpApp *gydp_application = NULL;

/* private functions */
static void     gydp_app_init       (GydpApp *self);
static void     gydp_app_class_init (GydpAppClass *klass);
static GObject *gydp_app_constructor(GType type, guint n, GObjectConstructParam *properties);
static void     gydp_app_finalize   (GObject *object);

GType gydp_app_get_type() {
	static GType type = G_TYPE_INVALID;
	if( G_UNLIKELY( type == G_TYPE_INVALID ) ) {
		static const GTypeInfo info = {
			sizeof(GydpAppClass),                       /* class size */
			NULL, NULL,                                 /* base init, finalize */
			(GClassInitFunc) gydp_app_class_init, NULL, /* class init, finalize */
			NULL,                                       /* class init, finalize user_data */
			sizeof(GydpApp), 0,                         /* base size, prealloc size */
			(GInstanceInitFunc) gydp_app_init,          /* instance init */
			NULL,                                       /* GValue table */
		};

		type = g_type_register_static(G_TYPE_OBJECT, "GydpApp", &info, 0);
	}

	return type;
}

static void gydp_app_init(GydpApp *self) {
	(void)self;
}

static void gydp_app_class_init(GydpAppClass *klass) {
	/* determine parent class */
	gydp_app_parent_class = g_type_class_peek_parent(klass);

	GObjectClass *gobject_klass = G_OBJECT_CLASS(klass);
	gobject_klass->constructor = gydp_app_constructor;
	gobject_klass->finalize = gydp_app_finalize;
}

static GObject *gydp_app_constructor(GType type, guint n, GObjectConstructParam *properties) {
	GObject *object = NULL;

	/* construct singleton */
	if( gydp_application == NULL ) {
		/* chain to parent constructor */
		object = gydp_app_parent_class->constructor(type, n, properties);

		/* set singleton variable */
		gydp_application = GYDP_APP(object);
	} else
		object = g_object_ref(G_OBJECT(gydp_application));

	return object;
}

static void gydp_app_finalize(GObject *object) {
	/* chain to parent finalize */
	gydp_app_parent_class->finalize(object);
}

GObject *gydp_app_new(int *argc, char ***argv) {
	GydpApp *app;

	/* initialize glib */
	g_type_init();

	/* initialize application object */
	app = g_object_new(GYDP_TYPE_APP, NULL);

	/* initialize GUI */
	gtk_init(argc, argv);

	return G_OBJECT(app);
}

GObject *gydp_app() {
	return G_OBJECT(gydp_application);
}

void gydp_app_run(GObject *self G_GNUC_UNUSED) {
	gtk_main();
}

GtkWidget *gydp_app_get_widget(GObject *self) {
	return GYDP_APP(self)->widget;
}

void gydp_app_set_widget(GObject *self, GtkWidget *widget) {
	GydpApp *app = GYDP_APP(self);

	/* remove previous widget */
	if( app->widget ) {
		g_signal_handlers_block_by_func(G_OBJECT(app->widget),
				G_CALLBACK(gtk_main_quit), NULL);
	}

	/* set new widget */
	app->widget = widget;

	g_signal_connect(G_OBJECT(app->widget), "destroy",
			G_CALLBACK(gtk_main_quit), NULL);
}

