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

#ifndef __GYDP_GLOBAL_H__
#define __GYDP_GLOBAL_H__

#include <glib.h>
#include <glib-object.h>
#include "gydp_config.h"

G_BEGIN_DECLS

/* Configuration location */
#define GYDP_FILE_RC  "gydpdictrc"
#define GYDP_FILE_DIR "gydpdict"

/* GydpApp properites */
#define GYDP_APP_DICT "dict"
#define GYDP_APP_CONF "cfg"

/* GtkTextBuffer tag names */
#define GYDP_TAG_ALIGN_CENTER  "center"
#define GYDP_TAG_ALIGN_LEFT    "left"
#define GYDP_TAG_ALIGN_RIGHT   "right"
#define GYDP_TAG_BOLD          "bold"
#define GYDP_TAG_ITALIC        "italic"
#define GYDP_TAG_SCRIPT_SUPER  "super"
#define GYDP_TAG_SCRIPT_SUB    "sub"
#define GYDP_TAG_SCRIPT_NORMAL "normal"
#define GYDP_TAG_COLOR_RED     "red"
#define GYDP_TAG_COLOR_GREEN   "green"
#define GYDP_TAG_COLOR_BLUE    "blue"

#define GYDP_TYPE_LANG (gydp_lang_get_type ())
GType gydp_lang_get_type() G_GNUC_CONST;
#define GYDP_TYPE_ENGINE (gydp_engine_get_type ())
GType gydp_engine_get_type() G_GNUC_CONST;

typedef enum {
	GYDP_LANG_NONE,
	GYDP_LANG_ENG_TO_POL,
	GYDP_LANG_ENG_FROM_POL,
} GydpLang;

typedef enum {
	GYDP_ENGINE_NONE,
	GYDP_ENGINE_DEFAULT,
	GYDP_ENGINE_SAP,
	GYDP_ENGINE_YDP,
} GydpEngine;

typedef enum {
	GYDP_ENUM_ENGINE,
	GYDP_ENUM_LANG,
} GydpEnum;

/* gydp enums management */
GEnumClass  *gydp_enum                (GydpEnum type);

/* gydp enum helper functions */
const gchar *gydp_lang_value_to_name  (GydpLang lang);
const gchar *gydp_lang_value_to_nick  (GydpLang lang);
GydpLang     gydp_lang_name_to_value  (const gchar *name);
GydpLang     gydp_lang_nick_to_value  (const gchar *name);

const gchar *gydp_engine_value_to_name(GydpEngine engine);
const gchar *gydp_engine_value_to_nick(GydpEngine engine);
GydpEngine   gydp_engine_name_to_value(const gchar *name);
GydpEngine   gydp_engine_nick_to_value(const gchar *name);

G_END_DECLS

#endif /* __GYDP_GLOBAL_H__ */

