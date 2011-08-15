/*
 * Copyright © 2011  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#include "common.hh"

#ifndef OPTIONS_HH
#define OPTIONS_HH


extern struct view_options_t
{
  view_options_t (void) {
    memset (this, 0, sizeof (*this));
    fore = "#000000";
    back = "#ffffff";
    margin.t = margin.r = margin.b = margin.l = 18.;
  }

  hb_bool_t annotate;
  const char *fore;
  const char *back;
  double line_space;
  struct margin_t {
    double t, r, b, l;
  } margin;
} view_opts[1];

extern struct shape_options_t
{
  shape_options_t (void) {
    memset (this, 0, sizeof (*this));
  }

  const char *text;
  const char *direction;
  const char *language;
  const char *script;
  hb_feature_t *features;
  unsigned int num_features;
  char **shapers;
} shape_opts[1];

extern struct font_options_t
{
  font_options_t (void) {
    memset (this, 0, sizeof (*this));
    font_size = 36.;
  }

  const char *font_file;
  int face_index;
  double font_size;
} font_opts[1];


extern const char *out_file;
extern hb_bool_t debug;


void parse_options (int argc, char *argv[]);


#endif
