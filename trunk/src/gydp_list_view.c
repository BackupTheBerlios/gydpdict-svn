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

#include "gydp_list_view.h"

#include <gdk/gdkkeysyms.h>
#include <string.h>

/* list model columns */
enum {
	COLUMN_WORD,
	COLUMN_ID,
	COLUMNS,
};

/* signals */
enum {
	SIGNAL_CHANGED,
	NO_SIGNALS,
};

/* properties */
enum {
	PROPERTY_0,
	PROPERTY_DATA,
	NO_PROPERTIES,
};

struct _GydpListViewClass {
	GtkWidgetClass __parent__;
};

struct _GydpListView {
	GtkWidget __parent__;

	/* internal widgets */
	GtkWidget *layout;
	GtkWidget *list;
	GtkWidget *scroll;

	/* internal data */
	gint height;            /* current widget height */
	gint selected;          /* currently selected item */

	/* properties */
	GydpListData *data;
};

static GObjectClass *gydp_list_view_parent_klass = NULL;
static guint         gydp_list_view_signals[NO_SIGNALS];

/* private GObject functions */
static void     gydp_list_view_init        (GydpListView *self);
static void     gydp_list_view_class_init  (GydpListViewClass *klass);
static GObject *gydp_list_view_constructor (GType type, guint n, GObjectConstructParam *properties);
static void     gydp_list_view_finalize    (GObject *object);
static void     gydp_list_view_set_property(GObject *object, guint id, const GValue *value, GParamSpec *pspec);
static void     gydp_list_view_get_property(GObject *object, guint id, GValue *value, GParamSpec *pspec);

/* callbacks */
static void     gydp_list_view_selection_changed   (GtkTreeSelection *selection, gpointer data);
static gboolean gydp_list_view_event_scroll        (GtkWidget *widget, GdkEventScroll *event, gpointer data);
static gboolean gydp_list_view_event_button_press  (GtkWidget *widget, GdkEventButton *event, gpointer data);
static void     gydp_list_view_size_allocate       (GtkWidget *widget, GtkAllocation *allocation, gpointer data);
static void     gydp_list_view_scroll_value_changed(GtkRange *range, gpointer data);

/* private update functions */
static void     gydp_list_view_select       (GydpListView *self, gint value, gint offset);
static void     gydp_list_view_update       (GydpListView *self);
static void     gydp_list_view_update_select(GydpListView *self);

GType gydp_list_view_get_type() {
	static GType type = G_TYPE_INVALID;
	if( G_UNLIKELY( type == G_TYPE_INVALID ) ) {
		static const GTypeInfo info = {
			sizeof(GydpListViewClass),                       /* class size */
			NULL, NULL,                                      /* base init, finalize */
			(GClassInitFunc) gydp_list_view_class_init, NULL, /* class init, finalize */
			NULL,                                            /* class init, finalize user_data */
			sizeof(GydpListView), 0,                         /* base size, prealloc size */
			(GInstanceInitFunc) gydp_list_view_init,          /* instance init */
			NULL,                                            /* GValue table */
		};

		type = g_type_register_static(GTK_TYPE_WIDGET, "GydpListView", &info, 0);
	}

	return type;
}

GtkWidget *gydp_list_view_new() {
	return g_object_new(GYDP_TYPE_LIST_VIEW, NULL);
}

void gydp_list_view_set_data(GydpListView *list_view, GydpListData *list_data) {
	g_return_if_fail( GYDP_IS_LIST_VIEW(list_view) );

	/* unref previous data */
	g_object_unref(list_view->data);
	list_view->data = NULL;

	/* hold reference for new data */
	if( list_data != NULL ) {
		g_return_if_fail( GYDP_IS_LIST_DATA(list_data) );

		list_view->data = list_data;
		g_object_unref(list_view->data);
	}
}

static void gydp_list_view_init(GydpListView *self) {

	/* init data */
	self->selected = -1;
	self->height = GTK_WIDGET(self)->allocation.height;

	/* init properties */

	/* list */
	self->list = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(self->list), FALSE);
	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(self->list), TRUE);
	GTK_WIDGET_UNSET_FLAGS(self->list, GTK_CAN_FOCUS);

	/* selection */
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->list));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	{ /* list/view model */
		GtkListStore *store = gtk_list_store_new(COLUMNS, G_TYPE_STRING, G_TYPE_INT);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
				"Word", renderer, "text", COLUMN_WORD, NULL);
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_append_column(GTK_TREE_VIEW(self->list), column);
		gtk_tree_view_set_model(GTK_TREE_VIEW(self->list), GTK_TREE_MODEL(store));
	}

	/* list scroll */
	self->scroll = gtk_vscrollbar_new(NULL);

	/* list layout */
	self->layout = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(self->layout), self->list, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(self->layout), self->scroll, FALSE, TRUE, 0);

	/* signals */
	g_signal_connect(G_OBJECT(selection), "changed",
			G_CALLBACK(gydp_list_view_selection_changed), self);
	g_signal_connect(G_OBJECT(self->list), "size-allocate",
			G_CALLBACK(gydp_list_view_size_allocate), self);
	g_signal_connect(G_OBJECT(self->list), "scroll-event",
			G_CALLBACK(gydp_list_view_event_scroll), self);
	g_signal_connect(G_OBJECT(self->list), "button-press-event",
			G_CALLBACK(gydp_list_view_event_button_press), self);
	g_signal_connect(G_OBJECT(self->scroll), "value-changed",
			G_CALLBACK(gydp_list_view_scroll_value_changed), self);
}

static void gydp_list_view_class_init(GydpListViewClass *klass) {
	/* determine parent class */
	gydp_list_view_parent_klass = g_type_class_peek_parent(klass);

	GObjectClass *gobject_klass = G_OBJECT_CLASS(klass);
	gobject_klass->constructor = gydp_list_view_constructor;
	gobject_klass->finalize = gydp_list_view_finalize;
	gobject_klass->set_property = gydp_list_view_set_property;
	gobject_klass->get_property = gydp_list_view_get_property;

	g_object_class_install_property(gobject_klass, PROPERTY_DATA,
			g_param_spec_object("data", "data", "data", GYDP_TYPE_LIST_DATA, G_PARAM_READWRITE));

	gydp_list_view_signals[SIGNAL_CHANGED] = g_signal_newv("changed",
			G_TYPE_FROM_CLASS(gobject_klass),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			NULL, NULL, NULL,           /* closure, accumulator, accumulator data */
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0 , NULL);     /* return type, params */
}

static GObject *gydp_list_view_constructor(GType type, guint n, GObjectConstructParam *properties) {
	GObject *object = NULL;

	/* chain to parent constructor */
	object = gydp_list_view_parent_klass->constructor(type, n, properties);

	return object;
}

static void gydp_list_view_finalize(GObject *object) {
	/* chain to parent finalize */
	gydp_list_view_parent_klass->finalize(object);
}

static void gydp_list_view_set_property(GObject *object, guint id, const GValue *value, GParamSpec *pspec) {
	GydpListView *list_view = GYDP_LIST_VIEW(object);

	switch( id ) {
	case PROPERTY_DATA:
		gydp_list_view_set_data(list_view, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
		break;
	}
}

static void gydp_list_view_get_property(GObject *object, guint id, GValue *value, GParamSpec *pspec) {
	GydpListView *list_view = GYDP_LIST_VIEW(object);

	switch( id ) {
	case PROPERTY_DATA:
		g_value_set_object(value, list_view->data);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
		break;
	}
}

static void gydp_list_view_selection_changed(GtkTreeSelection *selection, gpointer data) {
	GydpListView *listview = GYDP_LIST_VIEW(data);
	GtkTreeModel *model;
	GtkTreeIter it;
	gchar *word;
	gint current;

	/* check if any item is actually selected */
	if( gtk_tree_selection_get_selected(selection, &model, &it) ) {
		gtk_tree_model_get(model, &it, COLUMN_WORD, &word, COLUMN_ID, &current, -1);

		/* check if selected new item */
		if( listview->selected != current ) {
			/* update the current selected item */
			listview->selected = current;
			/* emit the changed signal */
			g_signal_emit(G_OBJECT(listview), gydp_list_view_signals[SIGNAL_CHANGED], 0);
		}

		/* free data obtained from store */
		g_free(word);
	}
}

static gboolean gydp_list_view_event_scroll(GtkWidget *widget G_GNUC_UNUSED, GdkEventScroll *event, gpointer data) {
	GydpListView *listview = GYDP_LIST_VIEW(data);

	/* copy event structure */
	event = (GdkEventScroll *)gdk_event_copy((GdkEvent *)event);

	/* set correnct GdkWindow */
	g_object_unref(G_OBJECT(event->window));
	event->window = g_object_ref(G_OBJECT(listview->scroll->window));

	/* send event */
	gtk_widget_event(listview->scroll, (GdkEvent *)event);

	/* free event structure */
	gdk_event_free((GdkEvent *)event);

	return TRUE;
}

static gboolean gydp_list_view_event_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
	GtkTreePath *path = NULL;

	switch( event->type ) {
	case GDK_BUTTON_PRESS:
		if( gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),
				event->x, event->y, &path, NULL, NULL, NULL) ) {
			gint index = gtk_tree_path_get_indices(path)[0];
			gydp_list_view_select(GYDP_LIST_VIEW(data), -1, index);
			gtk_tree_path_free(path);
		}
		return TRUE;
	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
		return TRUE;
	default:
		g_return_val_if_reached(TRUE);
	}
}

static void gydp_list_view_size_allocate(GtkWidget *widget, GtkAllocation *allocation, gpointer data) {
	GydpListView *listview = GYDP_LIST_VIEW(data);

	/* check if widget size actually changed */
	if( listview->height != widget->allocation.height ||
			listview->height != allocation->height ) {
		/* update the desired height */
		listview->height = allocation->height;

		/* update the list and list selection */
		gydp_list_view_update(listview);
		gydp_list_view_update_select(listview);
	}
}

static void gydp_list_view_scroll_value_changed(GtkRange G_GNUC_UNUSED *range, gpointer data) {
	GydpListView *listview = GYDP_LIST_VIEW(data);

	/* update the list and list selection */
	gydp_list_view_update(listview);
	gydp_list_view_update_select(listview);
}

static void gydp_list_view_select(GydpListView *self, gint value, gint offset) {
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->list));
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->list));
	GtkTreeIter it;

	/* check if we should scroll */
	if( value >= 0 &&
			value != gtk_range_get_value(GTK_RANGE(self->scroll)) ) {
		/* set invalid selected item (prevent emitting signal) and scroll */
		self->selected = -1;
		gtk_tree_selection_unselect_all(selection);
		gtk_range_set_value(GTK_RANGE(self->scroll), value);

		/* validate that internal selection is same */
		g_assert( self->selected == -1 );
	}

	/* find n-th item on list and select it if present */
	if( offset >= 0 &&
			gtk_tree_model_iter_nth_child(model, &it, NULL, offset) ) {
		/* select item and save internal selection state */
		gtk_tree_selection_select_iter(selection, &it);
	} else {
		/* specify invalid selected item */
		self->selected = -1;
		gtk_tree_selection_unselect_all(selection);

		/* check if there are any items in model */
		if( offset >= 0 &&
				gtk_tree_model_iter_n_children(model, NULL) > 0 ) {
			g_warning("Invalid item to select: '%d/%d'\n",
					offset, gtk_tree_model_iter_n_children(model, NULL));
			return;
		}
	}
}

static void gydp_list_view_update(GydpListView *self) {
	GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(self->list));
	GtkTreeIter it;
	gdouble position;
	gint cell_height, separator, id;
	const gchar *word;

	/* get scroll for word list */
	GtkAdjustment *adjustment = gtk_range_get_adjustment(GTK_RANGE(self->scroll));

	/* top items on word list */
	g_object_get(G_OBJECT(adjustment), "value", &position, NULL);

	/* get single cell size */
	GtkTreeViewColumn *column = gtk_tree_view_get_column(GTK_TREE_VIEW(self->list), 0);
	gtk_tree_view_column_cell_get_size(column, NULL, NULL, NULL, NULL, &cell_height);
	gtk_widget_style_get(self->list, "vertical-separator", &separator, NULL);
	cell_height += separator;

	/* clear store */
	gtk_list_store_clear(GTK_LIST_STORE(store));

	/* NOTICE:
	 * setting store explicitely sends selection-changed signal */

	const gint size = self->list->allocation.height / cell_height;
	for(gint i = 0; i < size; ++i) {

		/* get id */
		id = i + (gint)position;

		/* extract item associated with id */
		word = gydp_list_data_get_item(self->data, id);
		g_warn_if_fail( word != NULL );

		/* add only available words */
		if( word != NULL ) {
			gtk_list_store_append(GTK_LIST_STORE(store), &it);
			gtk_list_store_set(GTK_LIST_STORE(store), &it,
					COLUMN_WORD, word,
					COLUMN_ID, id, -1);
		}
	}

	/* adjust scroll (rounding is important) */
	g_object_set(G_OBJECT(adjustment),
			"page-size", (gdouble)(gint)size,
			"page-increment", (gdouble)(gint)(0.9 * size), NULL);

	/* set small as possible (permit full resize by user) */
	gtk_widget_set_size_request(self->list, -1, 0);
}

static void gydp_list_view_update_select(GydpListView *self) {
	GtkAdjustment *adjustment = gtk_range_get_adjustment(GTK_RANGE(self->scroll));
	gdouble value, size;

	/* obtain adjustment properties */
	g_object_get(G_OBJECT(adjustment), "value", &value, "page-size", &size, NULL);

	/* select item if we are in corrent range */
	if( self->selected >= (gint)value &&
			self->selected < (gint)(value + size) ) {

		/* select item on current visible set of items */
		gydp_list_view_select(self, -1, self->selected - (gint)value);
	}
}

