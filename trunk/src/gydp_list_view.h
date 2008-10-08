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
 * along with gydpdict. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GYDP_LIST_VIEW_H__
#define __GYDP_LIST_VIEW_H__

#include "gydp_global.h"
#include "gydp_list_data.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _GydpListViewClass GydpListViewClass;
typedef struct _GydpListView      GydpListView;

typedef enum _GydpSelectMode {
	GYDP_SELECT_TOP,
	GYDP_SELECT_MIDDLE,
	GYDP_SELECT_BOTTOM,
	GYDP_SELECT_ALWAYS_TOP,
	GYDP_SELECT_ALWAYS_MIDDLE,
	GYDP_SELECT_ALWAYS_BOTTOM,
} GydpSelectMode;

#define GYDP_TYPE_LIST_VIEW            (gydp_list_view_get_type())
#define GYDP_LIST_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GYDP_TYPE_LIST_VIEW, GydpListView))
#define GYDP_LIST_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  GYDP_TYPE_LIST_VIEW, GydpListViewClass))
#define GYDP_IS_LIST_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GYDP_TYPE_LIST_VIEW))
#define GYDP_IS_LIST_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  GYDP_TYPE_LIST_VIEW))
#define GYDP_LIST_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  GYDP_TYPE_LIST_VIEW, GydpListViewClass))

GType         gydp_list_view_get_type    () G_GNUC_CONST;
GtkWidget    *gydp_list_view_new         ();

void          gydp_list_view_set_data    (GydpListView *list_view, GydpListData *list_data);
GydpListData *gydp_list_view_get_data    (GydpListView *list_view);

gint          gydp_list_view_get_selected(GydpListView *list_view);
void          gydp_list_view_set_selected(GydpListView *list_view, gint item, GydpSelectMode mode);

G_END_DECLS

#endif /* __GYDP_LISTVIEW_H__ */

