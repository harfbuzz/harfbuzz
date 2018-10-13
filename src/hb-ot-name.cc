/*
 * Copyright © 2018  Ebrahim Byagowi
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

#include "hb-ot-face.hh"
#include "hb-ot-name-table.hh"

static inline const OT::name& _get_name (hb_face_t *face)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face))) return Null(OT::name);
  return *hb_ot_face_data (face)->name;
}

static inline unsigned int
_to_ot_language_id (hb_language_t language HB_UNUSED)
{
  // our super secrect converter
  return 0;
}


/**
 * hb_ot_name_get_string:
 * @face: #hb_face_t
 * @name_id:
 * @languages_id:
 * @languages_count:
 * @buffer_length: (in/out)
 * @buffer: (out)
 *
 * Return value: The string buffer size
 *
 * Since: REPLACEME
 **/
unsigned int
hb_ot_name_get_string (hb_face_t     *face,
		       hb_name_id_t   name_id,
		       hb_language_t *languages,
		       unsigned int   languages_count,
		       unsigned int   start_offset,
		       unsigned int  *buffer_length /* IN/OUT.  May be NULL */,
		       char          *buffer        /* OUT.     May be NULL */)
{
  const OT::name& table = _get_name (face);
#define SIZE 16
  unsigned int name_indexes[SIZE];
  unsigned int name_indexes_count = 0;
  for (unsigned int i = 0; i < table.count && name_indexes_count != SIZE; i++)
    if (table.get_record (i).nameID == name_id)
      name_indexes[name_indexes_count++] = i;
#undef SIZE

  unsigned int language_id = languages_count ? _to_ot_language_id (languages[0]) : 0;

  hb_bytes_t string = hb_bytes_t ();
  for (unsigned int i = 0; i < languages_count; i++)
  {
    if (language_id != table.get_record (i).languageID)
      continue;
    string = table.get_record_string (name_indexes[i]);
    if (buffer_length && buffer && start_offset < string.len)
    {
      unsigned int length = MIN (*buffer_length, string.len - start_offset);
      memcpy (buffer, string.arrayZ + start_offset, length);
      if (buffer_length) *buffer_length = length;
    }
    return string.len;
  }

  if (buffer_length) *buffer_length = 0;
  if (buffer) *buffer = 0;
  return string.len;
}
