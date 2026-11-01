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
#include "hb-machinery.hh"
#include "hb-open-type.hh"
#include "hb-ot-layout-common.hh"
#include "OT/Color/COLR/COLR.hh"
#include <string.h>
#include <type_traits>

#define HB_MACHINERY_CHECK(cond) \
  do { \
    if (!(cond)) { \
      fprintf (stderr, "[FAILED] %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      exit (1); \
    } \
  } while (0)

template <typename T, typename = void>
struct has_tuple_count_values : std::false_type {};

template <typename T>
struct has_tuple_count_values<
  T,
  hb_void_t<decltype (T::count_values ((const unsigned char *) nullptr,
                                       (const unsigned char *) nullptr,
                                       (unsigned *) nullptr))>> : std::true_type {};

template <typename TupleT = OT::TupleValues>
static void
verify_tuple_count_values_impl (typename std::enable_if<has_tuple_count_values<TupleT>::value, int>::type = 0)
{
  unsigned count = 999;
  const unsigned char empty = 0;
  HB_MACHINERY_CHECK (TupleT::count_values (&empty, &empty, &count));
  HB_MACHINERY_CHECK (count == 0);

  /* 64 zeros (0xBF) + 2 bytes (0x01: 10, 20) + 1 word (0x40: 256) + 1 long (0xC0: 1) = 68 */
  const unsigned char valid_mix[] = {
    0xBF,
    0x01, 10, 20,
    0x40, 0x01, 0x00,
    0xC0, 0x00, 0x00, 0x00, 0x01
  };
  HB_MACHINERY_CHECK (TupleT::count_values (valid_mix, valid_mix + sizeof (valid_mix), &count));
  HB_MACHINERY_CHECK (count == 68);

  /* Truncated word run: claims 2 words (4B), only 2B left. */
  const unsigned char trunc_words[] = {0x41, 0x00, 0x05};
  HB_MACHINERY_CHECK (!TupleT::count_values (trunc_words, trunc_words + sizeof (trunc_words), &count));

  /* Truncated long run: claims 1 long (4B), only 3B left. */
  const unsigned char trunc_longs[] = {0xC0, 0x00, 0x00, 0x01};
  HB_MACHINERY_CHECK (!TupleT::count_values (trunc_longs, trunc_longs + sizeof (trunc_longs), &count));

  /* Total count > INT_MAX: 33,554,433 * 64 = 2,147,483,712 > INT_MAX. */
  hb_vector_t<unsigned char> huge_zeros;
  HB_MACHINERY_CHECK (huge_zeros.resize_dirty (33554433u));
  memset (huge_zeros.arrayZ, 0xBF, huge_zeros.length);
  HB_MACHINERY_CHECK (!TupleT::count_values (huge_zeros.arrayZ,
                                           huge_zeros.arrayZ + huge_zeros.length,
                                           &count));
}

template <typename TupleT = OT::TupleValues>
static void
verify_tuple_count_values_impl (typename std::enable_if<!has_tuple_count_values<TupleT>::value, int>::type = 0)
{
  HB_MACHINERY_CHECK (false);
}

static void
test_machinery_valid_control ()
{
  const unsigned char valid_bytes[] = {0x81, 0x01, 42, 0xF9};
  OT::TupleValues::iter_t it (valid_bytes, sizeof (valid_bytes));
  HB_MACHINERY_CHECK (hb_len (it) == 4);

  OT::TupleValues::fetcher_t fetcher (valid_bytes + 1, 3);
  float acc = 0.f;
  fetcher.add_to (hb_array (&acc, 1), 1.f);
  HB_MACHINERY_CHECK (acc == 42.f);
  fetcher.skip (1);

  hb_vector_t<int> out;
  const OT::HBUINT8 *p = reinterpret_cast<const OT::HBUINT8 *> (valid_bytes);
  HB_MACHINERY_CHECK (OT::TupleValues::decompile (p, out, p + sizeof (valid_bytes), true));
  if (out.length != 4 || out.length == 0) { fprintf (stderr, "[FAILED] out.length != 4\n"); exit (1); } HB_MACHINERY_CHECK (out.length != 0);
  HB_MACHINERY_CHECK (out[0] == 0 && out[1] == 0 && out[2] == 42 && out[3] == -7);

  hb_set_t glyphs, layers, palettes, vars;
  OT::hb_colrv1_closure_context_t ctx (nullptr, &glyphs, &layers, &palettes, &vars);
  ctx.add_layer_indices (2, 3);
  HB_MACHINERY_CHECK (layers.get_population () == 3 && layers.has (2) && layers.has (4));
  ctx.add_var_idxes (5, 2);
  HB_MACHINERY_CHECK (vars.get_population () == 2 && vars.has (5) && vars.has (6));

  const uint8_t clip_bytes[13] = {
    2, 0, 10, 0, 20, 0, 100, 0, 120, 0, 0, 0, 10
  };
  vars.clear ();
  reinterpret_cast<const OT::ClipBoxFormat2 *> (clip_bytes)->closurev1 (&ctx);
  HB_MACHINERY_CHECK (vars.get_population () == 4 && vars.has (10) && vars.has (13));
}

static void
test_tuple_values_count_values ()
{
  verify_tuple_count_values_impl<> ();
}

static void
test_tuple_values_iter_trunc ()
{
  /* 0x41 claims 2 word values (4B), with only 2B {0x00, 0x05} left.
   * Must stop cleanly instead of yielding 0 and re-parsing 0x00 as a 1-byte run (+5). */
  const unsigned char trunc_bytes[] = {0x41, 0x00, 0x05};
  OT::TupleValues::iter_t it (trunc_bytes, sizeof (trunc_bytes));
  HB_MACHINERY_CHECK (hb_len (it) == 0);
}

static void
test_tuple_values_fetcher_trunc ()
{
  /* Truncated word run must advance fetcher_t::p to end so subsequent
   * add_to() or skip() + add_to() calls do not re-parse {0x00, 0x05} as +5. */
  const unsigned char trunc_bytes[] = {0x41, 0x00, 0x05};
  {
    OT::TupleValues::fetcher_t f (trunc_bytes, sizeof (trunc_bytes));
    float acc = 0.f;
    f.add_to (hb_array (&acc, 1), 1.f);
    f.add_to (hb_array (&acc, 1), 1.f);
    HB_MACHINERY_CHECK (acc == 0.f);
  }
  {
    OT::TupleValues::fetcher_t f (trunc_bytes, sizeof (trunc_bytes));
    float acc = 0.f;
    f.skip (1);
    f.add_to (hb_array (&acc, 1), 1.f);
    HB_MACHINERY_CHECK (acc == 0.f);
  }
}

static void
test_tuple_values_decompile_trunc ()
{
  /* 1024 * 64 = 65,536 compressed zeros followed by a truncated word header (0x41):
   * decompile(consume_all=true) must pre-validate via count_values and reject
   * without expanding 65,536 elements into out. */
  hb_vector_t<unsigned char> buf;
  HB_MACHINERY_CHECK (buf.resize_dirty (1025));
  memset (buf.arrayZ, 0xBF, 1024);
  buf.arrayZ[1024] = 0x41;

  hb_vector_t<int> out;
  const OT::HBUINT8 *p = reinterpret_cast<const OT::HBUINT8 *> (buf.arrayZ);
  HB_MACHINERY_CHECK (!OT::TupleValues::decompile (p, out, p + buf.length, true));
  HB_MACHINERY_CHECK (out.length == 0);
}

static void
test_item_varstore_instancer_varidx_overflow ()
{
  /* Minimal sanitized ItemVariationStore: 1 axis, 1 region [0, 1, 1],
   * 1 VarData item [0][0] with 8-bit delta = +42. */
  alignas (4) static const uint8_t store_bytes[] = {
    0, 1,             /* format = 1 */
    0, 0, 0, 12,      /* regionListOffset = 12 */
    0, 1,             /* dataCount = 1 */
    0, 0, 0, 22,      /* dataSets[0] = 22 */
    /* VarRegionList @ 12 (10 bytes): axisCount=1, regionCount=1, [0, 1, 1] */
    0, 1, 0, 1,
    0x00, 0x00, 0x40, 0x00, 0x40, 0x00,
    /* VarData @ 22 (9 bytes): itemCount=1, wordDeltaCount=0, regionIdxCount=1, reg[0]=0, delta=42 */
    0, 1, 0, 0, 0, 1, 0, 0, 42
  };
  hb_blob_t *blob = hb_blob_create ((const char *) store_bytes, sizeof (store_bytes),
                                    HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  hb_sanitize_context_t sc (blob);
  const auto &store = *reinterpret_cast<const OT::ItemVariationStore *> (store_bytes);
  HB_MACHINERY_CHECK (store.sanitize (&sc));
  hb_blob_destroy (blob);

  int coords[] = {16384};
  OT::ItemVarStoreInstancer instancer (&store, nullptr, hb_array (coords));
#ifdef HB_NO_VAR
  HB_MACHINERY_CHECK (instancer (0, 0) == 0.f);
#else
  HB_MACHINERY_CHECK (instancer (0, 0) == 42.f);
#endif
  HB_MACHINERY_CHECK (instancer (OT::VarIdx::NO_VARIATION, 0) == 0.f);
  HB_MACHINERY_CHECK (instancer (OT::VarIdx::NO_VARIATION, 1) == 0.f);
  /* 0xFFFFFFFE + 1 == NO_VARIATION and 0xFFFFFFFE + 2 wraps 32-bit to 0. */
  HB_MACHINERY_CHECK (instancer (0xFFFFFFFEu, 1) == 0.f);
  HB_MACHINERY_CHECK (instancer (0xFFFFFFFEu, 2) == 0.f);
}

static void
test_colr_add_layer_indices_overflow ()
{
  hb_set_t glyphs, layers, palettes, vars;
  OT::hb_colrv1_closure_context_t ctx (nullptr, &glyphs, &layers, &palettes, &vars);

  ctx.add_layer_indices (0, 0);
  HB_MACHINERY_CHECK (layers.is_empty ());
  ctx.add_layer_indices (hb_set_t::INVALID, 2);
  HB_MACHINERY_CHECK (layers.is_empty ());

  /* 0xFFFFFFFE + 2 - 1 == INVALID must clamp last to INVALID - 1. */
  ctx.add_layer_indices (0xFFFFFFFEu, 2);
  HB_MACHINERY_CHECK (layers.get_population () == 1 && layers.has (0xFFFFFFFEu));

  /* 0xFFFFFFFD + 5 - 1 wraps 32-bit unsigned; must clamp to [0xFFFFFFFD..0xFFFFFFFE]. */
  layers.clear ();
  ctx.add_layer_indices (0xFFFFFFFDu, 5);
  HB_MACHINERY_CHECK (layers.get_population () == 2 &&
                    layers.has (0xFFFFFFFDu) &&
                    layers.has (0xFFFFFFFEu));
}

static void
test_colr_add_var_idxes_overflow ()
{
  hb_set_t glyphs, layers, palettes, vars;
  OT::hb_colrv1_closure_context_t ctx (nullptr, &glyphs, &layers, &palettes, &vars);

  ctx.add_var_idxes (0, 0);
  HB_MACHINERY_CHECK (vars.is_empty ());
  ctx.add_var_idxes (OT::VarIdx::NO_VARIATION, 4);
  HB_MACHINERY_CHECK (vars.is_empty ());

  /* 0xFFFFFFFE + 2 - 1 == NO_VARIATION must clamp last to NO_VARIATION - 1. */
  ctx.add_var_idxes (0xFFFFFFFEu, 2);
  HB_MACHINERY_CHECK (vars.get_population () == 1 && vars.has (0xFFFFFFFEu));

  /* 0xFFFFFFFC + 6 - 1 wraps 32-bit unsigned; must clamp to [0xFFFFFFFC..0xFFFFFFFE]. */
  vars.clear ();
  ctx.add_var_idxes (0xFFFFFFFCu, 6);
  HB_MACHINERY_CHECK (vars.get_population () == 3 &&
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
  HB_MACHINERY_CHECK (vars.is_empty ());

  /* varIdxBase == 0xFFFFFFFC: +3 hits NO_VARIATION; must retain [0xFFFFFFFC..0xFFFFFFFE]. */
  clip_bytes[12] = 0xFC;
  clip->closurev1 (&ctx);
  HB_MACHINERY_CHECK (vars.get_population () == 3 &&
                    vars.has (0xFFFFFFFCu) &&
                    vars.has (0xFFFFFFFDu) &&
                    vars.has (0xFFFFFFFEu));

  /* varIdxBase == 0xFFFFFFFE: +3 wraps 32-bit to 1; must retain {0xFFFFFFFE}. */
  vars.clear ();
  clip_bytes[12] = 0xFE;
  clip->closurev1 (&ctx);
  HB_MACHINERY_CHECK (vars.get_population () == 1 && vars.has (0xFFFFFFFEu));
}

struct hb_intp_lazy_loader_t : hb_lazy_loader_t<int, hb_intp_lazy_loader_t>
{
  static int* create () { return nullptr; }
  static void destroy (int* l) {}
  static int* get_null () { return nullptr; }
};

struct hb_void_lazy_loader_t : hb_lazy_loader_t<void, hb_void_lazy_loader_t>
{
  static void* create () { return nullptr; }
  static void destroy (void* l) {}
  static void* get_null () { return nullptr; }
};

int
main (int argc, char **argv)
{
  const char *mode = argc > 1 ? argv[1] : nullptr;
  bool ran_any = (mode == nullptr);
  if (!mode || !strcmp (mode, "valid_control"))
  {
    ran_any = true;
  test_machinery_valid_control ();
  }
  if (!mode || !strcmp (mode, "tuple_count_values"))
  {
    ran_any = true;
  test_tuple_values_count_values ();
  }
  if (!mode || !strcmp (mode, "tuple_iter_trunc"))
  {
    ran_any = true;
  test_tuple_values_iter_trunc ();
  }
  if (!mode || !strcmp (mode, "tuple_fetcher_trunc"))
  {
    ran_any = true;
  test_tuple_values_fetcher_trunc ();
  }
  if (!mode || !strcmp (mode, "tuple_decompile_trunc"))
  {
    ran_any = true;
  test_tuple_values_decompile_trunc ();
  }
  if (!mode || !strcmp (mode, "varidx_instancer"))
  {
    ran_any = true;
  test_item_varstore_instancer_varidx_overflow ();
  }
  if (!mode || !strcmp (mode, "colr_add_layer_indices"))
  {
    ran_any = true;
  test_colr_add_layer_indices_overflow ();
  }
  if (!mode || !strcmp (mode, "colr_add_var_idxes"))
  {
    ran_any = true;
  test_colr_add_var_idxes_overflow ();
  }
  if (!mode || !strcmp (mode, "colr_clipbox_closure"))
  {
    ran_any = true;
  test_colr_clipbox_closure_overflow ();
  }
  if (!ran_any) { fprintf (stderr, "[FAILED] unknown mode %s\n", mode); return 1; }
  return 0;
}
