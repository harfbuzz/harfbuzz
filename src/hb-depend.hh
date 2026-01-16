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

#ifndef HB_DEPEND_HH
#define HB_DEPEND_HH

#include "hb.hh"

#include "hb-depend.h"
#include "hb-depend-data.hh"

#ifdef HB_DEPEND_API

/**
 * hb_depend_t:
 *
 * Internal structure implementing the dependency graph API.
 * Contains the dependency data (data), the source font face (face),
 * and the set of features referenced by dependencies (features).
 *
 * Initialized via hb_depend_from_face() which computes the dependency
 * graph once. The graph remains immutable for the lifetime of the object.
 */
struct hb_depend_t
{
  HB_INTERNAL hb_depend_t (hb_face_t *face);

  HB_INTERNAL ~hb_depend_t();

  hb_object_header_t header;

  bool successful;

  bool in_error () const { return !successful; }

  bool check_success (bool success)
  {
    successful = (successful && success);
    return successful;
  }

  void print() { data.print(); }
  bool get_glyph_entry(hb_codepoint_t gid, hb_codepoint_t index,
                       hb_tag_t *table_tag, hb_codepoint_t *dependent,
                       hb_tag_t *layout_tag, hb_codepoint_t *ligature_set)
  {
    return data.get_glyph_entry(gid, index, table_tag, dependent, layout_tag,
                                ligature_set);
  }
  bool get_set_from_index(hb_codepoint_t index, hb_set_t *out)
  {
    return data.get_set_from_index(index, out);
  }

  void get_cmap_dependencies ();
  void get_gsub_dependencies ();
  void get_math_dependencies ();
  void get_colr_dependencies ();
  void get_glyf_dependencies ();
  void get_cff_dependencies ();

  hb_face_t *face;
  hb_set_t features;
  hb_depend_data_t data;
};

#endif /* HB_DEPEND_API */

#endif /* HB_DEPEND_HH */
