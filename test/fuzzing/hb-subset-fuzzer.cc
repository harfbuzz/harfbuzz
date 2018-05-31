#include "hb-fuzzer.hh"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hb-subset.h"


void trySubset (hb_face_t *face,
                const hb_codepoint_t text[],
                int text_length,
                bool drop_hints)
{
  hb_subset_profile_t *profile = hb_subset_profile_create ();

  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  *hb_subset_input_drop_hints(input) = drop_hints;
  hb_set_t *codepoints = hb_subset_input_unicode_set (input);

  for (int i = 0; i < text_length; i++)
  {
    hb_set_add (codepoints, text[i]);
  }

  hb_face_t *result = hb_subset (face, profile, input);
  hb_face_destroy (result);

  hb_subset_input_destroy (input);
  hb_subset_profile_destroy (profile);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  hb_blob_t *blob = hb_blob_create ((const char *)data, size,
                                    HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_face_t *face = hb_face_create (blob, 0);

  const hb_codepoint_t text[] =
      {
        'A', 'B', 'C', 'D', 'E', 'X', 'Y', 'Z', '1', '2',
        '3', '@', '_', '%', '&', ')', '*', '$', '!'
      };

  trySubset (face, text, sizeof (text) / sizeof (hb_codepoint_t), true);
  trySubset (face, text, sizeof (text) / sizeof (hb_codepoint_t), false);

  hb_codepoint_t text_from_data[16];
  if (size > sizeof(text_from_data)) {
    memcpy(text_from_data,
           data + size - sizeof(text_from_data),
           sizeof(text_from_data));
    unsigned int text_size = sizeof (text_from_data) / sizeof (hb_codepoint_t);
    trySubset (face, text_from_data, text_size, true);
    trySubset (face, text_from_data, text_size, false);
  }

  hb_face_destroy (face);
  hb_blob_destroy (blob);

  return 0;
}
