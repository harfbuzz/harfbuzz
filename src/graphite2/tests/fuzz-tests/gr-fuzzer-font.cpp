// SPDX-License-Identifier: MIT
// Copyright 2018, SIL International, All rights reserved.
#include <graphite2/Font.h>

#include "graphite-fuzzer.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  test_case<common_parameters> test(data, size);
  auto face = gr_make_face_with_ops(&test.ttf, &test.ttf, gr_face_preloadAll);
  // gr_make_face: directly calls gr_make_face_with_ops
  // gr_make_face_with_seg_cache_*: just call their non-segcache counterparts
  // gr_make_file_face_*: untestable in this environment. but calls the same
  //                      routines as the memory_face class.

  gr_face_info(face, test.params.script_tag);

  // Examine all the features for every supported language
  uint16_t lang_id = test.params.feat.lang_id;

  if (face)
  {
    gr_face_n_glyphs(face);
    gr_face_n_languages(face);
    const auto n_frefs = gr_face_n_fref(face);

    gr_face_lang_by_index(face, 0);
    auto features = gr_face_featureval_for_lang(face, test.params.lang_tag);

    gr_face_fref(face, n_frefs-1);
    auto fref = gr_face_find_fref(face, test.params.feat.id);
    gr_fref_id(fref);
    uint32_t label_len = 0;
    gr_label_destroy(gr_fref_label(fref, &lang_id,
                               gr_encform(test.params.encoding),
                               &label_len));
    gr_fref_feature_value(fref, features);
    gr_fref_value(fref, gr_fref_n_values(fref)-1);
    gr_label_destroy(gr_fref_value_label(fref, test.params.feat.value,
                          &lang_id, gr_encform(test.params.encoding),
                          &label_len));

    gr_fref_set_feature_value(fref, test.params.feat.value, features);
    gr_featureval_destroy(features);
  }

  gr_face_destroy(face);

  return 0;
}
