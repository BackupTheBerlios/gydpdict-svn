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
#include "gydp_dict_ydp.h"
#include "gydp_util.h"
#include "gydp_conf.h"
#include "gydp_app.h"

#include <stdlib.h>
#include <string.h>

typedef struct GydpDictYDPWord {
	gchar *str;    /* word data */
	gsize offset;  /* definition offset */
} GydpDictYDPWord;

struct GydpDictYDPClass {
	GydpDictClass __parent__;
};

struct GydpDictYDP {
	GydpDict __parent__;

	/* dictionary stream */
	GInputStream *stream;

	/* dictionary data */
	GydpDictYDPWord *word;
	gsize words;
};

/* perent class holder */
static GObjectClass *gydp_dict_ydp_parent_class = NULL;

/* private functions */
static void     gydp_dict_ydp_init       (GydpDictYDP *self);
static void     gydp_dict_ydp_class_init (GydpDictYDPClass *klass);
static GObject *gydp_dict_ydp_constructor(GType type, guint n, GObjectConstructParam *properties);
static void     gydp_dict_ydp_finalize   (GObject *object);

/* virtual functions */
static gboolean     gydp_dict_ydp_load(GydpDict *dict, gchar **locations, GydpLang lang);
static gboolean     gydp_dict_ydp_lang(GydpDict *dict, GydpLang lang);
static guint        gydp_dict_ydp_size(GydpDict *dict);
static const gchar *gydp_dict_ydp_word(GydpDict *dict, guint n);
static gboolean     gydp_dict_ydp_text(GydpDict *dict, guint n, GtkTextBuffer *buffer);

/* private utility functions */
static void         gydp_dict_ydp_unload(GydpDictYDP *dict);

/* external private conversion functions */
gchar    *gydp_convert_ydp       (const gchar *text);
void      gydp_convert_ydp_buffer(const gchar *text, gboolean phonetic, gchar *buffer);
gboolean  gydp_convert_ydp_widget(const gchar *word, const gchar *text, GtkTextBuffer *widget);

GType gydp_dict_ydp_get_type() {
	static GType type = G_TYPE_INVALID;
	if( G_UNLIKELY( type == G_TYPE_INVALID ) ) {
		static const GTypeInfo info = {
			sizeof(GydpDictYDPClass),                         /* class size */
			NULL, NULL,                                       /* base init, finalize */
			(GClassInitFunc) gydp_dict_ydp_class_init, NULL,  /* class init, finalize */
			NULL,                                             /* class init, finalize user_data */
			sizeof(GydpDictYDP), 0,                           /* base size, prealloc size */
			(GInstanceInitFunc) gydp_dict_ydp_init,           /* instance init */
			NULL,                                             /* GValue table */
		};

		type = g_type_register_static(GYDP_TYPE_DICT, "GydpDictYDP", &info, 0);
	}

	return type;
}

GObject *gydp_dict_ydp_new() {
	return g_object_new(GYDP_TYPE_DICT_YDP, NULL);
}

static void gydp_dict_ydp_init(GydpDictYDP *self) {
	GYDP_DICT(self)->engine = GYDP_ENGINE_YDP;
	GYDP_DICT(self)->language = GYDP_LANG_NONE;
}

static void gydp_dict_ydp_class_init(GydpDictYDPClass *klass) {
	/* determine parent class */
	gydp_dict_ydp_parent_class = g_type_class_peek_parent(klass);

	GObjectClass *gobject_klass = G_OBJECT_CLASS(klass);
	gobject_klass->constructor = gydp_dict_ydp_constructor;
	gobject_klass->finalize = gydp_dict_ydp_finalize;

	GydpDictClass *dict_klass = GYDP_DICT_CLASS(klass);
	dict_klass->load = gydp_dict_ydp_load;
	dict_klass->lang = gydp_dict_ydp_lang;

	dict_klass->size = gydp_dict_ydp_size;
	dict_klass->word = gydp_dict_ydp_word;
	dict_klass->text = gydp_dict_ydp_text;
	dict_klass->find = gydp_dict_find_f;
}

static GObject *gydp_dict_ydp_constructor(GType type, guint n, GObjectConstructParam *properties) {
	GObject *object = NULL;

	/* chain to parent constructor */
	object = gydp_dict_ydp_parent_class->constructor(type, n, properties);

	return object;
}

static void gydp_dict_ydp_finalize(GObject *object) {
	GydpDictYDP *self = GYDP_DICT_YDP(object);

	/* unload dictionary */
	gydp_dict_ydp_unload(self);

	/* chain to parent finalize */
	gydp_dict_ydp_parent_class->finalize(object);
}

static gboolean gydp_dict_ydp_load(GydpDict *dict, gchar **locations, GydpLang lang) {

	/* dictionary file names */
	static const gchar *filenames[] = {
		"DICT100.DAT", "DICT100.IDX", NULL,
		"DICT101.DAT", "DICT101.IDX", NULL,
	};

	/* validation */
	g_return_val_if_fail(GYDP_IS_DICT_YDP(dict), FALSE);
	g_return_val_if_fail(dict->engine == GYDP_ENGINE_YDP, FALSE);

	GydpDictYDP *self = GYDP_DICT_YDP(dict);
	const gchar **filename = NULL;

	/* close previously opened dictionary */
	gydp_dict_ydp_unload(self);

	/* get location */
	switch( lang ) {
	case GYDP_LANG_ENG_TO_POL:   filename = &filenames[0]; break;
	case GYDP_LANG_ENG_FROM_POL: filename = &filenames[3]; break;
	default:
		g_printerr("Language '%s' is not supported by YDP engine.\n",
				gydp_lang_value_to_name(lang));

		/* indicate that dictionary changed */
		gydp_dict_changed(dict);

		return FALSE;
	}

	/* load variables */
	gboolean if_ok = FALSE, if_error = FALSE;
	GString *buffer = g_string_sized_new(128);
	GInputStream *index = NULL, *stream = NULL;

	while( TRUE ) {
		guint16 words;
		guint32 offset;

		/* try to open input streams */
		for(; *locations != NULL; ++locations) {
			if( (stream = gydp_file_open(*locations, filename[0])) != NULL &&
					(index = gydp_file_open(*locations, filename[1])) != NULL )
				break;

			if( stream != NULL ) {
				g_input_stream_close(stream, NULL, NULL);
				stream = NULL;
			}

			if( index != NULL ) {
				g_input_stream_close(index, NULL, NULL);
				index = NULL;
			}
		}

		/* detect failure and save data stream */
		if( (self->stream = stream) == NULL || index == NULL )
			break;

		/* read size of dictionary */
		if( !g_seekable_seek(G_SEEKABLE(index), 8, G_SEEK_SET, NULL, NULL) ||
				g_input_stream_read(index, &words, 2, NULL, NULL) != 2 )
			break;
		words = GUINT16_FROM_LE(words);

		/* allocate data in dictionary */
		self->word = g_malloc0(words * sizeof(GydpDictYDPWord));
		self->words = words;

		/* read index offset */
		if( !g_seekable_seek(G_SEEKABLE(index), 16, G_SEEK_SET, NULL, NULL) ||
				g_input_stream_read(index, &offset, 4, NULL, NULL) != 4 )
			break;
		offset = GUINT32_FROM_LE(offset);

		/* move to index */
		if( !g_seekable_seek(G_SEEKABLE(index), offset, G_SEEK_SET, NULL, NULL) )
			break;

		for(gsize i = 0; i < words; ++i, if_error = FALSE) {
			guint32 length;

			/* assume error */
			if_error = TRUE;

			/* read word properties */
			if( g_input_stream_read(index, &length, 4, NULL, NULL) != 4 ||
				g_input_stream_read(index, &offset, 4, NULL, NULL) != 4 )
				break;
			length = GUINT32_FROM_LE(length) & 0xff;
			offset = GUINT32_FROM_LE(offset);

			/* expand buffer if needed */
			if( length > buffer->allocated_len )
				g_string_set_size(buffer, length);

			/* read word */
			if( g_input_stream_read(index, buffer->str, length, NULL, NULL) != (gssize)length )
				break;

			/* finalize word structure */
			self->word[i].str = gydp_convert_ydp(buffer->str);
			self->word[i].offset = offset;
		}

		/* check if all words load correctly */
		if( if_error )
			break;

		/* confirm successful read */
		if_ok = TRUE;
		break;
	}

	g_string_free(buffer, TRUE);
	if( index ) g_input_stream_close(index, NULL, NULL);

	/* check if import was correct */
	if( !if_ok || if_error ) {
		gydp_dict_ydp_unload(self);

		if( *locations == NULL )
			g_printerr("Error loading '%s' dictionary by YDP engine. Missing dictionary file(s).\n",
					gydp_lang_value_to_nick(lang));
		else
			g_printerr("Error loading '%s' dictionary by YDP engine at '%s'.\n",
					gydp_lang_value_to_nick(lang), *locations);

		/* indicate that dictionary changed */
		gydp_dict_changed(dict);

		return FALSE;
	}

	/* set current language */
	dict->language = lang;

	/* indicate that dictionary changed */
	gydp_dict_changed(dict);

	return TRUE;
}

static gboolean gydp_dict_ydp_lang(GydpDict *dict G_GNUC_UNUSED, GydpLang lang) {
	/* check supported languages */
	if( lang == GYDP_LANG_ENG_TO_POL ||
			lang == GYDP_LANG_ENG_FROM_POL )
		return TRUE;

	return FALSE;
}

static guint gydp_dict_ydp_size(GydpDict *dict) {
	GydpDictYDP *self = GYDP_DICT_YDP(dict);
	return self->words;
}

static const gchar *gydp_dict_ydp_word(GydpDict *dict, guint n) {
	GydpDictYDP *self = GYDP_DICT_YDP(dict);

	if( n >= self->words )
		return NULL;
	return self->word[n].str;
}

static gboolean gydp_dict_ydp_text(GydpDict *dict, guint n, GtkTextBuffer *buffer) {
	GydpDictYDP *self = GYDP_DICT_YDP(dict);
	GtkTextIter begin, end;
	guint32 length;

	/* clear buffer */
	gtk_text_buffer_get_start_iter(buffer, &begin);
	gtk_text_buffer_get_end_iter(buffer, &end);
	gtk_text_buffer_delete(buffer, &begin, &end);

	if( n >= self->words )
		return FALSE;

	if( !g_seekable_seek(G_SEEKABLE(self->stream), self->word[n].offset, G_SEEK_SET, NULL, NULL) ||
			g_input_stream_read(self->stream, &length, 4, NULL, NULL) != 4 )
		return FALSE;
	length = GUINT32_FROM_LE(length);

	/* variadic array for definition */
	gchar text[ length+1 ];

	/* read word definition */
	if( g_input_stream_read(self->stream, text, length, NULL, NULL) != (gssize)length )
		return FALSE;
	text[length] = '\0';

	/* convert raw text to buffer */
	gydp_convert_ydp_widget(self->word[n].str, text, buffer);

	return TRUE;
}

static void gydp_dict_ydp_unload(GydpDictYDP *dict) {

	/* validation */
	g_return_if_fail(GYDP_IS_DICT_YDP(dict));
	g_return_if_fail(GYDP_DICT(dict)->engine == GYDP_ENGINE_YDP);

	/* detach stream */
	if( dict->stream )
		g_input_stream_close(dict->stream, NULL, NULL);

	/* free words and definitions */
	for(gsize i = 0; i < dict->words; ++i)
		g_free(dict->word[i].str);

	/* free arrays */
	g_free(dict->word);

	/* reset data */
	dict->stream = NULL;
	dict->word = NULL;
	dict->words = 0;

	/* reset current dictionary */
	GYDP_DICT(dict)->language = GYDP_LANG_NONE;
}

