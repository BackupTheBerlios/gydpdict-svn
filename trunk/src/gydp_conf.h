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

#ifndef __GYDP_CONF_H__
#define __GYDP_CONF_H__

#include "gydp_global.h"

G_BEGIN_DECLS

typedef struct _GydpConf GydpConf;

GydpConf *gydp_conf_new       ();
void      gydp_conf_free      (GydpConf *self);

gboolean  gydp_conf_load      (GydpConf *self);
gboolean  gydp_conf_save      (GydpConf *self);

gchar    *gydp_conf_get_string (GydpConf *self, const char *group, const char *key);
void      gydp_conf_set_string (GydpConf *self, const char *group, const char *key, const char *value);
gint      gydp_conf_get_integer(GydpConf *self, const char *group, const char *key);
void      gydp_conf_set_integer(GydpConf *self, const char *group, const char *key, int value);

G_END_DECLS

#endif /* __GYDP_CONF_H__ */

