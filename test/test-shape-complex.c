/*
 * Copyright © 2011  Google, Inc.
 * Copyright © 2008  Nokia Corporation and/or its subsidiary(-ies)
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

#include "hb-test.h"

/* Unit tests for complex text shaping */

/*
 * This test provides a framework to test aspects of hb_shape() that are
 * font-dependent.  Please add tests for any feature that fits that
 * description.
 */

#include <hb-ft.h>

typedef struct
{
  const char *font_file;
  unsigned int face_index;
  /* TODO add min/max face version */
} font_data_t;

static char *
get_font_file (const font_data_t *font_data)
{
  return g_strdup_printf ("%s/fonts/%s", srcdir (), font_data->font_file);
}


typedef struct
{
  char           comments[64];
  hb_codepoint_t characters[16];
  hb_codepoint_t glyphs[16];
} test_data_t;

typedef struct
{
  const font_data_t font_data;
  const test_data_t test_datas[];
} test_set_t;

typedef struct
{
  const font_data_t *font_data;
  const test_data_t *test_data;
} test_t;


static const test_set_t tests_devanagari1 = {
  {"raghu.ttf", 0},
  {
    { "Ka",
      { 0x0915, 0 },
      { 0x0080, 0 }
    },
    { "Ka Halant",
      { 0x0915, 0x094d, 0 },
      { 0x0080, 0x0051, 0 }
    },
    { "Ka Halant Ka",
      { 0x0915, 0x094d, 0x0915, 0 },
      { 0x00c8, 0x0080, 0 }
    },
    { "Ka MatraI",
      { 0x0915, 0x093f, 0 },
      { 0x01d1, 0x0080, 0 }
    },
    { "Ra Halant Ka",
      { 0x0930, 0x094d, 0x0915, 0 },
      { 0x0080, 0x005b, 0 }
    },
    { "Ra Halant Ka MatraI",
      { 0x0930, 0x094d, 0x0915, 0x093f, 0 },
      { 0x01d1, 0x0080, 0x005b, 0 }
    },
    { "MatraI",
      { 0x093f, 0 },
      { 0x01d4, 0x029c, 0 }
    },
    { "Ka Nukta",
      { 0x0915, 0x093c, 0 },
      { 0x00a4, 0 }
    },
    { "Ka Halant Ra",
      { 0x0915, 0x094d, 0x0930, 0 },
      { 0x0110, 0 }
    },
    { "Ka Halant Ra Halant Ka",
      { 0x0915, 0x094d, 0x0930, 0x094d, 0x0915, 0 },
      { 0x0158, 0x0080, 0 }
    },
    { "",
      { 0x0930, 0x094d, 0x200d, 0 },
      { 0x00e2, 0 }
    },
    { "",
      { 0x0915, 0x094d, 0x0930, 0x094d, 0x200d, 0 },
      { 0x0158, 0 }
    },
    {{0}}
  }
};

static const test_set_t tests_devanagari2 = {
  {"mangal.ttf", 0},
  {
    { "Ka",
      { 0x0915, 0 },
      { 0x0080, 0 }
    },
    { "Ka Halant",
      { 0x0915, 0x094d, 0 },
      { 0x0080, 0x0051, 0 }
    },
    { "Ka Halant Ka",
      { 0x0915, 0x094d, 0x0915, 0 },
      { 0x00c8, 0x0080, 0 }
    },
    { "Ka MatraI",
      { 0x0915, 0x093f, 0 },
      { 0x01d1, 0x0080, 0 }
    },
    { "Ra Halant Ka",
      { 0x0930, 0x094d, 0x0915, 0 },
      { 0x0080, 0x005b, 0 }
    },
    { "Ra Halant Ka MatraI",
      { 0x0930, 0x094d, 0x0915, 0x093f, 0 },
      { 0x01d1, 0x0080, 0x005b, 0 }
    },
    { "MatraI",
      { 0x093f, 0 },
      { 0x01d4, 0x029c, 0 }
    },
    { "Ka Nukta",
      { 0x0915, 0x093c, 0 },
      { 0x00a4, 0 }
    },
    { "Ka Halant Ra",
      { 0x0915, 0x094d, 0x0930, 0 },
      { 0x0110, 0 }
    },
    { "Ka Halant Ra Halant Ka",
      { 0x0915, 0x094d, 0x0930, 0x094d, 0x0915, 0 },
      { 0x0158, 0x0080, 0 }
    },
    { "",
      { 0x92b, 0x94d, 0x930, 0 },
      { 0x125, 0 }
    },
    { "",
      { 0x92b, 0x93c, 0x94d, 0x930, 0 },
      { 0x149, 0 }
    },
    {{0}}
  }
};

static const test_set_t tests_bengali1 = {
  {"AkaashNormal.ttf", 0},
  {
    { "Ka",
      { 0x0995, 0 },
      { 0x0151, 0 }
    },
    { "Ka Halant",
      { 0x0995, 0x09cd, 0 },
      { 0x0151, 0x017d, 0 }
    },
    { "Ka Halant Ka",
      { 0x0995, 0x09cd, 0x0995, 0 },
      { 0x019b, 0 }
    },
    { "Ka MatraI",
      { 0x0995, 0x09bf, 0 },
      { 0x0173, 0x0151, 0 }
    },
    { "Ra Halant Ka",
      { 0x09b0, 0x09cd, 0x0995, 0 },
      { 0x0151, 0x0276, 0 }
    },
    { "Ra Halant Ka MatraI",
      { 0x09b0, 0x09cd, 0x0995, 0x09bf, 0 },
      { 0x0173, 0x0151, 0x0276, 0 }
    },
    { "Ka Nukta",
      { 0x0995, 0x09bc, 0 },
      { 0x0151, 0x0171, 0 }
    },
    { "Ka Halant Ra",
      { 0x0995, 0x09cd, 0x09b0, 0 },
      { 0x01f4, 0 }
    },
    { "Ka Halant Ra Halant Ka",
      { 0x0995, 0x09cd, 0x09b0, 0x09cd, 0x0995, 0 },
      { 0x025c, 0x0276, 0x0151, 0 }
    },
    { "Ya + Halant",
      { 0x09af, 0x09cd, 0 },
      { 0x016a, 0x017d, 0 }
    },
    { "Da Halant Ya -> Da Ya-Phala",
      { 0x09a6, 0x09cd, 0x09af, 0 },
      { 0x01e5, 0 }
    },
    { "A Halant Ya -> A Ya-phala",
      { 0x0985, 0x09cd, 0x09af, 0 },
      { 0x0145, 0x01cf, 0 }
    },
    { "Na Halant Ka",
      { 0x09a8, 0x09cd, 0x0995, 0 },
      { 0x026f, 0x0151, 0 }
    },
    { "Na Halant ZWNJ Ka",
      { 0x09a8, 0x09cd, 0x200c, 0x0995, 0 },
      { 0x0164, 0x017d, 0x0151, 0 }
    },
    { "Na Halant ZWJ Ka",
      { 0x09a8, 0x09cd, 0x200d, 0x0995, 0 },
      { 0x026f, 0x0151, 0 }
    },
    { "Ka Halant ZWNJ Ka",
      { 0x0995, 0x09cd, 0x200c, 0x0995, 0 },
      { 0x0151, 0x017d, 0x0151, 0 }
    },
    { "Ka Halant ZWJ Ka",
      { 0x0995, 0x09cd, 0x200d, 0x0995, 0 },
      { 0x025c, 0x0151, 0 }
    },
    { "Na Halant Ra",
      { 0x09a8, 0x09cd, 0x09b0, 0 },
      { 0x0207, 0 }
    },
    { "Na Halant ZWNJ Ra",
      { 0x09a8, 0x09cd, 0x200c, 0x09b0, 0 },
      { 0x0164, 0x017d, 0x016b, 0 }
    },
    { "Na Halant ZWJ Ra",
      { 0x09a8, 0x09cd, 0x200d, 0x09b0, 0 },
      { 0x026f, 0x016b, 0 }
    },
    { "Na Halant Ba",
      { 0x09a8, 0x09cd, 0x09ac, 0 },
      { 0x022f, 0 }
    },
    { "Na Halant ZWNJ Ba",
      { 0x09a8, 0x09cd, 0x200c, 0x09ac, 0 },
      { 0x0164, 0x017d, 0x0167, 0 }
    },
    { "Na Halant ZWJ Ba",
      { 0x09a8, 0x09cd, 0x200d, 0x09ac, 0 },
      { 0x026f, 0x0167, 0 }
    },
    { "Na Halant Dha",
      { 0x09a8, 0x09cd, 0x09a7, 0 },
      { 0x01d3, 0 }
    },
    { "Na Halant ZWNJ Dha",
      { 0x09a8, 0x09cd, 0x200c, 0x09a7, 0 },
      { 0x0164, 0x017d, 0x0163, 0 }
    },
    { "Na Halant ZWJ Dha",
      { 0x09a8, 0x09cd, 0x200d, 0x09a7, 0 },
      { 0x026f, 0x0163, 0 }
    },
    { "Ra Halant Ka MatraAU",
      { 0x09b0, 0x09cd, 0x0995, 0x09cc, 0 },
      { 0x0179, 0x0151, 0x0276, 0x017e, 0 }
    },
    { "Ra Halant Ba Halant Ba",
      { 0x09b0, 0x09cd, 0x09ac, 0x09cd, 0x09ac, 0 },
      { 0x0232, 0x0276, 0 }
    },
    { "",
      { 0x9b0, 0x9cd, 0x995, 0x9be, 0x982, 0 },
      { 0x151, 0x276, 0x172, 0x143, 0 }
    },
    { "",
      { 0x9b0, 0x9cd, 0x995, 0x9be, 0x983, 0 },
      { 0x151, 0x276, 0x172, 0x144, 0 }
    },
    /*  Test decomposed two part matras */
    { "",
      { 0x995, 0x9c7, 0x9be, 0 },
      { 0x179, 0x151, 0x172, 0 }
    },
    { "",
      { 0x995, 0x9c7, 0x9d7, 0 },
      { 0x179, 0x151, 0x17e, 0 }
    },
    { "",
      { 0x9b0, 0x9cd, 0x9ad, 0 },
      { 0x168, 0x276, 0 }
    },
    { "",
      { 0x9f0, 0x9cd, 0x9ad, 0 },
      { 0x168, 0x276, 0 }
    },
    { "",
      { 0x9f1, 0x9cd, 0x9ad, 0 },
      { 0x191, 0x17d, 0x168, 0 }
    },
    {{0}}
  }
};

static const test_set_t tests_bengali2 = {
  {"MuktiNarrow.ttf", 0},
  {
    { "Ka",
      { 0x0995, 0 },
      { 0x0073, 0 }
    },
    { "Ka Halant",
      { 0x0995, 0x09cd, 0 },
      { 0x00b9, 0 }
    },
    { "Ka Halant Ka",
      { 0x0995, 0x09cd, 0x0995, 0 },
      { 0x0109, 0 }
    },
    { "Ka MatraI",
      { 0x0995, 0x09bf, 0 },
      { 0x0095, 0x0073, 0 }
    },
    { "Ra Halant Ka",
      { 0x09b0, 0x09cd, 0x0995, 0 },
      { 0x0073, 0x00e1, 0 }
    },
    { "Ra Halant Ka MatraI",
      { 0x09b0, 0x09cd, 0x0995, 0x09bf, 0 },
      { 0x0095, 0x0073, 0x00e1, 0 }
    },
    { "MatraI",
      { 0x09bf, 0 },
      { 0x0095, 0x01c8, 0 }
    },
    { "Ka Nukta",
      { 0x0995, 0x09bc, 0 },
      { 0x0073, 0x0093, 0 }
    },
    { "Ka Halant Ra",
      { 0x0995, 0x09cd, 0x09b0, 0 },
      { 0x00e5, 0 }
    },
    { "Ka Halant Ra Halant Ka",
      { 0x995, 0x9cd, 0x9b0, 0x9cd, 0x995, 0 },
      { 0x234, 0x24e, 0x73, 0 }
    },
    { "Ya + Halant",
      { 0x09af, 0x09cd, 0 },
      { 0x00d2, 0 }
    },
    { "Da Halant Ya -> Da Ya-Phala",
      { 0x09a6, 0x09cd, 0x09af, 0 },
      { 0x0084, 0x00e2, 0 }
    },
    { "A Halant Ya -> A Ya-phala",
      { 0x0985, 0x09cd, 0x09af, 0 },
      { 0x0067, 0x00e2, 0 }
    },
    { "Na Halant Ka",
      { 0x09a8, 0x09cd, 0x0995, 0 },
      { 0x0188, 0 }
    },
    { "Na Halant ZWNJ Ka",
      { 0x9a8, 0x9cd, 0x200c, 0x995, 0 },
      { 0xcc, 0x73, 0 }
    },
    { "Na Halant ZWJ Ka",
      { 0x9a8, 0x9cd, 0x200d, 0x995, 0 },
      { 0x247, 0x73, 0 }
    },
    { "Ka Halant ZWNJ Ka",
      { 0x9a8, 0x9cd, 0x200d, 0x995, 0 },
      { 0x247, 0x73, 0 }
    },
    { "Ka Halant ZWJ Ka",
      { 0x9a8, 0x9cd, 0x200d, 0x995, 0 },
      { 0x247, 0x73, 0 }
    },
    { "Na Halant Ra",
      { 0x09a8, 0x09cd, 0x09b0, 0 },
      { 0x00f8, 0 }
    },
    { "Na Halant ZWNJ Ra",
      { 0x09a8, 0x09cd, 0x200c, 0x09b0, 0 },
      { 0xcc, 0x8d, 0 }
    },
    { "Na Halant ZWJ Ra",
      { 0x9a8, 0x9cd, 0x200d, 0x9b0, 0 },
      { 0x247, 0x8d, 0 }
    },
    { "Na Halant Ba",
      { 0x09a8, 0x09cd, 0x09ac, 0 },
      { 0x0139, 0 }
    },
    { "Na Halant ZWNJ Ba",
      { 0x9a8, 0x9cd, 0x200c, 0x9ac, 0 },
      { 0xcc, 0x89, 0 }
    },
    { "Na Halant ZWJ Ba",
      { 0x9a8, 0x9cd, 0x200d, 0x9ac, 0 },
      { 0x247, 0x89, 0 }
    },
    { "Na Halant Dha",
      { 0x09a8, 0x09cd, 0x09a7, 0 },
      { 0x0145, 0 }
    },
    { "Na Halant ZWNJ Dha",
      { 0x09a8, 0x09cd, 0x200c, 0x09a7, 0 },
      { 0xcc, 0x85, 0 }
    },
    { "Na Halant ZWJ Dha",
      { 0x09a8, 0x09cd, 0x200d, 0x09a7, 0 },
      { 0x247, 0x85, 0 }
    },
    { "Ra Halant Ka MatraAU",
      { 0x9b0, 0x9cd, 0x995, 0x9cc, 0 },
      { 0x232, 0x73, 0xe1, 0xa0, 0 }
    },
    { "Ra Halant Ba Halant Ba",
      { 0x09b0, 0x09cd, 0x09ac, 0x09cd, 0x09ac, 0 },
      { 0x013b, 0x00e1, 0 }
    },
    {{0}}
  }
};

static const test_set_t tests_bengali3 = {
  {"LikhanNormal.ttf", 0},
  {
    { "",
      { 0x09a8, 0x09cd, 0x09af, 0 },
      { 0x01ca, 0 }
    },
    { "",
      { 0x09b8, 0x09cd, 0x09af, 0 },
      { 0x020e, 0 }
    },
    { "",
      { 0x09b6, 0x09cd, 0x09af, 0 },
      { 0x01f4, 0 }
    },
    { "",
      { 0x09b7, 0x09cd, 0x09af, 0 },
      { 0x01fe, 0 }
    },
    { "",
      { 0x09b0, 0x09cd, 0x09a8, 0x09cd, 0x200d, 0 },
      { 0x10b, 0x167, 0 }
    },
    { "",
      { 0x9b0, 0x9cd, 0x9ad, 0 },
      { 0xa1, 0x167, 0 }
    },
    { "",
      { 0x9f0, 0x9cd, 0x9ad, 0 },
      { 0xa1, 0x167, 0 }
    },
    { "",
      { 0x9f1, 0x9cd, 0x9ad, 0 },
      { 0x11c, 0xa1, 0 }
    },
    {{0}}
  }
};

static const test_set_t tests_gurmukhi = {
  {"lohit_pa.ttf", 0},
  {
    { "",
      { 0xA15, 0xA4D, 0xa39, 0 },
      { 0x3b, 0x8b, 0 }
    },
    {{0}}
  }
};

static const test_set_t tests_oriya = {
  {"utkalm.ttf", 0},
  {
    { "",
      { 0xb15, 0xb4d, 0xb24, 0xb4d, 0xb30, 0 },
      { 0x150, 0x125, 0 }
    },
    { "",
      { 0xb24, 0xb4d, 0xb24, 0xb4d, 0xb2c, 0 },
      { 0x151, 0x120, 0 }
    },
    { "",
      { 0xb28, 0xb4d, 0xb24, 0xb4d, 0xb2c, 0 },
      { 0x152, 0x120, 0 }
    },
    { "",
      { 0xb28, 0xb4d, 0xb24, 0xb4d, 0xb2c, 0 },
      { 0x152, 0x120, 0 }
    },
    { "",
      { 0xb28, 0xb4d, 0xb24, 0xb4d, 0xb30, 0 },
      { 0x176, 0 }
    },
    { "",
      { 0xb38, 0xb4d, 0xb24, 0xb4d, 0xb30, 0 },
      { 0x177, 0 }
    },
    { "",
      { 0xb28, 0xb4d, 0xb24, 0xb4d, 0xb30, 0xb4d, 0xb2f, 0 },
      { 0x176, 0x124, 0 }
    },
    {{0}}
  }
};

static const test_set_t tests_tamil = {
  {"akruti1.ttf", 0},
  {
    { "",
      { 0x0b95, 0x0bc2, 0 },
      { 0x004e, 0 }
    },
    { "",
      { 0x0bae, 0x0bc2, 0 },
      { 0x009e, 0 }
    },
    { "",
      { 0x0b9a, 0x0bc2, 0 },
      { 0x0058, 0 }
    },
    { "",
      { 0x0b99, 0x0bc2, 0 },
      { 0x0053, 0 }
    },
    { "",
      { 0x0bb0, 0x0bc2, 0 },
      { 0x00a8, 0 }
    },
    { "",
      { 0x0ba4, 0x0bc2, 0 },
      { 0x008e, 0 }
    },
    { "",
      { 0x0b9f, 0x0bc2, 0 },
      { 0x0062, 0 }
    },
    { "",
      { 0x0b95, 0x0bc6, 0 },
      { 0x000a, 0x0031, 0 }
    },
    { "",
      { 0x0b95, 0x0bca, 0 },
      { 0x000a, 0x0031, 0x0007, 0 }
    },
    { "",
      { 0x0b95, 0x0bc6, 0x0bbe, 0 },
      { 0x000a, 0x0031, 0x007, 0 }
    },
    { "",
      { 0x0b95, 0x0bcd, 0x0bb7, 0 },
      { 0x0049, 0 }
    },
    { "",
      { 0x0b95, 0x0bcd, 0x0bb7, 0x0bca, 0 },
      { 0x000a, 0x0049, 0x007, 0 }
    },
    { "",
      { 0x0b95, 0x0bcd, 0x0bb7, 0x0bc6, 0x0bbe, 0 },
      { 0x000a, 0x0049, 0x007, 0 }
    },
    { "",
      { 0x0b9f, 0x0bbf, 0 },
      { 0x005f, 0 }
    },
    { "",
      { 0x0b9f, 0x0bc0, 0 },
      { 0x0060, 0 }
    },
    { "",
      { 0x0bb2, 0x0bc0, 0 },
      { 0x00ab, 0 }
    },
    { "",
      { 0x0bb2, 0x0bbf, 0 },
      { 0x00aa, 0 }
    },
    { "",
      { 0x0bb0, 0x0bcd, 0 },
      { 0x00a4, 0 }
    },
    { "",
      { 0x0bb0, 0x0bbf, 0 },
      { 0x00a5, 0 }
    },
    { "",
      { 0x0bb0, 0x0bc0, 0 },
      { 0x00a6, 0 }
    },
    { "",
      { 0x0b83, 0 },
      { 0x0025, 0 }
    },
    { "",
      { 0x0b83, 0x0b95, 0 },
      { 0x0025, 0x0031, 0 }
    },
    {{0}}
  }
};

static const test_set_t tests_telugu = {
  {"Pothana2000.ttf", 0},
  {
    { "",
      { 0xc15, 0xc4d, 0 },
      { 0xbb, 0 }
    },
    { "",
      { 0xc15, 0xc4d, 0xc37, 0 },
      { 0x4b, 0 }
    },
    { "",
      { 0xc15, 0xc4d, 0xc37, 0xc4d, 0 },
      { 0xe0, 0 }
    },
    { "",
      { 0xc15, 0xc4d, 0xc37, 0xc4d, 0xc23, 0 },
      { 0x4b, 0x91, 0 }
    },
    { "",
      { 0xc15, 0xc4d, 0xc30, 0 },
      { 0x5a, 0xb2, 0 }
    },
    { "",
      { 0xc15, 0xc4d, 0xc30, 0xc4d, 0 },
      { 0xbb, 0xb2, 0 }
    },
    { "",
      { 0xc15, 0xc4d, 0xc30, 0xc4d, 0xc15, 0 },
      { 0x5a, 0xb2, 0x83, 0 }
    },
    { "",
      { 0xc15, 0xc4d, 0xc30, 0xc3f, 0 },
      { 0xe2, 0xb2, 0 }
    },
    { "",
      { 0xc15, 0xc4d, 0xc15, 0xc48, 0 },
      { 0xe6, 0xb3, 0x83, 0 }
    },
    { "",
      { 0xc15, 0xc4d, 0xc30, 0xc48, 0 },
      { 0xe6, 0xb3, 0x9f, 0 }
    },
    { "",
      { 0xc15, 0xc46, 0xc56, 0 },
      { 0xe6, 0xb3, 0 }
    },
    {{0}}
  }
};

static const test_set_t tests_kannada1 = {
  {"Sampige.ttf", 0},
  {
    { "",
      { 0x0ca8, 0x0ccd, 0x0ca8, 0 },
      { 0x0049, 0x00ba, 0 }
    },
    { "",
      { 0x0ca8, 0x0ccd, 0x0ca1, 0 },
      { 0x0049, 0x00b3, 0 }
    },
    { "",
      { 0x0caf, 0x0cc2, 0 },
      { 0x004f, 0x005d, 0 }
    },
    { "",
      { 0x0ce0, 0 },
      { 0x006a, 0 }
    },
    { "",
      { 0x0ce6, 0x0ce7, 0x0ce8, 0 },
      { 0x006b, 0x006c, 0x006d, 0 }
    },
    { "",
      { 0x0cb5, 0x0ccb, 0 },
      { 0x015f, 0x0067, 0 }
    },
    { "",
      { 0x0cb0, 0x0ccd, 0x0cae, 0 },
      { 0x004e, 0x0082, 0 }
    },
    { "",
      { 0x0cb0, 0x0ccd, 0x0c95, 0 },
      { 0x0036, 0x0082, 0 }
    },
    { "",
      { 0x0c95, 0x0ccd, 0x0cb0, 0 },
      { 0x0036, 0x00c1, 0 }
    },
    { "",
      { 0x0cb0, 0x0ccd, 0x200d, 0x0c95, 0 },
      { 0x0050, 0x00a7, 0 }
    },
    {{0}}
  }
};

static const test_set_t tests_kannada2 = {
  {"tunga.ttf", 0},
  {
    { "",
      { 0x0cb7, 0x0cc6, 0 },
      { 0x00b0, 0x006c, 0 }
    },
    { "",
      { 0x0cb7, 0x0ccd, 0 },
      { 0x0163, 0 }
    },
    { "",
      { 0xc95, 0xcbf, 0xcd5, 0 },
      { 0x114, 0x73, 0 }
    },
    { "",
      { 0xc95, 0xcc6, 0xcd5, 0 },
      { 0x90, 0x6c, 0x73, 0 }
    },
    { "",
      { 0xc95, 0xcc6, 0xcd6, 0 },
      { 0x90, 0x6c, 0x74, 0 }
    },
    { "",
      { 0xc95, 0xcc6, 0xcc2, 0 },
      { 0x90, 0x6c, 0x69, 0 }
    },
    { "",
      { 0xc95, 0xcca, 0xcd5, 0 },
      { 0x90, 0x6c, 0x69, 0x73, 0 }
    },
    {{0}}
  }
};

static const test_set_t tests_malayalam1 = {
  {"AkrutiMal2Normal.ttf", 0},
  {
    { "",
      { 0x0d15, 0x0d46, 0 },
      { 0x005e, 0x0034, 0 }
    },
    { "",
      { 0x0d15, 0x0d47, 0 },
      { 0x005f, 0x0034, 0 }
    },
    { "",
      { 0x0d15, 0x0d4b, 0 },
      { 0x005f, 0x0034, 0x0058, 0 }
    },
    { "",
      { 0x0d15, 0x0d48, 0 },
      { 0x0060, 0x0034, 0 }
    },
    { "",
      { 0x0d15, 0x0d4a, 0 },
      { 0x005e, 0x0034, 0x0058, 0 }
    },
    { "",
      { 0x0d30, 0x0d4d, 0x0d15, 0 },
      { 0x009e, 0x0034, 0 }
    },
    { "",
      { 0x0d15, 0x0d4d, 0x0d35, 0 },
      { 0x0034, 0x007a, 0 }
    },
    { "",
      { 0x0d15, 0x0d4d, 0x0d2f, 0 },
      { 0x0034, 0x00a2, 0 }
    },
    { "",
      { 0x0d1f, 0x0d4d, 0x0d1f, 0 },
      { 0x0069, 0 }
    },
    { "",
      { 0x0d26, 0x0d4d, 0x0d26, 0 },
      { 0x0074, 0 }
    },
    { "",
      { 0x0d30, 0x0d4d, 0 },
      { 0x009e, 0 }
    },
    { "",
      { 0x0d30, 0x0d4d, 0x200c, 0 },
      { 0x009e, 0 }
    },
    { "",
      { 0x0d30, 0x0d4d, 0x200d, 0 },
      { 0x009e, 0 }
    },
    { "",
      { 0xd15, 0xd46, 0xd3e, 0 },
      { 0x5e, 0x34, 0x58, 0 }
    },
    { "",
      { 0xd15, 0xd47, 0xd3e, 0 },
      { 0x5f, 0x34, 0x58, 0 }
    },
    { "",
      { 0xd15, 0xd46, 0xd57, 0 },
      { 0x5e, 0x34, 0x65, 0 }
    },
    { "",
      { 0xd15, 0xd57, 0 },
      { 0x34, 0x65, 0 }
    },
    { "",
      { 0xd1f, 0xd4d, 0xd1f, 0xd41, 0xd4d, 0 },
      { 0x69, 0x5b, 0x64, 0 }
    },

    {{0}}
  }
};

static const test_set_t tests_malayalam2 = {
  {"Rachana.ttf", 0},
  {
    { "",
      { 0xd37, 0xd4d, 0xd1f, 0xd4d, 0xd30, 0xd40, 0 },
      { 0x385, 0xa3, 0 }
    },
    { "",
      { 0xd2f, 0xd4d, 0xd15, 0xd4d, 0xd15, 0xd41, 0 },
      { 0x2ff, 0 }
    },
    { "",
      { 0xd33, 0xd4d, 0xd33, 0 },
      { 0x3f8, 0 }
    },
    { "",
      { 0xd2f, 0xd4d, 0xd15, 0xd4d, 0xd15, 0xd41, 0 },
      { 0x2ff, 0 }
    },
    { "",
      { 0xd30, 0xd4d, 0x200d, 0xd35, 0xd4d, 0xd35, 0 },
      { 0xf3, 0x350, 0 }
    },

    {{0}}
  }
};

static const test_set_t tests_sinhala = {
  {"FM-MalithiUW46.ttf", 0},
  {
    { "",
      { 0xd9a, 0xdd9, 0xdcf, 0 },
      { 0x4a, 0x61, 0x42, 0 }
    },
    { "",
      { 0xd9a, 0xdd9, 0xddf, 0 },
      { 0x4a, 0x61, 0x50, 0 }
    },
    { "",
      { 0xd9a, 0xdd9, 0xdca, 0 },
      { 0x4a, 0x62, 0 }
    },
    { "",
      { 0xd9a, 0xddc, 0xdca, 0 },
      { 0x4a, 0x61, 0x42, 0x41, 0 }
    },
    { "",
      { 0xd9a, 0xdda, 0 },
      { 0x4a, 0x62, 0 }
    },
    { "",
      { 0xd9a, 0xddd, 0 },
      { 0x4a, 0x61, 0x42, 0x41, 0 }
    },
    {{0}}
  }
};

static const test_set_t tests_khmer = {
  {"KhmerOS.ttf", 0},
  {
    { "",
      { 0x179a, 0x17cd, 0 },
      { 0x24c, 0x27f, 0 }
    },
    { "",
      { 0x179f, 0x17c5, 0 },
      { 0x273, 0x203, 0 }
    },
    { "",
      { 0x1790, 0x17d2, 0x1784, 0x17c3, 0 },
      { 0x275, 0x242, 0x182, 0 }
    },
    { "",
      { 0x179a, 0 },
      { 0x24c, 0 }
    },
    { "",
      { 0x1781, 0x17d2, 0x1798, 0x17c2, 0 },
      { 0x274, 0x233, 0x197, 0 }
    },
    { "",
      { 0x1798, 0x17b6, 0 },
      { 0x1cb, 0 }
    },
    { "",
      { 0x179a, 0x17b8, 0 },
      { 0x24c, 0x26a, 0 }
    },
    { "",
      { 0x1787, 0x17b6, 0 },
      { 0x1ba, 0 }
    },
    { "",
      { 0x1798, 0x17d2, 0x1796, 0x17bb, 0 },
      { 0x24a, 0x195, 0x26d, 0 }
    },
    {{0}}
  }
};

static const test_set_t tests_nko = {
  {"DejaVuSans.ttf", 0},
  {
    { "",
      { 0x7ca, 0 },
      { 0x5c1, 0 }
    },
    { "",
      { 0x7ca, 0x7ca, 0 },
      { 0x14db, 0x14d9, 0 }
    },
    { "",
      { 0x7ca, 0x7fa, 0x7ca, 0 },
      { 0x14db, 0x5ec, 0x14d9, 0 }
    },
    { "",
      { 0x7ca, 0x7f3, 0x7ca, 0 },
      { 0x14db, 0x5e7, 0x14d9, 0 }
    },
    { "",
      { 0x7ca, 0x7f3, 0x7fa, 0x7ca, 0 },
      { 0x14db, 0x5e7, 0x5ec, 0x14d9, 0 }
    },
    {{0}}
  }
};

static const test_set_t tests_linearb = {
  {"penuture.ttf", 0},
  {
    { "",
      { 0xd800, 0xdc01, 0xd800, 0xdc02, 0xd800, 0xdc03,  0 },
      { 0x5, 0x6, 0x7, 0 },
    },
    {{0}}
  }
};





typedef struct {
  FT_Library ft_library;
  FT_Face ft_face;
  hb_font_t *font;
} ft_fixture_t;

static void
ft_fixture_init (ft_fixture_t *f, gconstpointer user_data)
{
  const test_t *test = user_data;
  char *font_file = get_font_file (test->font_data);
  FT_Error err;

  FT_Init_FreeType (&f->ft_library);

  err = FT_New_Face (f->ft_library, font_file, test->font_data->face_index, &f->ft_face);
  g_assert_cmpint (err, ==, 0);

  f->font = hb_ft_font_create (f->ft_face, NULL);

  g_free (font_file);
}

static void
ft_fixture_finish (ft_fixture_t *f, gconstpointer user_data)
{
  hb_font_destroy (f->font);

  FT_Done_Face (f->ft_face);
  FT_Done_FreeType (f->ft_library);
}

static void
test_shape_complex (ft_fixture_t *f, gconstpointer user_data)
{
  const test_t *test = user_data;
  const test_data_t *data = test->test_data;
  hb_buffer_t *buffer;
  unsigned int i, len, expected_len;
  hb_glyph_info_t *glyphs;
  hb_bool_t fail;

  g_assert (f->font);

  if (data->comments[0])
    g_test_message ("Test comments: %s", data->comments);

  buffer =  hb_buffer_create (0);
  for (len = 0; data->characters[len]; len++) ;
  hb_buffer_add_utf32 (buffer, data->characters, len, 0, len);

  hb_shape (f->font, buffer, NULL, 0);

  for (len = 0; data->glyphs[len]; len++) ;
  expected_len = len;

  glyphs = hb_buffer_get_glyph_infos (buffer, &len);
  fail = len != expected_len;
  if (!fail)
    for (i = 0; i < len; i++)
      if (glyphs[i].codepoint != data->glyphs[i]) {
        fail = TRUE;
	break;
      }

  if (fail) {
    GString *str = g_string_new ("");
    for (i = 0; i < expected_len; i++)
      g_string_append_printf (str, " %4d", data->glyphs[i]);
    g_test_message ("Expected glyphs: %s", str->str);
    g_string_truncate (str, 0);
    for (i = 0; i < len; i++)
      g_string_append_printf (str, " %4d", glyphs[i].codepoint);
    g_test_message ("Received glyphs: %s", str->str);
    g_string_free (str, TRUE);

    g_test_fail ();
  }

  hb_buffer_destroy (buffer);
}

static void
test_shape_complex_skipped (gconstpointer user_data)
{
  const test_t *test = user_data;
  const test_data_t *data = test->test_data;

  if (data->comments[0])
    g_test_message ("Test comments: %s", data->comments);

  g_test_message ("Skipping test");
}


static void
add_test_set (const test_set_t *test_set, const char *set_name)
{
  const test_data_t *data;
  char *font_file;
  hb_bool_t skip;

  font_file = get_font_file (&test_set->font_data);
  skip = !g_file_test (font_file, G_FILE_TEST_EXISTS);
  g_free (font_file);

  for (data = test_set->test_datas; data->characters[0]; data++) {
    char *flavor;
    GString *str;
    const hb_codepoint_t *p;

    test_t *test = g_slice_new0 (test_t);
    test->font_data = &test_set->font_data;
    test->test_data = data;

    str = g_string_new ("<");
    for (p = data->characters; *p; p++)
      g_string_append_printf (str, "%04X,", *p);
    str->str[str->len - 1] = '>';

    flavor = g_strdup_printf ("%s/%s/%s", set_name, test_set->font_data.font_file, str->str);

    g_string_free (str, TRUE);

    if (skip)
      hb_test_add_data_flavor ((const void *) test, flavor, test_shape_complex_skipped);
    else
      hb_test_add_fixture_flavor (ft_fixture, (const void *) test, flavor, test_shape_complex);

    g_free (flavor);
  }
}


int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

#define TEST_SET(name) add_test_set (&tests_##name, #name)

  TEST_SET (devanagari1);
  TEST_SET (devanagari2);
  TEST_SET (bengali1);
  TEST_SET (bengali2);
  TEST_SET (bengali3);
  TEST_SET (gurmukhi);
  TEST_SET (oriya);
  TEST_SET (tamil);
  TEST_SET (telugu);
  TEST_SET (kannada1);
  TEST_SET (kannada2);
  TEST_SET (malayalam1);
  TEST_SET (malayalam2);
  TEST_SET (sinhala);

  TEST_SET (khmer);

  TEST_SET (nko);
  TEST_SET (linearb);

  return hb_test_run();
}
