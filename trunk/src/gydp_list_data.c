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

#include "gydp_list_data.h"

/* signals */
enum {
	SIGNAL_CHANGED,
	NO_SIGNALS,
};

static guint gydp_list_data_signals[NO_SIGNALS];

/* private gobject functions */
static void gydp_list_data_base_init(gpointer klass);

GType gydp_list_data_get_type() {
	static GType type = G_TYPE_INVALID;
	if( G_UNLIKELY( type == G_TYPE_INVALID ) ) {
		static const GTypeInfo info = {
			sizeof(GydpListDataIface),       /* class size */
			gydp_list_data_base_init, NULL,  /* base init, finalize */
			NULL, NULL,                      /* class init, finalize */
			NULL,                            /* class init, finalize user_data */
			0, 0,                            /* base size, prealloc size */
			NULL,                            /* instance init */
			NULL,                            /* GValue table */
	};

		type = g_type_register_static(G_TYPE_INTERFACE, "GydpListData", &info, 0);
		g_type_interface_add_prerequisite(type, G_TYPE_OBJECT);
	}

	return type;
}

static void gydp_list_data_base_init(gpointer klass) {
	static gboolean initialized = FALSE;

	if( !initialized ) {
		gydp_list_data_signals[SIGNAL_CHANGED] = g_signal_newv("changed",
				GYDP_TYPE_LIST_DATA,
				G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
				NULL, NULL, NULL,           /* closure, accumulator, accumulator data */
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0 , NULL);     /* return type, params */

		initialized = TRUE;
	}
}

guint gydp_list_data_get_items(GydpListData *list_data) {
	g_return_val_if_fail(GYDP_IS_LIST_DATA(list_data), 0);
	return GYDP_LIST_DATA_GET_IFACE(list_data)->get_items(list_data);
}

const gchar *gydp_list_data_get_item(GydpListData *list_data, guint n) {
	g_return_val_if_fail(GYDP_IS_LIST_DATA(list_data), NULL);
	return GYDP_LIST_DATA_GET_IFACE(list_data)->get_item(list_data, n);
}

void gydp_list_data_changed(GydpListData *list_data) {
	g_return_if_fail(GYDP_IS_LIST_DATA(list_data));
	g_signal_emit(list_data, gydp_list_data_signals[SIGNAL_CHANGED], 0);
}

