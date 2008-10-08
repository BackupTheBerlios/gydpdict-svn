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

#ifndef __GYDP_DICT_H__
#define __GYDP_DICT_H__

#include "gydp_global.h"
#include <gtk/gtktextbuffer.h>

G_BEGIN_DECLS

typedef struct _GydpDict      GydpDict;
typedef struct _GydpDictClass GydpDictClass;

#define GYDP_TYPE_DICT            (gydp_dict_get_type ())
#define GYDP_DICT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GYDP_TYPE_DICT, GydpDict))
#define GYDP_DICT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GYDP_TYPE_DICT, GydpDictClass))
#define GYDP_IS_DICT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GYDP_TYPE_DICT))
#define GYDP_IS_DICT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GYDP_TYPE_DICT))
#define GYDP_DICT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GYDP_TYPE_DICT, GydpDictClass))

struct _GydpDict {
  GObject __parent__;

	/* READ ONLY */
	GydpEngine engine;
	GydpLang language;
};

struct _GydpDictClass {
	GObjectClass __parent__;

	/* virtual table */
	gboolean     (*load)(GydpDict *dict, gchar **locations, GydpLang lang);
	gboolean     (*lang)(GydpDict *dict, GydpLang lang);
	guint        (*size)(GydpDict *dict);
	const gchar *(*word)(GydpDict *dict, guint n);
	gboolean     (*text)(GydpDict *dict, guint n, GtkTextBuffer *buffer);
	guint        (*find)(GydpDict *dict, const gchar *word);
};

GType        gydp_dict_get_type();

/* language management */
gboolean     gydp_dict_load    (GydpDict *dict, gchar **locations, GydpLang lang);
gboolean     gydp_dict_lang    (GydpDict *dict, GydpLang lang);

/* translation management */
guint        gydp_dict_size    (GydpDict *dict);
const gchar *gydp_dict_word    (GydpDict *dict, guint n);
gboolean     gydp_dict_text    (GydpDict *dict, guint n, GtkTextBuffer *buffer);
guint        gydp_dict_find    (GydpDict *dict, const gchar *word);

/* default implementations for virual functions */
guint        gydp_dict_find_f  (GydpDict *dict, const gchar *word);

G_END_DECLS

#endif /* __GYDP_DICT_H__ */

