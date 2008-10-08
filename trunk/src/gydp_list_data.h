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

#ifndef __GYDP_LIST_DATA_H__
#define __GYDP_LIST_DATA_H__

#include "gydp_global.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _GydpListDataIface GydpListDataIface;
typedef struct _GydpListData      GydpListData;

#define GYDP_TYPE_LIST_DATA           (gydp_list_data_get_type())
#define GYDP_LIST_DATA(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj),    GYDP_TYPE_LIST_DATA, GydpListData))
#define GYDP_IS_LIST_DATA(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj),    GYDP_TYPE_LIST_DATA))
#define GYDP_LIST_DATA_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), GYDP_TYPE_LIST_DATA, GydpListDataIface))

struct _GydpListDataIface {
	GTypeInterface __parent__;

	/* virtual table */
	guint        (*get_items)(GydpListData *list_data);
	const gchar *(*get_item) (GydpListData *list_data, guint n);
};

GType        gydp_list_data_get_type () G_GNUC_CONST;
guint        gydp_list_data_get_items(GydpListData *list_data);
const gchar *gydp_list_data_get_item (GydpListData *list_data, guint n);
void         gydp_list_data_changed  (GydpListData *list_data);

G_END_DECLS

#endif /* __GYDP_LIST_DATA_H__ */

