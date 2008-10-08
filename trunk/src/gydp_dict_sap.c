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
#include "gydp_dict_sap.h"
#include "gydp_util.h"
#include "gydp_conf.h"
#include "gydp_app.h"

#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

typedef struct GydpDictSAPWord {
	gchar *str;    /* word data */
	gsize offset;  /* definition offset */
	gsize length;  /* definition length */
} GydpDictSAPWord;

struct GydpDictSAPClass {
	GydpDictClass __parent__;
};

struct GydpDictSAP {
	GydpDict __parent__;

	/* dictionary stream */
	GInputStream *stream;

	/* dictionary data */
	GydpDictSAPWord *word;
	gsize words;
};

/* perent class holder */
static GObjectClass *gydp_dict_sap_parent_class = NULL;

/* private functions */
static void     gydp_dict_sap_init       (GydpDictSAP *self);
static void     gydp_dict_sap_class_init (GydpDictSAPClass *klass);
static GObject *gydp_dict_sap_constructor(GType type, guint n, GObjectConstructParam *properties);
static void     gydp_dict_sap_finalize   (GObject *object);

/* virtual functions */
static gboolean     gydp_dict_sap_load(GydpDict *dict, gchar **locations, GydpLang lang);
static gboolean     gydp_dict_sap_lang(GydpDict *dict, GydpLang lang);
static guint        gydp_dict_sap_size(GydpDict *dict);
static const gchar *gydp_dict_sap_word(GydpDict *dict, guint n);
static gboolean     gydp_dict_sap_text(GydpDict *dict, guint n, GtkTextBuffer *buffer);

/* private utility functions */
static void         gydp_dict_sap_unload(GydpDictSAP *dict);

/* external private conversion functions */
gchar    *gydp_convert_sap       (const gchar *text);
gboolean  gydp_convert_sap_widget(const gchar *word, const gchar *text, gsize len, GtkTextBuffer *widget);

GType gydp_dict_sap_get_type() {
	static GType type = G_TYPE_INVALID;
	if( G_UNLIKELY( type == G_TYPE_INVALID ) ) {
		static const GTypeInfo info = {
			sizeof(GydpDictSAPClass),                         /* class size */
			NULL, NULL,                                       /* base init, finalize */
			(GClassInitFunc) gydp_dict_sap_class_init, NULL,  /* class init, finalize */
			NULL,                                             /* class init, finalize user_data */
			sizeof(GydpDictSAP), 0,                           /* base size, prealloc size */
			(GInstanceInitFunc) gydp_dict_sap_init,           /* instance init */
			NULL,                                             /* GValue table */
		};

		type = g_type_register_static(GYDP_TYPE_DICT, "GydpDictSAP", &info, 0);
	}

	return type;
}

GObject *gydp_dict_sap_new() {
	return g_object_new(GYDP_TYPE_DICT_SAP, NULL);
}

static void gydp_dict_sap_init(GydpDictSAP *self) {
	GYDP_DICT(self)->engine = GYDP_ENGINE_SAP;
	GYDP_DICT(self)->language = GYDP_LANG_NONE;
}

static void gydp_dict_sap_class_init(GydpDictSAPClass *klass) {
	/* determine parent class */
	gydp_dict_sap_parent_class = g_type_class_peek_parent(klass);

	GObjectClass *gobject_klass = G_OBJECT_CLASS(klass);
	gobject_klass->constructor = gydp_dict_sap_constructor;
	gobject_klass->finalize = gydp_dict_sap_finalize;

	GydpDictClass *dict_klass = GYDP_DICT_CLASS(klass);
	dict_klass->load = gydp_dict_sap_load;
	dict_klass->lang = gydp_dict_sap_lang;

	dict_klass->size = gydp_dict_sap_size;
	dict_klass->word = gydp_dict_sap_word;
	dict_klass->text = gydp_dict_sap_text;
	dict_klass->find = gydp_dict_find_f;
}

static GObject *gydp_dict_sap_constructor(GType type, guint n, GObjectConstructParam *properties) {
	GObject *object = NULL;

	/* chain to parent constructor */
	object = gydp_dict_sap_parent_class->constructor(type, n, properties);

	return object;
}

static void gydp_dict_sap_finalize(GObject *object) {
	GydpDictSAP *self = GYDP_DICT_SAP(object);

	/* unload dictionary */
	gydp_dict_sap_unload(self);

	/* chain to parent finalize */
	gydp_dict_sap_parent_class->finalize(object);
}

static gboolean gydp_dict_sap_load(GydpDict *dict, gchar **locations, GydpLang lang) {

	/* validation */
	g_return_val_if_fail(GYDP_IS_DICT_SAP(dict), FALSE);
	g_return_val_if_fail(dict->engine == GYDP_ENGINE_SAP, FALSE);

	GydpDictSAP *self = GYDP_DICT_SAP(dict);
	const char *filename = NULL;

	/* close previously opened dictionary */
	gydp_dict_sap_unload(self);

	/* get location */
	switch( lang ) {
	case GYDP_LANG_ENG_TO_POL:   filename = "dvp_1.dic"; break;
	case GYDP_LANG_ENG_FROM_POL: filename = "dvp_2.dic"; break;
	default:
		g_printerr("Language '%s' is not supported by SAP engine.\n",
				gydp_lang_value_to_name(lang));
		return FALSE;
	}

	/* load variables */
	gboolean if_ok = FALSE, if_error = FALSE;
	gsize offset_words = 0;
	guint32 *offset = NULL;
	GInputStream *stream = NULL;

	/* load dictionary */
	while( TRUE ) {
		guint32 magic, words, pages;
		gchar page[16384];

		/* try to open input streams */
		for(; *locations != NULL; ++locations)
			if( (stream = gydp_file_open(*locations, filename)) != NULL )
				break;

		/* detect failure and save data stream */
		if( (self->stream = stream) == NULL )
			break;

		/* read main header and validate format */
		if( g_input_stream_read(self->stream, &magic, 4, NULL, NULL) != 4 ||
				GUINT32_FROM_LE(magic) != 0xFADEABBA ||
				g_input_stream_read(self->stream, &words, 4, NULL, NULL) != 4 ||
				g_input_stream_read(self->stream, &pages, 4, NULL, NULL) != 4 )
			break;

		words = GUINT32_FROM_LE(words);
		pages = GUINT32_FROM_LE(pages);

		/* allocate data in dictionary */
		self->word = g_malloc0(words * sizeof(GydpDictSAPWord ));
		self->words = words;

		/* read offsets */
		offset = g_malloc(pages * sizeof(guint32));
		if( g_input_stream_read(self->stream, offset, pages * 4, NULL, NULL) != (gssize)(pages * 4) )
			break;

		for(gsize i = 0; i < pages; ++i)
			offset[i] = GUINT32_FROM_LE(offset[i]);

		/* read pages */
		for(gsize i = 0; i < pages; ++i, if_error = FALSE) {
			guint16 page_words, page_size, page_offset;

			/* assume error */
			if_error = TRUE;

			/* read page header */
			if( !g_seekable_seek(G_SEEKABLE(self->stream), offset[i], G_SEEK_SET, NULL, NULL) ||
					g_input_stream_read(self->stream, &page_words, 2, NULL, NULL) != 2 ||
					g_input_stream_read(self->stream, &page_size, 2, NULL, NULL) != 2 ||
					g_input_stream_read(self->stream, &page_offset, 2, NULL, NULL) != 2 )
				break;

			page_words = GUINT16_FROM_LE(page_words);
			page_size = GUINT16_FROM_LE(page_size);
			page_offset = GUINT16_FROM_LE(page_offset);

			/* read page */
			if( g_input_stream_read(self->stream, page, page_size, NULL, NULL) != (gssize)page_size )
				break;

			/* extract word links and definition offsets */
			gchar *page_word = page + page_words * sizeof(gint16);
			gint16 *length = (gint16 *)page;
			gsize definition_offset = offset[i] + page_offset + 6;
			for(gsize x = 0; x < page_words; definition_offset += length[x], ++x) {
				/* fill all word fields */
				self->word[x + offset_words].str = gydp_convert_sap(page_word);
				self->word[x + offset_words].offset = definition_offset;
				self->word[x + offset_words].length = length[x];

				/* move to next word */
				page_word += strlen(page_word) + 1;
			}

			/* update offset */
			offset_words += page_words;
		}

		/* check if all pages load correctly */
		if( if_error )
			break;

		/* confirm successful read */
		if_ok = TRUE;
		break;
	}

	g_free(offset);

	/* check if import was correct */
	if( !if_ok || if_error ) {
		gydp_dict_sap_unload(self);

		if( *locations == NULL )
			g_printerr("Error loading '%s' dictionary by SAP engine. Missing dictionary files.\n",
					gydp_lang_value_to_nick(lang));
		else
			g_printerr("Error loading '%s' dictionary by SAP engine at '%s'.\n",
					gydp_lang_value_to_nick(lang), *locations);

		return FALSE;
	}

	/* set current language */
	dict->language = lang;

	return TRUE;
}

static gboolean gydp_dict_sap_lang(GydpDict *dict G_GNUC_UNUSED, GydpLang lang) {
	/* check supported languages */
	if( lang == GYDP_LANG_ENG_TO_POL ||
			lang == GYDP_LANG_ENG_FROM_POL )
		return TRUE;

	return FALSE;
}

static guint gydp_dict_sap_size(GydpDict *dict) {
	GydpDictSAP *self = GYDP_DICT_SAP(dict);
	return self->words;
}

static const gchar *gydp_dict_sap_word(GydpDict *dict, guint n) {
	GydpDictSAP *self = GYDP_DICT_SAP(dict);

	if( n >= self->words )
		return NULL;
	return self->word[n].str;
}

static gboolean gydp_dict_sap_text(GydpDict *dict, guint n, GtkTextBuffer *buffer) {
	GydpDictSAP *self = GYDP_DICT_SAP(dict);
	GtkTextIter begin, end;

	/* clear buffer */
	gtk_text_buffer_get_start_iter(buffer, &begin);
	gtk_text_buffer_get_end_iter(buffer, &end);
	gtk_text_buffer_delete(buffer, &begin, &end);

	if( n >= self->words )
		return FALSE;

	/* variadic array for definition */
	gchar text[ self->word[n].length ];

	if( !g_seekable_seek(G_SEEKABLE(self->stream), self->word[n].offset, G_SEEK_SET, NULL, NULL) ||
			g_input_stream_read(self->stream, text, self->word[n].length, NULL, NULL) != (gssize)self->word[n].length )
		return FALSE;

	/* convert raw text to buffer */
	gydp_convert_sap_widget(self->word[n].str, text, self->word[n].length, buffer);

	return TRUE;
}

static void gydp_dict_sap_unload(GydpDictSAP *dict) {

	/* validation */
	g_return_if_fail(GYDP_IS_DICT_SAP(dict));
	g_return_if_fail(GYDP_DICT(dict)->engine == GYDP_ENGINE_SAP);

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

