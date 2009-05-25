/*
 * Copyright (C) 1998-2004  David Turner and Werner Lemberg
 * Copyright (C) 2004,2007  Red Hat, Inc.
 *
 * This is part of HarfBuzz, an OpenType Layout engine library.
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

#include "hb-buffer-private.h"

#include <string.h>

/* Here is how the buffer works internally:
 *
 * There are two string pointers: in_string and out_string.  They
 * always have same allocated size, but different length and positions.
 *
 * As an optimization, both in_string and out_string may point to the
 * same piece of memory, which is owned by in_string.  This remains the
 * case as long as:
 *
 *   - copy_glyph() is called
 *   - replace_glyph() is called with inplace=TRUE
 *   - add_output_glyph() and add_output_glyphs() are not called
 *
 * In that case swap(), and copy_glyph(), and replace_glyph() are all
 * mostly no-op.
 *
 * As soon an add_output_glyph[s]() or replace_glyph() with inplace=FALSE is
 * called, out_string is moved over to an alternate buffer (alt_string), and
 * its current contents (out_length entries) are copied to the alt buffer.
 * This should all remain transparent to the user.  swap() then switches
 * in_string and alt_string.  alt_string is not allocated until its needed,
 * but after that it's grown with in_string unconditionally.
 *
 * The buffer->separate_out boolean keeps status of whether out_string points
 * to in_string (FALSE) or alt_string (TRUE).
 */

/* XXX err handling */

/* Internal API */

static void
hb_buffer_ensure (hb_buffer_t *buffer, unsigned int size)
{
  unsigned int new_allocated = buffer->allocated;

  if (size > new_allocated)
    {
      while (size > new_allocated)
	new_allocated += (new_allocated >> 1) + 8;

      if (buffer->positions)
	buffer->positions = realloc (buffer->positions, new_allocated * sizeof (buffer->positions[0]));

      buffer->in_string = realloc (buffer->in_string, new_allocated * sizeof (buffer->in_string[0]));

      if (buffer->separate_out)
        {
	  buffer->alt_string = realloc (buffer->alt_string, new_allocated * sizeof (buffer->alt_string[0]));
	  buffer->out_string = buffer->alt_string;
	}
      else
        {
	  buffer->out_string = buffer->in_string;

	  if (buffer->alt_string)
	    {
	      free (buffer->alt_string);
	      buffer->alt_string = NULL;
	    }
	}

      buffer->allocated = new_allocated;
    }
}

static void
hb_buffer_duplicate_out_buffer (hb_buffer_t *buffer)
{
  if (!buffer->alt_string)
    buffer->alt_string = malloc (buffer->allocated * sizeof (buffer->alt_string[0]));

  buffer->out_string = buffer->alt_string;
  memcpy (buffer->out_string, buffer->in_string, buffer->out_length * sizeof (buffer->out_string[0]));
  buffer->separate_out = TRUE;
}

static void
hb_buffer_ensure_separate (hb_buffer_t *buffer, unsigned int size)
{
  hb_buffer_ensure (buffer, size);
  if ( !buffer->separate_out )
    hb_buffer_duplicate_out_buffer (buffer);
}

/* Public API */

hb_buffer_t *
hb_buffer_new (void)
{
  hb_buffer_t *buffer;

  buffer = malloc (sizeof (hb_buffer_t));
  if (HB_UNLIKELY (!buffer))
    return NULL;

  buffer->allocated = 0;
  buffer->in_string = NULL;
  buffer->alt_string = NULL;
  buffer->positions = NULL;

  hb_buffer_clear (buffer);

  return buffer;
}

void
hb_buffer_free (hb_buffer_t *buffer)
{
  free (buffer->in_string);
  free (buffer->alt_string);
  free (buffer->positions);
  free (buffer);
}

void
hb_buffer_clear (hb_buffer_t *buffer)
{
  buffer->in_length = 0;
  buffer->out_length = 0;
  buffer->in_pos = 0;
  buffer->out_pos = 0;
  buffer->out_string = buffer->in_string;
  buffer->separate_out = FALSE;
  buffer->max_lig_id = 0;
}

void
hb_buffer_add_glyph (hb_buffer_t    *buffer,
		     hb_codepoint_t  glyph_index,
		     unsigned int    properties,
		     unsigned int    cluster)
{
  hb_glyph_info_t *glyph;

  hb_buffer_ensure (buffer, buffer->in_length + 1);

  glyph = &buffer->in_string[buffer->in_length];
  glyph->gindex = glyph_index;
  glyph->properties = properties;
  glyph->cluster = cluster;
  glyph->component = 0;
  glyph->ligID = 0;
  glyph->gproperty = HB_BUFFER_GLYPH_PROPERTIES_UNKNOWN;

  buffer->in_length++;
}

/* HarfBuzz-Internal API */

HB_INTERNAL void
_hb_buffer_clear_output (hb_buffer_t *buffer)
{
  buffer->out_length = 0;
  buffer->out_pos = 0;
  buffer->out_string = buffer->in_string;
  buffer->separate_out = FALSE;
}

HB_INTERNAL void
_hb_buffer_clear_positions (hb_buffer_t *buffer)
{
  _hb_buffer_clear_output (buffer);

  if (HB_UNLIKELY (!buffer->positions))
  {
    buffer->positions = calloc (buffer->allocated, sizeof (buffer->positions[0]));
    return;
  }

  memset (buffer->positions, 0, sizeof (buffer->positions[0]) * buffer->in_length);
}

HB_INTERNAL void
_hb_buffer_swap (hb_buffer_t *buffer)
{
  unsigned int tmp;

  if (buffer->separate_out)
    {
      hb_glyph_info_t *tmp_string;
      tmp_string = buffer->in_string;
      buffer->in_string = buffer->out_string;
      buffer->out_string = tmp_string;
      buffer->alt_string = buffer->out_string;
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

   If `ligID' is 0xFFFF, the ligID value from buffer->in_pos
   will copied `num_out' times, otherwise `ligID' itself will
   be used to fill the `ligID' fields.

   The properties for all replacement glyphs are taken
   from the glyph at position `buffer->in_pos'.

   The cluster value for the glyph at position buffer->in_pos is used
   for all replacement glyphs */
HB_INTERNAL void
_hb_buffer_add_output_glyphs (hb_buffer_t *buffer,
			      unsigned int num_in,
			      unsigned int num_out,
			      const uint16_t *glyph_data_be,
			      unsigned short component,
			      unsigned short ligID)
{
  unsigned int i;
  unsigned int properties;
  unsigned int cluster;

  hb_buffer_ensure_separate (buffer, buffer->out_pos + num_out);

  properties = buffer->in_string[buffer->in_pos].properties;
  cluster = buffer->in_string[buffer->in_pos].cluster;
  if (component == 0xFFFF)
    component = buffer->in_string[buffer->in_pos].component;
  if (ligID == 0xFFFF)
    ligID = buffer->in_string[buffer->in_pos].ligID;

  for (i = 0; i < num_out; i++)
  {
    hb_glyph_info_t *info = &buffer->out_string[buffer->out_pos + i];

    info->gindex = hb_be_uint16_t (glyph_data_be[i]);
    info->properties = properties;
    info->cluster = cluster;
    info->component = component;
    info->ligID = ligID;
    info->gproperty = HB_BUFFER_GLYPH_PROPERTIES_UNKNOWN;
  }

  buffer->in_pos  += num_in;
  buffer->out_pos += num_out;

  buffer->out_length = buffer->out_pos;
}


HB_INTERNAL void
_hb_buffer_add_output_glyph (hb_buffer_t *buffer,
			     hb_codepoint_t glyph_index,
			     unsigned short component,
			     unsigned short ligID)
{
  hb_glyph_info_t *info;

  hb_buffer_ensure_separate (buffer, buffer->out_pos + 1);

  info = &buffer->out_string[buffer->out_pos];
  *info = buffer->in_string[buffer->in_pos];

  info->gindex = glyph_index;
  if (component != 0xFFFF)
    info->component = component;
  if (ligID != 0xFFFF)
    info->ligID = ligID;
  info->gproperty = HB_BUFFER_GLYPH_PROPERTIES_UNKNOWN;

  buffer->in_pos++;
  buffer->out_pos++;

  buffer->out_length = buffer->out_pos;
}

HB_INTERNAL void
_hb_buffer_next_glyph (hb_buffer_t *buffer)
{
  if (buffer->separate_out)
    {
      hb_buffer_ensure (buffer, buffer->out_pos + 1);

      buffer->out_string[buffer->out_pos] = buffer->in_string[buffer->in_pos];
    }

  buffer->in_pos++;
  buffer->out_pos++;
  buffer->out_length = buffer->out_pos;
}

HB_INTERNAL void
_hb_buffer_replace_glyph (hb_buffer_t *buffer,
			  hb_codepoint_t glyph_index)
{
  if (!buffer->separate_out)
    {
      buffer->out_string[buffer->out_pos].gindex = glyph_index;

      buffer->in_pos++;
      buffer->out_pos++;
      buffer->out_length = buffer->out_pos;
    }
  else
    {
      return _hb_buffer_add_output_glyph (buffer, glyph_index, 0xFFFF, 0xFFFF);
    }
}

HB_INTERNAL unsigned short
_hb_buffer_allocate_lig_id (hb_buffer_t *buffer)
{
  return ++buffer->max_lig_id;
}
