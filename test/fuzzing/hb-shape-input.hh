#include "hb-fuzzer.hh"

#include <hb-ot.h>
#include <string.h>

#include <algorithm>
#include <cmath>
#include <stdlib.h>
#include <vector>

enum _fuzzing_shape_input_status_t
{
  HB_FUZZING_SHAPE_INPUT_RAW = 0,
  HB_FUZZING_SHAPE_INPUT_EXTENDED = 1,
  HB_FUZZING_SHAPE_INPUT_MALFORMED = 2,
};

struct _fuzzing_shape_input_t
{
  hb_blob_t *blob = nullptr;
  hb_face_t *face = nullptr;
  hb_font_t *font = nullptr;
  std::vector<hb_codepoint_t> text;
  std::vector<hb_variation_t> variations;

  ~_fuzzing_shape_input_t ()
  {
    hb_font_destroy (font);
    hb_face_destroy (face);
    hb_blob_destroy (blob);
  }
};

static bool
_fuzzing_apply_extended_shape_ops (hb_face_t *face,
                                   std::vector<hb_codepoint_t> *text,
                                   std::vector<hb_variation_t> *variations,
                                   const uint8_t *ops,
                                   size_t ops_len)
{
  variations->clear ();
  unsigned axis_count = hb_ot_var_get_axis_count (face);
  std::vector<hb_ot_var_axis_info_t> axes;
  if (axis_count)
  {
    axes.resize (axis_count);
    (void) hb_ot_var_get_axis_infos (face, 0, &axis_count, axes.data ());
  }

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

          if (!axis_count)
            continue;

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

  return true;
}

static bool
_fuzzing_extract_extended_ops (const uint8_t *data,
                               size_t size,
                               size_t *font_len,
                               const uint8_t **ops,
                               size_t *ops_len)
{
  if (size < sizeof (_fuzzing_extended_magic) + 4)
    return false;

  size_t magic_offset = size - sizeof (_fuzzing_extended_magic);
  if (0 != memcmp (data + magic_offset, _fuzzing_extended_magic, sizeof (_fuzzing_extended_magic)))
    return false;

  size_t ops_len_offset = magic_offset - 4;
  uint32_t parsed_ops_len = _fuzzing_read_u32_le (data + ops_len_offset);
  if (parsed_ops_len > ops_len_offset)
    return false;

  *font_len = ops_len_offset - parsed_ops_len;
  *ops = data + *font_len;
  *ops_len = parsed_ops_len;
  return true;
}

static _fuzzing_shape_input_status_t
_fuzzing_prepare_shape_input (const uint8_t *data,
                              size_t size,
                              int x_scale,
                              int y_scale,
                              _fuzzing_shape_input_t *input)
{
  size_t font_len = 0;
  const uint8_t *ops = nullptr;
  size_t ops_len = 0;
  bool is_extended = _fuzzing_extract_extended_ops (data, size, &font_len, &ops, &ops_len);

  input->blob = hb_blob_create ((const char *) data,
                                is_extended ? font_len : size,
                                HB_MEMORY_MODE_READONLY,
                                nullptr, nullptr);
  input->face = hb_face_create (input->blob, 0);
  input->font = hb_font_create (input->face);
  hb_font_set_scale (input->font, x_scale, y_scale);

  if (is_extended)
  {
    if (!_fuzzing_apply_extended_shape_ops (input->face, &input->text, &input->variations, ops, ops_len))
      return HB_FUZZING_SHAPE_INPUT_MALFORMED;

    hb_font_set_variations (input->font,
                            input->variations.empty () ? nullptr : input->variations.data (),
                            input->variations.size ());
    return HB_FUZZING_SHAPE_INPUT_EXTENDED;
  }

  unsigned num_coords = 0;
  if (size)
    num_coords = data[size - 1];
  num_coords = hb_ot_var_get_axis_count (input->face) > num_coords ? num_coords : hb_ot_var_get_axis_count (input->face);
  int *coords = (int *) calloc (num_coords, sizeof (int));
  if (size > num_coords + 1)
    for (unsigned i = 0; i < num_coords; ++i)
      coords[i] = ((int) data[size - num_coords + i - 1] - 128) * 10;
  hb_font_set_var_coords_normalized (input->font, coords, num_coords);
  free (coords);

  uint32_t text32[16] = {0};
  unsigned int len = sizeof (text32);
  if (size < len)
    len = size;
  if (len)
    memcpy (text32, data + size - len, len);

  input->text.assign (text32, text32 + sizeof (text32) / sizeof (text32[0]));
  return HB_FUZZING_SHAPE_INPUT_RAW;
}

