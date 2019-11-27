/*
 * Copyright © 2019  Ebrahim Byagowi
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
 */

#include "hb.hh"

#ifndef HB_NO_OT_GLYPH

#include "hb-ot.h"
#include "hb-ot-glyf-table.hh"
#include "hb-ot-cff1-table.hh"
#include "hb-ot-cff2-table.hh"

struct hb_ot_glyph_path_t
{
  hb_object_header_t header;

  const hb_position_t *coords;
  unsigned int coords_count;

  const uint8_t *commands;
  unsigned int commands_count;

  void *user_data;
  hb_destroy_func_t destroy;
};

hb_ot_glyph_path_t *
hb_ot_glyph_path_empty ()
{
  return const_cast<hb_ot_glyph_path_t *> (&Null (hb_ot_glyph_path_t));
}

hb_ot_glyph_path_t *
hb_ot_glyph_path_create (hb_position_t     *coords,
			 unsigned int       coords_count,
			 uint8_t           *commands,
			 unsigned int       commands_count,
			 void              *user_data,
			 hb_destroy_func_t  destroy)
{
  hb_ot_glyph_path_t *path;

  if (!coords_count || !commands_count ||
      !(path = hb_object_create<hb_ot_glyph_path_t> ()))
  {
    if (destroy)
      destroy (user_data);
    return hb_ot_glyph_path_empty ();
  }

  path->coords = coords;
  path->coords_count =  coords_count;
  path->commands = commands;
  path->commands_count = commands_count;
  path->user_data = user_data;
  path->destroy = destroy;

  return path;
}

const hb_position_t *
hb_ot_glyph_path_get_coords (hb_ot_glyph_path_t *path, unsigned int *count)
{
  if (count) *count = path->coords_count;
  return path->coords;
}

const uint8_t *
hb_ot_glyph_path_get_commands (hb_ot_glyph_path_t *path, unsigned int *count)
{
  if (count) *count = path->commands_count;
  return path->commands;
}

hb_ot_glyph_path_t *
hb_ot_glyph_path_reference (hb_ot_glyph_path_t *path)
{
  return hb_object_reference (path);
}

void
hb_ot_glyph_path_destroy (hb_ot_glyph_path_t *path)
{
  if (!hb_object_destroy (path)) return;
}

struct _hb_ot_glyph_path_vectors
{
  hb_vector_t<hb_position_t> *coords;
  hb_vector_t<uint8_t> *commands;

  bool init ()
  {
    coords = (hb_vector_t<hb_position_t> *) malloc (sizeof (hb_vector_t<hb_position_t>));
    commands = (hb_vector_t<uint8_t> *) malloc (sizeof (hb_vector_t<uint8_t>));
    if (unlikely (!coords || !commands))
    {
      free (coords);
      free (commands);
      return false;
    }
    coords->init ();
    commands->init ();
    return true;
  }

  void fini ()
  {
    coords->fini ();
    commands->fini ();
    free (coords);
    free (commands);
  }
};

static void
_hb_ot_glyph_path_free_vectors (void *user_data)
{
  ((_hb_ot_glyph_path_vectors *) user_data)->fini ();
  free (user_data);
}

hb_ot_glyph_path_t *
hb_ot_glyph_path_create_from_font (hb_font_t *font, hb_codepoint_t glyph)
{
  _hb_ot_glyph_path_vectors *user_data = (_hb_ot_glyph_path_vectors *)
					 malloc (sizeof (_hb_ot_glyph_path_vectors));
  if (unlikely (!user_data->init ())) return hb_ot_glyph_path_empty ();

  hb_vector_t<hb_position_t> &coords = *user_data->coords;
  hb_vector_t<uint8_t> &commands = *user_data->commands;

  bool ret = false;

  if (!ret) ret = font->face->table.glyf->get_path (font, glyph, &coords, &commands);
#ifndef HB_NO_OT_FONT_CFF
  if (!ret) ret = font->face->table.cff1->get_path (font, glyph, &coords, &commands);
  if (!ret) ret = font->face->table.cff2->get_path (font, glyph, &coords, &commands);
#endif

  assert (coords.length % 2 == 0); /* coords pairs, should be an even number */

  if (unlikely (!ret)) return hb_ot_glyph_path_empty ();

  return hb_ot_glyph_path_create (coords.arrayZ, coords.length, commands.arrayZ, commands.length,
				  user_data, (hb_destroy_func_t) _hb_ot_glyph_path_free_vectors);
}

#endif
