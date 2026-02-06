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
    // Iterate through dependency data and exercise full API
    unsigned int num_glyphs = hb_face_get_glyph_count (face);
    hb_set_t *temp_set = hb_set_create ();

    for (unsigned int gid = 0; gid < num_glyphs && gid < 100; gid++)
    {
      hb_codepoint_t index = 0;
      hb_tag_t table_tag;
      hb_codepoint_t dependent;
      hb_tag_t layout_tag;
      hb_codepoint_t ligature_set;
      hb_codepoint_t context_set;
      uint8_t flags;

      // Iterate through up to 10 entries per glyph
      while (index < 10 &&
             hb_depend_get_glyph_entry (depend, gid, index++,
                                        &table_tag, &dependent,
                                        &layout_tag, &ligature_set,
                                        &context_set, &flags))
      {
        // Access basic edge data
        (void) table_tag;
        (void) dependent;
        (void) layout_tag;
        (void) flags;

        // If this edge has a ligature set, retrieve and check it
        if (ligature_set != HB_CODEPOINT_INVALID)
        {
          if (hb_depend_get_set_from_index (depend, ligature_set, temp_set))
          {
            unsigned pop = hb_set_get_population (temp_set);
            // Ligature sets should be non-empty
            if (pop == 0)
              fprintf (stderr, "Warning: Empty ligature set at index %u\n", ligature_set);
          }
        }

        // If this edge has a context set, retrieve and check it
        if (context_set != HB_CODEPOINT_INVALID)
        {
          if (hb_depend_get_set_from_index (depend, context_set, temp_set))
          {
            unsigned pop = hb_set_get_population (temp_set);
            // Context sets should be non-empty
            if (pop == 0)
              fprintf (stderr, "Warning: Empty context set at index %u\n", context_set);

            // Check for nested context sets (indirect references with 0x80000000 bit)
            hb_codepoint_t elem = HB_SET_VALUE_INVALID;
            while (hb_set_next (temp_set, &elem))
            {
              if (elem >= 0x80000000)
              {
                // Indirect reference - retrieve the nested set
                hb_codepoint_t nested_idx = elem & 0x7FFFFFFF;
                hb_set_t *nested_set = hb_set_create ();
                if (hb_depend_get_set_from_index (depend, nested_idx, nested_set))
                {
                  unsigned nested_pop = hb_set_get_population (nested_set);
                  // Nested sets should also be non-empty
                  if (nested_pop == 0)
                    fprintf (stderr, "Warning: Empty nested context set at index %u\n", nested_idx);
                }
                hb_set_destroy (nested_set);
              }
            }
          }
        }
      }
    }

    hb_set_destroy (temp_set);
    hb_depend_destroy (depend);
  }

  hb_face_destroy (face);
  hb_blob_destroy (blob);

  return 0;
}
