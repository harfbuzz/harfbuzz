/*
 * Copyright © 2009,2010  Red Hat, Inc.
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
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#include "hb-private.hh"

HB_BEGIN_DECLS


/* hb_tag_t */

hb_tag_t
hb_tag_from_string (const char *s)
{
  char tag[4];
  unsigned int i;

  if (!s || !*s)
    return HB_TAG_NONE;

  for (i = 0; i < 4 && s[i]; i++)
    tag[i] = s[i];
  for (; i < 4; i++)
    tag[i] = ' ';

  return HB_TAG_CHAR4 (tag);
}


/* hb_direction_t */

const char direction_strings[][4] = {
  "ltr",
  "rtl",
  "ttb",
  "btt"
};

hb_direction_t
hb_direction_from_string (const char *str)
{
  if (unlikely (!str || !*str))
    return HB_DIRECTION_INVALID;

  /* Lets match loosely: just match the first letter, such that
   * all of "ltr", "left-to-right", etc work!
   */
  char c = TOLOWER (str[0]);
  for (unsigned int i = 0; i < ARRAY_LENGTH (direction_strings); i++)
    if (c == direction_strings[i][0])
      return (hb_direction_t) i;

  return HB_DIRECTION_INVALID;
}

const char *
hb_direction_to_string (hb_direction_t direction)
{
  if (likely ((unsigned int) direction < ARRAY_LENGTH (direction_strings)))
    return direction_strings[direction];

  return "invalid";
}


/* hb_language_t */

struct _hb_language_t {
  const char s[1];
};

static const char canon_map[256] = {
   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,  '-',  0,   0,
  '0', '1', '2', '3', '4', '5', '6', '7',  '8', '9',  0,   0,   0,   0,   0,   0,
  '-', 'a', 'b', 'c', 'd', 'e', 'f', 'g',  'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w',  'x', 'y', 'z',  0,   0,   0,   0,  '-',
   0,  'a', 'b', 'c', 'd', 'e', 'f', 'g',  'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w',  'x', 'y', 'z',  0,   0,   0,   0,   0
};

static hb_bool_t
lang_equal (const void *v1,
	    const void *v2)
{
  const unsigned char *p1 = (const unsigned char *) v1;
  const unsigned char *p2 = (const unsigned char *) v2;

  while (canon_map[*p1] && canon_map[*p1] == canon_map[*p2])
    {
      p1++, p2++;
    }

  return (canon_map[*p1] == canon_map[*p2]);
}

#if 0
static unsigned int
lang_hash (const void *key)
{
  const unsigned char *p = key;
  unsigned int h = 0;
  while (canon_map[*p])
    {
      h = (h << 5) - h + canon_map[*p];
      p++;
    }

  return h;
}
#endif


hb_language_t
hb_language_from_string (const char *str)
{
  static unsigned int num_langs;
  static unsigned int num_alloced;
  static hb_language_t *langs;
  unsigned int i;
  unsigned char *p;

  /* TODO Use a hash table or something */

  if (!str || !*str)
    return NULL;

  for (i = 0; i < num_langs; i++)
    if (lang_equal (str, langs[i]->s))
      return langs[i];

  if (unlikely (num_langs == num_alloced)) {
    unsigned int new_alloced = 2 * (8 + num_alloced);
    hb_language_t *new_langs = (hb_language_t *) realloc (langs, new_alloced * sizeof (langs[0]));
    if (!new_langs)
      return NULL;
    num_alloced = new_alloced;
    langs = new_langs;
  }

  langs[i] = (hb_language_t) strdup (str);
  for (p = (unsigned char *) langs[i]->s; *p; p++)
    *p = canon_map[*p];

  num_langs++;

  return langs[i];
}

const char *
hb_language_to_string (hb_language_t language)
{
  return language->s;
}


/* hb_script_t */

hb_script_t
hb_script_from_iso15924_tag (hb_tag_t tag)
{
  if (unlikely (tag == HB_TAG_NONE))
    return HB_SCRIPT_INVALID;

  /* Be lenient, adjust case (one capital letter followed by three small letters) */
  tag = (tag & 0xDFDFDFDF) | 0x00202020;

  switch (tag) {

    /* These graduated from the 'Q' private-area codes, but
     * the old code is still aliased by Unicode, and the Qaai
     * one in use by ICU. */
    case HB_TAG('Q','a','a','i'): return HB_SCRIPT_INHERITED;
    case HB_TAG('Q','a','a','c'): return HB_SCRIPT_COPTIC;

    /* Script variants from http://unicode.org/iso15924/ */
    case HB_TAG('C','y','r','s'): return HB_SCRIPT_CYRILLIC;
    case HB_TAG('L','a','t','f'): return HB_SCRIPT_LATIN;
    case HB_TAG('L','a','t','g'): return HB_SCRIPT_LATIN;
    case HB_TAG('S','y','r','e'): return HB_SCRIPT_SYRIAC;
    case HB_TAG('S','y','r','j'): return HB_SCRIPT_SYRIAC;
    case HB_TAG('S','y','r','n'): return HB_SCRIPT_SYRIAC;
  }

  /* If it looks right, just use the tag as a script */
  if (((uint32_t) tag & 0xE0E0E0E0) == 0x40606060)
    return (hb_script_t) tag;

  /* Otherwise, return unknown */
  return HB_SCRIPT_UNKNOWN;
}

hb_script_t
hb_script_from_string (const char *s)
{
  return hb_script_from_iso15924_tag (hb_tag_from_string (s));
}

hb_tag_t
hb_script_to_iso15924_tag (hb_script_t script)
{
  return (hb_tag_t) script;
}

hb_direction_t
hb_script_get_horizontal_direction (hb_script_t script)
{
  switch ((hb_tag_t) script)
  {
    case HB_SCRIPT_ARABIC:
    case HB_SCRIPT_HEBREW:
    case HB_SCRIPT_SYRIAC:
    case HB_SCRIPT_THAANA:

    /* Unicode-4.0 additions */
    case HB_SCRIPT_CYPRIOT:

    /* Unicode-5.0 additions */
    case HB_SCRIPT_PHOENICIAN:
    case HB_SCRIPT_NKO:

    /* Unicode-5.2 additions */
    case HB_SCRIPT_AVESTAN:
    case HB_SCRIPT_IMPERIAL_ARAMAIC:
    case HB_SCRIPT_INSCRIPTIONAL_PAHLAVI:
    case HB_SCRIPT_INSCRIPTIONAL_PARTHIAN:
    case HB_SCRIPT_OLD_SOUTH_ARABIAN:
    case HB_SCRIPT_OLD_TURKIC:
    case HB_SCRIPT_SAMARITAN:

    /* Unicode-6.0 additions */
    case HB_SCRIPT_MANDAIC:

      return HB_DIRECTION_RTL;
  }

  return HB_DIRECTION_LTR;
}


HB_END_DECLS
