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
#include "OT/Color/COLR/COLR.hh"
#include "OT/Color/COLR/colrv1-depend.hh"

#ifdef HB_DEPEND_API

/**
 * SECTION:hb-depend
 * @title: hb-depend
 * @short_description: Represents dependencies between glyphs in an OpenType font
 * @include: hb-depend.h
 *
 * XXX Todo.
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

  get_cmap_dependencies();
  get_gsub_dependencies();
  get_math_dependencies();
  get_colr_dependencies();
  get_glyf_dependencies();
}

void hb_depend_t::get_cmap_dependencies() {
  hb_face_collect_nominal_glyph_mapping(face, &data.nominal_glyphs, &data.unicodes);

  OT::cmap_accelerator_t cmap (face);
  cmap.table->depend(&data);
}

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
        // auto rec = data.lookup_info[lookup_index].get (ft);
        // rec.fv_indexes.add(i);
      }
    }
  }

  OT::hb_depend_context_t c (&data, face);
  int i = -1;
  for (auto &feature_set : data.lookup_features) {
    i++;
    if (feature_set.is_empty ())
      continue;
    c.lookup_index = i;
    c.lookups_seen.clear ();
    c.set_recurse_func (OT::Layout::GSUB_impl::SubstLookup::dispatch_recurse_func<OT::hb_depend_context_t>);
    c.recurse(i);
  }
}

void hb_depend_t::get_math_dependencies()
{
  hb_blob_t *math_blob = hb_sanitize_context_t ().reference_table<OT::MATH> (face);
  auto math = math_blob->as<OT::MATH> ();
  math->depend (&data);
}

void hb_depend_t::get_colr_dependencies()
{
  OT::COLR::accelerator_t colr (face);
  if (!colr.is_valid ()) return;
  colr.depend (&data);
}

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

hb_depend_t::~hb_depend_t ()
{
  hb_face_destroy(face);
}


/**
 * hb_depend_from_face:
 * @face: font face to collect dependencies from
 *
 * Calculates the dependencies between glyphs in the supplied face
 *
 * Return value: (transfer full): New depend object. Destroy with
 * hb_depend_destroy(). If there is a failure creating the depend
 * object * nullptr will be returned.
 *
 * Since: ?.0.0
 **/
hb_depend_t *
hb_depend_from_face (hb_face_t *face)
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
 * @gid: GID to retrive dependencies from
 * @index: The index of the entry values to retrieve
 * @table_tag: Returns the tag of the table associated with the entry
 * @dependent: Returns the GID of the dependent glyph
 * @layout_tag: Returns the layout tag associated with the dependency
 * (when table_tag is GSUB) or the UVS codepoint (when table_tag is cmap)
 * Otherwise is HB_CODEPOINT_INVALID
 * @ligature_set: Returns the index of the ligature set for this entry, or
 * HB_CODEPOINT_INVALID if there is no such set
 *
 * Get the values associated with a dependency entry for a glyph
 *
 * Return value: true if there is such an entry, false otherwise
 *
 * Since: ?.0.0
 **/
hb_bool_t
hb_depend_get_glyph_entry(hb_depend_t *depend, hb_codepoint_t gid,
                          hb_codepoint_t index, hb_tag_t *table_tag,
                          hb_codepoint_t *dependent, hb_tag_t *layout_tag,
                          hb_codepoint_t *ligature_set)
{
  return depend->get_glyph_entry(gid, index, table_tag, dependent, layout_tag,
                                 ligature_set);
}

/**
 * hb_depend_get_set_from_index:
 * @depend: depend object
 * @index: the index of the set
 * @out: A pointer to a set to copy into
 *
 * Return value: true if there is such an entry, false otherwise
 *
 * Since: ?.0.0
 **/
hb_bool_t
hb_depend_get_set_from_index(hb_depend_t *depend, hb_codepoint_t index,
                             hb_set_t *out) {
  return depend->get_set_from_index(index, out);
}

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
 * Since: ?.0.0
 **/
void
hb_depend_destroy (hb_depend_t *depend)
{
  if (!hb_object_destroy (depend)) return;

  hb_free (depend);
}

#endif /* HB_DEPEND_API */
