/*
 * Copyright © 2026  Behdad Esfahbod
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
 * Author(s): Behdad Esfahbod
 */

#include "hb.hh"

#include "hb-raster-svg-defs.hh"

#include <string.h>

hb_svg_defs_t::~hb_svg_defs_t ()
{
  for (unsigned i = 0; i < owned_id_strings.length; i++)
    hb_free (owned_id_strings.arrayZ[i]);
}

bool
hb_svg_defs_t::add_id_mapping (hb_hashmap_t<hb_bytes_t, unsigned> *map,
                               const char *id,
                               unsigned idx)
{
  hb_bytes_t key = hb_bytes_t (id, (unsigned) strlen (id));
  if (map->has (key))
    return true;

  unsigned n = (unsigned) strlen (id);
  char *owned = (char *) hb_malloc (n + 1);
  if (unlikely (!owned))
    return false;
  hb_memcpy (owned, id, n + 1);
  if (unlikely (!owned_id_strings.push (owned)))
  {
    hb_free (owned);
    return false;
  }

  hb_bytes_t owned_key = hb_bytes_t (owned, n);
  if (unlikely (!map->set (owned_key, idx)))
    return false;
  return true;
}

bool
hb_svg_defs_t::add_gradient (const char *id, const hb_svg_gradient_t &grad)
{
  unsigned idx = gradients.length;
  gradients.push (grad);
  if (unlikely (gradients.in_error ()))
    return false;

  hb_svg_def_t def;
  unsigned n = hb_min (strlen (id), (size_t) sizeof (def.id) - 1);
  memcpy (def.id, id, n);
  def.id[n] = '\0';
  def.type = hb_svg_def_t::DEF_GRADIENT;
  def.index = idx;
  defs.push (def);
  if (unlikely (defs.in_error ()))
  {
    gradients.pop ();
    return false;
  }
  if (unlikely (!add_id_mapping (&gradient_by_id, def.id, idx)))
  {
    defs.pop ();
    gradients.pop ();
    return false;
  }
  return true;
}

const hb_svg_gradient_t *
hb_svg_defs_t::find_gradient (const char *id) const
{
  unsigned *idx = nullptr;
  if (id && gradient_by_id.has (hb_bytes_t (id, (unsigned) strlen (id)), &idx))
    return &gradients[*idx];
  for (unsigned i = 0; i < defs.length; i++)
    if (defs[i].type == hb_svg_def_t::DEF_GRADIENT &&
        strcmp (defs[i].id, id) == 0)
      return &gradients[defs[i].index];
  return nullptr;
}

bool
hb_svg_defs_t::add_clip_path (const char *id, const hb_svg_clip_path_def_t &clip)
{
  unsigned idx = clip_paths.length;
  clip_paths.push (clip);
  if (unlikely (clip_paths.in_error ()))
    return false;

  hb_svg_def_t def;
  unsigned n = hb_min (strlen (id), (size_t) sizeof (def.id) - 1);
  memcpy (def.id, id, n);
  def.id[n] = '\0';
  def.type = hb_svg_def_t::DEF_CLIP_PATH;
  def.index = idx;
  defs.push (def);
  if (unlikely (defs.in_error ()))
  {
    clip_paths.pop ();
    return false;
  }
  if (unlikely (!add_id_mapping (&clip_path_by_id, def.id, idx)))
  {
    defs.pop ();
    clip_paths.pop ();
    return false;
  }
  return true;
}

const hb_svg_clip_path_def_t *
hb_svg_defs_t::find_clip_path (const char *id) const
{
  unsigned *idx = nullptr;
  if (id && clip_path_by_id.has (hb_bytes_t (id, (unsigned) strlen (id)), &idx))
    return &clip_paths[*idx];
  for (unsigned i = 0; i < defs.length; i++)
    if (defs[i].type == hb_svg_def_t::DEF_CLIP_PATH &&
        strcmp (defs[i].id, id) == 0)
      return &clip_paths[defs[i].index];
  return nullptr;
}
