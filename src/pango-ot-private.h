/* Pango
 * pango-ot-private.h: Implementation details for Pango OpenType code
 *
 * Copyright (C) 2000 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __PANGO_OT_PRIVATE_H__
#define __PANGO_OT_PRIVATE_H__

#include <freetype/freetype.h>

#include <glib-object.h>

#include <pango/pango-ot.h>
#include "ftxopen.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PANGO_TYPE_OT_INFO              (pango_ot_info_get_type ())
#define PANGO_OT_INFO(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_OT_INFO, PangoOTInfo))
#define PANGO_OT_INFO_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_OT_INFO, PangoOTInfoClass))
#define PANGO_IS_OT_INFO(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_OT_INFO))
#define PANGO_IS_OT_INFO_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_OT_INFO))
#define PANGO_OT_INFO_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_OT_INFO, PangoOTInfoClass))

typedef struct _PangoOTInfoClass PangoOTInfoClass;

struct _PangoOTInfo 
{
  GObject parent_instance;

  guint loaded;

  FT_Face face;

  TTO_GSUB gsub;
  TTO_GDEF gdef;
  TTO_GPOS gpos;
};

struct _PangoOTInfoClass
{
  GObjectClass parent_class;
};

#define PANGO_TYPE_OT_RULESET              (pango_ot_ruleset_get_type ())
#define PANGO_OT_RULESET(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_OT_RULESET, PangoOTRuleset))
#define PANGO_OT_RULESET_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_OT_RULESET, PangoOTRulesetClass))f
#define PANGO_OT_IS_RULESET(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_OT_RULESET))
#define PANGO_OT_IS_RULESET_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_OT_RULESET))
#define PANGO_OT_RULESET_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_OT_RULESET, PangoOTRulesetClass))

typedef struct _PangoOTRulesetClass PangoOTRulesetClass;

struct _PangoOTRuleset
{
  GObject parent_instance;

  GArray *rules;
  PangoOTInfo *info;
};

struct _PangoOTRulesetClass
{
  GObjectClass parent_class;
};

GType pango_ot_info_get_type (void);

TTO_GDEF pango_ot_info_get_gdef (PangoOTInfo *info);
TTO_GSUB pango_ot_info_get_gsub (PangoOTInfo *info);
TTO_GPOS pango_ot_info_get_gpos (PangoOTInfo *info);

GType pango_ot_ruleset_get_type (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __PANGO_OT_PRIVATE_H__ */
