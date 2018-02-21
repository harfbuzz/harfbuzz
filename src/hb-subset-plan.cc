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

#include <unordered_map>
#include <vector>
#include <utility>

#include "hb-subset-private.hh"

#include "hb-subset-plan.hh"
#include "hb-ot-cmap-table.hh"
#include "hb-ot-glyf-table.hh"

struct hb_subset_plan_t {
  hb_object_header_t header;

  hb_prealloced_array_t<hb_codepoint_t> codepoints_sorted;

  /* codepoint => old gid based on cmap*/
  std::unordered_map<hb_codepoint_t, hb_codepoint_t> *dest_gid_by_codepoint;

  /* new gid by old gid convenience lookup */
  std::unordered_map<hb_codepoint_t, hb_codepoint_t> *dest_gid_by_source_gid;

  /* Source gids to retain in dest order.
   * Ordered to produce runs for sequential codepoints:
   *  0
   *  gids for codepoints in the same order as codepoints_sorted
   *  gids added by closure over composite glyph or G*
   */
  hb_prealloced_array_t<hb_codepoint_t> gids_to_retain;

  // Plan is only good for a specific source/dest so keep them with it
  hb_face_t *source;
  hb_face_t *dest;
};

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

HB_INTERNAL size_t
hb_subset_plan_get_num_glyphs(hb_subset_plan_t *plan)
{
  return plan->gids_to_retain.len;
}

HB_INTERNAL hb_prealloced_array_t<hb_codepoint_t> &
hb_subset_plan_get_codepoints_sorted(hb_subset_plan_t *plan)
{
  return plan->codepoints_sorted;
}

HB_INTERNAL hb_prealloced_array_t<hb_codepoint_t> &
hb_subset_plan_get_source_gids_in_dest_order (hb_subset_plan_t *plan)
{
  return plan->gids_to_retain;
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
hb_subset_plan_new_gid_for_codepoint (hb_subset_plan_t *plan,
                                      hb_codepoint_t codepoint,
                                      hb_codepoint_t *new_gid)
{
  auto it = plan->dest_gid_by_codepoint->find (codepoint);
  if (it == plan->dest_gid_by_codepoint->end ()) return false;
  *new_gid = it->second;
  return true;
}

hb_bool_t
hb_subset_plan_new_gid_for_old_gid (hb_subset_plan_t *plan,
                                   hb_codepoint_t old_gid,
                                   hb_codepoint_t *new_gid)
{
  auto it = plan->dest_gid_by_source_gid->find(old_gid);
  if (it == plan->dest_gid_by_source_gid->end()) return false;
  *new_gid = it->second;
  return true;
}

hb_bool_t
hb_subset_plan_add_table (hb_subset_plan_t *plan,
                          hb_tag_t tag,
                          hb_blob_t *contents)
{
  return hb_subset_face_add_table(plan->dest, tag, contents);
}

static void
_populate_codepoints (hb_set_t *input_codepoints,
                      hb_prealloced_array_t<hb_codepoint_t>& plan_codepoints)
{
  plan_codepoints.alloc (hb_set_get_population (input_codepoints));
  hb_codepoint_t cp = -1;
  while (hb_set_next (input_codepoints, &cp)) {
    hb_codepoint_t *wr = plan_codepoints.push();
    *wr = cp;
  }
  plan_codepoints.qsort (_hb_codepoint_t_cmp);
}

static hb_codepoint_t
_retain_gid(hb_subset_plan_t *plan, hb_codepoint_t source_gid)
{
  std::unordered_map<hb_codepoint_t, hb_codepoint_t> &dest_gid_by_source_gid = *plan->dest_gid_by_source_gid;
  *plan->gids_to_retain.push() = source_gid;
  hb_codepoint_t dest_gid = (hb_codepoint_t) dest_gid_by_source_gid.size();
  dest_gid_by_source_gid[source_gid] = dest_gid;
  return dest_gid;
}

static void
_add_gid_and_children (const OT::glyf::accelerator_t &glyf,
                       hb_subset_plan_t *plan,
                       hb_codepoint_t source_gid)
{
  std::unordered_map<hb_codepoint_t, hb_codepoint_t> &dest_gid_by_source_gid = *plan->dest_gid_by_source_gid;
  if (dest_gid_by_source_gid.find(source_gid) == dest_gid_by_source_gid.end())
  {
    DEBUG_MSG(SUBSET, nullptr, "Add source_gid %d for composite, dest_gid %d", source_gid, dest_gid_by_source_gid.size());
    _retain_gid(plan, source_gid);
  }

  OT::glyf::CompositeGlyphHeader::Iterator composite;
  if (!glyf.get_composite (source_gid, &composite)) return;
  {
    do
    {
      _add_gid_and_children (glyf, plan, (hb_codepoint_t) composite.current->glyphIndex);
    } while (composite.move_to_next());
  }
}

static void
_populate_gids_to_retain (hb_face_t *face, hb_subset_plan_t *plan)
{

  OT::cmap::accelerator_t cmap;
  OT::glyf::accelerator_t glyf;
  cmap.init (face);
  glyf.init (face);

  _retain_gid(plan, 0); /* always keep 0 */

  /* then all the gids for codepoints by cmap */
  hb_prealloced_array_t<hb_codepoint_t> &codepoints_sorted = plan->codepoints_sorted;
  for (unsigned int i = 0; i < codepoints_sorted.len; i++)
  {
    hb_codepoint_t source_gid;
    hb_codepoint_t codepoint = codepoints_sorted[i];
    if (cmap.get_nominal_glyph (codepoints_sorted[i], &source_gid))
    {
      hb_codepoint_t dest_gid = _retain_gid(plan, source_gid);
      (*plan->dest_gid_by_codepoint)[codepoint] = dest_gid;
      DEBUG_MSG(SUBSET, nullptr, "U+%04X was gid %u now %u", codepoint, source_gid, dest_gid);
    }
    else
      DEBUG_MSG(SUBSET, nullptr, "Drop U+%04X; no gid", codepoint);
  }

  /* Then all the gids by closure over composite glyphs */
  /* Add them on the end of gids_to_retain to avoid breaking runs */
  unsigned int gids_before_children = plan->gids_to_retain.len;
  for (unsigned int i = 0; i < gids_before_children; i++)
  {
    _add_gid_and_children (glyf, plan, plan->gids_to_retain[i]);
  }
  DEBUG_MSG(SUBSET, nullptr, "Added %u glphs for composite closure", (unsigned int) plan->dest_gid_by_source_gid->size() - gids_before_children);

  // TODO expand with glyphs reached by G*

  glyf.fini ();
  cmap.fini ();
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
  hb_subset_plan_t *plan = hb_object_create<hb_subset_plan_t> ();

  plan->codepoints_sorted.init();
  plan->gids_to_retain.init();
  plan->dest_gid_by_codepoint = new std::unordered_map<hb_codepoint_t, hb_codepoint_t>();
  plan->dest_gid_by_source_gid = new std::unordered_map<hb_codepoint_t, hb_codepoint_t>();
  plan->source = hb_face_reference (face);
  plan->dest = hb_subset_face_create ();
  _populate_codepoints (input->unicodes, plan->codepoints_sorted);
  _populate_gids_to_retain (face, plan);

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

  plan->codepoints_sorted.finish ();
  plan->gids_to_retain.finish ();
  delete plan->dest_gid_by_codepoint;
  delete plan->dest_gid_by_source_gid;

  hb_face_destroy (plan->source);
  hb_face_destroy (plan->dest);

  free (plan);
}
