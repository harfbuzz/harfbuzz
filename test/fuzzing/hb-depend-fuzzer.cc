#include "hb-fuzzer.hh"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hb-subset.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  alloc_state = _fuzzing_alloc_state (data, size);

  hb_blob_t *blob = hb_blob_create ((const char *) data, size,
                                    HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  hb_face_t *face = hb_face_create (blob, 0);

  hb_depend_t *depend = hb_depend_from_face_or_fail (face);

  if (depend)
  {
    // Iterate through some dependency data to ensure it's accessible
    unsigned int num_glyphs = hb_face_get_glyph_count (face);
    for (unsigned int gid = 0; gid < num_glyphs && gid < 100; gid++)
    {
      hb_codepoint_t index = 0;
      hb_tag_t table_tag;
      hb_codepoint_t dependent;
      hb_tag_t layout_tag;
      hb_codepoint_t ligature_set;
      hb_codepoint_t context_set;

      // Iterate through up to 10 entries per glyph
      while (index < 10 &&
             hb_depend_get_glyph_entry (depend, gid, index++,
                                        &table_tag, &dependent,
                                        &layout_tag, &ligature_set,
                                        &context_set, NULL))
      {
        // Just access the data; success = no crash
        (void) table_tag;
        (void) dependent;
        (void) layout_tag;
        (void) ligature_set;
        (void) context_set;
      }
    }

    hb_depend_destroy (depend);
  }

  hb_face_destroy (face);
  hb_blob_destroy (blob);

  return 0;
}
