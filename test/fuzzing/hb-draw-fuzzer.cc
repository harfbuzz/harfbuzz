#include <assert.h>
#include <stdlib.h>

#include <hb-ot.h>

#include "hb-fuzzer.hh"

#ifdef HB_EXPERIMENTAL_API
struct _user_data_t
{
  bool is_open;
  unsigned path_len;
  hb_position_t path_start_x;
  hb_position_t path_start_y;
  hb_position_t path_last_x;
  hb_position_t path_last_y;
};

static void
_move_to (hb_position_t to_x, hb_position_t to_y, void *user_data_)
{
  _user_data_t *user_data = (_user_data_t *) user_data_;
  assert (!user_data->is_open);
  user_data->is_open = true;
  user_data->path_start_x = user_data->path_last_x = to_x;
  user_data->path_start_y = user_data->path_last_y = to_y;
}

static void
_line_to (hb_position_t to_x, hb_position_t to_y, void *user_data_)
{
  _user_data_t *user_data = (_user_data_t *) user_data_;
  assert (user_data->is_open);
  assert (user_data->path_last_x != to_x || user_data->path_last_y != to_y);
  ++user_data->path_len;
  user_data->path_last_x = to_x;
  user_data->path_last_y = to_y;
}

static void
_quadratic_to (hb_position_t control_x, hb_position_t control_y,
	       hb_position_t to_x, hb_position_t to_y, void *user_data_)
{
  _user_data_t *user_data = (_user_data_t *) user_data_;
  assert (user_data->is_open);
  assert (user_data->path_last_x != control_x || user_data->path_last_y != control_y ||
	  user_data->path_last_x != to_x || user_data->path_last_y != to_y);
  ++user_data->path_len;
  user_data->path_last_x = to_x;
  user_data->path_last_y = to_y;
}

static void
_cubic_to (hb_position_t control1_x, hb_position_t control1_y,
	   hb_position_t control2_x, hb_position_t control2_y,
	   hb_position_t to_x, hb_position_t to_y, void *user_data_)
{
  _user_data_t *user_data = (_user_data_t *) user_data_;
  assert (user_data->is_open);
  assert (user_data->path_last_x != control1_x || user_data->path_last_y != control1_y ||
	  user_data->path_last_x != control2_x || user_data->path_last_y != control2_y ||
	  user_data->path_last_x != to_x || user_data->path_last_y != to_y);
  ++user_data->path_len;
  user_data->path_last_x = to_x;
  user_data->path_last_y = to_y;
}

static void
_close_path (void *user_data_)
{
  _user_data_t *user_data = (_user_data_t *) user_data_;
  assert (user_data->is_open && user_data->path_len != 0);
  user_data->path_len = 0;
  user_data->is_open = false;
  assert (user_data->path_start_x == user_data->path_last_x &&
	  user_data->path_start_y == user_data->path_last_y);
}
#endif

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  hb_blob_t *blob = hb_blob_create ((const char *) data, size,
				    HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  hb_face_t *face = hb_face_create (blob, 0);
  hb_font_t *font = hb_font_create (face);

  unsigned num_coords = 0;
  if (size) num_coords = data[size - 1];
  num_coords = hb_ot_var_get_axis_count (face) > num_coords ? num_coords : hb_ot_var_get_axis_count (face);
  int *coords = (int *) calloc (num_coords, sizeof (int));
  if (size > num_coords + 1)
    for (unsigned i = 0; i < num_coords; ++i)
      coords[i] = ((int) data[size - num_coords + i - 1] - 128) * 10;
  hb_font_set_var_coords_normalized (font, coords, num_coords);
  free (coords);

  unsigned glyph_count = hb_face_get_glyph_count (face);
  glyph_count = glyph_count > 16 ? 16 : glyph_count;

#ifdef HB_EXPERIMENTAL_API
  _user_data_t user_data = {false, 0, 0, 0, 0, 0};

  hb_draw_funcs_t *funcs = hb_draw_funcs_create ();
  hb_draw_funcs_set_move_to_func (funcs, (hb_draw_move_to_func_t) _move_to);
  hb_draw_funcs_set_line_to_func (funcs, (hb_draw_line_to_func_t) _line_to);
  hb_draw_funcs_set_quadratic_to_func (funcs, (hb_draw_quadratic_to_func_t) _quadratic_to);
  hb_draw_funcs_set_cubic_to_func (funcs, (hb_draw_cubic_to_func_t) _cubic_to);
  hb_draw_funcs_set_close_path_func (funcs, (hb_draw_close_path_func_t) _close_path);
#endif
  volatile unsigned counter = !glyph_count;
  for (unsigned gid = 0; gid < glyph_count; ++gid)
  {
#ifdef HB_EXPERIMENTAL_API
    hb_font_draw_glyph (font, gid, funcs, &user_data);
    assert (!user_data.is_open);
#endif

    /* Glyph extents also may practices the similar path, call it now that is related */
    hb_glyph_extents_t extents;
    if (hb_font_get_glyph_extents (font, gid, &extents))
      counter += !!extents.width + !!extents.height + !!extents.x_bearing + !!extents.y_bearing;

    if (!counter) counter += 1;
  }
  assert (counter);
#ifdef HB_EXPERIMENTAL_API
  hb_draw_funcs_destroy (funcs);
#endif

  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_blob_destroy (blob);
  return 0;
}
