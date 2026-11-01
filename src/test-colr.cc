/*
 * Copyright © 2022  Behdad Esfahbod
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
#include "OT/Color/COLR/COLR.hh"

static void
test_colr_valid_control ()
{
  hb_set_t glyphs, layers, palettes, vars;
  OT::hb_colrv1_closure_context_t ctx (nullptr, &glyphs, &layers, &palettes, &vars);
  ctx.add_layer_indices (2, 3);
  hb_always_assert (layers.get_population () == 3 && layers.has (2) && layers.has (4));
  ctx.add_var_idxes (5, 2);
  hb_always_assert (vars.get_population () == 2 && vars.has (5) && vars.has (6));

  const uint8_t clip_bytes[13] = {
    2, 0, 10, 0, 20, 0, 100, 0, 120, 0, 0, 0, 10
  };
  vars.clear ();
  reinterpret_cast<const OT::ClipBoxFormat2 *> (clip_bytes)->closurev1 (&ctx);
  hb_always_assert (vars.get_population () == 4 && vars.has (10) && vars.has (13));
}

static void
test_colr_add_layer_indices_overflow ()
{
  hb_set_t glyphs, layers, palettes, vars;
  OT::hb_colrv1_closure_context_t ctx (nullptr, &glyphs, &layers, &palettes, &vars);

  ctx.add_layer_indices (0, 0);
  hb_always_assert (layers.is_empty ());
  ctx.add_layer_indices (hb_set_t::INVALID, 2);
  hb_always_assert (layers.is_empty ());

  /* 0xFFFFFFFE + 2 - 1 == INVALID must clamp last to INVALID - 1. */
  ctx.add_layer_indices (0xFFFFFFFEu, 2);
  hb_always_assert (layers.get_population () == 1 && layers.has (0xFFFFFFFEu));

  /* 0xFFFFFFFD + 5 - 1 wraps 32-bit unsigned; must clamp to [0xFFFFFFFD..0xFFFFFFFE]. */
  layers.clear ();
  ctx.add_layer_indices (0xFFFFFFFDu, 5);
  hb_always_assert (layers.get_population () == 2 &&
                    layers.has (0xFFFFFFFDu) &&
                    layers.has (0xFFFFFFFEu));
}

static void
test_colr_add_var_idxes_overflow ()
{
  hb_set_t glyphs, layers, palettes, vars;
  OT::hb_colrv1_closure_context_t ctx (nullptr, &glyphs, &layers, &palettes, &vars);

  ctx.add_var_idxes (0, 0);
  hb_always_assert (vars.is_empty ());
  ctx.add_var_idxes (OT::VarIdx::NO_VARIATION, 4);
  hb_always_assert (vars.is_empty ());

  /* 0xFFFFFFFE + 2 - 1 == NO_VARIATION must clamp last to NO_VARIATION - 1. */
  ctx.add_var_idxes (0xFFFFFFFEu, 2);
  hb_always_assert (vars.get_population () == 1 && vars.has (0xFFFFFFFEu));

  /* 0xFFFFFFFC + 6 - 1 wraps 32-bit unsigned; must clamp to [0xFFFFFFFC..0xFFFFFFFE]. */
  vars.clear ();
  ctx.add_var_idxes (0xFFFFFFFCu, 6);
  hb_always_assert (vars.get_population () == 3 &&
                    vars.has (0xFFFFFFFCu) &&
                    vars.has (0xFFFFFFFDu) &&
                    vars.has (0xFFFFFFFEu));
}

static void
test_colr_clipbox_closure_overflow ()
{
  hb_set_t glyphs, layers, palettes, vars;
  OT::hb_colrv1_closure_context_t ctx (nullptr, &glyphs, &layers, &palettes, &vars);

  uint8_t clip_bytes[13] = {
    2, 0, 10, 0, 20, 0, 110, 0, 120, 0xFF, 0xFF, 0xFF, 0xFF
  };
  const auto *clip = reinterpret_cast<const OT::ClipBoxFormat2 *> (clip_bytes);

  /* varIdxBase == NO_VARIATION: no variation indices added. */
  clip->closurev1 (&ctx);
  hb_always_assert (vars.is_empty ());

  /* varIdxBase == 0xFFFFFFFC: +3 hits NO_VARIATION; must retain [0xFFFFFFFC..0xFFFFFFFE]. */
  clip_bytes[12] = 0xFC;
  clip->closurev1 (&ctx);
  hb_always_assert (vars.get_population () == 3 &&
                    vars.has (0xFFFFFFFCu) &&
                    vars.has (0xFFFFFFFDu) &&
                    vars.has (0xFFFFFFFEu));

  /* varIdxBase == 0xFFFFFFFE: +3 wraps 32-bit to 1; must retain {0xFFFFFFFE}. */
  vars.clear ();
  clip_bytes[12] = 0xFE;
  clip->closurev1 (&ctx);
  hb_always_assert (vars.get_population () == 1 && vars.has (0xFFFFFFFEu));
}

int
main (int argc, char **argv)
{
  test_colr_valid_control ();
  test_colr_add_layer_indices_overflow ();
  test_colr_add_var_idxes_overflow ();
  test_colr_clipbox_closure_overflow ();
  return 0;
}
