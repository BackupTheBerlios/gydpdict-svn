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

#ifndef __GYDP_DICT_SAP_H__
#define __GYDP_DICT_SAP_H__

#include "gydp_global.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct GydpDictSAP      GydpDictSAP;
typedef struct GydpDictSAPClass GydpDictSAPClass;

#define GYDP_TYPE_DICT_SAP            (gydp_dict_sap_get_type ())
#define GYDP_DICT_SAP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GYDP_TYPE_DICT_SAP, GydpDictSAP))
#define GYDP_DICT_SAP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GYDP_TYPE_DICT_SAP, GydpDictSAPClass))
#define GYDP_IS_DICT_SAP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GYDP_TYPE_DICT_SAP))
#define GYDP_IS_DICT_SAP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GYDP_TYPE_DICT_SAP))
#define GYDP_DICT_SAP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GYDP_TYPE_DICT_SAP, GydpDictSAPClass))

GType       gydp_dict_sap_get_type() G_GNUC_CONST;
GObject    *gydp_dict_sap_new     ();

G_END_DECLS

#endif /* __GYDP_DICT_SAP_H__ */

