/*
 * Copyright (C) 1998-2004  David Turner and Werner Lemberg
 * Copyright (C) 2004,2007,2009,2010  Red Hat, Inc.
 *
 * This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Red Hat Author(s): Owen Taylor, Behdad Esfahbod
 */

#include "hb-buffer-private.hh"

#include <string.h>


static hb_buffer_t _hb_buffer_nil = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */

  &_hb_unicode_funcs_nil  /* unicode */
};

/* Here is how the buffer works internally:
 *
 * There are two string pointers: in_string and out_string.  They
 * always have same allocated size, but different length and positions.
 *
 * As an optimization, both in_string and out_string may point to the
 * same piece of memory, which is owned by in_string.  This remains the
 * case as long as out_length doesn't exceed in_length at any time.
 * In that case, swap() is no-op and the glyph operations operate mostly
 * in-place.
 *
 * As soon as out_string gets longer than in_string, out_string is moved over
 * to an alternate buffer (which we reuse the positions buffer for!), and its
 * current contents (out_length entries) are copied to the alt buffer.
 * This should all remain transparent to the user.  swap() then switches
 * in_string and out_string.
 */

/* XXX err handling */

/* Internal API */

static void
hb_buffer_ensure_separate (hb_buffer_t *buffer, unsigned int size)
{
  hb_buffer_ensure (buffer, size);
  if (buffer->out_string == buffer->in_string)
  {
    assert (buffer->have_output);
    if (!buffer->positions)
      buffer->positions = (hb_internal_glyph_position_t *) calloc (buffer->allocated, sizeof (buffer->positions[0]));

    buffer->out_string = (hb_internal_glyph_info_t *) buffer->positions;
    memcpy (buffer->out_string, buffer->in_string, buffer->out_length * sizeof (buffer->out_string[0]));
  }
}

/* Public API */

hb_buffer_t *
hb_buffer_create (unsigned int pre_alloc_size)
{
  hb_buffer_t *buffer;

  if (!HB_OBJECT_DO_CREATE (hb_buffer_t, buffer))
    return &_hb_buffer_nil;

  if (pre_alloc_size)
    hb_buffer_ensure(buffer, pre_alloc_size);

  buffer->unicode = &_hb_unicode_funcs_nil;

  return buffer;
}

hb_buffer_t *
hb_buffer_reference (hb_buffer_t *buffer)
{
  HB_OBJECT_DO_REFERENCE (buffer);
}

unsigned int
hb_buffer_get_reference_count (hb_buffer_t *buffer)
{
  HB_OBJECT_DO_GET_REFERENCE_COUNT (buffer);
}

void
hb_buffer_destroy (hb_buffer_t *buffer)
{
  HB_OBJECT_DO_DESTROY (buffer);

  hb_unicode_funcs_destroy (buffer->unicode);

  free (buffer->in_string);
  free (buffer->positions);

  free (buffer);
}


void
hb_buffer_set_unicode_funcs (hb_buffer_t        *buffer,
			     hb_unicode_funcs_t *unicode)
{
  if (!unicode)
    unicode = &_hb_unicode_funcs_nil;

  hb_unicode_funcs_reference (unicode);
  hb_unicode_funcs_destroy (buffer->unicode);
  buffer->unicode = unicode;
}

hb_unicode_funcs_t *
hb_buffer_get_unicode_funcs (hb_buffer_t        *buffer)
{
  return buffer->unicode;
}

void
hb_buffer_set_direction (hb_buffer_t    *buffer,
			 hb_direction_t  direction)

{
  buffer->direction = direction;
}

hb_direction_t
hb_buffer_get_direction (hb_buffer_t    *buffer)
{
  return buffer->direction;
}

void
hb_buffer_set_script (hb_buffer_t *buffer,
		      hb_script_t  script)
{
  buffer->script = script;
}

hb_script_t
hb_buffer_get_script (hb_buffer_t *buffer)
{
  return buffer->script;
}

void
hb_buffer_set_language (hb_buffer_t   *buffer,
			hb_language_t  language)
{
  buffer->language = language;
}

hb_language_t
hb_buffer_get_language (hb_buffer_t *buffer)
{
  return buffer->language;
}


void
hb_buffer_clear (hb_buffer_t *buffer)
{
  buffer->have_output = FALSE;
  buffer->have_positions = FALSE;
  buffer->in_length = 0;
  buffer->out_length = 0;
  buffer->in_pos = 0;
  buffer->out_pos = 0;
  buffer->out_string = buffer->in_string;
  buffer->max_lig_id = 0;
}

void
hb_buffer_ensure (hb_buffer_t *buffer, unsigned int size)
{
  unsigned int new_allocated = buffer->allocated;

  if (size > new_allocated)
  {
    while (size > new_allocated)
      new_allocated += (new_allocated >> 1) + 8;

    if (buffer->positions)
      buffer->positions = (hb_internal_glyph_position_t *) realloc (buffer->positions, new_allocated * sizeof (buffer->positions[0]));

    if (buffer->out_string != buffer->in_string)
    {
      buffer->in_string = (hb_internal_glyph_info_t *) realloc (buffer->in_string, new_allocated * sizeof (buffer->in_string[0]));
      buffer->out_string = (hb_internal_glyph_info_t *) buffer->positions;
    }
    else
    {
      buffer->in_string = (hb_internal_glyph_info_t *) realloc (buffer->in_string, new_allocated * sizeof (buffer->in_string[0]));
      buffer->out_string = buffer->in_string;
    }

    buffer->allocated = new_allocated;
  }
}

void
hb_buffer_add_glyph (hb_buffer_t    *buffer,
		     hb_codepoint_t  codepoint,
		     hb_mask_t       mask,
		     unsigned int    cluster)
{
  hb_internal_glyph_info_t *glyph;

  hb_buffer_ensure (buffer, buffer->in_length + 1);

  glyph = &buffer->in_string[buffer->in_length];
  glyph->codepoint = codepoint;
  glyph->mask = mask;
  glyph->cluster = cluster;
  glyph->component = 0;
  glyph->lig_id = 0;
  glyph->gproperty = HB_BUFFER_GLYPH_PROPERTIES_UNKNOWN;

  buffer->in_length++;
}


/* HarfBuzz-Internal API */

void
_hb_buffer_clear_output (hb_buffer_t *buffer)
{
  buffer->have_output = TRUE;
  buffer->have_positions = FALSE;
  buffer->out_length = 0;
  buffer->out_pos = 0;
  buffer->out_string = buffer->in_string;
}

void
hb_buffer_clear_positions (hb_buffer_t *buffer)
{
  _hb_buffer_clear_output (buffer);
  buffer->have_output = FALSE;
  buffer->have_positions = TRUE;

  if (unlikely (!buffer->positions))
  {
    buffer->positions = (hb_internal_glyph_position_t *) calloc (buffer->allocated, sizeof (buffer->positions[0]));
    return;
  }

  memset (buffer->positions, 0, sizeof (buffer->positions[0]) * buffer->in_length);
}

void
_hb_buffer_swap (hb_buffer_t *buffer)
{
  unsigned int tmp;

  assert (buffer->have_output);

  if (buffer->out_string != buffer->in_string)
  {
    hb_internal_glyph_info_t *tmp_string;
    tmp_string = buffer->in_string;
    buffer->in_string = buffer->out_string;
    buffer->out_string = tmp_string;
    buffer->positions = (hb_internal_glyph_position_t *) buffer->out_string;
  }

  tmp = buffer->in_length;
  buffer->in_length = buffer->out_length;
  buffer->out_length = tmp;

  tmp = buffer->in_pos;
  buffer->in_pos = buffer->out_pos;
  buffer->out_pos = tmp;
}

/* The following function copies `num_out' elements from `glyph_data'
   to `buffer->out_string', advancing the in array pointer in the structure
   by `num_in' elements, and the out array pointer by `num_out' elements.
   Finally, it sets the `length' field of `out' equal to
   `pos' of the `out' structure.

   If `component' is 0xFFFF, the component value from buffer->in_pos
   will copied `num_out' times, otherwise `component' itself will
   be used to fill the `component' fields.

   If `lig_id' is 0xFFFF, the lig_id value from buffer->in_pos
   will copied `num_out' times, otherwise `lig_id' itself will
   be used to fill the `lig_id' fields.

   The mask for all replacement glyphs are taken
   from the glyph at position `buffer->in_pos'.

   The cluster value for the glyph at position buffer->in_pos is used
   for all replacement glyphs */

void
_hb_buffer_add_output_glyphs (hb_buffer_t *buffer,
			      unsigned int num_in,
			      unsigned int num_out,
			      const hb_codepoint_t *glyph_data,
			      unsigned short component,
			      unsigned short lig_id)
{
  unsigned int i;
  unsigned int mask;
  unsigned int cluster;

  if (buffer->out_string != buffer->in_string ||
      buffer->out_pos + num_out > buffer->in_pos + num_in)
  {
    hb_buffer_ensure_separate (buffer, buffer->out_pos + num_out);
  }

  mask = buffer->in_string[buffer->in_pos].mask;
  cluster = buffer->in_string[buffer->in_pos].cluster;
  if (component == 0xFFFF)
    component = buffer->in_string[buffer->in_pos].component;
  if (lig_id == 0xFFFF)
    lig_id = buffer->in_string[buffer->in_pos].lig_id;

  for (i = 0; i < num_out; i++)
  {
    hb_internal_glyph_info_t *info = &buffer->out_string[buffer->out_pos + i];
    info->codepoint = glyph_data[i];
    info->mask = mask;
    info->cluster = cluster;
    info->component = component;
    info->lig_id = lig_id;
    info->gproperty = HB_BUFFER_GLYPH_PROPERTIES_UNKNOWN;
  }

  buffer->in_pos  += num_in;
  buffer->out_pos += num_out;
  buffer->out_length = buffer->out_pos;
}

void
_hb_buffer_add_output_glyphs_be16 (hb_buffer_t *buffer,
				   unsigned int num_in,
				   unsigned int num_out,
				   const uint16_t *glyph_data_be,
				   unsigned short component,
				   unsigned short lig_id)
{
  unsigned int i;
  unsigned int mask;
  unsigned int cluster;

  if (buffer->out_string != buffer->in_string ||
      buffer->out_pos + num_out > buffer->in_pos + num_in)
  {
    hb_buffer_ensure_separate (buffer, buffer->out_pos + num_out);
  }

  mask = buffer->in_string[buffer->in_pos].mask;
  cluster = buffer->in_string[buffer->in_pos].cluster;
  if (component == 0xFFFF)
    component = buffer->in_string[buffer->in_pos].component;
  if (lig_id == 0xFFFF)
    lig_id = buffer->in_string[buffer->in_pos].lig_id;

  for (i = 0; i < num_out; i++)
  {
    hb_internal_glyph_info_t *info = &buffer->out_string[buffer->out_pos + i];
    info->codepoint = hb_be_uint16 (glyph_data_be[i]);
    info->mask = mask;
    info->cluster = cluster;
    info->component = component;
    info->lig_id = lig_id;
    info->gproperty = HB_BUFFER_GLYPH_PROPERTIES_UNKNOWN;
  }

  buffer->in_pos  += num_in;
  buffer->out_pos += num_out;
  buffer->out_length = buffer->out_pos;
}

void
_hb_buffer_add_output_glyph (hb_buffer_t *buffer,
			     hb_codepoint_t glyph_index,
			     unsigned short component,
			     unsigned short lig_id)
{
  hb_internal_glyph_info_t *info;

  if (buffer->out_string != buffer->in_string)
  {
    hb_buffer_ensure (buffer, buffer->out_pos + 1);
    buffer->out_string[buffer->out_pos] = buffer->in_string[buffer->in_pos];
  }
  else if (buffer->out_pos != buffer->in_pos)
    buffer->out_string[buffer->out_pos] = buffer->in_string[buffer->in_pos];

  info = &buffer->out_string[buffer->out_pos];
  info->codepoint = glyph_index;
  if (component != 0xFFFF)
    info->component = component;
  if (lig_id != 0xFFFF)
    info->lig_id = lig_id;
  info->gproperty = HB_BUFFER_GLYPH_PROPERTIES_UNKNOWN;

  buffer->in_pos++;
  buffer->out_pos++;
  buffer->out_length = buffer->out_pos;
}

void
_hb_buffer_next_glyph (hb_buffer_t *buffer)
{
  if (buffer->have_output)
  {
    if (buffer->out_string != buffer->in_string)
    {
      hb_buffer_ensure (buffer, buffer->out_pos + 1);
      buffer->out_string[buffer->out_pos] = buffer->in_string[buffer->in_pos];
    }
    else if (buffer->out_pos != buffer->in_pos)
      buffer->out_string[buffer->out_pos] = buffer->in_string[buffer->in_pos];

    buffer->out_pos++;
    buffer->out_length = buffer->out_pos;
  }

  buffer->in_pos++;
}


unsigned int
hb_buffer_get_length (hb_buffer_t *buffer)
{
  return buffer->in_length;
}

/* Return value valid as long as buffer not modified */
hb_glyph_info_t *
hb_buffer_get_glyph_infos (hb_buffer_t *buffer)
{
  return (hb_glyph_info_t *) buffer->in_string;
}

/* Return value valid as long as buffer not modified */
hb_glyph_position_t *
hb_buffer_get_glyph_positions (hb_buffer_t *buffer)
{
  if (!buffer->have_positions)
    hb_buffer_clear_positions (buffer);

  return (hb_glyph_position_t *) buffer->positions;
}


static void
reverse_range (hb_buffer_t *buffer,
	       unsigned int start,
	       unsigned int end)
{
  unsigned int i, j;

  for (i = start, j = end - 1; i < j; i++, j--) {
    hb_internal_glyph_info_t t;

    t = buffer->in_string[i];
    buffer->in_string[i] = buffer->in_string[j];
    buffer->in_string[j] = t;
  }

  if (buffer->positions) {
    for (i = 0, j = end - 1; i < j; i++, j--) {
      hb_internal_glyph_position_t t;

      t = buffer->positions[i];
      buffer->positions[i] = buffer->positions[j];
      buffer->positions[j] = t;
    }
  }
}

void
hb_buffer_reverse (hb_buffer_t *buffer)
{
  if (unlikely (!buffer->in_length))
    return;

  reverse_range (buffer, 0, buffer->in_length);
}

void
hb_buffer_reverse_clusters (hb_buffer_t *buffer)
{
  unsigned int i, start, count, last_cluster;

  if (unlikely (!buffer->in_length))
    return;

  hb_buffer_reverse (buffer);

  count = buffer->in_length;
  start = 0;
  last_cluster = buffer->in_string[0].cluster;
  for (i = 1; i < count; i++) {
    if (last_cluster != buffer->in_string[i].cluster) {
      reverse_range (buffer, start, i);
      start = i;
      last_cluster = buffer->in_string[i].cluster;
    }
  }
  reverse_range (buffer, start, i);
}


#define ADD_UTF(T) \
	HB_STMT_START { \
	  const T *next = (const T *) text + item_offset; \
	  const T *end = next + item_length; \
	  while (next < end) { \
	    hb_codepoint_t u; \
	    const T *old_next = next; \
	    next = UTF_NEXT (next, end, u); \
	    hb_buffer_add_glyph (buffer, u, 0,  old_next - (const T *) text); \
	  } \
	} HB_STMT_END


#define UTF8_COMPUTE(Char, Mask, Len) \
  if (Char < 128) { Len = 1; Mask = 0x7f; } \
  else if ((Char & 0xe0) == 0xc0) { Len = 2; Mask = 0x1f; } \
  else if ((Char & 0xf0) == 0xe0) { Len = 3; Mask = 0x0f; } \
  else if ((Char & 0xf8) == 0xf0) { Len = 4; Mask = 0x07; } \
  else Len = 0;

static inline const uint8_t *
hb_utf8_next (const uint8_t *text,
	      const uint8_t *end,
	      hb_codepoint_t *unicode)
{
  uint8_t c = *text;
  unsigned int mask, len;

  UTF8_COMPUTE (c, mask, len);
  if (unlikely (!len || (unsigned int) (end - text) < len)) {
    *unicode = -1;
    return text + 1;
  } else {
    hb_codepoint_t result;
    unsigned int i;
    result = c & mask;
    for (i = 1; i < len; i++)
      {
	if (unlikely ((text[i] & 0xc0) != 0x80))
	  {
	    *unicode = -1;
	    return text + 1;
	  }
	result <<= 6;
	result |= (text[i] & 0x3f);
      }
    *unicode = result;
    return text + len;
  }
}

void
hb_buffer_add_utf8 (hb_buffer_t  *buffer,
		    const char   *text,
		    unsigned int  text_length HB_UNUSED,
		    unsigned int  item_offset,
		    unsigned int  item_length)
{
#define UTF_NEXT(S, E, U)	hb_utf8_next (S, E, &(U))
  ADD_UTF (uint8_t);
#undef UTF_NEXT
}

static inline const uint16_t *
hb_utf16_next (const uint16_t *text,
	       const uint16_t *end,
	       hb_codepoint_t *unicode)
{
  uint16_t c = *text++;

  if (unlikely (c >= 0xd800 && c < 0xdc00)) {
    /* high surrogate */
    uint16_t l;
    if (text < end && ((l = *text), unlikely (l >= 0xdc00 && l < 0xe000))) {
      /* low surrogate */
      *unicode = ((hb_codepoint_t) ((c) - 0xd800) * 0x400 + (l) - 0xdc00 + 0x10000);
       text++;
    } else
      *unicode = -1;
  } else
    *unicode = c;

  return text;
}

void
hb_buffer_add_utf16 (hb_buffer_t    *buffer,
		     const uint16_t *text,
		     unsigned int    text_length HB_UNUSED,
		     unsigned int    item_offset,
		     unsigned int    item_length)
{
#define UTF_NEXT(S, E, U)	hb_utf16_next (S, E, &(U))
  ADD_UTF (uint16_t);
#undef UTF_NEXT
}

void
hb_buffer_add_utf32 (hb_buffer_t    *buffer,
		     const uint32_t *text,
		     unsigned int    text_length HB_UNUSED,
		     unsigned int    item_offset,
		     unsigned int    item_length)
{
#define UTF_NEXT(S, E, U)	((U) = *(S), (S)+1)
  ADD_UTF (uint32_t);
#undef UTF_NEXT
}
