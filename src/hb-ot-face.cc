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
 * Google Author(s): Behdad Esfahbod
 */

#include "hb-ot-face.hh"


void hb_ot_face_data_t::tables_t::init0 (hb_face_t *face)
{
  this->face = face;
#define HB_OT_LAYOUT_TABLE(Namespace, Type) Type.init0 ();
#define HB_OT_LAYOUT_ACCELERATOR(Namespace, Type) HB_OT_LAYOUT_TABLE (Namespace, Type)
  HB_OT_LAYOUT_TABLES
#undef HB_OT_LAYOUT_ACCELERATOR
#undef HB_OT_LAYOUT_TABLE
}
void hb_ot_face_data_t::tables_t::fini (void)
{
#define HB_OT_LAYOUT_TABLE(Namespace, Type) Type.fini ();
#define HB_OT_LAYOUT_ACCELERATOR(Namespace, Type) HB_OT_LAYOUT_TABLE (Namespace, Type)
  HB_OT_LAYOUT_TABLES
#undef HB_OT_LAYOUT_ACCELERATOR
#undef HB_OT_LAYOUT_TABLE
}

hb_ot_face_data_t *
_hb_ot_face_data_create (hb_face_t *face)
{
  hb_ot_face_data_t *data = (hb_ot_face_data_t *) calloc (1, sizeof (hb_ot_face_data_t));
  if (unlikely (!data))
    return nullptr;

  data->table.init0 (face);

  return data;
}

void
_hb_ot_face_data_destroy (hb_ot_face_data_t *data)
{
  data->table.fini ();
  free (data);
}

