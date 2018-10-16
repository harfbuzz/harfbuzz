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

#include "hb.hh"

#include "hb-ot-name-table.hh"

#include "hb-ot-face.hh"
#include "hb-utf.hh"


static inline const OT::name_accelerator_t&
_get_name (hb_face_t *face)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face))) return Null(OT::name_accelerator_t);
  return *(hb_ot_face_data (face)->name.get ());
}


unsigned int
hb_ot_name_get_names (hb_face_t                 *face,
		      const hb_ot_name_entry_t **entries /* OUT */)
{
  const OT::name_accelerator_t &name = _get_name (face);
  if (entries)
    *entries = name.names.arrayZ();
  return name.names.len;
}


template <typename utf_t>
static inline hb_bool_t
hb_ot_name_get_utf (hb_face_t     *face,
		    hb_name_id_t   name_id,
		    hb_language_t  language,
		    unsigned int  *text_size /* IN/OUT */,
		    typename utf_t::codepoint_t *text /* OUT */)
{
  const OT::name_accelerator_t &name = _get_name (face);
  unsigned int idx = 0; //XXX
  hb_bytes_t bytes = name.table->get_name (idx);
  if (text_size && *text_size)
  {
    /* TODO Switch to walking string and validating. */
    memcpy (text, bytes.arrayZ, MIN (*text_size * 2, bytes.len));
  }
  /* XXX Null-terminate. */
  if (text_size)
    *text_size = bytes.len / 2; //TODO
  /* TODO Fallback? */
  return true; //XXX
}

hb_bool_t
hb_ot_name_get_utf16 (hb_face_t     *face,
		      hb_name_id_t   name_id,
		      hb_language_t  language,
		      unsigned int  *text_size /* IN/OUT */,
		      uint16_t      *text      /* OUT */)
{
  return hb_ot_name_get_utf<hb_utf16_t> (face, name_id, language, text_size, text);
}
