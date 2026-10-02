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
 */

#include "hb.hh"
#include "hb-ot-layout-jstf-table.hh"

static hb_face_t *
create_face (const char *data, unsigned length)
{
  hb_face_t *face = hb_face_builder_create ();
  hb_blob_t *blob = hb_blob_create (data, length, HB_MEMORY_MODE_READONLY,
				    nullptr, nullptr);
  hb_always_assert (hb_face_builder_add_table (face, HB_OT_TAG_JSTF, blob));
  hb_blob_destroy (blob);
  return face;
}

int
main (int argc, char **argv)
{
  static const char data[] = {
    0, 1, 0, 1,				/* version 1.1 */
    0, 0,				/* JstfScriptRecord count */
    0, 1,				/* JstfScriptRecord2 count */
    'a', 'r', 'a', 'b', 0, 0, 0, 16,	/* JstfScriptRecord2 */
    0, 0, 0, 10,			/* ExtenderGlyph2 offset */
    0, 0, 0, 0,				/* default JstfLangSys offset */
    0, 0,				/* JstfLangSysRecord count */
    0, 1, 1, 0, 1,			/* ExtenderGlyph2 */
  };

  hb_face_t *face = create_face (data, sizeof (data));
  hb_blob_ptr_t<OT::JSTF> jstf =
      hb_sanitize_context_t ().reference_table<OT::JSTF> (face);
#ifndef HB_NO_BEYOND_64K
  hb_always_assert (jstf.get_length ());
  hb_always_assert (jstf->get_script_count () == 1);
  hb_always_assert (jstf->get_script_tag (0) == HB_TAG ('a','r','a','b'));
  unsigned index;
  hb_always_assert (jstf->find_script_index (HB_TAG ('a','r','a','b'), &index));
  hb_always_assert (index == 0);
  hb_always_assert (!jstf->get_script<OT::Layout::MediumTypes> (0).get_lang_sys_count ());
#else
  hb_always_assert (jstf.get_length ());
  hb_always_assert (!jstf->get_script_count ());
#endif
  jstf.destroy ();
  hb_face_destroy (face);

#ifndef HB_NO_BEYOND_64K
  face = create_face (data, sizeof (data) - 1);
  jstf = hb_sanitize_context_t ().reference_table<OT::JSTF> (face);
  hb_always_assert (!jstf.get_length ());
  jstf.destroy ();
  hb_face_destroy (face);
#endif

  return 0;
}
