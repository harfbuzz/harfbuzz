#include "hb-fuzzer.hh"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hb-subset.h"

static void
trySubset (hb_face_t *face,
	   const hb_codepoint_t text[],
	   int text_length,
	   bool drop_hints,
	   bool drop_layout,
	   bool retain_gids)
{
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_subset_input_set_drop_hints (input, drop_hints);
  hb_subset_input_set_retain_gids (input, retain_gids);
  hb_set_t *codepoints = hb_subset_input_unicode_set (input);

  if (!drop_layout)
  {
    hb_set_del (hb_subset_input_drop_tables_set (input), HB_TAG ('G', 'S', 'U', 'B'));
    hb_set_del (hb_subset_input_drop_tables_set (input), HB_TAG ('G', 'P', 'O', 'S'));
    hb_set_del (hb_subset_input_drop_tables_set (input), HB_TAG ('G', 'D', 'E', 'F'));
  }

  for (int i = 0; i < text_length; i++)
  {
    hb_set_add (codepoints, text[i]);
  }

  hb_face_t *result = hb_subset (face, input);
  hb_face_destroy (result);

  hb_subset_input_destroy (input);
}

static void
trySubset (hb_face_t *face,
	   const hb_codepoint_t text[],
	   int text_length,
	   const uint8_t flags[1])
{
  bool drop_hints =  flags[0] & (1 << 0);
  bool drop_layout = flags[0] & (1 << 1);
  bool retain_gids = flags[0] & (1 << 2);
  trySubset (face, text, text_length,
	     drop_hints, drop_layout, retain_gids);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  hb_blob_t *blob = hb_blob_create ((const char *)data, size,
				    HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  hb_face_t *face = hb_face_create (blob, 0);

  /* Just test this API here quickly. */
  hb_set_t *output = hb_set_create();
  hb_face_collect_unicodes (face, output);
  hb_set_destroy (output);

  uint8_t flags[1] = {0};
  const hb_codepoint_t text[] =
      {
	'A', 'B', 'C', 'D', 'E', 'X', 'Y', 'Z', '1', '2',
	'3', '@', '_', '%', '&', ')', '*', '$', '!'
      };

  trySubset (face, text, sizeof (text) / sizeof (hb_codepoint_t), flags);

  hb_codepoint_t text_from_data[16];
  if (size > sizeof(text_from_data) + sizeof(flags)) {
    memcpy (text_from_data,
	    data + size - sizeof(text_from_data),
	    sizeof(text_from_data));

    memcpy (flags,
	    data + size - sizeof(text_from_data) - sizeof(flags),
	    sizeof(flags));
    unsigned int text_size = sizeof (text_from_data) / sizeof (hb_codepoint_t);

    trySubset (face, text_from_data, text_size, flags);
  }

  hb_face_destroy (face);
  hb_blob_destroy (blob);

  return 0;
}
