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

#ifndef __GYDP_UTIL_H__
#define __GYDP_UTIL_H__

#include "gydp_global.h"
#include <gio/gio.h>

G_BEGIN_DECLS

/* license text */
#define GYDP_LICENSE (gydp_license_text())

/* convinience engine creation */
GObject       *gydp_engine_new  (GydpEngine engine);

/* get program license */
const gchar   *gydp_license_text();

/* open file input stream */
GInputStream  *gydp_file_open   (const gchar *dirname, const gchar *filename);

/* provide data system dictories */
gchar         *gydp_config_file ();
gchar        **gydp_data_dirs   (GydpEngine engine);

G_END_DECLS

#endif /* __GYDP_UTIL_H__ */

