#include "hb-fuzzer.hh"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "hb-subset.h"

static void
trySubset (hb_face_t *face,
	   const hb_codepoint_t text[],
	   int text_length,
           unsigned flag_bits,
           hb_subset_input_t *input)
{
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
    if (!(bytes_count || !length))
      abort ();

    hb_blob_destroy (blob);
  }
  hb_face_destroy (result);

  hb_subset_input_destroy (input);
}

static bool
read_ranges (const uint8_t *&p,
             const uint8_t *end,
             hb_set_t *set,
             bool is_add)
{
  uint32_t count;
  if (!_fuzzing_read_u32_value (p, end, &count))
    return false;

  for (uint32_t i = 0; i < count; i++)
  {
    uint32_t start, finish;
    if (!_fuzzing_read_u32_value (p, end, &start) ||
        !_fuzzing_read_u32_value (p, end, &finish))
      return false;

    if (finish < start)
      return false;

    if (is_add)
      hb_set_add_range (set, start, finish);
    else
      hb_set_del_range (set, start, finish);
  }
  return true;
}

static bool
apply_extended_ops (hb_face_t *face,
                    hb_subset_input_t *input,
                    std::vector<hb_codepoint_t> *text,
                    unsigned *flags,
                    const uint8_t *ops,
                    size_t ops_len)
{
  const uint8_t *p = ops;
  const uint8_t *end = ops + ops_len;

  while (p < end)
  {
    uint8_t op;
    if (!_fuzzing_read_value (p, end, &op))
      return false;

    switch (op)
    {
      case HB_FUZZING_OP_SET_FLAGS:
      {
        uint32_t value;
        if (!_fuzzing_read_u32_value (p, end, &value))
          return false;
        *flags = value;
        break;
      }

      case HB_FUZZING_OP_KEEP_EVERYTHING:
        hb_subset_input_keep_everything (input);
        *flags = hb_subset_input_get_flags (input);
        break;

      case HB_FUZZING_OP_SET_CLEAR:
      case HB_FUZZING_OP_SET_INVERT:
      {
        uint8_t set_type_u8;
        if (!_fuzzing_read_value (p, end, &set_type_u8))
          return false;
        if (set_type_u8 > HB_SUBSET_SETS_LAYOUT_SCRIPT_TAG)
          return false;

        hb_set_t *set = hb_subset_input_set (input, (hb_subset_sets_t) set_type_u8);
        if (op == HB_FUZZING_OP_SET_CLEAR)
          hb_set_clear (set);
        else
          hb_set_invert (set);
        break;
      }

      case HB_FUZZING_OP_SET_ADD_RANGES:
      case HB_FUZZING_OP_SET_DEL_RANGES:
      {
        uint8_t set_type_u8;
        if (!_fuzzing_read_value (p, end, &set_type_u8))
          return false;
        if (set_type_u8 > HB_SUBSET_SETS_LAYOUT_SCRIPT_TAG)
          return false;

        hb_set_t *set = hb_subset_input_set (input, (hb_subset_sets_t) set_type_u8);
        if (!read_ranges (p, end, set, op == HB_FUZZING_OP_SET_ADD_RANGES))
          return false;
        break;
      }

      case HB_FUZZING_OP_TEXT_ADD:
      case HB_FUZZING_OP_TEXT_DEL:
      {
        uint32_t count;
        if (!_fuzzing_read_u32_value (p, end, &count))
          return false;

        for (uint32_t i = 0; i < count; i++)
        {
          uint32_t cp;
          if (!_fuzzing_read_u32_value (p, end, &cp))
            return false;
          if (op == HB_FUZZING_OP_TEXT_ADD)
            text->push_back (cp);
          else
            text->erase (std::remove (text->begin (), text->end (), cp), text->end ());
        }
        break;
      }

      case HB_FUZZING_OP_AXIS_PIN_ALL_TO_DEFAULT:
        if (!hb_subset_input_pin_all_axes_to_default (input, face))
          return false;
        break;

      case HB_FUZZING_OP_AXIS_SET:
      {
        uint32_t count;
        if (!_fuzzing_read_u32_value (p, end, &count))
          return false;

        for (uint32_t i = 0; i < count; i++)
        {
          uint32_t tag;
          uint8_t mode;
          float minimum, middle, maximum;
          if (!_fuzzing_read_u32_value (p, end, &tag) ||
              !_fuzzing_read_value (p, end, &mode) ||
              !_fuzzing_read_f32_value (p, end, &minimum) ||
              !_fuzzing_read_f32_value (p, end, &middle) ||
              !_fuzzing_read_f32_value (p, end, &maximum))
            return false;

          if (mode == HB_FUZZING_AXIS_PIN_TO_DEFAULT)
          {
            if (!hb_subset_input_pin_axis_to_default (input, face, tag))
              return false;
          }
          else if (mode == HB_FUZZING_AXIS_SET_RANGE)
          {
            if (!hb_subset_input_set_axis_range (input, face, tag, minimum, maximum, middle))
              return false;
          }
          else
            return false;
        }
        break;
      }

      default:
        return false;
    }
  }

  return true;
}

static bool
try_extended_input (const uint8_t *data, size_t size)
{
  if (size < sizeof (_fuzzing_extended_magic) + 4)
    return false;

  size_t magic_offset = size - sizeof (_fuzzing_extended_magic);
  if (0 != memcmp (data + magic_offset, _fuzzing_extended_magic, sizeof (_fuzzing_extended_magic)))
    return false;

  size_t ops_len_offset = magic_offset - 4;
  uint32_t ops_len = _fuzzing_read_u32_le (data + ops_len_offset);
  if (ops_len > ops_len_offset)
    return false;

  size_t font_len = ops_len_offset - ops_len;
  const uint8_t *ops = data + font_len;

  hb_blob_t *blob = hb_blob_create ((const char *) data, font_len,
				    HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  hb_face_t *face = hb_face_create (blob, 0);

  hb_set_t *output = hb_set_create ();
  hb_face_collect_unicodes (face, output);
  hb_set_destroy (output);

  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  if (!input)
  {
    hb_face_destroy (face);
    hb_blob_destroy (blob);
    return true;
  }

  std::vector<hb_codepoint_t> text;
  unsigned flags = HB_SUBSET_FLAGS_DEFAULT;
  if (apply_extended_ops (face, input, &text, &flags, ops, ops_len))
  {
    trySubset (face,
               text.empty () ? nullptr : text.data (),
               (int) text.size (),
               flags,
               input);
  }
  else
    hb_subset_input_destroy (input);

  hb_face_destroy (face);
  hb_blob_destroy (blob);

  return true;
}

static void
try_legacy_input (const uint8_t *data, size_t size)
{
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

  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  if (!input)
  {
    hb_face_destroy (face);
    hb_blob_destroy (blob);
    return;
  }
  trySubset (face, text, sizeof (text) / sizeof (hb_codepoint_t), flags, input);

  unsigned num_axes;
  hb_codepoint_t text_from_data[16];
  if (size > sizeof (text_from_data) + sizeof (flags) + sizeof(num_axes)) {
    hb_subset_input_t *legacy_input = hb_subset_input_create_or_fail ();
    if (!legacy_input)
    {
      hb_face_destroy (face);
      hb_blob_destroy (blob);
      return;
    }
    size -= sizeof (text_from_data);
    memcpy (text_from_data,
	    data + size,
	    sizeof (text_from_data));

    size -= sizeof (flags);
    memcpy (&flags,
	    data + size,
	    sizeof (flags));

    size -= sizeof (num_axes);
    memcpy (&num_axes,
	    data + size,
	    sizeof (num_axes));

    if (num_axes > 0 && num_axes < 8 && size > num_axes * (sizeof(hb_tag_t) + sizeof(int)))
    {
      for (unsigned i = 0; i < num_axes; i++) {
        hb_tag_t tag;
        int value;
        size -= sizeof (tag);
        memcpy (&tag,
                data + size,
                sizeof (tag));
        size -= sizeof (value);
        memcpy (&value,
                data + size,
                sizeof (value));

        hb_subset_input_pin_axis_location(legacy_input,
                                          face,
                                          tag,
                                          (float) value);
      }
    }

    unsigned int text_size = sizeof (text_from_data) / sizeof (hb_codepoint_t);
    trySubset (face, text_from_data, text_size, flags, legacy_input);
  }

  hb_face_destroy (face);
  hb_blob_destroy (blob);
}

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  alloc_state = _fuzzing_alloc_state (data, size);

  if (try_extended_input (data, size))
    return 0;

  try_legacy_input (data, size);
  return 0;
}
