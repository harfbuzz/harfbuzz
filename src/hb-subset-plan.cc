/*
 * Copyright © 2018  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Google Author(s): Garret Rieger, Roderick Sheeter
 */

#include "hb-subset-plan.hh"
#include "hb-map.hh"
#include "hb-set.hh"

#include "hb-ot-cmap-table.hh"
#include "hb-ot-glyf-table.hh"
#include "hb-ot-layout-gdef-table.hh"
#include "hb-ot-layout-gpos-table.hh"
#include "hb-ot-layout-gsub-table.hh"
#include "hb-ot-cff1-table.hh"
#include "hb-ot-color-colr-table.hh"
#include "hb-ot-color-colrv1-closure.hh"
#include "hb-ot-var-fvar-table.hh"
#include "hb-ot-stat-table.hh"
#include "hb-ot-math-table.hh"

using OT::Layout::GSUB::GSUB;


typedef hb_hashmap_t<unsigned, hb_set_t *> script_langsys_map;
#ifndef HB_NO_SUBSET_CFF
static inline void
_add_cff_seac_components (const OT::cff1::accelerator_t &cff,
			  hb_codepoint_t gid,
			  hb_set_t *gids_to_retain)
{
  hb_codepoint_t base_gid, accent_gid;
  if (cff.get_seac_components (gid, &base_gid, &accent_gid))
  {
    gids_to_retain->add (base_gid);
    gids_to_retain->add (accent_gid);
  }
}
#endif

static void
_remap_palette_indexes (const hb_set_t *palette_indexes,
			hb_map_t       *mapping /* OUT */)
{
  unsigned new_idx = 0;
  for (unsigned palette_index : palette_indexes->iter ())
  {
    if (palette_index == 0xFFFF)
    {
      mapping->set (palette_index, palette_index);
      continue;
    }
    mapping->set (palette_index, new_idx);
    new_idx++;
  }
}

static void
_remap_indexes (const hb_set_t *indexes,
		hb_map_t       *mapping /* OUT */)
{
  unsigned count = indexes->get_population ();

  for (auto _ : + hb_zip (indexes->iter (), hb_range (count)))
    mapping->set (_.first, _.second);

}

#ifndef HB_NO_SUBSET_LAYOUT
typedef void (*layout_collect_func_t) (hb_face_t *face, hb_tag_t table_tag, const hb_tag_t *scripts, const hb_tag_t *languages, const hb_tag_t *features, hb_set_t *lookup_indexes /* OUT */);


template <typename T>
static void _collect_layout_indices (hb_face_t		  *face,
                                     const T&              table,
                                     const hb_set_t	  *layout_features_to_retain,
                                     layout_collect_func_t layout_collect_func,
                                     hb_set_t		  *indices /* OUT */)
{
  hb_vector_t<hb_tag_t> features;
  if (!features.alloc (table.get_feature_count () + 1))
    return;

  hb_set_t visited_features;
  bool retain_all_features = true;
  for (unsigned i = 0; i < table.get_feature_count (); i++)
  {
    hb_tag_t tag = table.get_feature_tag (i);
    if (!tag) continue;
    if (!layout_features_to_retain->has (tag))
    {
      retain_all_features = false;
      continue;
    }

    if (visited_features.has (tag))
      continue;

    features.push (tag);
    visited_features.add (tag);
  }

  if (!features)
    return;

  // The collect function needs a null element to signal end of the array.
  features.push (0);

  if (retain_all_features)
  {
    // Looking for all features, trigger the faster collection method.
    layout_collect_func (face,
                         T::tableTag,
                         nullptr,
                         nullptr,
                         nullptr,
                         indices);
    return;
  }

  layout_collect_func (face,
                       T::tableTag,
		       nullptr,
		       nullptr,
		       features.arrayZ,
		       indices);
}

template <typename T>
static inline void
_closure_glyphs_lookups_features (hb_face_t	     *face,
				  hb_set_t	     *gids_to_retain,
				  const hb_set_t     *layout_features_to_retain,
				  hb_map_t	     *lookups,
				  hb_map_t	     *features,
				  script_langsys_map *langsys_map)
{
  hb_blob_ptr_t<T> table = hb_sanitize_context_t ().reference_table<T> (face);
  hb_tag_t table_tag = table->tableTag;
  hb_set_t lookup_indices;
  _collect_layout_indices<T> (face,
                              *table,
                              layout_features_to_retain,
                              hb_ot_layout_collect_lookups,
                              &lookup_indices);

  if (table_tag == HB_OT_TAG_GSUB)
    hb_ot_layout_lookups_substitute_closure (face,
					    &lookup_indices,
					     gids_to_retain);
  table->closure_lookups (face,
			  gids_to_retain,
			 &lookup_indices);
  _remap_indexes (&lookup_indices, lookups);

  // Collect and prune features
  hb_set_t feature_indices;
  _collect_layout_indices<T> (face,
                              *table,
                              layout_features_to_retain,
                              hb_ot_layout_collect_features,
                              &feature_indices);

  table->prune_features (lookups, &feature_indices);
  hb_map_t duplicate_feature_map;
  table->find_duplicate_features (lookups, &feature_indices, &duplicate_feature_map);

  feature_indices.clear ();
  table->prune_langsys (&duplicate_feature_map, langsys_map, &feature_indices);
  _remap_indexes (&feature_indices, features);

  table.destroy ();
}

#endif

#ifndef HB_NO_VAR
static inline void
  _collect_layout_variation_indices (hb_face_t *face,
				     const hb_set_t *glyphset,
				     const hb_map_t *gpos_lookups,
				     hb_set_t  *layout_variation_indices,
				     hb_map_t  *layout_variation_idx_map)
{
  hb_blob_ptr_t<OT::GDEF> gdef = hb_sanitize_context_t ().reference_table<OT::GDEF> (face);
  hb_blob_ptr_t<OT::GPOS> gpos = hb_sanitize_context_t ().reference_table<OT::GPOS> (face);

  if (!gdef->has_data ())
  {
    gdef.destroy ();
    gpos.destroy ();
    return;
  }
  OT::hb_collect_variation_indices_context_t c (layout_variation_indices, glyphset, gpos_lookups);
  gdef->collect_variation_indices (&c);

  if (hb_ot_layout_has_positioning (face))
    gpos->collect_variation_indices (&c);

  gdef->remap_layout_variation_indices (layout_variation_indices, layout_variation_idx_map);

  gdef.destroy ();
  gpos.destroy ();
}
#endif

static inline void
_cmap_closure (hb_face_t	   *face,
	       const hb_set_t	   *unicodes,
	       hb_set_t		   *glyphset)
{
  OT::cmap::accelerator_t cmap (face);
  cmap.table->closure_glyphs (unicodes, glyphset);
}

static void _colr_closure (hb_face_t *face,
                           hb_map_t *layers_map,
                           hb_map_t *palettes_map,
                           hb_set_t *glyphs_colred)
{
  OT::COLR::accelerator_t colr (face);
  if (!colr.is_valid ()) return;

  unsigned iteration_count = 0;
  hb_set_t palette_indices, layer_indices;
  unsigned glyphs_num;
  {
    glyphs_num = glyphs_colred->get_population ();
    // Collect all glyphs referenced by COLRv0
    hb_set_t glyphset_colrv0;
    for (hb_codepoint_t gid : glyphs_colred->iter ())
      colr.closure_glyphs (gid, &glyphset_colrv0);

    glyphs_colred->union_ (glyphset_colrv0);

    //closure for COLRv1
    colr.closure_forV1 (glyphs_colred, &layer_indices, &palette_indices);
  } while (iteration_count++ <= HB_CLOSURE_MAX_STAGES &&
           glyphs_num != glyphs_colred->get_population ());

  colr.closure_V0palette_indices (glyphs_colred, &palette_indices);
  _remap_indexes (&layer_indices, layers_map);
  _remap_palette_indexes (&palette_indices, palettes_map);
}

static inline void
_math_closure (hb_face_t           *face,
               hb_set_t            *glyphset)
{
  hb_blob_ptr_t<OT::MATH> math = hb_sanitize_context_t ().reference_table<OT::MATH> (face);
  if (math->has_data ())
    math->closure_glyphs (glyphset);
  math.destroy ();
}


static inline void
_remove_invalid_gids (hb_set_t *glyphs,
		      unsigned int num_glyphs)
{
  glyphs->del_range (num_glyphs, HB_SET_VALUE_INVALID);
}

static void
_populate_unicodes_to_retain (const hb_set_t *unicodes,
                              const hb_set_t *glyphs,
                              hb_subset_plan_t *plan)
{
  OT::cmap::accelerator_t cmap (plan->source);

  unsigned size_threshold = plan->source->get_num_glyphs ();
  if (glyphs->is_empty () && unicodes->get_population () < size_threshold)
  {
    // This is approach to collection is faster, but can only be used  if glyphs
    // are not being explicitly added to the subset and the input unicodes set is
    // not excessively large (eg. an inverted set).
    plan->unicode_to_new_gid_list.alloc (unicodes->get_population ());
    for (hb_codepoint_t cp : *unicodes)
    {
      hb_codepoint_t gid;
      if (!cmap.get_nominal_glyph (cp, &gid))
      {
        DEBUG_MSG(SUBSET, nullptr, "Drop U+%04X; no gid", cp);
        continue;
      }

      plan->codepoint_to_glyph->set (cp, gid);
      plan->unicode_to_new_gid_list.push (hb_pair (cp, gid));
    }
  }
  else
  {
    // This approach is slower, but can handle adding in glyphs to the subset and will match
    // them with cmap entries.
    hb_map_t unicode_glyphid_map;
    hb_set_t cmap_unicodes;
    cmap.collect_mapping (&cmap_unicodes, &unicode_glyphid_map);
    plan->unicode_to_new_gid_list.alloc (hb_min(unicodes->get_population ()
                                                + glyphs->get_population (),
                                                cmap_unicodes.get_population ()));

    for (hb_codepoint_t cp : cmap_unicodes)
    {
      hb_codepoint_t gid = unicode_glyphid_map[cp];
      if (!unicodes->has (cp) && !glyphs->has (gid))
        continue;

      plan->codepoint_to_glyph->set (cp, gid);
      plan->unicode_to_new_gid_list.push (hb_pair (cp, gid));
    }

    /* Add gids which where requested, but not mapped in cmap */
    for (hb_codepoint_t gid : *glyphs)
    {
      if (gid >= plan->source->get_num_glyphs ())
	break;
      plan->_glyphset_gsub->add (gid);
    }
  }

  auto &arr = plan->unicode_to_new_gid_list;
  if (arr.length)
  {
    plan->unicodes->add_sorted_array (&arr.arrayZ->first, arr.length, sizeof (*arr.arrayZ));
    plan->_glyphset_gsub->add_array (&arr.arrayZ->second, arr.length, sizeof (*arr.arrayZ));
  }
}

static void
_populate_gids_to_retain (hb_subset_plan_t* plan,
			  bool close_over_gsub,
			  bool close_over_gpos,
			  bool close_over_gdef)
{
  OT::glyf::accelerator_t glyf (plan->source);
#ifndef HB_NO_SUBSET_CFF
  OT::cff1::accelerator_t cff (plan->source);
#endif

  plan->_glyphset_gsub->add (0); // Not-def

  _cmap_closure (plan->source, plan->unicodes, plan->_glyphset_gsub);

#ifndef HB_NO_SUBSET_LAYOUT
  if (close_over_gsub)
    // closure all glyphs/lookups/features needed for GSUB substitutions.
    _closure_glyphs_lookups_features<GSUB> (
        plan->source,
        plan->_glyphset_gsub,
        plan->layout_features,
        plan->gsub_lookups,
        plan->gsub_features,
        plan->gsub_langsys);

  if (close_over_gpos)
    _closure_glyphs_lookups_features<OT::GPOS> (
        plan->source,
        plan->_glyphset_gsub,
        plan->layout_features,
        plan->gpos_lookups,
        plan->gpos_features,
        plan->gpos_langsys);
#endif
  _remove_invalid_gids (plan->_glyphset_gsub, plan->source->get_num_glyphs ());

  hb_set_set (plan->_glyphset_mathed, plan->_glyphset_gsub);
  _math_closure (plan->source, plan->_glyphset_mathed);
  _remove_invalid_gids (plan->_glyphset_mathed, plan->source->get_num_glyphs ());

  hb_set_t cur_glyphset = *plan->_glyphset_mathed;
  _colr_closure (plan->source, plan->colrv1_layers, plan->colr_palettes, &cur_glyphset);
  _remove_invalid_gids (&cur_glyphset, plan->source->get_num_glyphs ());

  hb_set_set (plan->_glyphset_colred, &cur_glyphset);

  /* Populate a full set of glyphs to retain by adding all referenced
   * composite glyphs. */
  if (glyf.has_data ())
    for (hb_codepoint_t gid : cur_glyphset)
      glyf.add_gid_and_children (gid, plan->_glyphset);
  else
    plan->_glyphset->union_ (cur_glyphset);
#ifndef HB_NO_SUBSET_CFF
  if (cff.is_valid ())
    for (hb_codepoint_t gid : cur_glyphset)
      _add_cff_seac_components (cff, gid, plan->_glyphset);
#endif

  _remove_invalid_gids (plan->_glyphset, plan->source->get_num_glyphs ());


#ifndef HB_NO_VAR
  if (close_over_gdef)
    _collect_layout_variation_indices (plan->source,
				       plan->_glyphset_gsub,
				       plan->gpos_lookups,
				       plan->layout_variation_indices,
				       plan->layout_variation_idx_map);
#endif
}

static void
_create_glyph_map_gsub (const hb_set_t* glyph_set_gsub,
                        const hb_map_t* glyph_map,
                        hb_map_t* out)
{
  + hb_iter (glyph_set_gsub)
  | hb_map ([&] (hb_codepoint_t gid) {
    return hb_pair_t<hb_codepoint_t, hb_codepoint_t> (gid,
                                                      glyph_map->get (gid));
  })
  | hb_sink (out)
  ;
}

static void
_create_old_gid_to_new_gid_map (const hb_face_t *face,
				bool		 retain_gids,
				const hb_set_t	*all_gids_to_retain,
				hb_map_t	*glyph_map, /* OUT */
				hb_map_t	*reverse_glyph_map, /* OUT */
				unsigned int	*num_glyphs /* OUT */)
{
  unsigned pop = all_gids_to_retain->get_population ();
  reverse_glyph_map->resize (pop);
  glyph_map->resize (pop);

  if (!retain_gids)
  {
    + hb_enumerate (hb_iter (all_gids_to_retain), (hb_codepoint_t) 0)
    | hb_sink (reverse_glyph_map)
    ;
    *num_glyphs = reverse_glyph_map->get_population ();
  }
  else
  {
    + hb_iter (all_gids_to_retain)
    | hb_map ([] (hb_codepoint_t _) {
		return hb_pair_t<hb_codepoint_t, hb_codepoint_t> (_, _);
	      })
    | hb_sink (reverse_glyph_map)
    ;

    hb_codepoint_t max_glyph = HB_SET_VALUE_INVALID;
    hb_set_previous (all_gids_to_retain, &max_glyph);

    *num_glyphs = max_glyph + 1;
  }

  + reverse_glyph_map->iter ()
  | hb_map (&hb_pair_t<hb_codepoint_t, hb_codepoint_t>::reverse)
  | hb_sink (glyph_map)
  ;
}

static void
_nameid_closure (hb_face_t *face,
		 hb_set_t  *nameids)
{
#ifndef HB_NO_STYLE
  face->table.STAT->collect_name_ids (nameids);
#endif
#ifndef HB_NO_VAR
  face->table.fvar->collect_name_ids (nameids);
#endif
}

/**
 * hb_subset_plan_create_or_fail:
 * @face: font face to create the plan for.
 * @input: a #hb_subset_input_t input.
 *
 * Computes a plan for subsetting the supplied face according
 * to a provided input. The plan describes
 * which tables and glyphs should be retained.
 *
 * Return value: (transfer full): New subset plan. Destroy with
 * hb_subset_plan_destroy(). If there is a failure creating the plan
 * nullptr will be returned.
 *
 * Since: 4.0.0
 **/
hb_subset_plan_t *
hb_subset_plan_create_or_fail (hb_face_t	 *face,
                               const hb_subset_input_t *input)
{
  hb_subset_plan_t *plan;
  if (unlikely (!(plan = hb_object_create<hb_subset_plan_t> ())))
    return nullptr;

  plan->successful = true;
  plan->flags = input->flags;
  plan->unicodes = hb_set_create ();

  plan->unicode_to_new_gid_list.init ();

  plan->name_ids = hb_set_copy (input->sets.name_ids);
  _nameid_closure (face, plan->name_ids);
  plan->name_languages = hb_set_copy (input->sets.name_languages);
  plan->layout_features = hb_set_copy (input->sets.layout_features);
  plan->glyphs_requested = hb_set_copy (input->sets.glyphs);
  plan->drop_tables = hb_set_copy (input->sets.drop_tables);
  plan->no_subset_tables = hb_set_copy (input->sets.no_subset_tables);
  plan->source = hb_face_reference (face);
  plan->dest = hb_face_builder_create ();

  plan->_glyphset = hb_set_create ();
  plan->_glyphset_gsub = hb_set_create ();
  plan->_glyphset_mathed = hb_set_create ();
  plan->_glyphset_colred = hb_set_create ();
  plan->codepoint_to_glyph = hb_map_create ();
  plan->glyph_map = hb_map_create ();
  plan->reverse_glyph_map = hb_map_create ();
  plan->glyph_map_gsub = hb_map_create ();
  plan->gsub_lookups = hb_map_create ();
  plan->gpos_lookups = hb_map_create ();

  if (plan->check_success (plan->gsub_langsys = hb_object_create<script_langsys_map> ()))
    plan->gsub_langsys->init_shallow ();
  if (plan->check_success (plan->gpos_langsys = hb_object_create<script_langsys_map> ()))
    plan->gpos_langsys->init_shallow ();

  plan->gsub_features = hb_map_create ();
  plan->gpos_features = hb_map_create ();
  plan->colrv1_layers = hb_map_create ();
  plan->colr_palettes = hb_map_create ();
  plan->layout_variation_indices = hb_set_create ();
  plan->layout_variation_idx_map = hb_map_create ();

  if (unlikely (plan->in_error ())) {
    hb_subset_plan_destroy (plan);
    return nullptr;
  }

  _populate_unicodes_to_retain (input->sets.unicodes, input->sets.glyphs, plan);

  _populate_gids_to_retain (plan,
			    !input->sets.drop_tables->has (HB_OT_TAG_GSUB),
			    !input->sets.drop_tables->has (HB_OT_TAG_GPOS),
			    !input->sets.drop_tables->has (HB_OT_TAG_GDEF));

  _create_old_gid_to_new_gid_map (face,
                                  input->flags & HB_SUBSET_FLAGS_RETAIN_GIDS,
				  plan->_glyphset,
				  plan->glyph_map,
				  plan->reverse_glyph_map,
				  &plan->_num_output_glyphs);

  _create_glyph_map_gsub (
      plan->_glyphset_gsub,
      plan->glyph_map,
      plan->glyph_map_gsub);

  // Now that we have old to new gid map update the unicode to new gid list.
  for (unsigned i = 0; i < plan->unicode_to_new_gid_list.length; i++)
  {
    // Use raw array access for performance.
    plan->unicode_to_new_gid_list.arrayZ[i].second =
        plan->glyph_map->get(plan->unicode_to_new_gid_list.arrayZ[i].second);
  }

  if (unlikely (plan->in_error ())) {
    hb_subset_plan_destroy (plan);
    return nullptr;
  }
  return plan;
}

/**
 * hb_subset_plan_destroy:
 * @plan: a #hb_subset_plan_t
 *
 * Decreases the reference count on @plan, and if it reaches zero, destroys
 * @plan, freeing all memory.
 *
 * Since: 4.0.0
 **/
void
hb_subset_plan_destroy (hb_subset_plan_t *plan)
{
  if (!hb_object_destroy (plan)) return;

  hb_set_destroy (plan->unicodes);
  plan->unicode_to_new_gid_list.fini ();
  hb_set_destroy (plan->name_ids);
  hb_set_destroy (plan->name_languages);
  hb_set_destroy (plan->layout_features);
  hb_set_destroy (plan->glyphs_requested);
  hb_set_destroy (plan->drop_tables);
  hb_set_destroy (plan->no_subset_tables);
  hb_face_destroy (plan->source);
  hb_face_destroy (plan->dest);
  hb_map_destroy (plan->codepoint_to_glyph);
  hb_map_destroy (plan->glyph_map);
  hb_map_destroy (plan->reverse_glyph_map);
  hb_map_destroy (plan->glyph_map_gsub);
  hb_set_destroy (plan->_glyphset);
  hb_set_destroy (plan->_glyphset_gsub);
  hb_set_destroy (plan->_glyphset_mathed);
  hb_set_destroy (plan->_glyphset_colred);
  hb_map_destroy (plan->gsub_lookups);
  hb_map_destroy (plan->gpos_lookups);
  hb_map_destroy (plan->gsub_features);
  hb_map_destroy (plan->gpos_features);
  hb_map_destroy (plan->colrv1_layers);
  hb_map_destroy (plan->colr_palettes);
  hb_set_destroy (plan->layout_variation_indices);
  hb_map_destroy (plan->layout_variation_idx_map);

  if (plan->gsub_langsys)
  {
    for (auto _ : plan->gsub_langsys->iter ())
      hb_set_destroy (_.second);

    hb_object_destroy (plan->gsub_langsys);
    plan->gsub_langsys->fini_shallow ();
    hb_free (plan->gsub_langsys);
  }

  if (plan->gpos_langsys)
  {
    for (auto _ : plan->gpos_langsys->iter ())
      hb_set_destroy (_.second);

    hb_object_destroy (plan->gpos_langsys);
    plan->gpos_langsys->fini_shallow ();
    hb_free (plan->gpos_langsys);
  }

  hb_free (plan);
}

/**
 * hb_subset_plan_old_to_new_glyph_mapping:
 * @plan: a subsetting plan.
 *
 * Returns the mapping between glyphs in the original font to glyphs in the
 * subset that will be produced by @plan
 *
 * Return value: (transfer none):
 * A pointer to the #hb_map_t of the mapping.
 *
 * Since: 4.0.0
 **/
const hb_map_t*
hb_subset_plan_old_to_new_glyph_mapping (const hb_subset_plan_t *plan)
{
  return plan->glyph_map;
}

/**
 * hb_subset_plan_new_to_old_glyph_mapping:
 * @plan: a subsetting plan.
 *
 * Returns the mapping between glyphs in the subset that will be produced by
 * @plan and the glyph in the original font.
 *
 * Return value: (transfer none):
 * A pointer to the #hb_map_t of the mapping.
 *
 * Since: 4.0.0
 **/
const hb_map_t*
hb_subset_plan_new_to_old_glyph_mapping (const hb_subset_plan_t *plan)
{
  return plan->reverse_glyph_map;
}

/**
 * hb_subset_plan_unicode_to_old_glyph_mapping:
 * @plan: a subsetting plan.
 *
 * Returns the mapping between codepoints in the original font and the
 * associated glyph id in the original font.
 *
 * Return value: (transfer none):
 * A pointer to the #hb_map_t of the mapping.
 *
 * Since: 4.0.0
 **/
const hb_map_t*
hb_subset_plan_unicode_to_old_glyph_mapping (const hb_subset_plan_t *plan)
{
  return plan->codepoint_to_glyph;
}

/**
 * hb_subset_plan_reference: (skip)
 * @plan: a #hb_subset_plan_t object.
 *
 * Increases the reference count on @plan.
 *
 * Return value: @plan.
 *
 * Since: 4.0.0
 **/
hb_subset_plan_t *
hb_subset_plan_reference (hb_subset_plan_t *plan)
{
  return hb_object_reference (plan);
}

/**
 * hb_subset_plan_set_user_data: (skip)
 * @plan: a #hb_subset_plan_t object.
 * @key: The user-data key to set
 * @data: A pointer to the user data
 * @destroy: (nullable): A callback to call when @data is not needed anymore
 * @replace: Whether to replace an existing data with the same key
 *
 * Attaches a user-data key/data pair to the given subset plan object.
 *
 * Return value: %true if success, %false otherwise
 *
 * Since: 4.0.0
 **/
hb_bool_t
hb_subset_plan_set_user_data (hb_subset_plan_t   *plan,
                              hb_user_data_key_t *key,
                              void               *data,
                              hb_destroy_func_t   destroy,
                              hb_bool_t	          replace)
{
  return hb_object_set_user_data (plan, key, data, destroy, replace);
}

/**
 * hb_subset_plan_get_user_data: (skip)
 * @plan: a #hb_subset_plan_t object.
 * @key: The user-data key to query
 *
 * Fetches the user data associated with the specified key,
 * attached to the specified subset plan object.
 *
 * Return value: (transfer none): A pointer to the user data
 *
 * Since: 4.0.0
 **/
void *
hb_subset_plan_get_user_data (const hb_subset_plan_t *plan,
                              hb_user_data_key_t     *key)
{
  return hb_object_get_user_data (plan, key);
}
