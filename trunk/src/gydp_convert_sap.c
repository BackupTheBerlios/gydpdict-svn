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

#include "gydp_global.h"
#include <gtk/gtktextbuffer.h>
#include <string.h>

typedef enum GydpSAPStyle {
	GYDP_SAP_NONE    = 0,
	GYDP_SAP_BOLD    = 1 << 0,
	GYDP_SAP_ITALIC  = 1 << 1,
	GYDP_SAP_SUPER   = 1 << 2,
	GYDP_SAP_RED     = 1 << 3,
	GYDP_SAP_GREEN   = 1 << 4,
	GYDP_SAP_BLUE    = 1 << 5,
} GydpSAPStyle;

typedef struct GydpSAPContext {
	const gchar *word;     /* word to translate */
	const gchar *sap;      /* translation text to convert */
	gsize len;             /* length of translation text */
	GtkTextBuffer *widget; /* output buffer */
	GString *text;         /* raw text without control characters (utf8) */
} GydpSAPContext;

/* structure management functions */
static GydpSAPContext *gydp_sap_context_new (const gchar *word, const gchar *text, gsize len, GtkTextBuffer *widget);
static void            gydp_sap_context_free(GydpSAPContext *self);

/* processing functions */
static void gydp_sap_parse        (GydpSAPContext *context);
static void gydp_sap_parse_type   (GydpSAPContext *context, gsize pos);
static void gydp_sap_append_text  (GydpSAPContext *context, const gchar *text);  /* append utf8 text */
static void gydp_sap_append_text_c(GydpSAPContext *context, gchar character);    /* append native character */
static void gydp_sap_commit_text  (GydpSAPContext *context, GydpSAPStyle style);

/* internal conversion functions */
gchar    *gydp_convert_sap       (const gchar *text);
gboolean  gydp_convert_sap_widget(const gchar *word, const gchar *text, gsize len, GtkTextBuffer *widget);

static const gchar *gydp_sap_encoding_iso88592[128] = {
	"?", "?", "?", "?", "?", "?", "?", "?",
	"?", "?", "?", "?", "?", "?", "?", "?",
	"?", "?", "?", "?", "?", "?", "?", "?",
	"?", "?", "?", "?", "?", "?", "?", "?",
	" ", "Ą", "˘", "Ł", "¤", "Ľ", "Ś", "§",
	"¨", "Š", "Ş", "Ť", "Ź", "­", "Ž", "Ż",
	"°", "ą", "˛", "ł", "´", "ľ", "ś", "ˇ",
	"¸", "š", "ş", "ť", "ź", "˝", "ž", "ż",
	"Ŕ", "Á", "Â", "Ă", "Ä", "Ĺ", "Ć", "Ç",
	"Č", "É", "Ę", "Ë", "Ě", "Í", "Î", "Ď",
	"Đ", "Ń", "Ň", "Ó", "Ô", "Ő", "Ö", "×",
	"Ř", "Ů", "Ú", "Ű", "Ü", "Ý", "Ţ", "ß",
	"ŕ", "á", "â", "ă", "ä", "ĺ", "ć", "ç",
	"č", "é", "ę", "ë", "ě", "í", "î", "ď",
	"đ", "ń", "ň", "ó", "ô", "ő", "ö", "÷",
	"ř", "ů", "ú", "ű", "ü", "ý", "ţ", "˙",
};

gchar *gydp_convert_sap(const gchar *text) {
	const gchar **convert = gydp_sap_encoding_iso88592;
	gchar *buffer;
	gsize len = 0;

	/* obtain required length for buffer */
	for(const guchar *pos = (const guchar *)text; *pos; ++pos) {
		if( *pos < 128 )
			len += 1;
		else
			len += strlen(gydp_sap_encoding_iso88592[*pos - 128]);
	}

	/* allocate buffer  */
	buffer = g_malloc(len + 1);

	/* convert characters */
	for(gchar *pos = buffer; TRUE; ) {
		const guchar c = *(text++);

		if( c < 128 )
			*(pos++) = c;
		else {
			switch( strlen(convert[c - 128]) ) {
			case 1: *(pos++) = *convert[c - 128]; break;
			case 2: memcpy(pos, convert[c - 128], 2); pos += 2; break;
			case 3: memcpy(pos, convert[c - 128], 3); pos += 3; break;
			default: g_free(buffer); g_return_val_if_reached(NULL); break;
			}
		}

		/* check for terminator */
		if( !c )
			break;
	}

	return buffer;
}

gboolean gydp_convert_sap_widget(const gchar *word, const gchar *text, gsize len, GtkTextBuffer *widget) {

	/* allocate and initialize context */
	GydpSAPContext *context = gydp_sap_context_new(word, text, len, widget);

	/* parse data in context */
	gydp_sap_parse(context);

	/* free context */
	gydp_sap_context_free(context);

	return TRUE;
}

GydpSAPContext *gydp_sap_context_new(const gchar *word, const gchar *text, gsize len, GtkTextBuffer *widget) {
	/* allocate context */
	GydpSAPContext *self = g_malloc(sizeof(GydpSAPContext));

	/* initialize input parameters */
	self->word = word;
	self->sap = text;
	self->len = len;
	self->widget = widget;

	/* initalize context state */
	self->text = g_string_sized_new(128);

	return self;
}

void gydp_sap_context_free(GydpSAPContext *self) {
	if( self != NULL ) {
		/* free strings */
		g_string_free(self->text, TRUE);

		/* free self */
		g_free(self);
	}
}

static void gydp_sap_parse(GydpSAPContext *context) {
	for(gsize i = 0; i < context->len; ++i) {
		switch( context->sap[i] ) {
		case ')':
			gydp_sap_append_text_c(context, context->sap[i]);
			gydp_sap_append_text_c(context, ' ');
			break;
		case '{':
			gydp_sap_commit_text(context, GYDP_SAP_NONE);
			break;
		case '}':
			gydp_sap_commit_text(context, GYDP_SAP_BOLD);
			gydp_sap_append_text(context, " - ");
			break;
		case '-':
			gydp_sap_append_text(context, " - ");
			break;
		case '*':
			gydp_sap_append_text(context, context->word);
			break;
		case '$':
			gydp_sap_append_text(context, "\n • ");
			break;
		case '#':
			gydp_sap_commit_text(context, GYDP_SAP_NONE);
			gydp_sap_parse_type(context, i);
			gydp_sap_commit_text(context, GYDP_SAP_ITALIC | GYDP_SAP_BLUE);
			i += 2; /* adjust position, (type specification length) */
			break;
		default:
			gydp_sap_append_text_c(context, context->sap[i]);
			break;
		}
	}

	/* commit pending text */
	gydp_sap_commit_text(context, GYDP_SAP_NONE);
}

static void gydp_sap_parse_type(GydpSAPContext *context, gsize pos) {
	/* all text is utf8 encoded */
	static struct type { char *type; guint16 id, mask; } type[] = {
		{ "przymiotnik",             0x0001, 0x000f },
		{ "przysłówek",              0x0002, 0x000f },
		{ "spójnik",                 0x0003, 0x000f },
		{ "liczebnik",               0x0004, 0x000f },
		{ "partykuła",               0x0005, 0x000f },
		{ "przedrostek",             0x0006, 0x000f },
		{ "przyimek",                0x0007, 0x000f },
		{ "zaimek",                  0x0008, 0x000f },
		{ "rzeczownik",              0x0009, 0x000f },
		{ "czasownik posiłkowy",     0x000a, 0x000f },
		{ "czasownik nieprzechodni", 0x000b, 0x000f },
		{ "czasownik nieosobowy",    0x000c, 0x000f },
		{ "czasownik zwrotny",       0x000d, 0x000f },
		{ "czasownik przechodni",    0x000e, 0x000f },
		{ "czasownik",               0x000f, 0x000f },

		{ "rodzaj żeński",           0x0010, 0x0030 },
		{ "rodzaj męski",            0x0020, 0x0030 },
		{ "rodzaj nijaki",           0x0030, 0x0030 },

		{ "liczba pojedyncza",       0x0040, 0x00c0 },
		{ "liczba mnoga",            0x0080, 0x00c0 },
		{ "tylko liczba mnoga",      0x00c0, 0x00c0 },

		{ "regularny",               0x0100, 0x0100 },
		{ "skrót",                   0x0200, 0x0200 },
		{ "wyraz potoczny",          0x0400, 0x0400 },

		{ "czas przeszły",           0x0800, 0x7800 },
		{ "czas teraźniejszy",       0x1000, 0x7800 },
		{ "czas przyszły",           0x1800, 0x7800 },
		{ "bezokolicznik",           0x2000, 0x7800 },

		{ "stopień najwyższy",       0x4000, 0x7800 },
		{ "stopień wyższy",          0x6000, 0x7800 },

		{ NULL,                      0,      0 },
	};

	if( context->sap[pos] == '#' ) {
		gboolean multi = FALSE;

		/* extract item it */
		guint16 id = ((guint8)context->sap[pos+1] << 8) + (guint8)context->sap[pos+2];

		for(gsize i = 0; type[i].type != NULL; ++i)
			if( ( id & type[i].mask ) == type[i].id ) {
				/* check if multiple types are set */
				if( multi )
					gydp_sap_append_text(context, ", ");
				multi = TRUE;

				/* append type */
				gydp_sap_append_text(context, type[i].type);
			}

		gydp_sap_append_text_c(context, ' ');
	}
}

static void gydp_sap_append_text(GydpSAPContext *context, const gchar *text) {
	g_string_append(context->text, text);
}

static void gydp_sap_append_text_c(GydpSAPContext *context, gchar character) {
	if( character >= 0 )
		g_string_append_c(context->text, character);
	else {
		const gchar **convert = gydp_sap_encoding_iso88592;
		const gsize i = (guchar)character - 128;

		switch( strlen(convert[i]) ) {
		case 1: g_string_append_c(context->text, *convert[i]); break;
		case 2: g_string_append_len(context->text, convert[i], 2); break;
		case 3: g_string_append_len(context->text, convert[i], 3); break;
		default: g_return_if_reached(); break;
		}
	}
}

static void gydp_sap_commit_text(GydpSAPContext *context, GydpSAPStyle style) {
	GtkTextIter begin, end;
	GtkTextMark *mark;

	/* skip if no text present in context */
	if( !context->text->len )
		return;

	if( style ) {
		/* get insert position and set mark */
		gtk_text_buffer_get_end_iter(context->widget, &end);
		mark = gtk_text_buffer_create_mark(context->widget, NULL, &end, TRUE);

		/* insert text */
		gtk_text_buffer_insert(context->widget, &end, context->text->str, -1);

		/* extract begin iter and delete mark */
		gtk_text_buffer_get_iter_at_mark(context->widget, &begin, mark);
		gtk_text_buffer_delete_mark(context->widget, mark);

		/*
		 * apply styles
		 */

		if( style & GYDP_SAP_BOLD )
			gtk_text_buffer_apply_tag_by_name(context->widget,
					GYDP_TAG_BOLD, &begin, &end);

		if( style & GYDP_SAP_ITALIC )
			gtk_text_buffer_apply_tag_by_name(context->widget,
					GYDP_TAG_ITALIC, &begin, &end);

		if( style & GYDP_SAP_RED )
			gtk_text_buffer_apply_tag_by_name(context->widget,
					GYDP_TAG_COLOR_RED, &begin, &end);

		if( style & GYDP_SAP_GREEN )
			gtk_text_buffer_apply_tag_by_name(context->widget,
					GYDP_TAG_COLOR_GREEN, &begin, &end);

		if( style & GYDP_SAP_BLUE )
			gtk_text_buffer_apply_tag_by_name(context->widget,
					GYDP_TAG_COLOR_BLUE, &begin, &end);

	} else {
		/* insert clean text at end of buffer */
		gtk_text_buffer_get_end_iter(context->widget, &end);
		gtk_text_buffer_insert(context->widget, &end, context->text->str, -1);
	}

	/* remove commited text */
	g_string_truncate(context->text, 0);
}

