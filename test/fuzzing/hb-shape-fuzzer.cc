#include "hb-fuzzer.hh"

#include <hb-ot.h>
#include <string.h>

#include <algorithm>
#include <cmath>
#include <stdlib.h>
#include <vector>

#define TEST_OT_FACE_NO_MAIN 1
#include "../api/test-ot-face.c"
#undef TEST_OT_FACE_NO_MAIN

static bool
apply_extended_shape_ops (hb_face_t *face,
                          std::vector<hb_codepoint_t> *text,
                          std::vector<hb_variation_t> *variations,
                          const uint8_t *ops,
                          size_t ops_len)
{
  variations->clear ();
  unsigned axis_count = hb_ot_var_get_axis_count (face);

  if (axis_count)
  {
    std::vector<hb_ot_var_axis_info_t> axes (axis_count);
    (void) hb_ot_var_get_axis_infos (face, 0, &axis_count, axes.data ());

    const uint8_t *p = ops;
    const uint8_t *end = ops + ops_len;

    while (p < end)
    {
      uint8_t op;
      if (!_fuzzing_read_value (p, end, &op))
        return false;

      switch (op)
      {
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
          variations->clear ();
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

            for (unsigned axis_index = 0; axis_index < axis_count; axis_index++)
            {
              if (axes[axis_index].tag != tag)
                continue;

              float value = axes[axis_index].default_value;
              if (mode == HB_FUZZING_AXIS_PIN_TO_DEFAULT)
                value = axes[axis_index].default_value;
              else if (mode == HB_FUZZING_AXIS_SET_RANGE)
              {
                if (!std::isnan (middle))
                  value = middle;
                else if (!std::isnan (minimum))
                  value = minimum;
                else if (!std::isnan (maximum))
                  value = maximum;
              }
              else
                return false;

              bool updated = false;
              for (auto &variation : *variations)
                if (variation.tag == tag)
                {
                  variation.value = value;
                  updated = true;
                  break;
                }
              if (!updated)
                variations->push_back ({tag, value});
              break;
            }
          }
          break;
        }

        case HB_FUZZING_OP_SET_FLAGS:
        {
          uint32_t ignored;
          if (!_fuzzing_read_u32_value (p, end, &ignored))
            return false;
          break;
        }

        case HB_FUZZING_OP_KEEP_EVERYTHING:
          break;

        case HB_FUZZING_OP_SET_CLEAR:
        case HB_FUZZING_OP_SET_INVERT:
        {
          uint8_t ignored;
          if (!_fuzzing_read_value (p, end, &ignored))
            return false;
          break;
        }

        case HB_FUZZING_OP_SET_ADD_RANGES:
        case HB_FUZZING_OP_SET_DEL_RANGES:
        {
          uint8_t ignored_set_type;
          uint32_t count;
          if (!_fuzzing_read_value (p, end, &ignored_set_type) ||
              !_fuzzing_read_u32_value (p, end, &count))
            return false;
          for (uint32_t i = 0; i < count; i++)
          {
            uint32_t ignored_start, ignored_end;
            if (!_fuzzing_read_u32_value (p, end, &ignored_start) ||
                !_fuzzing_read_u32_value (p, end, &ignored_end))
              return false;
          }
          break;
        }

        default:
          return false;
      }
    }
  }
  else
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
        case HB_FUZZING_OP_SET_FLAGS:
        {
          uint32_t ignored;
          if (!_fuzzing_read_u32_value (p, end, &ignored))
            return false;
          break;
        }
        case HB_FUZZING_OP_KEEP_EVERYTHING:
        case HB_FUZZING_OP_AXIS_PIN_ALL_TO_DEFAULT:
          break;
        case HB_FUZZING_OP_SET_CLEAR:
        case HB_FUZZING_OP_SET_INVERT:
        {
          uint8_t ignored;
          if (!_fuzzing_read_value (p, end, &ignored))
            return false;
          break;
        }
        case HB_FUZZING_OP_SET_ADD_RANGES:
        case HB_FUZZING_OP_SET_DEL_RANGES:
        {
          uint8_t ignored_set_type;
          uint32_t count;
          if (!_fuzzing_read_value (p, end, &ignored_set_type) ||
              !_fuzzing_read_u32_value (p, end, &count))
            return false;
          for (uint32_t i = 0; i < count; i++)
          {
            uint32_t ignored_start, ignored_end;
            if (!_fuzzing_read_u32_value (p, end, &ignored_start) ||
                !_fuzzing_read_u32_value (p, end, &ignored_end))
              return false;
          }
          break;
        }
        case HB_FUZZING_OP_AXIS_SET:
        {
          uint32_t count;
          if (!_fuzzing_read_u32_value (p, end, &count))
            return false;
          for (uint32_t i = 0; i < count; i++)
          {
            uint32_t ignored_tag;
            uint8_t ignored_mode;
            float ignored_minimum, ignored_middle, ignored_maximum;
            if (!_fuzzing_read_u32_value (p, end, &ignored_tag) ||
                !_fuzzing_read_value (p, end, &ignored_mode) ||
                !_fuzzing_read_f32_value (p, end, &ignored_minimum) ||
                !_fuzzing_read_f32_value (p, end, &ignored_middle) ||
                !_fuzzing_read_f32_value (p, end, &ignored_maximum))
              return false;
          }
          break;
        }
        default:
          return false;
      }
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
  hb_font_t *font = hb_font_create (face);
  hb_font_set_scale (font, 12, 12);

  std::vector<hb_codepoint_t> text;
  std::vector<hb_variation_t> variations;
  if (!apply_extended_shape_ops (face, &text, &variations, ops, ops_len))
  {
    hb_font_destroy (font);
    hb_face_destroy (face);
    hb_blob_destroy (blob);
    return true;
  }

  hb_font_set_variations (font,
                          variations.empty () ? nullptr : variations.data (),
                          variations.size ());

  {
    hb_buffer_t *buffer = hb_buffer_create ();
    hb_buffer_set_flags (buffer, (hb_buffer_flags_t) (HB_BUFFER_FLAG_VERIFY));
    hb_buffer_add_utf8 (buffer, "ABCDEXYZ123@_%&)*$!", -1, 0, -1);
    hb_buffer_guess_segment_properties (buffer);
    hb_shape (font, buffer, nullptr, 0);
    hb_buffer_destroy (buffer);
  }

  uint32_t text32[16] = {0};
  unsigned int len = text.size () > 16 ? 16 : (unsigned int) text.size ();
  for (unsigned int i = 0; i < len; i++)
    text32[i] = text[i];

  text32[10] = test_font (font, text32[15]) % 256;

  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_add_utf32 (buffer, text32, sizeof (text32) / sizeof (text32[0]), 0, -1);
  hb_buffer_guess_segment_properties (buffer);
  hb_shape (font, buffer, nullptr, 0);
  hb_buffer_destroy (buffer);

  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_blob_destroy (blob);
  return true;
}

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  _fuzzing_skip_leading_comment (&data, &size);
  alloc_state = _fuzzing_alloc_state (data, size);

  if (try_extended_input (data, size))
    return 0;

  hb_blob_t *blob = hb_blob_create ((const char *)data, size,
				    HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  hb_face_t *face = hb_face_create (blob, 0);
  hb_font_t *font = hb_font_create (face);
  hb_font_set_scale (font, 12, 12);

  unsigned num_coords = 0;
  if (size) num_coords = data[size - 1];
  num_coords = hb_ot_var_get_axis_count (face) > num_coords ? num_coords : hb_ot_var_get_axis_count (face);
  int *coords = (int *) calloc (num_coords, sizeof (int));
  if (size > num_coords + 1)
    for (unsigned i = 0; i < num_coords; ++i)
      coords[i] = ((int) data[size - num_coords + i - 1] - 128) * 10;
  hb_font_set_var_coords_normalized (font, coords, num_coords);
  free (coords);

  {
    const char text[] = "ABCDEXYZ123@_%&)*$!";
    hb_buffer_t *buffer = hb_buffer_create ();
    hb_buffer_set_flags (buffer, (hb_buffer_flags_t) (HB_BUFFER_FLAG_VERIFY /* | HB_BUFFER_FLAG_PRODUCE_UNSAFE_TO_CONCAT */));
    hb_buffer_add_utf8 (buffer, text, -1, 0, -1);
    hb_buffer_guess_segment_properties (buffer);
    hb_shape (font, buffer, nullptr, 0);
    hb_buffer_destroy (buffer);
  }

  uint32_t text32[16] = {0};
  unsigned int len = sizeof (text32);
  if (size < len)
    len = size;
  if (len)
    memcpy (text32, data + size - len, len);

  /* Misc calls on font. */
  text32[10] = test_font (font, text32[15]) % 256;

  hb_buffer_t *buffer = hb_buffer_create ();
 // hb_buffer_set_flags (buffer, (hb_buffer_flags_t) (HB_BUFFER_FLAG_VERIFY | HB_BUFFER_FLAG_PRODUCE_UNSAFE_TO_CONCAT));
  hb_buffer_add_utf32 (buffer, text32, sizeof (text32) / sizeof (text32[0]), 0, -1);
  hb_buffer_guess_segment_properties (buffer);
  hb_shape (font, buffer, nullptr, 0);
  hb_buffer_destroy (buffer);

  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_blob_destroy (blob);
  return 0;
}
