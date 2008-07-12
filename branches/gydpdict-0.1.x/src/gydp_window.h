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

#ifndef __GYDP_WINDOW_H__
#define __GYDP_WINDOW_H__

#include "gydp_global.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _GydpWindowClass GydpWindowClass;
typedef struct _GydpWindow      GydpWindow;

#define GYDP_TYPE_WINDOW            (gydp_window_get_type ())
#define GYDP_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GYDP_TYPE_WINDOW, GydpWindow))
#define GYDP_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GYDP_TYPE_WINDOW, GydpWindowClass))
#define GYDP_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GYDP_TYPE_WINDOW))
#define GYDP_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GYDP_TYPE_WINDOW))
#define GYDP_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GYDP_TYPE_WINDOW, GydpWindowClass))

GType      gydp_window_get_type() G_GNUC_CONST;
GtkWidget *gydp_window_new     ();

G_END_DECLS

#endif /* __GYDP_WINDOW_H__ */

