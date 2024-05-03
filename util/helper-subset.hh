/*
 * Copyright © 2023  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
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
 * Google Author(s): Garret Rieger
 */
#ifndef HELPER_SUBSET_HH
#define HELPER_SUBSET_HH

#include "glib.h"
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include "hb-subset.h"

#ifndef HB_NO_VAR

// Parses an axis position string and sets min, default, and max to
// the requested values. If a value should be set to it's default value
// then it will be set to NaN.
static gboolean
parse_axis_position(const char* s,
                    float* min,
                    float* def,
                    float* max,
                    gboolean* drop,
                    GError **error)
{
  const char* part = strpbrk(s, ":");
  *drop = false;
  if (!part) {
    // Single value.
    if (strcmp (s, "drop") == 0)
    {
      *min = NAN;
      *def = NAN;
      *max = NAN;
      *drop = true;
      return true;
    }

    errno = 0;
    char *p;
    float axis_value = strtof (s, &p);
    if (errno || s == p)
    {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                   "Failed parsing axis value at: '%s'", s);
      return false;
    }

    *min = axis_value;
    *def = axis_value;
    *max = axis_value;
    return true;
  }


  float values[3];
  int count = 0;
  for (int i = 0; i < 3; i++) {
    errno = 0;
    count++;
    if (!*s || part == s) {
      values[i] = NAN;

      if (part == NULL) break;
      s = part + 1;
      part = strpbrk(s, ":");
      continue;
    }

    char *pend;
    values[i] = strtof (s, &pend);
    if (errno || s == pend || (part && pend != part))
    {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                   "Failed parsing axis value at: '%s'", s);
      return false;
    }

    if (part == NULL) break;
    s = pend + 1;
    part = strpbrk(s, ":");
  }

  if (count == 2) {
    *min = values[0];
    *def = NAN;
    *max = values[1];
    return true;
  } else if (count == 3) {
    *min = values[0];
    *def = values[1];
    *max = values[2];
    return true;
  }

  g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                   "Failed parsing axis value at: '%s'", s);
  return false;
}

static gboolean
parse_instancing_spec (const char *arg,
                       hb_face_t* face,
                       hb_subset_input_t* input,
                       GError    **error)
{
  char* s;
  while ((s = strtok((char *) arg, "=")))
  {
    arg = NULL;
    unsigned len = strlen (s);
    if (len > 4)  //Axis tags are 4 bytes.
    {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                   "Failed parsing axis tag at: '%s'", s);
      return false;
    }

    /* support *=drop */
    if (0 == strcmp (s, "*"))
    {
      s = strtok(NULL, ", ");
      if (0 != strcmp (s, "drop"))
      {
        g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                     "Failed parsing axis position at: '%s'", s);
        return false;
      }

      if (!hb_subset_input_pin_all_axes_to_default (input, face))
      {
        g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                     "Failed pinning all axes to default.");
        return false;
      }
      continue;
    }

    hb_tag_t axis_tag = hb_tag_from_string (s, len);

    s = strtok(NULL, ", ");
    if (!s)
    {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                   "Value not specified for axis: %c%c%c%c", HB_UNTAG (axis_tag));
      return false;
    }

    gboolean drop;
    float min, def, max;
    if (!parse_axis_position(s, &min, &def, &max, &drop, error))
      return false;

    if (drop)
    {
      if (!hb_subset_input_pin_axis_to_default (input,
                                                face,
                                                axis_tag))
      {
        g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                     "Cannot pin axis: '%c%c%c%c', not present in fvar", HB_UNTAG (axis_tag));
        return false;
      }
      continue;
    }

    if (min == def && def == max) {
      if (!hb_subset_input_pin_axis_location (input,
                                              face, axis_tag,
                                              def))
      {
        g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                     "Cannot pin axis: '%c%c%c%c', not present in fvar", HB_UNTAG (axis_tag));
        return false;
      }
      continue;
    }

    if (!hb_subset_input_set_axis_range (input,
                                         face, axis_tag,
                                         min, max, def))
    {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                   "Cannot pin axis: '%c%c%c%c', not present in fvar", HB_UNTAG (axis_tag));
      return false;
    }
    continue;

    g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                 "Partial instancing is not supported.");
    return false;
  }

  return true;
}

#endif

#endif
