/* Pango
 * pango-ot-info.c: Store tables for OpenType
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

#include "pango-ot-private.h"
#include "fterrcompat.h"
#include <freetype/internal/ftobjs.h>
#include <freetype/ftmodule.h>

static void pango_ot_info_class_init (GObjectClass *object_class);
static void pango_ot_info_finalize   (GObject *object);

static GObjectClass *parent_class;

enum
{
  INFO_LOADED_GDEF = 1 << 0,
  INFO_LOADED_GSUB = 1 << 1,
  INFO_LOADED_GPOS = 1 << 2
};

GType
pango_ot_info_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoOTInfoClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc)pango_ot_info_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoOTInfo),
        0,              /* n_preallocs */
	NULL            /* init */
      };
      
      object_type = g_type_register_static (G_TYPE_OBJECT,
                                            "PangoOTInfo",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void 
pango_ot_info_class_init (GObjectClass *object_class)
{
  parent_class = g_type_class_peek_parent (object_class);
  
  object_class->finalize = pango_ot_info_finalize;
}

static void 
pango_ot_info_finalize (GObject *object)
{
  PangoOTInfo *info = PANGO_OT_INFO (object);
  
  if (info->gdef)
    {
      TT_Done_GDEF_Table (info->gdef);
      info->gdef = NULL;
    }
  if (info->gsub)
    {
      TT_Done_GSUB_Table (info->gsub);
      info->gsub = NULL;
    }
  if (info->gpos)
    {
      TT_Done_GPOS_Table (info->gpos);
      info->gpos = NULL;
    }
}

/**
 * pango_ot_info_new:
 * @face: a #FT_Face.
 * @returns: a new #PangoOTInfo for @face.
 * 
 * Creates a new #PangoOTInfo structure for the given FreeType font.
 **/
PangoOTInfo *
pango_ot_info_new (FT_Face face)
{
  PangoOTInfo *info;

  info = g_object_new (PANGO_TYPE_OT_INFO, NULL);

  info->face = face;

  return info;
}

/* There must be be a better way to do this
 */
static gboolean
is_truetype (FT_Face face)
{
  return strcmp (FT_MODULE_CLASS (face->driver)->module_name, "truetype") == 0;
}

TTO_GDEF 
pango_ot_info_get_gdef (PangoOTInfo *info)
{
  g_return_val_if_fail (PANGO_IS_OT_INFO (info), NULL);

  if (!(info->loaded & INFO_LOADED_GDEF))
    {
      FT_Error error;
      
      info->loaded |= INFO_LOADED_GDEF;

      if (is_truetype (info->face))
	{
	  error = TT_Load_GDEF_Table (info->face, &info->gdef);
	  
	  if (error && error != TT_Err_Table_Missing)
	    g_warning ("Error loading GDEF table %d", error);
	}
    }

  return info->gdef;
}

TTO_GSUB
pango_ot_info_get_gsub (PangoOTInfo *info)
{
  g_return_val_if_fail (PANGO_IS_OT_INFO (info), NULL);
  
  if (!(info->loaded & INFO_LOADED_GSUB))
    {
      FT_Error error;
      TTO_GDEF gdef = pango_ot_info_get_gdef (info);
      
      info->loaded |= INFO_LOADED_GSUB;

      if (is_truetype (info->face))
	{
	  error = TT_Load_GSUB_Table (info->face, &info->gsub, gdef);

	  if (error && error != TT_Err_Table_Missing)
	    g_warning ("Error loading GSUB table %d", error);
	}
    }
  
  return info->gsub;
}

TTO_GPOS
pango_ot_info_get_gpos (PangoOTInfo *info)
{
  g_return_val_if_fail (PANGO_IS_OT_INFO (info), NULL);
  
  if (!(info->loaded & INFO_LOADED_GPOS))
    {
      FT_Error error;
      TTO_GDEF gdef = pango_ot_info_get_gdef (info);

      info->loaded |= INFO_LOADED_GPOS;

      if (is_truetype (info->face))
	{
	  error = TT_Load_GPOS_Table (info->face, &info->gpos, gdef);

	  if (error && error != TT_Err_Table_Missing)
	    g_warning ("Error loading GPOS table %d", error);
	}
    }

  return info->gpos;
}

static gboolean
get_tables (PangoOTInfo      *info,
	    PangoOTTableType  table_type,
	    TTO_ScriptList  **script_list,
	    TTO_FeatureList **feature_list)
{
  if (table_type == PANGO_OT_TABLE_GSUB)
    {
      TTO_GSUB gsub = pango_ot_info_get_gsub (info);

      if (!gsub)
	return FALSE;
      else
	{
	  if (script_list)
	    *script_list = &gsub->ScriptList;
	  if (feature_list)
	    *feature_list = &gsub->FeatureList;
	  return TRUE;
	}
    }
  else
    {
      TTO_GPOS gpos = pango_ot_info_get_gpos (info);

      if (!gpos)
	return FALSE;
      else
	{
	  if (script_list)
	    *script_list = &gpos->ScriptList;
	  if (feature_list)
	    *feature_list = &gpos->FeatureList;
	  return TRUE;
	}
    }
}

/**
 * pango_ot_info_find_script:
 * @info: a #PangoOTInfo.
 * @table_type: the table type to obtain information about.
 * @script_tag: the tag of the script to find.
 * @script_index: location to store the index of the script, or %NULL.
 * @returns: %TRUE if the script was found.
 * 
 * Finds the index of a script.
 **/
gboolean 
pango_ot_info_find_script (PangoOTInfo      *info,
			   PangoOTTableType  table_type,
			   PangoOTTag        script_tag,
			   guint            *script_index)
{
  TTO_ScriptList *script_list;
  int i;

  g_return_val_if_fail (PANGO_IS_OT_INFO (info), FALSE);

  if (!get_tables (info, table_type, &script_list, NULL))
    return FALSE;

  for (i=0; i < script_list->ScriptCount; i++)
    {
      if (script_list->ScriptRecord[i].ScriptTag == script_tag)
	{
	  if (script_index)
	    *script_index = i;

	  return TRUE;
	}
    }

  return FALSE;
}

/**
 * pango_ot_info_find_language:
 * @info: a #PangoOTInfo.
 * @table_type: the table type to obtain information about.
 * @script_index: the index of the script whose languages are searched.
 * @language_tag: the tag of the language to find.
 * @language_index: location to store the index of the language, or %NULL.
 * @required_feature_index: location to store the required feature index of 
 *    the language, or %NULL.
 * @returns: %TRUE if the language was found.
 * 
 * Finds the index of a language and its required feature index.  
 **/
gboolean
pango_ot_info_find_language (PangoOTInfo      *info,
			     PangoOTTableType  table_type,
			     guint             script_index,
			     PangoOTTag        language_tag,
			     guint            *language_index,
			     guint            *required_feature_index)
{
  TTO_ScriptList *script_list;
  TTO_Script *script;
  int i;

  g_return_val_if_fail (PANGO_IS_OT_INFO (info), FALSE);

  if (!get_tables (info, table_type, &script_list, NULL))
    return FALSE;

  g_return_val_if_fail (script_index < script_list->ScriptCount, FALSE);

  script = &script_list->ScriptRecord[script_index].Script;

  for (i = 0; i < script->LangSysCount; i++)
    {
      if (script->LangSysRecord[i].LangSysTag == language_tag)
	{
	  if (language_index)
	    *language_index = i;
	  if (required_feature_index)
	    *required_feature_index = script->LangSysRecord[i].LangSys.ReqFeatureIndex;
	  return TRUE;
	}
    }

  return FALSE;
}

/**
 * pango_ot_info_find_feature:
 * @info: a #PangoOTInfo.
 * @table_type: the table type to obtain information about.
 * @feature_tag: the tag of the feature to find.
 * @script_index: the index of the script.
 * @language_index: the index of the language whose features are searched,
 *     or 0xffff to use the default language of the script.
 * @feature_index: location to store the index of the feature, or %NULL. 
 * @returns: %TRUE if the feature was found.
 * 
 * Finds the index of a feature.
 **/
gboolean
pango_ot_info_find_feature  (PangoOTInfo      *info,
			     PangoOTTableType  table_type,
			     PangoOTTag        feature_tag,
			     guint             script_index,
			     guint             language_index,
			     guint            *feature_index)
{
  TTO_ScriptList *script_list;
  TTO_FeatureList *feature_list;
  TTO_Script *script;
  TTO_LangSys *lang_sys;

  int i;

  g_return_val_if_fail (PANGO_IS_OT_INFO (info), FALSE);

  if (!get_tables (info, table_type, &script_list, &feature_list))
    return FALSE;

  g_return_val_if_fail (script_index < script_list->ScriptCount, FALSE);

  script = &script_list->ScriptRecord[script_index].Script;

  if (language_index == 0xffff)
    lang_sys = &script->DefaultLangSys;
  else
    {
      g_return_val_if_fail (language_index < script->LangSysCount, FALSE);
      lang_sys = &script->LangSysRecord[language_index].LangSys;
    }

  for (i = 0; i < lang_sys->FeatureCount; i++)
    {
      FT_UShort index = lang_sys->FeatureIndex[i];

      if (feature_list->FeatureRecord[index].FeatureTag == feature_tag)
	{
	  if (feature_index)
	    *feature_index = index;

	  return TRUE;
	}
    }

  return FALSE;
}

/**
 * pango_ot_info_list_scripts:
 * @info: a #PangoOTInfo.
 * @table_type: the table type to obtain information about.
 * @returns: a newly-allocated array containing the tags of the
 *   available scripts.
 *
 * Obtains the list of available scripts. 
 **/
PangoOTTag *
pango_ot_info_list_scripts (PangoOTInfo      *info,
			    PangoOTTableType  table_type)
{
  PangoOTTag *result;
  TTO_ScriptList *script_list;
  int i;

  g_return_val_if_fail (PANGO_IS_OT_INFO (info), NULL);

  if (!get_tables (info, table_type, &script_list, NULL))
    return NULL;

  result = g_new (PangoOTTag, script_list->ScriptCount + 1);
  
  for (i=0; i < script_list->ScriptCount; i++)
    result[i] = script_list->ScriptRecord[i].ScriptTag;

  result[i] = 0;

  return result;
}

/**
 * pango_ot_info_list_languages:
 * @info: a #PangoOTInfo.
 * @table_type: the table type to obtain information about.
 * @script_index: the index of the script to list languages for.
 * @language_tag: unused parameter.
 * @returns: a newly-allocated array containing the tags of the
 *   available languages.
 * 
 * Obtains the list of available languages for a given script.
 **/ 
PangoOTTag *
pango_ot_info_list_languages (PangoOTInfo      *info,
			      PangoOTTableType  table_type,
			      guint             script_index,
			      PangoOTTag        language_tag)
{
  PangoOTTag *result;
  TTO_ScriptList *script_list;
  TTO_Script *script;
  int i;

  g_return_val_if_fail (PANGO_IS_OT_INFO (info), NULL);

  if (!get_tables (info, table_type, &script_list, NULL))
    return NULL;

  g_return_val_if_fail (script_index < script_list->ScriptCount, NULL);

  script = &script_list->ScriptRecord[script_index].Script;
  
  result = g_new (PangoOTTag, script->LangSysCount + 1);
  
  for (i = 0; i < script->LangSysCount; i++)
    result[i] = script->LangSysRecord[i].LangSysTag;

  result[i] = 0;

  return result;
}

/** 
 * pango_ot_info_list_features:
 * @info: a #PangoOTInfo.
 * @table_type: the table type to obtain information about.
 * @tag: unused parameter.
 * @script_index: the index of the script to obtain information about. 
 * @language_index: the indes of the language to list features for, or
 *     0xffff, to list features for the default language of the script.
 * @returns: a newly-allocated array containing the tags of the available
 *    features. 
 *
 * Obtains the list of features for the given language of the given script.
 **/
PangoOTTag *
pango_ot_info_list_features  (PangoOTInfo      *info,
			      PangoOTTableType  table_type,
			      PangoOTTag        tag,
			      guint             script_index,
			      guint             language_index)
{
  PangoOTTag *result;

  TTO_ScriptList *script_list;
  TTO_FeatureList *feature_list;
  TTO_Script *script;
  TTO_LangSys *lang_sys;

  int i;

  g_return_val_if_fail (PANGO_IS_OT_INFO (info), NULL);

  if (!get_tables (info, table_type, &script_list, &feature_list))
    return NULL;

  g_return_val_if_fail (script_index < script_list->ScriptCount, NULL);

  script = &script_list->ScriptRecord[script_index].Script;

  if (language_index == 0xffff)
    lang_sys = &script->DefaultLangSys;
  else
    {
      g_return_val_if_fail (language_index < script->LangSysCount, NULL);
      lang_sys = &script->LangSysRecord[language_index].LangSys;
    }

  result = g_new (PangoOTTag, lang_sys->FeatureCount + 1);
  
  for (i = 0; i < lang_sys->FeatureCount; i++)
    {
      FT_UShort index = lang_sys->FeatureIndex[i];

      result[i] = feature_list->FeatureRecord[index].FeatureTag;
    }

  result[i] = 0;

  return result;
}


