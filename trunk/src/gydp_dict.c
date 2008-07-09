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

#include "gydp_dict.h"
#include "gydp_conf.h"
#include "gydp_app.h"

/* private methods */
static void gydp_dict_class_init(GydpDictClass *klass);

GType gydp_dict_get_type() {
	static GType type = G_TYPE_INVALID;
	if( G_UNLIKELY( type == G_TYPE_INVALID ) ) {
		static const GTypeInfo info = {
			sizeof(GydpDictClass),                        /* class size */
			NULL, NULL,                                   /* base init, finalize */
			(GClassInitFunc) gydp_dict_class_init, NULL,  /* class init, finalize */
			NULL,                                         /* class init, finalize user_data */
			sizeof(GydpDict), 0,                          /* base size, prealloc size */
			NULL,                                         /* instance init */
			NULL,                                         /* GValue table */
		};

		type = g_type_register_static(G_TYPE_OBJECT, "GydpDict",
				&info, G_TYPE_FLAG_ABSTRACT);
	}

	return type;
}

static void gydp_dict_class_init(GydpDictClass *klass) {
	/* all methods are pure virtual */
	klass->load = NULL;
	klass->lang = NULL;

	klass->size = NULL;
	klass->word = NULL;
	klass->text = NULL;
	klass->find = NULL;
}

gboolean gydp_dict_load(GydpDict *dict, gchar **locations, GydpLang lang) {
	return GYDP_DICT_GET_CLASS(dict)->load(dict, locations, lang);
}

gboolean gydp_dict_lang(GydpDict *dict, GydpLang lang) {
	return GYDP_DICT_GET_CLASS(dict)->lang(dict, lang);
}

guint gydp_dict_size(GydpDict *dict) {
	return GYDP_DICT_GET_CLASS(dict)->size(dict);
}

const gchar *gydp_dict_word(GydpDict *dict, guint n) {
	return GYDP_DICT_GET_CLASS(dict)->word(dict, n);
}

gboolean gydp_dict_text(GydpDict *dict, guint n, GtkTextBuffer *buffer) {
	return GYDP_DICT_GET_CLASS(dict)->text(dict, n, buffer);
}

guint gydp_dict_find(GydpDict *dict, const gchar *word) {
	return GYDP_DICT_GET_CLASS(dict)->find(dict, word);
}

