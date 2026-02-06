/*
 * Copyright © 2024  Adobe, Inc.
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
 * Adobe Author(s): Skef Iterum
 */

#include "hb.hh"

#include "hb-depend.hh"

#include "hb-face.hh"
#include "hb-ot-cmap-table.hh"
#include "hb-ot-glyf-table.hh"
#include "hb-ot-layout-gsub-table.hh"
#include "hb-ot-math-table.hh"
#include "hb-ot-cff1-table.hh"
#include "OT/Color/COLR/COLR.hh"
#include "OT/Color/COLR/colrv1-depend.hh"

#ifdef HB_DEPEND_API

/**
 * SECTION:hb-depend
 * @title: hb-depend
 * @short_description: Glyph dependency graph for font optimization
 * @include: hb-depend.h
 *
 * <warning>This API is highly experimental and subject to change. It is not
 * enabled by default and should not be used in production code without
 * understanding the stability implications.</warning>
 *
 * Functions for extracting a glyph dependency graph.
 *
 * A dependency graph represents the relationships between glyphs in a font,
 * tracking which glyphs reference or produce other glyphs through OpenType
 * mechanisms such as character mapping, glyph substitution, composite
 * construction, color layering, and math variants.
 *
 * Dependency graphs enable finding all glyphs reachable from a given input set.
 * This is useful for font subsetting, analyzing glyph coverage, optimizing
 * font delivery, and determining which glyphs are needed to render specific
 * characters.
 */

hb_depend_t::hb_depend_t (hb_face_t *f)
{
  if (unlikely (f == nullptr)) {
    successful = false;
    return;
  }
  face = hb_face_reference(f);
  if (unlikely (!data.glyph_dependencies.resize_exact (face->get_num_glyphs ()))) {
    successful = false;
    return;
  }
  successful = true;

  /* Extract dependencies from all relevant OpenType tables */
  get_cmap_dependencies();
  get_gsub_dependencies();
  get_math_dependencies();
  get_colr_dependencies();
  get_glyf_dependencies();
  get_cff_dependencies();

  /* Free temporary structures - no longer needed after graph extraction */
  data.seen_edges.fini();
  data.set_to_index.fini();
  data.unicodes.fini();
  data.nominal_glyphs.fini();
  for (auto &fs : data.lookup_features)
    fs.fini();
  data.lookup_features.fini();
}

/*
 * Algorithm: Character Map (cmap) Dependencies
 *
 * Collects nominal glyph mappings (Unicode codepoint -> glyph ID) and
 * Unicode Variation Sequence mappings (codepoint + selector -> glyph ID).
 * These form the base layer of dependencies showing which glyphs are directly
 * reachable from character input.
 */
void hb_depend_t::get_cmap_dependencies() {
  hb_face_collect_nominal_glyph_mapping(face, &data.nominal_glyphs, &data.unicodes);

  OT::cmap_accelerator_t cmap (face);
  cmap.table->depend(&data);
}

/*
 * Algorithm: Glyph Substitution (GSUB) Dependencies
 *
 * For each lookup in the GSUB table:
 * 1. Determine which features reference the lookup
 * 2. Call the lookup's depend() method which:
 *    - Iterates through covered glyphs (input glyphs)
 *    - Records each substitution (input -> output) as a dependency
 *    - Tags with the feature tag and table tag (GSUB)
 *    - For ligatures, assigns a ligature_set ID to group alternatives
 *
 * The result is a complete graph of all possible glyph substitutions
 * that could occur through OpenType Layout features, regardless of
 * script, language, or shaping context.
 */
void hb_depend_t::get_gsub_dependencies() {
  hb_blob_t *gsub_blob = hb_sanitize_context_t ().reference_table<OT::Layout::GSUB> (face);
  auto table = gsub_blob->as<OT::Layout::GSUB> ();
  unsigned num_features = table->get_feature_count ();
  unsigned num_lookups = table->get_lookup_count ();
  hb_vector_t<hb_tag_t> feature_tags;
  if (!check_success (feature_tags.resize (num_features)))
    return;
  table->get_feature_tags(0, &num_features, feature_tags.arrayZ);
  if (!check_success (data.lookup_features.resize (num_lookups)))
    return;

  hb_vector_t<hb_tag_t> feature_query_v;
  feature_query_v.resize (2);
  feature_query_v[1] = 0;

  hb_set_t feature_indexes, lookup_indexes;

  for (auto ft : feature_tags) {
    if (features.has (ft))
      continue;
    features.add (ft);
    feature_query_v[0] = ft;
    feature_indexes.reset ();
    hb_ot_layout_collect_features (face, HB_OT_TAG_GSUB, nullptr, nullptr,
                                   feature_query_v.arrayZ, &feature_indexes);
    lookup_indexes.reset ();
    for (auto feature_index : feature_indexes)
      table->get_feature (feature_index).add_lookup_indexes_to (&lookup_indexes);

    for (auto lookup_index : lookup_indexes) {
      data.lookup_features[lookup_index].add (ft);
    }

    auto &fv = table->get_feature_variations ();
    auto fi_count = fv.record_count();
    for (unsigned i = 0; i < fi_count; i++) {
      lookup_indexes.reset ();
      for (auto feature_index : feature_indexes) {
        auto feature_ptr = fv.find_substitute(i, feature_index);
        if (feature_ptr != nullptr)
          feature_ptr->add_lookup_indexes_to (&lookup_indexes);
      }
      for (auto lookup_index : lookup_indexes) {
        if (!data.lookup_features[lookup_index].has (ft))
          data.lookup_features[lookup_index].add (ft);
      }
    }
  }

  hb_set_t all_glyphs;
  all_glyphs.add_range (0, face->get_num_glyphs () - 1);

  // During graph extraction, all lookups see ALL glyphs, allowing us to record all possible edges.
  OT::hb_depend_context_t c (&data, face, &all_glyphs);

  /* Debug output at level 1: per-lookup processing details.
   * Use HB_DEBUG_DEPEND=2 or higher to see these messages. */
  int i = -1;
  for (auto &feature_set : data.lookup_features) {
    i++;
    if (feature_set.is_empty ()) {
      DEBUG_MSG_LEVEL (DEPEND, nullptr, 1, 0,
                       "Skipping lookup %d (no features)", i);
      continue;
    }
    DEBUG_MSG_LEVEL (DEPEND, nullptr, 1, 0,
                     "Processing lookup %d with features:", i);
    c.lookup_index = i;
    c.lookups_seen.clear ();
    table->get_lookup (i).depend (&c);
  }
}

/*
 * Algorithm: Math Variants (MATH) Dependencies
 *
 * Extracts dependencies from the MATH table for mathematical typesetting.
 * Records which glyphs are used as size variants or assembly components
 * for other glyphs, enabling proper rendering of mathematical expressions.
 */
void hb_depend_t::get_math_dependencies()
{
  hb_blob_t *math_blob = hb_sanitize_context_t ().reference_table<OT::MATH> (face);
  auto math = math_blob->as<OT::MATH> ();
  math->depend (&data);
}

/*
 * Algorithm: Color Layers (COLR) Dependencies
 *
 * For fonts with color glyph support, records which glyphs are used as
 * color layers for other glyphs. Both COLRv0 (simple layers) and COLRv1
 * (gradient/transform-based) are supported. Each base glyph depends on
 * its layer glyphs.
 */
void hb_depend_t::get_colr_dependencies()
{
  OT::COLR::accelerator_t colr (face);
  if (!colr.is_valid ()) return;
  colr.depend (&data);
}

/*
 * Algorithm: Composite Glyph (glyf) Dependencies
 *
 * For TrueType fonts, iterates through all glyphs and records component
 * dependencies for composite glyphs. A composite glyph depends on each
 * of its component glyphs. This is essential for subset operations to
 * preserve glyph integrity.
 */
void hb_depend_t::get_glyf_dependencies()
{
  OT::glyf_accelerator_t glyf (face);
  if (!glyf.has_data())
    return;
  for (hb_codepoint_t gid = 0; gid < glyf.get_num_glyphs (); gid++) {
    auto glyph = glyf.glyph_for_gid(gid);
    for (auto &item : glyph.get_composite_iterator ())
      data.add_depend (gid, HB_OT_TAG_glyf, item.get_gid ());
  }
}

/*
 * Algorithm: CFF/CFF2 SEAC Dependencies
 *
 * CFF1 supports SEAC (Standard Encoding Accented Character), which allows
 * a glyph to be defined as a combination of two component glyphs (base and
 * accent). This is similar to glyf composite glyphs but specific to CFF1.
 *
 * CFF2 does not support SEAC, so we only check CFF1 tables.
 *
 * For each glyph, we check if it uses SEAC and record dependencies to its
 * base and accent components.
 */
void hb_depend_t::get_cff_dependencies()
{
  OT::cff1::accelerator_subset_t cff (face);
  if (!cff.is_valid())
    return;

  unsigned int num_glyphs = face->get_num_glyphs();
  for (hb_codepoint_t gid = 0; gid < num_glyphs; gid++) {
    hb_codepoint_t base_gid, accent_gid;
    if (cff.get_seac_components(gid, &base_gid, &accent_gid)) {
      // SEAC glyph found - add dependencies to base and accent
      data.add_depend(gid, HB_TAG('C','F','F',' '), base_gid);
      data.add_depend(gid, HB_TAG('C','F','F',' '), accent_gid);
    }
  }
}

hb_depend_t::~hb_depend_t ()
{
  hb_face_destroy(face);
}


/**
 * hb_depend_from_face_or_fail:
 * @face: font face to collect dependencies from
 *
 * Calculates the dependencies between glyphs in the supplied face.
 * Extracts dependency information from cmap, GSUB, glyf, CFF, COLR,
 * and MATH tables.
 *
 * Example:
 * ```c
 * hb_depend_t *depend = hb_depend_from_face_or_fail (face);
 * if (!depend) {
 *   // Handle error (OOM or invalid face)
 *   return;
 * }
 * // ... use depend ...
 * hb_depend_destroy (depend);
 * ```
 *
 * Return value: (transfer full): New depend object, or `nullptr` if creation
 * failed (out of memory or invalid face). Destroy with hb_depend_destroy().
 *
 * Since: REPLACEME
 **/
hb_depend_t *
hb_depend_from_face_or_fail (hb_face_t *face)
{
  hb_depend_t *depend;
  if (unlikely (!(depend = hb_object_create<hb_depend_t> (face))))
    return nullptr;

  if (unlikely (depend->in_error ()))
  {
    hb_depend_destroy (depend);
    return nullptr;
  }

  return depend;
}

/**
 * hb_depend_get_glyph_entry:
 * @depend: depend object
 * @gid: GID to retrieve dependencies from
 * @index: The index of the entry values to retrieve
 * @table_tag: (out): Returns the tag of the table associated with the entry
 * @dependent: (out): Returns the GID of the dependent glyph
 * @layout_tag: (out): Returns the layout tag associated with the dependency
 * (when table_tag is GSUB) or the UVS codepoint (when table_tag is cmap).
 * Otherwise is HB_CODEPOINT_INVALID
 * @ligature_set: (out): Returns the index of the ligature set for this entry, or
 * HB_CODEPOINT_INVALID if there is no such set
 * @context_set: (out): Returns the index of the context set for this entry, or
 * HB_CODEPOINT_INVALID if there is no such set. Context sets specify which
 * backtrack and lookahead glyphs must be present for this dependency to apply
 * (currently only populated for contextual GSUB substitutions).
 *
 * Get the values associated with a dependency entry for a glyph.
 * Dependencies are indexed sequentially starting from 0.
 *
 * Example:
 * ```c
 * hb_codepoint_t index = 0;
 * hb_tag_t table_tag;
 * hb_codepoint_t dependent;
 * hb_tag_t layout_tag;
 * hb_codepoint_t ligature_set;
 * hb_codepoint_t context_set;
 *
 * while (hb_depend_get_glyph_entry (depend, gid, index++,
 *                                    &table_tag, &dependent,
 *                                    &layout_tag, &ligature_set, &context_set)) {
 *   // Process dependency...
 * }
 * ```
 *
 * Return value: true if there is such an entry, false otherwise
 *
 * Since: REPLACEME
 **/
hb_bool_t
hb_depend_get_glyph_entry(hb_depend_t *depend, hb_codepoint_t gid,
                          hb_codepoint_t index, hb_tag_t *table_tag,
                          hb_codepoint_t *dependent, hb_tag_t *layout_tag,
                          hb_codepoint_t *ligature_set, hb_codepoint_t *context_set,
                          uint8_t *flags)
{
  return depend->get_glyph_entry(gid, index, table_tag, dependent, layout_tag,
                                 ligature_set, context_set, flags);
}

/**
 * hb_depend_get_set_from_index:
 * @depend: depend object
 * @index: the index of the set
 * @out: (out): A pointer to a set to copy into
 *
 * Get all glyphs in a ligature set identified by @index.
 * The ligature set index comes from the ligature_set field
 * returned by hb_depend_get_glyph_entry().
 *
 * Example:
 * ```c
 * hb_set_t *ligature_glyphs = hb_set_create ();
 * if (hb_depend_get_set_from_index (depend, ligature_set, ligature_glyphs)) {
 *   // Process glyphs in the set...
 * }
 * hb_set_destroy (ligature_glyphs);
 * ```
 *
 * Return value: true if there is such a set, false otherwise
 *
 * Since: REPLACEME
 **/
hb_bool_t
hb_depend_get_set_from_index(hb_depend_t *depend, hb_codepoint_t index,
                             hb_set_t *out) {
  return depend->get_set_from_index(index, out);
}

/**
 * hb_depend_print:
 * @depend: a #hb_depend_t
 *
 * Prints the dependency graph to standard output for debugging purposes.
 * For each glyph that has dependencies, prints the glyph ID followed by
 * its dependent glyphs, including the source table (cmap, GSUB, glyf,
 * COLR, MATH) and any relevant feature tags or variant information.
 *
 * Note: This function is primarily for debugging and testing. The output
 * format is not stable and may change in future versions. For programmatic
 * access to dependency information, use hb_depend_get_glyph_entry().
 *
 * Since: REPLACEME
 **/
void
hb_depend_print (hb_depend_t *depend)
{
  depend->print();
}

/**
 * hb_depend_destroy:
 * @depend: a #hb_depend_t
 *
 * Decreases the reference count on @depend, and if it reaches zero, destroys
 * @depend, freeing all memory.
 *
 * Since: REPLACEME
 **/
void
hb_depend_destroy (hb_depend_t *depend)
{
  if (!hb_object_destroy (depend)) return;

  hb_free (depend);
}

#endif /* HB_DEPEND_API */
