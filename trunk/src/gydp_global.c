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

#include "gydp_global.h"

void gydp_enums_ref() __attribute__ ((constructor));
void gydp_enums_unref() __attribute__ ((destructor));

static GEnumClass *gydp_engine = NULL;
static GEnumClass *gydp_language = NULL;

void gydp_enums_ref() {
	/* initialize type system */
	g_type_init();

	/* initialize enum classes */
	gydp_engine = g_type_class_ref(GYDP_TYPE_ENGINE);
	gydp_language = g_type_class_ref(GYDP_TYPE_LANG);
}

void gydp_enums_unref() {
	g_type_class_unref(gydp_engine);
	g_type_class_unref(gydp_language);
}

GType gydp_lang_get_type() {
	static GType type = G_TYPE_INVALID;
	if( G_UNLIKELY( type == G_TYPE_INVALID ) ) {
		static const GEnumValue values[] = {
			{ GYDP_LANG_ENG_TO_POL, "GYDP_LANG_ENG_TO_POL", "English to Polish" },
			{ GYDP_LANG_ENG_FROM_POL, "GYDP_LANG_ENG_FROM_POL", "Polish to English" },
			{ 0, NULL, NULL}
		};

		type = g_enum_register_static("GydpLang", values);
	}

	return type;
}

GType gydp_engine_get_type() {
	static GType type = G_TYPE_INVALID;
	if( G_UNLIKELY( type == G_TYPE_INVALID ) ) {
		static const GEnumValue values[] = {
			{ GYDP_ENGINE_SAP, "GYDP_ENGINE_SAP", "sap" },
			{ GYDP_ENGINE_YDP, "GYDP_ENGINE_YDP", "ydp" },
			{ 0, NULL, NULL}
		};

		type = g_enum_register_static("GydpEngine", values);
	}

	return type;
}

GEnumClass *gydp_enum(GydpEnum type) {
	switch( type ) {
	case GYDP_ENUM_ENGINE: return gydp_engine;
	case GYDP_ENUM_LANG:   return gydp_language;
	default:               g_return_val_if_reached(NULL);
	}
}

const gchar *gydp_lang_value_to_name(GydpLang lang) {
	return g_enum_get_value(gydp_language, lang)->value_name;
}

const gchar *gydp_lang_value_to_nick(GydpLang lang) {
	return g_enum_get_value(gydp_language, lang)->value_nick;
}

GydpLang gydp_lang_name_to_value(const gchar *name) {
	return g_enum_get_value_by_name(gydp_language, name)->value;
}

GydpLang gydp_lang_nick_to_value(const gchar *name) {
	return g_enum_get_value_by_nick(gydp_language, name)->value;
}

const gchar *gydp_engine_value_to_name(GydpEngine engine) {
	return g_enum_get_value(gydp_engine, engine)->value_name;
}

const gchar *gydp_engine_value_to_nick(GydpEngine engine) {
	return g_enum_get_value(gydp_engine, engine)->value_nick;
}

GydpEngine gydp_engine_name_to_value(const gchar *name) {
	return g_enum_get_value_by_name(gydp_engine, name)->value;
}

GydpEngine gydp_engine_nick_to_value(const gchar *name) {
	return g_enum_get_value_by_nick(gydp_engine, name)->value;

}

