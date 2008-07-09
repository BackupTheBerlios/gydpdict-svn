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

#include "gydp_window.h"
#include "gydp_util.h"
#include "gydp_dict.h"
#include "gydp_conf.h"
#include "gydp_app.h"

#include <gdk/gdkkeysyms.h>
#include <string.h>

/* list model columns */
enum {
	COLUMN_WORD,
	COLUMN_ID,
	COLUMNS
};

struct _GydpWindowClass {
	GtkWindowClass __parent__;
};

struct _GydpWindow {
	GtkWindow __parent__;

	/* dynamic interface */
	struct {
		GtkActionGroup *actions;    /* menu actions */
		GtkUIManager *manager;      /* ui manager */
	} ui;

	/* important menu items */
	struct {
		GtkWidget *dict;            /* dictionary menu item (dynamic submenu) */
		GSList *dicts;              /* dictionary languages available in current dictionary menu */
		GtkWidget *main;            /* main menu item */
	} menu;

	/* widget layout */
	struct {
		GtkWidget *window;          /* main window layout */
		GtkWidget *central;         /* central widget layout */
		GtkWidget *selection;       /* selection panel layout */
		GtkWidget *words;           /* word list layout */
	} layout;

	/* output widgets */
	GtkWidget *definition;        /* translation text */

	/* input widgets */
	GtkWidget *word;              /* single word entry */
	GtkWidget *words;             /* word list */

	/* scrolls */
	GtkWidget *definition_scroll; /* definition scrolled window */
	GtkWidget *words_scroll;      /* word list scroll */

	/* additional data */
	gint words_height;            /* current words widget height */
	gint words_selected;          /* currently selected item in words widget */
};

/* parent class holder */
static GObjectClass *gydp_window_parent_klass = NULL;

/* private GObject functions */
static void     gydp_window_init                      (GydpWindow *self);
static void     gydp_window_class_init                (GydpWindowClass *klass);
static GObject *gydp_window_constructor               (GType type, guint n, GObjectConstructParam *properties);
static void     gydp_window_finalize                  (GObject *object);

/* callbacks */
static void     gydp_window_hide                      (GtkWidget *widget, gpointer data);
static void     gydp_window_dict_toggled              (GtkCheckMenuItem *item, gpointer data);
static void     gydp_window_word_changed              (GtkEntry *entry, gpointer data);
static gboolean gydp_window_word_event_key_press      (GtkWidget *widget, GdkEventKey *event, gpointer data);
static gboolean gydp_window_words_event_button_press  (GtkWidget *widget, GdkEventButton *event, gpointer data);
static gboolean gydp_window_words_event_scroll        (GtkWidget *widget, GdkEventScroll *event, gpointer data);
static void     gydp_window_words_selection_changed   (GtkTreeSelection *selection, gpointer data);
static void     gydp_window_words_size_allocate       (GtkWidget *widget, GtkAllocation *allocation, gpointer data);
static void     gydp_window_words_scroll_value_changed(GtkRange *range, gpointer data);

static void     gydp_window_action_quit               (GtkAction *action, GydpWindow *window);
static void     gydp_window_action_toggle_engine      (GtkAction *action, GydpWindow *window);
static void     gydp_window_action_toggle_language    (GtkAction *action, GydpWindow *window);
static void     gydp_window_action_preferences        (GtkAction *action, GydpWindow *window);
static void     gydp_window_action_about              (GtkAction *action, GydpWindow *window);

/* private functions */
static void     gydp_window_dict_update               (GydpWindow *self, GydpLang lang);
static void     gydp_window_word_sync                 (GydpWindow *self);
static void     gydp_window_words_select              (GydpWindow *self, gint value, gint offset);
static void     gydp_window_words_update              (GydpWindow *self);
static void     gydp_window_words_update_select       (GydpWindow *self);

/* actions */
static const GtkActionEntry gydp_window_actions[] = {
	{ "file-menu", NULL, "File", NULL, NULL, NULL },
	{ "quit", GTK_STOCK_QUIT, "Quit", "<Ctrl>Q", NULL, G_CALLBACK(gydp_window_action_quit) },
	{ "edit-menu", NULL, "Edit", NULL, NULL, NULL},
	{ "toggle-engine", GTK_STOCK_JUMP_TO, "Toggle engine", NULL, NULL, G_CALLBACK(gydp_window_action_toggle_engine) },
	{ "toggle-language", GTK_STOCK_JUMP_TO, "Toggle language", "<Ctrl>T", NULL, G_CALLBACK(gydp_window_action_toggle_language) },
	{ "preferences", GTK_STOCK_PREFERENCES, "Preferences", NULL, NULL, G_CALLBACK(gydp_window_action_preferences) },
	{ "dict-menu", NULL, "Dictionary", NULL, NULL, NULL},
	{ "help-menu", NULL, "Help", NULL, NULL, NULL},
	{ "about", GTK_STOCK_ABOUT, "About", NULL, NULL, G_CALLBACK(gydp_window_action_about) },
};

/* ui */
static const char gydp_window_ui[] =
	"<ui>"
		"<menubar name=\"main-menu\">"
			"<menu action=\"file-menu\">"
				"<menuitem action=\"quit\" />"
			"</menu>"
			"<menu action=\"edit-menu\">"
				"<menuitem action=\"toggle-engine\" />"
				"<menuitem action=\"toggle-language\" />"
				"<menuitem action=\"preferences\" />"
			"</menu>"
			"<menu action=\"dict-menu\">"
			"</menu>"
			"<menu action=\"help-menu\">"
				"<menuitem action=\"about\" />"
			"</menu>"
		"</menubar>"
	"</ui>";

GType gydp_window_get_type() {
	static GType type = G_TYPE_INVALID;
	if( G_UNLIKELY( type == G_TYPE_INVALID ) ) {
		static const GTypeInfo info = {
			sizeof(GydpWindowClass),                       /* class size */
			NULL, NULL,                                    /* base init, finalize */
			(GClassInitFunc) gydp_window_class_init, NULL, /* class init, finalize */
			NULL,                                          /* class init, finalize user_data */
			sizeof(GydpWindow), 0,                         /* base size, prealloc size */
			(GInstanceInitFunc) gydp_window_init,          /* instance init */
			NULL,                                          /* GValue table */
		};

		type = g_type_register_static(GTK_TYPE_WINDOW, "GydpWindow", &info, 0);
	}

	return type;
}

GtkWidget *gydp_window_new() {
	return g_object_new(GYDP_TYPE_WINDOW, NULL);
}

static void gydp_window_init(GydpWindow *self) {
	/*
	 * customize self
	 */

	/*
	 * input widgets
	 */

	/* word entry */
	self->word = gtk_entry_new();

	/* word list */
	self->words = gtk_tree_view_new();
	self->words_height = self->words->allocation.height;
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(self->words), FALSE);

	{ /* specify selection */
		GtkTreeSelection *self_words_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->words));
		gtk_tree_selection_set_mode(self_words_selection, GTK_SELECTION_SINGLE);
	}

	{ /* create list and view model */
		GtkListStore *list = gtk_list_store_new(COLUMNS, G_TYPE_STRING, G_TYPE_INT);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
				"Word", renderer, "text", COLUMN_WORD, NULL);
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_append_column(GTK_TREE_VIEW(self->words), column);
		gtk_tree_view_set_model(GTK_TREE_VIEW(self->words), GTK_TREE_MODEL(list));
	}

	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(self->words), TRUE);
	GTK_WIDGET_UNSET_FLAGS(self->words, GTK_CAN_FOCUS);

	/* word list scroll */
	self->words_scroll = gtk_vscrollbar_new(NULL);

	/*
	 * output widgets
	 */

	/* word definition */
	self->definition = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(self->definition), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(self->definition), GTK_WRAP_WORD);
	GTK_WIDGET_UNSET_FLAGS(self->definition, GTK_CAN_FOCUS);

	self->definition_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(self->definition_scroll),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(self->definition_scroll), self->definition);

	{ /* create tags */
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->definition));
		gint font_size = pango_font_description_get_size(GTK_WIDGET(self)->style->font_desc);
		if( pango_font_description_get_size_is_absolute(GTK_WIDGET(self)->style->font_desc) )
			font_size *= PANGO_SCALE;

		gtk_text_buffer_create_tag(buffer, GYDP_TAG_BOLD,
				"weight", PANGO_WEIGHT_BOLD, NULL);
		gtk_text_buffer_create_tag(buffer, GYDP_TAG_ITALIC,
				"style", PANGO_STYLE_ITALIC, NULL);
		gtk_text_buffer_create_tag(buffer, GYDP_TAG_ALIGN_CENTER,
				"justification", GTK_JUSTIFY_CENTER, "justification-set", TRUE, NULL);
		gtk_text_buffer_create_tag(buffer, GYDP_TAG_ALIGN_LEFT,
				"justification", GTK_JUSTIFY_LEFT, "justification-set", TRUE, NULL);
		gtk_text_buffer_create_tag(buffer, GYDP_TAG_ALIGN_RIGHT,
				"justification", GTK_JUSTIFY_RIGHT, "justification-set", TRUE, NULL);
		gtk_text_buffer_create_tag(buffer, GYDP_TAG_SCRIPT_NORMAL,
				"scale", 1.0, "scale-set", TRUE, "rise", 0, "rise-set", TRUE, NULL);
		gtk_text_buffer_create_tag(buffer, GYDP_TAG_SCRIPT_SUPER,
				"scale", 0.5, "scale-set", TRUE, "rise", font_size/2, "rise-set", TRUE, NULL);
		gtk_text_buffer_create_tag(buffer, GYDP_TAG_SCRIPT_SUB,
				"scale", 0.5, "scale-set", TRUE, "rise", -font_size/16, "rise-set", TRUE, NULL);
		gtk_text_buffer_create_tag(buffer, GYDP_TAG_COLOR_RED,
				"foreground", "red", "foreground-set", TRUE, NULL);
		gtk_text_buffer_create_tag(buffer, GYDP_TAG_COLOR_GREEN,
				"foreground", "green", "foreground-set", TRUE, NULL);
		gtk_text_buffer_create_tag(buffer, GYDP_TAG_COLOR_BLUE,
				"foreground", "blue", "foreground-set", TRUE, NULL);
	}

	/*
	 * dynamic interface
	 */

	/* actions */
	self->ui.actions = gtk_action_group_new("GydpWindow");
	gtk_action_group_add_actions(self->ui.actions,
			gydp_window_actions, G_N_ELEMENTS(gydp_window_actions),
			GTK_WIDGET(self));
	/* prevent hiding of dict menu */
	g_object_set(gtk_action_group_get_action(self->ui.actions, "dict-menu"),
			"hide-if-empty", FALSE, NULL);

	/* ui manager */
	self->ui.manager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(self->ui.manager, self->ui.actions, 0);
	gtk_ui_manager_add_ui_from_string(self->ui.manager, gydp_window_ui, -1, NULL);
	gtk_window_add_accel_group(GTK_WINDOW(self), gtk_ui_manager_get_accel_group(self->ui.manager));

	/* extract important menu items */
	self->menu.main = gtk_ui_manager_get_widget(self->ui.manager, "/main-menu");
	self->menu.dict = gtk_ui_manager_get_widget(self->ui.manager, "/main-menu/dict-menu");
	self->menu.dict = gtk_menu_item_get_submenu(GTK_MENU_ITEM(self->menu.dict));
	self->menu.dicts = NULL;

	{ /* disable unused menu items */
		GtkWidget *preferences = gtk_ui_manager_get_widget(self->ui.manager, "/main-menu/edit-menu/preferences");
		gtk_widget_set_sensitive(preferences, FALSE);
	}

	/*
	 * layout
	 */

	/* list layout */
	self->layout.words = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(self->layout.words), self->words, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(self->layout.words), self->words_scroll, FALSE, TRUE, 0);

	/* selection layout */
	self->layout.selection = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(self->layout.selection), self->word, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(self->layout.selection), self->layout.words, TRUE, TRUE, 0);

	/* central widget layout */
	self->layout.central = gtk_hpaned_new();
	gtk_paned_pack1(GTK_PANED(self->layout.central), self->layout.selection, FALSE, FALSE);
	gtk_paned_pack2(GTK_PANED(self->layout.central), self->definition_scroll, TRUE, FALSE);
	gtk_widget_set_size_request(GTK_WIDGET(self->layout.selection), 100, -1);
	gtk_widget_set_size_request(GTK_WIDGET(self->definition), 100, -1);

	/* main window layout */
	self->layout.window = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(self->layout.window), self->menu.main, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(self->layout.window), self->layout.central, TRUE, TRUE, 0);

	/* set window widget layout */
	gtk_container_add(GTK_CONTAINER(self), self->layout.window);

	/* extract configuration from global app singleton */
	GydpConf *config = g_object_get_data(G_OBJECT(gydp_app()), GYDP_APP_CONF);

	/* signals */
	g_signal_connect(G_OBJECT(self), "hide",
		 G_CALLBACK(gydp_window_hide), NULL);
	g_signal_connect(G_OBJECT(self->word), "changed",
			G_CALLBACK(gydp_window_word_changed), self);
	g_signal_connect(G_OBJECT(self->word), "key-press-event",
			G_CALLBACK(gydp_window_word_event_key_press), self);
	g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(self->words))), "changed",
			G_CALLBACK(gydp_window_words_selection_changed), self);
	g_signal_connect(G_OBJECT(self->words), "size-allocate",
			G_CALLBACK(gydp_window_words_size_allocate), self);
	g_signal_connect(G_OBJECT(self->words), "button-press-event",
			G_CALLBACK(gydp_window_words_event_button_press), self);
	g_signal_connect(G_OBJECT(self->words), "scroll-event",
			G_CALLBACK(gydp_window_words_event_scroll), self);
	g_signal_connect(G_OBJECT(self->words_scroll), "value-changed",
			G_CALLBACK(gydp_window_words_scroll_value_changed), self);

	{ /* geometry */
		gchar *geometry = gydp_conf_get_string(config, "window", "geometry");
		gtk_window_parse_geometry(GTK_WINDOW(self), geometry);
		g_free(geometry);
	}

	{ /* split */
		gint split = gydp_conf_get_integer(config, "window", "split");
		gtk_paned_set_position(GTK_PANED(self->layout.central), split);
	}

	/* update current default dictionary (initialize view) */
	GydpDict *dict = g_object_get_data(G_OBJECT(gydp_app()), GYDP_APP_DICT);
	const gchar *nick = gydp_engine_value_to_nick(dict->engine);
	const gchar *lang = gydp_conf_get_string(config, nick, "lang");
	gydp_window_dict_update(self, gydp_lang_name_to_value(lang));

	/* set initial focus on the words */
	gtk_widget_grab_focus(self->words);
}

static void gydp_window_class_init(GydpWindowClass *klass) {
	/* determine parent class */
	gydp_window_parent_klass = g_type_class_peek_parent(klass);

	GObjectClass *gobject_klass = G_OBJECT_CLASS(klass);
	gobject_klass->constructor = gydp_window_constructor;
	gobject_klass->finalize = gydp_window_finalize;
}

static GObject *gydp_window_constructor(GType type, guint n, GObjectConstructParam *properties) {
	GObject *object = NULL;

	/* chain to parent constructor */
	object = gydp_window_parent_klass->constructor(type, n, properties);

	return object;
}

static void gydp_window_finalize(GObject *object) {
	GydpWindow *window = GYDP_WINDOW(object);
	g_slist_free(window->menu.dicts);

	/* chain to parent finalize */
	gydp_window_parent_klass->finalize(object);
}

static void gydp_window_hide(GtkWidget *widget, gpointer data G_GNUC_UNUSED) {
	GydpWindow *window = GYDP_WINDOW(widget);

	/* extract configuration from global app singleton */
	GydpConf *config = g_object_get_data(G_OBJECT(gydp_app()), GYDP_APP_CONF);

	/* get window configuration to save */
	gint width, height, split;
	gtk_window_get_size(GTK_WINDOW(window), &width, &height);
	split = gtk_paned_get_position(GTK_PANED(window->layout.central));

	/* save window configuration */
	{ /* geometry */
		gchar *geometry = g_strdup_printf("%dx%d", width, height);
		gydp_conf_set_string(config, "window", "geometry", geometry);
		g_free(geometry);
	}

	/* split */
	gydp_conf_set_integer(config, "window", "split", split);
}

static void gydp_window_dict_toggled(GtkCheckMenuItem *item, gpointer data) {
	GydpDict *dict = g_object_get_data(gydp_app(), GYDP_APP_DICT);
	GydpConf *config = g_object_get_data(gydp_app(), GYDP_APP_CONF);
	GydpWindow *window = GYDP_WINDOW(data);

	/* check if menu item is active */
	if( !gtk_check_menu_item_get_active(item) )
		return;

	/* get dictionary language */
	const gchar *label = gtk_label_get_text(
			GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))));
	GydpLang lang = gydp_lang_nick_to_value(label);

	/* get data dirs */
	gchar **paths = gydp_data_dirs(dict->engine);

	if( !gydp_dict_load(dict, paths, lang) )
		g_printerr("Failed to load dictionary.\n");

	/* free temporary data */
	g_strfreev(paths);

	{ /* clear definition */
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->definition));
		GtkTextIter start, end;

		gtk_text_buffer_get_start_iter(buffer, &start);
		gtk_text_buffer_get_end_iter(buffer, &end);
		gtk_text_buffer_delete(buffer, &start, &end);
	}

	/* clear word */
	gtk_editable_delete_text(GTK_EDITABLE(window->word), 0, -1);

	{ /* initialize scrollbar */
		GtkObject *adjustment = gtk_adjustment_new(0, 0, gydp_dict_size(dict), 1, 1, 1);
		gtk_range_set_adjustment(GTK_RANGE(window->words_scroll), GTK_ADJUSTMENT(adjustment));
	}

	/* update current dictionary language view and specify invalid item to select */
	gydp_window_words_update(window);
	gydp_window_words_select(window, 0, -1);

	/* save current language */
	const gchar *nick = gydp_engine_value_to_nick(dict->engine);
	gydp_conf_set_string(config, nick, "lang", gydp_lang_value_to_name(lang));
}

static void gydp_window_word_changed(GtkEntry *entry, gpointer data) {
	GydpDict *dict = GYDP_DICT(g_object_get_data(gydp_app(), GYDP_APP_DICT));
	const gchar *text = gtk_entry_get_text(entry);
	GydpWindow *window = GYDP_WINDOW(data);

	/* check if entry is non empty */
	if( strlen(text) ) {
		/* find best compatible item */
		guint index = gydp_dict_find(dict, text);
		/* move scroll to specified item */
		gydp_window_words_select(window, index, 0);
	}
}

static gboolean gydp_window_word_event_key_press(GtkWidget *widget G_GNUC_UNUSED, GdkEventKey *event, gpointer data) {
	GydpWindow *window = GYDP_WINDOW(data);

	const gint id = window->words_selected;
	gdouble value, size, upper;

	/* get actual page */
	GtkAdjustment *adjustment = gtk_range_get_adjustment(GTK_RANGE(window->words_scroll));
	g_object_get(G_OBJECT(adjustment), "upper", &upper,
			"value", &value, "page-size", &size, NULL);

	switch( event->keyval ) {
	case GDK_Home:
		if( !(event->state & GDK_CONTROL_MASK) )
			break;
		gydp_window_words_select(window, 0, 0);
		gydp_window_word_sync(window);
		return TRUE;
	case GDK_End:
		if( !(event->state & GDK_CONTROL_MASK) )
			break;
		gydp_window_words_select(window, upper - size, size - 1);
		gydp_window_word_sync(window);
		return TRUE;
	case GDK_Page_Up:
		gydp_window_words_select(window, id >= size? id - (size - 1): 0, 0);
		gydp_window_word_sync(window);
		return TRUE;
	case GDK_Page_Down:
		gydp_window_words_select(window, id < upper - size? id: upper - size, size - 1);
		gydp_window_word_sync(window);
		return TRUE;
	case GDK_Up:
		if( id < (gint)value || id >= (gint)(value + size) )
			return TRUE;
		else if( id == (gint)value ) {
			if( id > 0 )
				gydp_window_words_select(window, value - 1, 0);
			else
				gydp_window_words_select(window, 0, 0);
		} else
			gydp_window_words_select(window, -1, (id - 1) - (gint)value);
		gydp_window_word_sync(window);
		return TRUE;
	case GDK_Down:
		if( id < (gint)value || id >= (gint)(value + size) )
			return TRUE;
		else if( id == (gint)(value + size - 1) ) {
			if( id < (gint)(upper - size) ) gydp_window_words_select(window, value + 1, size - 1);
			else                            gydp_window_words_select(window, upper - size, size - 1);
		} else                            gydp_window_words_select(window, -1, (id + 1) - (gint)value);
		gydp_window_word_sync(window);
		return TRUE;
	default:
		break;;
	}
	return FALSE;
}

static gboolean gydp_window_words_event_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
	GtkTreePath *path = NULL;

	switch( event->type ) {
	case GDK_BUTTON_PRESS:
		if( gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),
				event->x, event->y, &path, NULL, NULL, NULL) ) {
			gint index = gtk_tree_path_get_indices(path)[0];
			gydp_window_words_select(GYDP_WINDOW(data), -1, index);
			gydp_window_word_sync(GYDP_WINDOW(data));
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

static gboolean gydp_window_words_event_scroll(GtkWidget *widget G_GNUC_UNUSED, GdkEventScroll *event, gpointer data) {
	GydpWindow *window = GYDP_WINDOW(data);

	/* copy event structure */
	event = (GdkEventScroll *)gdk_event_copy((GdkEvent *)event);

	/* set correnct GdkWindow */
	g_object_unref(G_OBJECT(event->window));
	event->window = g_object_ref(G_OBJECT(window->words_scroll->window));

	/* send event */
	gtk_widget_event(window->words_scroll, (GdkEvent *)event);

	/* free event structure */
	gdk_event_free((GdkEvent *)event);

	return TRUE;
}

/** gydp_window_words_selection_changed
 * extract definition of the word and set it to corresponding entry after
 * selection change, we have handle some exceptions:
 *  * scrolling refresh (all view items are first removed)
 *  * no selected item during scrolling (moving out of selected item scope)
 *  * selected item during scrolling (scrolling in the scope of the selected item)
 */
static void gydp_window_words_selection_changed(GtkTreeSelection *selection, gpointer data) {
	GydpWindow *window = GYDP_WINDOW(data);
	GtkTreeModel *model;
	GtkTreeIter it;
	gchar *word;
	gint id;

	/* check if any item is actually selected */
	if( gtk_tree_selection_get_selected(selection, &model, &it) ) {
		gtk_tree_model_get(model, &it, COLUMN_WORD, &word, COLUMN_ID, &id, -1);

		/* check if selected new item */
		if( window->words_selected != id ) {
			/* extract translation text */
			gydp_dict_text(g_object_get_data(gydp_app(), GYDP_APP_DICT),
					id, gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->definition)));

			/* update internal selected item */
			window->words_selected = id;
		}

		/* free data obtained from store */
		g_free(word);
	}
}

/** gydp_window_words_size_allocate
 * update words list in GtkTreeView only when there is *actual* size change
 * Size allocate is emited after chenges in the GtkTreeView GtkListStore, we
 * have to discard these signals, but process real widget resize
 */
static void gydp_window_words_size_allocate(GtkWidget *widget, GtkAllocation *allocation, gpointer data) {
	GydpWindow *window = GYDP_WINDOW(data);

	/* check if widget size actually changed */
	if( window->words_height != widget->allocation.height ||
			window->words_height != allocation->height ) {
		window->words_height = allocation->height;
		gydp_window_words_update(window);
		gydp_window_words_update_select(window);
	}
}

/** gydp_window_words_scroll_value_changed
 * process custom scrolling, refresh current view at GtkTreeView and update
 * selection
 */
static void gydp_window_words_scroll_value_changed(GtkRange *range G_GNUC_UNUSED, gpointer data) {
	GydpWindow *window = GYDP_WINDOW(data);
	gydp_window_words_update(window);
	gydp_window_words_update_select(window);
}

static void gydp_window_action_quit(GtkAction *action G_GNUC_UNUSED, GydpWindow *window) {
	gtk_widget_destroy(GTK_WIDGET(window));
}

static void gydp_window_action_toggle_engine(GtkAction *action G_GNUC_UNUSED, GydpWindow *window) {
	GydpConf *config = g_object_get_data(G_OBJECT(gydp_app()), GYDP_APP_CONF);
	GydpDict *dict = GYDP_DICT(g_object_get_data(gydp_app(), GYDP_APP_DICT));
	GydpEngine engine;

	switch( dict->engine ) {
	case GYDP_ENGINE_SAP: engine = GYDP_ENGINE_YDP; break;
	case GYDP_ENGINE_YDP: engine = GYDP_ENGINE_SAP; break;
	default: g_return_if_reached();
	}

	/* toggle engine */
	g_object_set_data_full(gydp_app(), GYDP_APP_DICT,
			gydp_engine_new(engine),
			(GDestroyNotify)g_object_unref);

	/* update language */
	const gchar *nick = gydp_engine_value_to_nick(engine);
	const gchar *lang = gydp_conf_get_string(config, nick, "lang");
	gydp_window_dict_update(window, gydp_lang_name_to_value(lang));
}

/** gydp_window_action_toggle
 * toggle dictionary language, first we detect currently active language and
 * then select next (order is same as in menu)
 */
static void gydp_window_action_toggle_language(GtkAction *action G_GNUC_UNUSED, GydpWindow *window) {
	/* find active item */
	GSList *dicts = window->menu.dicts, *dict = dicts;
	for(; dict != NULL; dict = dict->next)
		if( gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(dict->data)) )
			break;

	/* find next language */
	if( dict->next != NULL )
		dict = dict->next;
	else
		dict = dicts;

	/* select next item */
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(dict->data), TRUE);
}

static void gydp_window_action_preferences(GtkAction *action G_GNUC_UNUSED, GydpWindow *window G_GNUC_UNUSED) {
}

static void gydp_window_action_about(GtkAction *action G_GNUC_UNUSED, GydpWindow *window G_GNUC_UNUSED) {
	const char *authors[] = { "Michał Kurgan <michal.kurgan@moloh.net>", NULL };
	const char *comments = "Simple gtk+ frontend for ydp dictionary";
	const char *copyright = "Copyright © 2008 Michał Kurgan";
	GtkWidget *dialog = gtk_about_dialog_new();

	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), comments);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), GYDP_VERSION);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), copyright);
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dialog), GYDP_LICENSE);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void gydp_window_dict_update(GydpWindow *self, GydpLang lang) {
	GydpDict *dict = GYDP_DICT(g_object_get_data(gydp_app(), GYDP_APP_DICT));

	/* remove current dictionary entries */
	gtk_container_foreach(GTK_CONTAINER(self->menu.dict),
			(GtkCallback)gtk_widget_destroy, NULL);
	/* free list with pointers to dictionaries */
	g_slist_free(self->menu.dicts);
	self->menu.dicts = NULL;

	GEnumClass *klass = g_type_class_ref(GYDP_TYPE_LANG);
	GEnumValue *enum_value = klass->values;
	GtkWidget *toggle = NULL;
	GSList *group = NULL;

	/* enumerate all languages */
	for(; enum_value->value_name; ++enum_value) {
		GtkWidget *child;

		/* add language if supported */
		if( gydp_dict_lang(dict, enum_value->value) ) {
			/* create radion menu item and update group */
			child = gtk_radio_menu_item_new_with_label(group, enum_value->value_nick);
			group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(child));

			g_signal_connect(G_OBJECT(child), "toggled",
					(GCallback)gydp_window_dict_toggled, self);

			/* add radio menu item to menu */
			gtk_menu_shell_append(GTK_MENU_SHELL(self->menu.dict), child);

			/* add radio menu item to list */
			self->menu.dicts = g_slist_prepend(self->menu.dicts, child);

			/* check if we should select item */
			if( lang == (GydpLang)enum_value->value )
				toggle = child;
		}
	}

	/* free enum klass */
	g_type_class_unref(klass);

	/* show created dynamic menus */
	gtk_widget_show_all(self->menu.dict);

	/* reverse internal list */
	self->menu.dicts = g_slist_reverse(self->menu.dicts);

	/* if selected language is not supported */
	if( toggle == NULL ) {
		toggle = g_slist_last(group)->data;

		if( lang != GYDP_LANG_NONE )
			g_printerr("Error loading selected language '%s', using first available.\n",
					gydp_lang_value_to_nick(lang));
	}

	/* toggle selected dictionary item (force toggle callback if needed */
	if( gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(toggle)) )
		gydp_window_dict_toggled(GTK_CHECK_MENU_ITEM(toggle), self);
	else
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(toggle), TRUE);
}

static void gydp_window_words_select(GydpWindow *self, gint value, gint offset) {
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->words));
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->words));
	GtkTreeIter it;

	/* check if we should scroll */
	if( value >= 0 &&
			value != gtk_range_get_value(GTK_RANGE(self->words_scroll)) ) {
		/* set invalid selected item (prevent emitting signal) and scroll */
		self->words_selected = -1;
		gtk_tree_selection_unselect_all(selection);
		gtk_range_set_value(GTK_RANGE(self->words_scroll), value);

		/* validate that internal selection is same */
		g_assert( self->words_selected == -1 );
	}

	/* find n-th item on list and select it if present */
	if( offset >= 0 &&
			gtk_tree_model_iter_nth_child(model, &it, NULL, offset) ) {
		/* select item and save internal selection state */
		gtk_tree_selection_select_iter(selection, &it);
	} else {
		/* specify invalid selected item */
		self->words_selected = -1;
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

static void gydp_window_word_sync(GydpWindow *self) {
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->words));
	GtkTreeModel *model;
	GtkTreeIter it;
	const gchar *entry;
	guint length, offset;
	gchar *word;

	if( gtk_tree_selection_get_selected(selection, &model, &it) ) {
		/* extract current position in edit */
		entry = gtk_entry_get_text(GTK_ENTRY(self->word));
		offset = gtk_editable_get_position(GTK_EDITABLE(self->word));
		length = g_utf8_strlen(entry, -1);

		gtk_tree_model_get(model, &it, COLUMN_WORD, &word, -1);

		/* block signals */
		g_signal_handlers_block_by_func(G_OBJECT(self->word),
				G_CALLBACK(gydp_window_word_changed), self);

		gtk_entry_set_text(GTK_ENTRY(self->word), word);
		gtk_editable_set_position(GTK_EDITABLE(self->word), offset);

		/* unblock signals */
		g_signal_handlers_unblock_by_func(G_OBJECT(self->word),
				G_CALLBACK(gydp_window_word_changed), self);

		g_free(word);
	}
}

static void gydp_window_words_update(GydpWindow *self) {
	GydpDict *dict = GYDP_DICT(g_object_get_data(gydp_app(), GYDP_APP_DICT));
	GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(self->words));
	GtkTreeIter it;
	gdouble position;
	gint cell_height, separator;

	/* get scroll for word list */
	GtkAdjustment *adjustment = gtk_range_get_adjustment(GTK_RANGE(self->words_scroll));

	/* top items on word list */
	g_object_get(G_OBJECT(adjustment), "value", &position, NULL);

	/* get single cell size */
	GtkTreeViewColumn *column = gtk_tree_view_get_column(GTK_TREE_VIEW(self->words), 0);
	gtk_tree_view_column_cell_get_size(column, NULL, NULL, NULL, NULL, &cell_height);
	gtk_widget_style_get(self->words, "vertical-separator", &separator, NULL);
	cell_height += separator;

	/* clear store */
	gtk_list_store_clear(GTK_LIST_STORE(store));

	/* NOTICE:
	 * setting store explicitely sends selection-changed signal */

	const gint size = self->words->allocation.height / cell_height;
	for(gint i = 0; i < size; ++i) {
		gint id = i + (gint)position;
		const char *word = gydp_dict_word(dict, id);

		gtk_list_store_append(GTK_LIST_STORE(store), &it);
		gtk_list_store_set(GTK_LIST_STORE(store), &it,
				COLUMN_WORD, word,
				COLUMN_ID, id, -1);
	}

	/* adjust scroll (very important rounding) */
	g_object_set(G_OBJECT(adjustment),
			"page-size", (gdouble)(gint)size,
			"page-increment", (gdouble)(gint)(0.9 * size), NULL);

	/* set small as possible (permit full user resize) */
	gtk_widget_set_size_request(self->words, -1, 0);
}

/** gydp_window_words_update_select
 * automatically select item if it is visible on our "hold-only-needed"
 * GtkTreeView, used for scrolling and moving pages with keys
 */
static void gydp_window_words_update_select(GydpWindow *self) {
	GtkAdjustment *adjustment = gtk_range_get_adjustment(GTK_RANGE(self->words_scroll));
	gdouble value, size;

	/* obtain adjustment properties */
	g_object_get(G_OBJECT(adjustment), "value", &value, "page-size", &size, NULL);

	/* select item if we are in corrent range */
	if( self->words_selected >= (gint)value &&
			self->words_selected < (gint)(value + size) )
		gydp_window_words_select(self, -1, self->words_selected - (gint)value);
}

