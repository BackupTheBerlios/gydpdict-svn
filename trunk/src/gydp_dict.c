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

#include "gydp_dict.h"
#include "gydp_conf.h"
#include "gydp_app.h"

#include <string.h>

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

gchar *gydp_str_process(const gchar *str) {
	gchar *result, *begin;

	/* process case */
	begin = g_utf8_casefold(str, -1);

	/* remove some characters */
	for(result = begin, str = begin; *str; ++str)
		switch( *str ) {
		case '/':
		case '.':
		case '-':
		case ' ': break;
		case '&': *(result++) = 'a'; break;
		default:  *(result++) = *str; break;
	}
	/* finalize copy */
	*result = '\0';

	/* normalize characters */
	result = g_utf8_normalize(begin, -1, G_NORMALIZE_DEFAULT);

	/* free temporary string */
	g_free(begin);

	return result;
}

guint gydp_dict_find_f(GydpDict *dict, const gchar *word) {
  GydpDictClass *klass = GYDP_DICT_GET_CLASS(dict);
	gint length, i = 0, prev = 0;
	guint pos = 0, size, n;
	gchar *find;

  /* validate size */
	if( (size = klass->size(dict)) == 0 )
		return 0;

	/* process word for comprison */
	word = gydp_str_process(word);
	length = strlen(word);

	/* check for string length */
	if( length == 0 ) {
		g_free((gchar *)word);
		return pos;
	}

	for(n = 0; n < size; ++n, prev = i) {
		/* get word */
		find = gydp_str_process(klass->word(dict, n));

		/* compare */
		for(i = 0; i < length; ++i)
			if( word[i] != find[i] )
				break;

		/* free temporart data */
		g_free(find);

		/* completely compatible item found */
		if( i == length ) {
			pos = n;
			break;
		}

		/* previous item is more compatible */
		if( i < prev ) {
			pos = n - 1;
			break;
		}
	}

	/* free temporart data */
	g_free((gchar *)word);

	return n < size? pos: size - 1;
}

