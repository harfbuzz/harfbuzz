/*
 * Copyright © 2011,2012,2014  Google, Inc.
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

#ifndef HB_UTF_PRIVATE_HH
#define HB_UTF_PRIVATE_HH

#include "hb-private.hh"


/* UTF-8 */

static inline const uint8_t *
hb_utf_next (const uint8_t *text,
	     const uint8_t *end,
	     hb_codepoint_t *unicode)
{
  /* Written to only accept well-formed sequences.
   * Based on ideas from ICU's U8_NEXT.
   * Generates a -1 for each ill-formed byte. */

  hb_codepoint_t c = *text++;

  if (c > 0x7Fu)
  {
    if (hb_in_range (c, 0xC2u, 0xDFu)) /* Two-byte */
    {
      unsigned int t1;
      if (likely (text < end &&
		  (t1 = text[0] - 0x80u) <= 0x3Fu))
      {
	c = ((c&0x1Fu)<<6) | t1;
	text++;
      }
      else
	goto error;
    }
    else if (hb_in_range (c, 0xE0u, 0xEFu)) /* Three-byte */
    {
      unsigned int t1, t2;
      if (likely (1 < end - text &&
		  (t1 = text[0] - 0x80u) <= 0x3Fu &&
		  (t2 = text[1] - 0x80u) <= 0x3Fu))
      {
	c = ((c&0xFu)<<12) | (t1<<6) | t2;
	if (unlikely (c < 0x0800u || hb_in_range (c, 0xD800u, 0xDFFFu)))
	  goto error;
	text += 2;
      }
      else
	goto error;
    }
    else if (hb_in_range (c, 0xF0u, 0xF4u)) /* Four-byte */
    {
      unsigned int t1, t2, t3;
      if (likely (2 < end - text &&
		  (t1 = text[0] - 0x80u) <= 0x3Fu &&
		  (t2 = text[1] - 0x80u) <= 0x3Fu &&
		  (t3 = text[2] - 0x80u) <= 0x3Fu))
      {
	c = ((c&0x7u)<<18) | (t1<<12) | (t2<<6) | t3;
	if (unlikely (!hb_in_range (c, 0x10000u, 0x10FFFFu)))
	  goto error;
	text += 3;
      }
      else
	goto error;
    }
    else
      goto error;
  }

  *unicode = c;
  return text;

error:
  *unicode = -1;
  return text;
}

static inline const uint8_t *
hb_utf_prev (const uint8_t *text,
	     const uint8_t *start,
	     hb_codepoint_t *unicode)
{
  const uint8_t *end = text--;
  while (start < text && (*text & 0xc0) == 0x80 && end - text < 4)
    text--;

  if (likely (hb_utf_next (text, end, unicode) == end))
    return text;

  *unicode = -1;
  return end - 1;
}


static inline unsigned int
hb_utf_strlen (const uint8_t *text)
{
  return strlen ((const char *) text);
}


/* UTF-16 */

static inline const uint16_t *
hb_utf_next (const uint16_t *text,
	     const uint16_t *end,
	     hb_codepoint_t *unicode)
{
  hb_codepoint_t c = *text++;

  if (likely (!hb_in_range (c, 0xD800u, 0xDFFFu)))
  {
    *unicode = c;
    return text;
  }

  if (likely (hb_in_range (c, 0xD800u, 0xDBFFu)))
  {
    /* High-surrogate in c */
    hb_codepoint_t l;
    if (text < end && ((l = *text), likely (hb_in_range (l, 0xDC00u, 0xDFFFu))))
    {
      /* Low-surrogate in l */
      *unicode = (c << 10) + l - ((0xD800u << 10) - 0x10000u + 0xDC00u);
       text++;
       return text;
    }
  }

  /* Lonely / out-of-order surrogate. */
  *unicode = -1;
  return text;
}

static inline const uint16_t *
hb_utf_prev (const uint16_t *text,
	     const uint16_t *start,
	     hb_codepoint_t *unicode)
{
  const uint16_t *end = text--;
  hb_codepoint_t c = *text;

  if (likely (!hb_in_range (c, 0xD800u, 0xDFFFu)))
  {
    *unicode = c;
    return text;
  }

  if (likely (start < text && hb_in_range (c, 0xDC00u, 0xDFFFu)))
    text--;

  if (likely (hb_utf_next (text, end, unicode) == end))
    return text;

  *unicode = -1;
  return end - 1;
}


static inline unsigned int
hb_utf_strlen (const uint16_t *text)
{
  unsigned int l = 0;
  while (*text++) l++;
  return l;
}


/* UTF-32 */

static inline const uint32_t *
hb_utf_next (const uint32_t *text,
	     const uint32_t *end HB_UNUSED,
	     hb_codepoint_t *unicode)
{
  *unicode = *text++;
  return text;
}

static inline const uint32_t *
hb_utf_prev (const uint32_t *text,
	     const uint32_t *start HB_UNUSED,
	     hb_codepoint_t *unicode)
{
  *unicode = *--text;
  return text;
}

static inline unsigned int
hb_utf_strlen (const uint32_t *text)
{
  unsigned int l = 0;
  while (*text++) l++;
  return l;
}


#endif /* HB_UTF_PRIVATE_HH */
