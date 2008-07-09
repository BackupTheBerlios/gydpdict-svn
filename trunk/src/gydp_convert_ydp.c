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

typedef enum GydpYDPAlign {
	GYDP_YDP_ALIGN_NONE   = 0,
	GYDP_YDP_ALIGN_LEFT   = 1,
	GYDP_YDP_ALIGN_CENTER = 2,
} GydpYDPAlign;

typedef enum GydpYDPScript {
	GYDP_YDP_SCRIPT_NONE   = 0,
	GYDP_YDP_SCRIPT_NORMAL = 1,
	GYDP_YDP_SCRIPT_SUPER  = 2,
	GYDP_YDP_SCRIPT_SUB    = 3,
} GydpYDPScript;

typedef enum GydpYDPColor {
	GYDP_YDP_COLOR_NONE    = 0,
	GYDP_YDP_COLOR_RED     = 1,
	GYDP_YDP_COLOR_GREEN   = 2,
	GYDP_YDP_COLOR_BLUE    = 3,
} GydpYDPColor;

typedef struct GydpYDPState {
	guint phonetic : 1;           /* encoding: phonetic <-> cp1250 */
	guint bold : 1;               /* bold */
	guint italic : 1;             /* italic */
	guint8 script;                /* type of script */
	guint8 color;                 /* text color */
	/* paragraph properties */
	guint8 align;                 /* text alignment */
	guint margin_left;            /* left margin */
	guint margin_right;           /* right margin */
	guint indent;                 /* indentation */
} GydpYDPState;

typedef struct GydpYDPContext {
	const gchar *word;     /* word to translate */
	const gchar *rtf;      /* translation text to convert */
	GtkTextBuffer *widget; /* output buffer */
	GSList *state;         /* control codes stack */
	GString *control;      /* raw control code */
	GString *text;         /* raw text without control codes */
	GString *buffer;       /* static temporary buffer */
} GydpYDPContext;

/* structure management functions */
GydpYDPState   *gydp_ydp_state_new    ();
GydpYDPState   *gydp_ydp_state_clone  (GydpYDPState *self);
void            gydp_ydp_state_free   (GydpYDPState *self);
GydpYDPContext *gydp_ydp_context_new  (const gchar *word, const gchar *text, GtkTextBuffer *widget);
void            gydp_ydp_context_free (GydpYDPContext *self);

/* processing functions */
static void     gydp_ydp_parse        (GydpYDPContext *context);
static void     gydp_ydp_parse_control(GydpYDPContext *context);
static void     gydp_ydp_push_state   (GydpYDPContext *context);
static void     gydp_ydp_pop_state    (GydpYDPContext *context);

static void     gydp_ydp_commit_text  (GydpYDPContext *context);

/* internal conversion functions */
gchar    *gydp_convert_ydp       (const gchar *text);
void      gydp_convert_ydp_buffer(const gchar *text, gboolean phonetic, gchar *buffer);
gboolean  gydp_convert_ydp_widget(const gchar *word, const gchar *text, GtkTextBuffer *widget);

/* phonetic conversion table */
static const gchar *gydp_ydp_encoding_phonetic[32] = {
	"?", "?", "ɔ", "ʒ", "?", "ʃ", "ɛ", "ʌ",
	"ə", "θ", "ɪ", "ɑ", "?", "ː", "ˈ", "?",
	"ŋ", "?", "?", "?", "?", "?", "?", "ð",
	"æ", "?", "?", "?", "?", "?", "?", "?",
};

/* cp1250 conversion table (with changes/fixes) */
static const gchar *gydp_ydp_encoding_cp1250[128] = {
	"€", "?", "‚", "?", "„", "…", "†", "ci", /* ‡ -> ci */
	"?", "‰", "Š", "‹", "Ś", "Ť", "Ž", "Ż",  /* Ź -> Ż */
	"?", "‘", "’", "“", "”", "•", "–", "—",
	"?", "™", "š", "›", "ś", "ť", "ž", "ź",
	" ", "ˇ", "˘", "Ł", "¤", "Ą", "¦", "§",
	"¨", "©", "Ş", "«", "¬", "­", "®", "Ż",
	"°", "±", "˛", "ł", "´", "µ", "¶", "·",
	"¸", "ą", "ş", "»", "Ľ", "˝", "ľ", "ż",
	"Ŕ", "Á", "Â", "Ă", "Ä", "Ĺ", "Ć", "Ç",
	"Č", "É", "Ę", "Ë", "Ě", "Í", "Î", "Ď",
	"Đ", "Ń", "Ň", "Ó", "Ô", "Ő", "Ö", "×",
	"Ř", "Ů", "Ú", "Ű", "Ü", "Ý", "Ţ", "ß",
	"à", "á", "â", "ă", "ä", "ĺ", "ć", "ç",  /* ŕ -> à */
	"č", "é", "ę", "ë", "ě", "í", "î", "ï",  /* č -> e, ď -> ï */
	"đ", "ń", "ň", "ó", "ô", "ő", "ö", "÷",
	"ř", "ů", "ú", "ű", "ü", "ý", "ţ", "˙",
};

gchar *gydp_convert_ydp(const gchar *text) {
	gchar *buffer;
	gsize len = 0;

	/* obtain required length for buffer */
	for(const guchar *pos = (const guchar *)text; *pos; ++pos) {
		if( *pos < 128 )
			len += 1;
		else
			len += strlen(gydp_ydp_encoding_cp1250[*pos - 128]);
	}

	/* convert text */
	buffer = g_malloc(len + 1);
	gydp_convert_ydp_buffer(text, FALSE, buffer);
	return buffer;
}

void gydp_convert_ydp_buffer(const gchar *text, gboolean phonetic, gchar *buffer) {
	const gchar **convert;
	gsize size;

	/* extract current encoding table */
	if( phonetic ) {
		convert = gydp_ydp_encoding_phonetic;
		size = 128 + G_N_ELEMENTS(gydp_ydp_encoding_phonetic);
	} else {
		convert = gydp_ydp_encoding_cp1250;
		size = 128 + G_N_ELEMENTS(gydp_ydp_encoding_cp1250);
	}

	/* convert characters */
	while( TRUE ) {
		const guchar c = *(text++);

		if( c < 128 || c >= size )
			*(buffer++) = c;
		else {
			switch( strlen(convert[c - 128]) ) {
			case 1: *(buffer++) = *convert[c - 128]; break;
			case 2: memcpy(buffer, convert[c - 128], 2); buffer += 2; break;
			case 3: memcpy(buffer, convert[c - 128], 3); buffer += 3; break;
			default: g_return_if_reached(); break;
			}
		}

		/* check for terminator */
		if( !c )
			break;
	}
}

gboolean gydp_convert_ydp_widget(const gchar *word, const gchar *text, GtkTextBuffer *widget) {

	/* allocate and initialize context */
	GydpYDPContext *context = gydp_ydp_context_new(word, text, widget);

	/* parse data in context */
	gydp_ydp_parse(context);

	/* free context */
	gydp_ydp_context_free(context);

	return TRUE;
}

GydpYDPState *gydp_ydp_state_new() {
	return gydp_ydp_state_clone(NULL);
}

GydpYDPState *gydp_ydp_state_clone(GydpYDPState *self) {
	/* allocate control code */
	GydpYDPState *code = g_slice_new(GydpYDPState);

	/* initialize control code */
	if( self == NULL ) {
		memset(code, 0, sizeof(GydpYDPState));
		code->align = GYDP_YDP_ALIGN_NONE;
		code->script = GYDP_YDP_SCRIPT_NONE;
		code->color = GYDP_YDP_COLOR_NONE;
	} else
		*code = *self;

	return code;
}

void gydp_ydp_state_free(GydpYDPState *self) {
	g_slice_free(GydpYDPState, self);
}

GydpYDPContext *gydp_ydp_context_new(const gchar *word, const gchar *text, GtkTextBuffer *widget) {
	/* allocate context */
	GydpYDPContext *self = g_malloc(sizeof(GydpYDPContext));

	/* initialize input parameters */
	self->word = word;
	self->rtf = text;
	self->widget = widget;

	/* initalize context state */
	self->state = g_slist_prepend(NULL, gydp_ydp_state_new());
	self->control = g_string_sized_new(16);
	self->text = g_string_sized_new(128);
	self->buffer = g_string_sized_new(256);

	return self;
}

void gydp_ydp_context_free(GydpYDPContext *self) {
	if( self != NULL ) {
		/* free control codes */
		g_slist_foreach(self->state, (GFunc)gydp_ydp_state_free, NULL);
		g_slist_free(self->state);

		/* free strings */
		g_string_free(self->control, TRUE);
		g_string_free(self->text, TRUE);
		g_string_free(self->buffer, TRUE);

		/* free self */
		g_free(self);
	}
}

static void gydp_ydp_parse(GydpYDPContext *context) {
	while( *context->rtf ) {
		switch( *context->rtf ) {
		case '{': /* begin group */
			gydp_ydp_push_state(context);
			context->rtf += 1;
			break;
		case '}': /* end group */
			gydp_ydp_pop_state(context);
			context->rtf += 1;
			break;
		case '\\': /* begin control code */
			gydp_ydp_parse_control(context);
			break;
		case 0x7f: /* broken character (DEL) in ydp */
			g_string_append_c(context->text, 0x7e);      /* '~' */
			context->rtf += 1;
			break;
		default:   /* other character */
			g_string_append_c(context->text, *context->rtf);
			context->rtf += 1;
			break;
		}
	};

	/* commit pending global text */
	gydp_ydp_commit_text(context);
}

static void gydp_ydp_parse_control(GydpYDPContext *context) {
	/* copy current position (after start of control code) */
	const gchar *rtf = ++context->rtf;

	/* clear previous control code */
	g_string_truncate(context->control, 0);

	/* check first character of control code */
	switch( *rtf ) {
	case '\\': /* it was escaped character */
	case '{':
	case '}':
		g_string_append_c(context->text, *(rtf++));
		return;
	default: /* read control code */
		for(gboolean is_control = TRUE; is_control; ++rtf)
			switch( *rtf ) {
			case ' ': ++rtf;                      /* omit, control code terminator */
			case '{':                             /* control code terminator */
			case '}':                             /* control code terminator */
			case '\\': is_control = FALSE; break; /* control code terminator */
			case 'a' ... 'z':                     /* control code letter */
			case 'A' ... 'Z':
			case '0' ... '9':                     /* control code parameter */
				g_string_append_c(context->control, *rtf); break;
			default:   is_control = FALSE; break; /* other symbol, terminator */
			}
		break;
	}

	/* empty control code */
	if( !context->control->len )
		return;

	/* extract numeric parameter */
	gint parameter = 0;

	{ /* extract numeric parameter form control code */
		const gssize last = context->control->len - 1;
		gchar *str = context->control->str;

		for(gssize pos = last, mul = 1; pos >= 0; --pos, mul *= 10)
			switch( str[pos] ) {
			case '0' ... '9':
				parameter += (str[pos] - '0') * mul;
				break;
			case '-':              /* negate parameter and break */
				g_string_truncate(context->control, pos);
				parameter *= -1;
				pos = 0;             /* set stop position */
				break;
			default:               /* found letter, complete control word and parameter */
				g_string_truncate(context->control, pos + 1);
				parameter = (pos == last)? 1: parameter;
				pos = 0;             /* set stop position */
				break;
			}
	}

	/* set context position */
	context->rtf = rtf - 1;

	/* extract current state */
	GydpYDPState *state = context->state->data;

	/* parse control code */
	const gchar *str = context->control->str;
	gboolean is_unknown = FALSE;

	switch( context->control->len ) {
	case 1:
		if( !strcmp(str, "b") ) {                      /* bold */
			if( parameter ) state->bold = TRUE;
			else            state->bold = FALSE;
		} else if( !strcmp(str, "i") ) {               /* italic */
			if( parameter ) state->italic = TRUE;
			else            state->italic = FALSE;
		} else if( !strcmp(str, "f") ) {               /* font */
			switch( parameter ) {
			case 0: state->phonetic = FALSE; break;
			case 1: state->phonetic = TRUE; break;
			case 2: state->phonetic = FALSE; break;
			}
		} else
			is_unknown = TRUE;
		break;
	case 2:
		if( !strcmp(str, "cf") ) {                    /* color foreground */
			switch( parameter ) {
			case 0: break;
			case 1: break;
			case 2: state->color = GYDP_YDP_COLOR_BLUE; break;
			case 4: break;
			}
		} else if( !strcmp(str, "cb") ) {             /* color background */
		} else if( !strcmp(str, "fi") ) {             /* first indent */
			state->indent = parameter;
		} else if( !strcmp(str, "li") ) {             /* left indent */
			state->margin_left = parameter;
		} else if( !strcmp(str, "ri") ) {             /* right indent */
			state->margin_right = parameter;
		} else if( !strcmp(str, "sa") ) {             /* space after */
			g_string_append_c(context->text, '\t');
		} else if( !strcmp(str, "sb") ) {             /* space before */
			g_string_prepend_c(context->text, '\t');
		} else if( !strcmp(str, "qc") ) {             /* center */
			state->align = GYDP_YDP_ALIGN_CENTER;
		} else
			is_unknown = TRUE;
		break;
	case 3:
		if( !strcmp(str, "par") ) {                    /* paragraph */
			g_string_append_c(context->text, '\n');
		} else
			is_unknown = TRUE;
		break;
	case 4:
		if( !strcmp(str, "pard") ) {                   /* reset paragraph */
			state->align = GYDP_YDP_ALIGN_NONE;
			state->indent = 0;
			state->margin_left = 0;
			state->margin_right = 0;
		} else if( !strcmp(str, "line") ) {            /* new line */
			g_string_append_c(context->text, '\n');
		} else
			is_unknown = TRUE;
		break;
	case 5:
		if( !strcmp(str, "super") ) {                  /* superscript */
			state->script = GYDP_YDP_SCRIPT_SUPER;
		} else
			is_unknown = TRUE;
		break;
	default:
		is_unknown = TRUE;
		break;
	}

	/* unknown control word, print */
	if( is_unknown ) {
		g_string_append_c(context->text, '\\');
		g_string_append(context->text, str);
	}
}

static void gydp_ydp_push_state(GydpYDPContext *context) {
	/* commit text before group opening */
	gydp_ydp_commit_text(context);

	/* clone current control code */
	GydpYDPState *state = gydp_ydp_state_clone(context->state->data);

	/* add to the control code list */
	context->state = g_slist_prepend(context->state, state);
}

static void gydp_ydp_pop_state(GydpYDPContext *context) {
	/* commit text before group closing */
	gydp_ydp_commit_text(context);

	/* remove current control code */
	gydp_ydp_state_free(context->state->data);
	context->state = g_slist_delete_link(context->state, context->state);
}

static void gydp_ydp_commit_text(GydpYDPContext *context) {
	GtkTextIter begin, end;
	GtkTextMark *mark;
	GydpYDPState *state;

	/* skip if no text present in context */
	if( !context->text->len )
		return;

	/* extract current code */
	state = context->state->data;

	/* convert text encoding (cp1250 or phonetic) */
	gydp_convert_ydp_buffer(context->text->str, state->phonetic,
			context->buffer->str);

	/* get insert position and set mark */
	gtk_text_buffer_get_end_iter(context->widget, &end);
	mark = gtk_text_buffer_create_mark(context->widget, NULL, &end, TRUE);

	/* insert text */
	gtk_text_buffer_insert(context->widget, &end, context->buffer->str, -1);

	/* extract begin iter and delete mark */
	gtk_text_buffer_get_iter_at_mark(context->widget, &begin, mark);
	gtk_text_buffer_delete_mark(context->widget, mark);

	/*
	 * apply styles
	 */

	/* script */
	switch( (GydpYDPScript)state->script ) {
	case GYDP_YDP_SCRIPT_NORMAL:
		gtk_text_buffer_apply_tag_by_name(context->widget,
				GYDP_TAG_SCRIPT_NORMAL, &begin, &end);
		break;
	case GYDP_YDP_SCRIPT_SUPER:
		gtk_text_buffer_apply_tag_by_name(context->widget,
				GYDP_TAG_SCRIPT_SUPER, &begin, &end);
		break;
	case GYDP_YDP_SCRIPT_SUB:
		gtk_text_buffer_apply_tag_by_name(context->widget,
				GYDP_TAG_SCRIPT_SUB, &begin, &end);
		break;
	case GYDP_YDP_SCRIPT_NONE:
		break;
	default:
		g_return_if_reached();
	}

	/* bold */
	if( state->bold )
		gtk_text_buffer_apply_tag_by_name(context->widget,
				GYDP_TAG_BOLD, &begin, &end);

	/* italic */
	if( state->italic )
		gtk_text_buffer_apply_tag_by_name(context->widget,
				GYDP_TAG_ITALIC, &begin, &end);

	/* align */
	switch( (GydpYDPAlign)state->align ) {
	case GYDP_YDP_ALIGN_CENTER:
		gtk_text_buffer_apply_tag_by_name(context->widget,
				GYDP_TAG_ALIGN_CENTER, &begin, &end);
		break;
	case GYDP_YDP_ALIGN_LEFT:
		gtk_text_buffer_apply_tag_by_name(context->widget,
				GYDP_TAG_ALIGN_LEFT, &begin, &end);
		break;
	case GYDP_YDP_ALIGN_NONE:
		break;
	default:
		g_return_if_reached();
	}

	/* color */
	switch( (GydpYDPColor)state->color ) {
	case GYDP_YDP_COLOR_RED:
		gtk_text_buffer_apply_tag_by_name(context->widget,
				GYDP_TAG_COLOR_RED, &begin, &end);
		break;
	case GYDP_YDP_COLOR_GREEN:
		gtk_text_buffer_apply_tag_by_name(context->widget,
				GYDP_TAG_COLOR_GREEN, &begin, &end);
		break;
	case GYDP_YDP_COLOR_BLUE:
		gtk_text_buffer_apply_tag_by_name(context->widget,
				GYDP_TAG_COLOR_BLUE, &begin, &end);
		break;
	case GYDP_YDP_COLOR_NONE:
		break;
	default:
		g_return_if_reached();
	}

	/* remove commited text */
	g_string_truncate(context->text, 0);
}

