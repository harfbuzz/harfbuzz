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


#include "hb-subset-private.hh"

#include "hb-subset-plan.hh"
#include "hb-ot-cmap-table.hh"
#include "hb-ot-glyf-table.hh"



HB_INTERNAL hb_face_t &
hb_subset_plan_source_face(hb_subset_plan_t *plan)
{
  return *(plan->source);
}

HB_INTERNAL hb_face_t &
hb_subset_plan_dest_face(hb_subset_plan_t *plan)
{
  return *(plan->dest);
}

HB_INTERNAL hb_blob_t *
hb_subset_plan_ref_source_table(hb_subset_plan_t *plan, hb_tag_t tag)
{
  return plan->source->reference_table(tag);
}

static int
_hb_codepoint_t_cmp (const void *pa, const void *pb)
{
  hb_codepoint_t a = * (hb_codepoint_t *) pa;
  hb_codepoint_t b = * (hb_codepoint_t *) pb;

  return a < b ? -1 : a > b ? +1 : 0;
}

hb_bool_t
hb_subset_plan_add_table (hb_subset_plan_t *plan,
                          hb_tag_t tag,
                          hb_blob_t *contents)
{
  return hb_subset_face_add_table(plan->dest, tag, contents);
}

static void
_add_gid_and_children (const OT::glyf::accelerator_t &glyf,
                       hb_subset_plan_t *plan,
                       hb_set_t *visited,
                       hb_codepoint_t source_gid)
{
  if (hb_set_has (visited, source_gid)) return;
  hb_set_add(visited, source_gid);

  hb_set_add (plan->gids_to_retain, source_gid);

  OT::glyf::CompositeGlyphHeader::Iterator composite;
  if (!glyf.get_composite (source_gid, &composite)) return;
  do
  {
    _add_gid_and_children (glyf, plan, visited, (hb_codepoint_t) composite.current->glyphIndex);
  } while (composite.move_to_next());
}

static void
_populate_gids_to_retain (hb_face_t *face, hb_subset_plan_t *plan,
                          std::unordered_map<hb_codepoint_t, hb_codepoint_t> &source_gid_by_codepoint)
{
  hb_set_add (plan->gids_to_retain, 0); /* always keep 0 */

  /* then all the gids for codepoints by cmap */
  OT::cmap::accelerator_t cmap;
  cmap.init (face);
  hb_codepoint_t codepoint = HB_SET_VALUE_INVALID;
  while (hb_set_next(plan->codepoints, &codepoint))
  {
    hb_codepoint_t source_gid;
    if (cmap.get_nominal_glyph (codepoint, &source_gid))
    {
      hb_set_add (plan->gids_to_retain, source_gid);
      source_gid_by_codepoint[codepoint] = source_gid;
    }
    else
    {
      hb_set_del (plan->codepoints, codepoint);
      DEBUG_MSG(SUBSET, nullptr, "Drop U+%04X; no gid", codepoint);
    }
  }
  cmap.fini ();

  /* Then all the gids by closure over composite glyphs */
  OT::glyf::accelerator_t glyf;
  glyf.init (face);
  unsigned int gids_before_children = hb_set_get_population(plan->gids_to_retain);
  hb_codepoint_t source_gid = HB_SET_VALUE_INVALID;
  hb_set_t *visited = hb_set_create();
  while (hb_set_next(plan->gids_to_retain, &source_gid))
  {
    _add_gid_and_children (glyf, plan, visited, source_gid);
  }
  hb_set_destroy (visited);
  DEBUG_MSG(SUBSET, nullptr, "Added %u glphs for composite closure", hb_set_get_population(plan->gids_to_retain) - gids_before_children);

  // TODO expand with glyphs reached by G*

  glyf.fini ();
}

static void _populate_dest_by_source (hb_subset_plan_t *plan)
{
  hb_codepoint_t source_gid = HB_SET_VALUE_INVALID;
  unsigned int i = 0;
  while (hb_set_next (plan->gids_to_retain, &source_gid))
  {
    hb_map_put (plan->dest_gid_by_source_gid, source_gid, i);
    i++;
  }
}

static void _populate_dest_by_codepoint (hb_subset_plan_t *plan,
                                         std::unordered_map<hb_codepoint_t, hb_codepoint_t> &source_gid_by_codepoint)
{
  for (auto it = source_gid_by_codepoint.begin(); it != source_gid_by_codepoint.end(); it++)
  {
    hb_codepoint_t dest_gid;
    hb_map_get (plan->dest_gid_by_source_gid, it->second, &dest_gid);
    hb_map_put (plan->dest_gid_by_codepoint, it->first, dest_gid);
    DEBUG_MSG(SUBSET, nullptr, "U+%04x src_gid %d dest_gid %d", it->first, it->second, dest_gid);
  }
}

/**
 * hb_subset_plan_create:
 * Computes a plan for subsetting the supplied face according
 * to a provide profile and input. The plan describes
 * which tables and glyphs should be retained.
 *
 * Return value: New subset plan.
 *
 * Since: 1.7.5
 **/
hb_subset_plan_t *
hb_subset_plan_create (hb_face_t           *face,
                       hb_subset_profile_t *profile,
                       hb_subset_input_t   *input)
{
  // TODO check that the creates here succeed before populating things
  hb_subset_plan_t *plan = hb_object_create<hb_subset_plan_t> ();
  plan->codepoints = hb_set_create ();
  hb_set_set (plan->codepoints, input->unicodes);
  plan->gids_to_retain = hb_set_create();
  plan->dest_gid_by_source_gid = hb_map_create_or_fail();
  plan->dest_gid_by_codepoint = hb_map_create_or_fail();
  plan->source = hb_face_reference (face);
  plan->dest = hb_subset_face_create ();

  std::unordered_map<hb_codepoint_t, hb_codepoint_t> source_gid_by_codepoint;
  _populate_gids_to_retain (face, plan, source_gid_by_codepoint);
  _populate_dest_by_source (plan);
  _populate_dest_by_codepoint (plan, source_gid_by_codepoint);

  return plan;
}

/**
 * hb_subset_plan_destroy:
 *
 * Since: 1.7.5
 **/
void
hb_subset_plan_destroy (hb_subset_plan_t *plan)
{
  if (!hb_object_destroy (plan)) return;

  hb_set_destroy (plan->codepoints);
  hb_set_destroy (plan->gids_to_retain);
  hb_map_destroy (plan->dest_gid_by_codepoint);
  hb_map_destroy (plan->dest_gid_by_source_gid);

  hb_face_destroy (plan->source);
  hb_face_destroy (plan->dest);

  free (plan);
}
