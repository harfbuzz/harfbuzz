#include "hb-fuzzer.hh"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "hb-subset.h"

static void
trySubset (hb_face_t *face,
	   const hb_codepoint_t text[],
	   int text_length,
           unsigned flag_bits)
{
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  if (!input) return;

  hb_subset_input_set_flags (input, (hb_subset_flags_t) flag_bits);

  hb_set_t *codepoints = hb_subset_input_unicode_set (input);

  for (int i = 0; i < text_length; i++)
    hb_set_add (codepoints, text[i]);

  hb_face_t *result = hb_subset_or_fail (face, input);
  if (result)
  {
    hb_blob_t *blob = hb_face_reference_blob (result);
    unsigned int length;
    const char *data = hb_blob_get_data (blob, &length);

    // Something not optimizable just to access all the blob data
    unsigned int bytes_count = 0;
    for (unsigned int i = 0; i < length; ++i)
      if (data[i]) ++bytes_count;
    assert (bytes_count || !length);

    hb_blob_destroy (blob);
  }
  hb_face_destroy (result);

  hb_subset_input_destroy (input);
}

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  alloc_state = _fuzzing_alloc_state (data, size);

  hb_blob_t *blob = hb_blob_create ((const char *) data, size,
				    HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  hb_face_t *face = hb_face_create (blob, 0);

  /* Just test this API here quickly. */
  hb_set_t *output = hb_set_create ();
  hb_face_collect_unicodes (face, output);
  hb_set_destroy (output);

  unsigned flags = HB_SUBSET_FLAGS_DEFAULT;
  const hb_codepoint_t text[] =
      {
	'A', 'B', 'C', 'D', 'E', 'X', 'Y', 'Z', '1', '2',
	'3', '@', '_', '%', '&', ')', '*', '$', '!'
      };

  trySubset (face, text, sizeof (text) / sizeof (hb_codepoint_t), flags);

  hb_codepoint_t text_from_data[16];
  if (size > sizeof (text_from_data) + sizeof (flags)) {
    memcpy (text_from_data,
	    data + size - sizeof (text_from_data),
	    sizeof (text_from_data));

    memcpy (&flags,
	    data + size - sizeof (text_from_data) - sizeof (flags),
	    sizeof (flags));
    unsigned int text_size = sizeof (text_from_data) / sizeof (hb_codepoint_t);

    trySubset (face, text_from_data, text_size, flags);
  }

  hb_face_destroy (face);
  hb_blob_destroy (blob);

  return 0;
}
