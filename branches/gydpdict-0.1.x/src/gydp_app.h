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

#ifndef __GYDP_APP_H__
#define __GYDP_APP_H__

#include "gydp_global.h"
#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

typedef struct _GydpApp      GydpApp;
typedef struct _GydpAppClass GydpAppClass;

#define GYDP_TYPE_APP            (gydp_app_get_type ())
#define GYDP_APP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GYDP_TYPE_APP, GydpApp))
#define GYDP_APP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GYDP_TYPE_APP, GydpAppClass))
#define GYDP_IS_APP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GYDP_TYPE_APP))
#define GYDP_IS_APP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GYDP_TYPE_APP))
#define GYDP_APP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GYDP_TYPE_APP, GydpAppClass))

GType      gydp_app_get_type  () G_GNUC_CONST;
GObject   *gydp_app_new       (int *argc, char ***argv);

/* get global singleton object */
GObject   *gydp_app();

void       gydp_app_run       (GObject *self);
GtkWidget *gydp_app_get_widget(GObject *self);
void       gydp_app_set_widget(GObject *self, GtkWidget *widget);

G_END_DECLS

#endif /* __GYDP_APP_H__ */

