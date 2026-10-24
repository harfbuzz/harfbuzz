/*
 * Copyright © 2019-2026  Ebrahim Byagowi
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
 */

#include <cmath>
#include <cstdlib>

#include "hb.hh"
#include "hb-number.hh"

static void
test_parse_int ()
{
  {
    const char str[] = "123";
    const char *pp = str;
    const char *end = str + 3;

    int pv;
    hb_always_assert (hb_parse_int (&pp, end, &pv));
    hb_always_assert (pv == 123);
    hb_always_assert (pp - str == 3);
    hb_always_assert (end - pp == 0);
    hb_always_assert (!*end);

    char *libc_end;
    hb_always_assert (pv == strtol (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }

  {
    const char str[] = "-123";
    const char *pp = str;
    const char *end = str + 4;

    int pv;
    hb_always_assert (hb_parse_int (&pp, end, &pv));
    hb_always_assert (pv == -123);
    hb_always_assert (pp - str == 4);
    hb_always_assert (end - pp == 0);
    hb_always_assert (!*end);

    char *libc_end;
    hb_always_assert (pv == strtol (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }

  /* Leading whitespace */
  {
    const char str[] = "   123";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv;
    hb_always_assert (hb_parse_int (&pp, end, &pv));
    hb_always_assert (pv == 123);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == strtol (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "\t123";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv;
    hb_always_assert (hb_parse_int (&pp, end, &pv));
    hb_always_assert (pv == 123);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == strtol (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "\n123";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv;
    hb_always_assert (hb_parse_int (&pp, end, &pv));
    hb_always_assert (pv == 123);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == strtol (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = " -123";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv;
    hb_always_assert (hb_parse_int (&pp, end, &pv));
    hb_always_assert (pv == -123);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == strtol (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }

  /* Leading '+' sign */
  {
    const char str[] = "+123";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv;
    hb_always_assert (hb_parse_int (&pp, end, &pv));
    hb_always_assert (pv == 123);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == strtol (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "+0";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv;
    hb_always_assert (hb_parse_int (&pp, end, &pv));
    hb_always_assert (pv == 0);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == strtol (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }

  /* Leading zeros are decimal, not octal (base is passed as 10) */
  {
    const char str[] = "007";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv;
    hb_always_assert (hb_parse_int (&pp, end, &pv));
    hb_always_assert (pv == 7);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == strtol (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }

  /* Negative zero */
  {
    const char str[] = "-0";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv;
    hb_always_assert (hb_parse_int (&pp, end, &pv));
    hb_always_assert (pv == 0);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == strtol (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }

  /* Partial parse: stops at first non-digit */
  {
    const char str[] = "123abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv;
    hb_always_assert (hb_parse_int (&pp, end, &pv));
    hb_always_assert (pv == 123);
    hb_always_assert (pp == str + 3);

    char *libc_end;
    hb_always_assert (pv == strtol (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }

  /* INT_MAX / INT_MIN (fits in long on every supported platform) */
  {
    const char str[] = "2147483647";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv;
    hb_always_assert (hb_parse_int (&pp, end, &pv));
    hb_always_assert (pv == 2147483647);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == strtol (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "-2147483648";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv;
    hb_always_assert (hb_parse_int (&pp, end, &pv));
    hb_always_assert (pv == -2147483647 - 1);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == strtol (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }

  /* Failures: no digits */
  {
    const char str[] = "   ";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv = 99;
    hb_always_assert (!hb_parse_int (&pp, end, &pv));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99);
  }
  {
    const char str[] = "+";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv = 99;
    hb_always_assert (!hb_parse_int (&pp, end, &pv));
  }
  {
    const char str[] = "-";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv = 99;
    hb_always_assert (!hb_parse_int (&pp, end, &pv));
  }
  {
    const char str[] = "abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv = 99;
    hb_always_assert (!hb_parse_int (&pp, end, &pv));
  }

  /* Multiple signs fail */
  {
    const char str[] = "++1";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv = 99;
    hb_always_assert (!hb_parse_int (&pp, end, &pv));
  }
  {
    const char str[] = "--1";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv = 99;
    hb_always_assert (!hb_parse_int (&pp, end, &pv));
  }
  {
    const char str[] = "+-1";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv = 99;
    hb_always_assert (!hb_parse_int (&pp, end, &pv));
  }
  {
    const char str[] = "-+1";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv = 99;
    hb_always_assert (!hb_parse_int (&pp, end, &pv));
  }

  /* whole_buffer: leading whitespace ok, trailing content rejected,
   * and *pp / *pv are left untouched on failure */
  {
    const char str[] = " 123";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv;
    hb_always_assert (hb_parse_int (&pp, end, &pv, true));
    hb_always_assert (pv == 123);
    hb_always_assert (pp == end);
  }
  {
    const char str[] = "123abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv = 99;
    hb_always_assert (!hb_parse_int (&pp, end, &pv, true));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99);
  }
  {
    /* 10 digits > INT_MAX.  The old strtol-based implementation silently
     * truncated on 64-bit platforms (long is 64-bit) but rejected on
     * 32-bit; the new implementation rejects uniformly. */
    const char str[] = "9999999999";   /* 10 digits, > INT_MAX */
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv = 99;
    hb_always_assert (!hb_parse_int (&pp, end, &pv));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99);
  }

  /* One past INT_MAX - must fail */
  {
    const char str[] = "2147483648";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv = 99;
    hb_always_assert (!hb_parse_int (&pp, end, &pv));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99);
  }
  /* One past INT_MIN - must fail */
  {
    const char str[] = "-2147483649";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv = 99;
    hb_always_assert (!hb_parse_int (&pp, end, &pv));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99);
  }

  /* Same for signed */
  {
    const char str[] = "9999999999999999999999999999999999999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv = 99;
    hb_always_assert (!hb_parse_int (&pp, end, &pv));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99);
  }

  {
    /* Verify *pv and *pp are untouched when a range check rejects. */
    const char str[] = "9999999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv = 42;
    hb_always_assert (!hb_parse_int (&pp, end, &pv));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 42);
  }

  /* Whitespace between sign and digits is not allowed */
  {
    const char str[] = "- 123";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv = 99;
    hb_always_assert (!hb_parse_int (&pp, end, &pv));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99);
  }

  /* Sign alone plus trailing whitespace still fails */
  {
    const char str[] = "-   ";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    int pv = 99;
    hb_always_assert (!hb_parse_int (&pp, end, &pv));
    hb_always_assert (pv == 99);
  }
}

static void
test_parse_uint ()
{
  {
    const char str[] = "123";
    const char *pp = str;
    const char *end = str + strlen (str);

    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pv == 123);
    hb_always_assert (pp - str == 3);
    hb_always_assert (end - pp == 0);
    hb_always_assert (!*end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }

  {
    const char str[] = "12F";
    const char *pp = str;
    const char *end = str + 3;

    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 16));
    hb_always_assert (pv == 0x12F);
    hb_always_assert (pp - str == 3);
    hb_always_assert (end - pp == 0);
    hb_always_assert (!*end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 16));
    hb_always_assert (pp == libc_end);
  }

  {
    const char str[] = "12Fq";
    const char *pp = str;
    const char *end = str + 4;

    unsigned int pv;
    hb_always_assert (!hb_parse_uint (&pp, end, &pv, true, 16));
    hb_always_assert (hb_parse_uint (&pp, end, &pv, false, 16));
    hb_always_assert (pv == 0x12F);
    hb_always_assert (pp - str == 3);
    hb_always_assert (end - pp == 1);
    hb_always_assert (!*end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 16));
    hb_always_assert (pp == libc_end);
  }

  {
    const char str[] = "123";
    const char *pp = str;
    hb_always_assert (ARRAY_LENGTH (str) == 4);
    const char *end = str + ARRAY_LENGTH (str);

    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pv == 123);
    hb_always_assert (pp - str == 3);
    hb_always_assert (end - pp == 1);
  }

  {
    const char str[] = "123\0";
    const char *pp = str;
    hb_always_assert (ARRAY_LENGTH (str) == 5);
    const char *end = str + ARRAY_LENGTH (str);

    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pv == 123);
    hb_always_assert (pp - str == 3);
    hb_always_assert (end - pp == 2);
  }

  {
    const char str[] = "123V";
    const char *pp = str;
    hb_always_assert (ARRAY_LENGTH (str) == 5);
    const char *end = str + ARRAY_LENGTH (str);

    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pv == 123);
    hb_always_assert (pp - str == 3);
    hb_always_assert (end - pp == 2);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }

  {
    const char str[] = "123abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv = 99;
    hb_always_assert (!hb_parse_uint (&pp, end, &pv, true));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99);
  }

  /* Leading whitespace */
  {
    const char str[] = " 123";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pv == 123);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "\t123";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pv == 123);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }

  /* Leading zeros (still decimal when base is 10) */
  {
    const char str[] = "007";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pv == 7);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "000";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pv == 0);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }

  /* UINT_MAX */
  {
    const char str[] = "4294967295";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pv == 4294967295u);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }

  /* Negative wraps in two's complement (matches strtoul on unsigned) */
  {
    const char str[] = "-1";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pv == 4294967295u);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "-0";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv = 99;
    hb_always_assert (hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pv == 0);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }

  /* Base 2 */
  {
    const char str[] = "1010";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 2));
    hb_always_assert (pv == 10);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 2));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "11111111";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 2));
    hb_always_assert (pv == 255);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 2));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "12";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    /* '2' is not a base-2 digit; parse stops at index 1 */
    hb_always_assert (hb_parse_uint (&pp, end, &pv, false, 2));
    hb_always_assert (pv == 1);
    hb_always_assert (pp == str + 1);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 2));
    hb_always_assert (pp == libc_end);
  }

  /* Base 8 */
  {
    const char str[] = "777";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 8));
    hb_always_assert (pv == 511);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 8));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "010";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 8));
    hb_always_assert (pv == 8);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 8));
    hb_always_assert (pp == libc_end);
  }

  /* Base 16, with and without 0x prefix, mixed case */
  {
    const char str[] = "FF";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 16));
    hb_always_assert (pv == 255);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 16));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "ff";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 16));
    hb_always_assert (pv == 255);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 16));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "Ff";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 16));
    hb_always_assert (pv == 255);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 16));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "0xFF";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 16));
    hb_always_assert (pv == 255);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 16));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "0XFF";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 16));
    hb_always_assert (pv == 255);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 16));
    hb_always_assert (pp == libc_end);
  }

  /* Base 36 */
  {
    const char str[] = "Z";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 36));
    hb_always_assert (pv == 35);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 36));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "z";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 36));
    hb_always_assert (pv == 35);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 36));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "10";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 36));
    hb_always_assert (pv == 36);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 36));
    hb_always_assert (pp == libc_end);
  }

  /* Failures */
  {
    const char str[] = "abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv = 99;
    /* 'a' isn't a decimal digit; whole parse fails */
    hb_always_assert (!hb_parse_uint (&pp, end, &pv, false, 10));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99);
  }
  {
    const char str[] = "+";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv = 99;
    hb_always_assert (!hb_parse_uint (&pp, end, &pv));
  }
  {
    const char str[] = "-";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv = 99;
    hb_always_assert (!hb_parse_uint (&pp, end, &pv));
  }
  {
    const char str[] = "";
    const char *pp = str;
    const char *end = str;
    unsigned int pv = 99;
    hb_always_assert (!hb_parse_uint (&pp, end, &pv));
  }

  {
    const char str[] = "0XFF";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 16));
    hb_always_assert (pv == 255);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 16));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "0xff";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 16));
    hb_always_assert (pv == 255);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 16));
    hb_always_assert (pp == libc_end);
  }
  {
    const char str[] = "0x0";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 16));
    hb_always_assert (pv == 0);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 16));
    hb_always_assert (pp == libc_end);
  }
  /* "0x" alone: consume only the 0, leave the x */
  {
    const char str[] = "0x";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, false, 16));
    hb_always_assert (pv == 0);
    hb_always_assert (pp == str + 1);

    // MSVCRT has a different behavior for "0x" alone, let's skip cross-check here.
    // char *libc_end;
    // hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 16));
    // hb_always_assert (pp == libc_end);
  }
  /* "0xG": 0 is consumed, xG left */
  {
    const char str[] = "0xG";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, false, 16));
    hb_always_assert (pv == 0);
    hb_always_assert (pp == str + 1);

    // MSVCRT has a different behavior for this, let's skip cross-check here.
    // char *libc_end;
    // hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 16));
    // hb_always_assert (pp == libc_end);
  }
  /* Negative with prefix */
  {
    const char str[] = "-0xFF";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 16));
    hb_always_assert (pv == (unsigned) -255);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 16));
    hb_always_assert (pp == libc_end);
  }
  /* Prefix rule doesn't fire for base 10 */
  {
    const char str[] = "0xFF";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, false, 10));
    hb_always_assert (pv == 0);
    hb_always_assert (pp == str + 1);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }
  {
    /* 10 digits > UINT_MAX.  The old strtol-based implementation silently
     * truncated on 64-bit platforms (unsigned long is 64-bit) but rejected on
     * 32-bit; the new implementation rejects uniformly. */
    const char str[] = "9999999999";   /* 10 digits, > UINT_MAX */
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv = 99;
    hb_always_assert (!hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99);
  }

  /* One past UINT_MAX - must fail (currently truncates to 0) */
  {
    const char str[] = "4294967296";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv = 99;
    hb_always_assert (!hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99);
  }

  /* 20 digits, way past UINT_MAX */
  {
    const char str[] = "99999999999999999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv = 99;
    hb_always_assert (!hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99);
  }
  /* 40 digits, past the old 32-byte stack buffer */
  {
    const char str[] = "9999999999999999999999999999999999999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv = 99;
    hb_always_assert (!hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99);
  }

  /* 0xFFFFFFFF = UINT_MAX */
  {
    const char str[] = "0xFFFFFFFF";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 16));
    hb_always_assert (pv == 4294967295u);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 16));
    hb_always_assert (pp == libc_end);
  }
  /* 0x100000000 = UINT_MAX + 1 */
  {
    const char str[] = "0x100000000";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv = 99;
    hb_always_assert (!hb_parse_uint (&pp, end, &pv, true, 16));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99);
  }

  /* 13 'Z's = 36^13 - 1, comfortably > UINT64_MAX would need ~12 digits */
  {
    const char str[] = "ZZZZZZZZZZZZZZ";   /* 14 Z's */
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv = 99;
    hb_always_assert (!hb_parse_uint (&pp, end, &pv, true, 36));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99);
  }

  {
    const char str[] = "9999999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv = 42;
    hb_always_assert (!hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 42);
  }

  /* Negative values whose magnitude exceeds UINT_MAX must fail, not wrap */
  {
    const char str[] = "-4294967296";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv = 99;
    hb_always_assert (!hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99);
  }

  /* INT_MIN magnitude as unsigned: wraps to 0x80000000 */
  {
    const char str[] = "-2147483648";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pv == 2147483648u);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 10));
    hb_always_assert (pp == libc_end);
  }

  /* Mixed case in base 36 */
  {
    const char str[] = "Az";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 36));
    hb_always_assert (pv == 10 * 36 + 35);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 36));
    hb_always_assert (pp == libc_end);
  }

  /* Whitespace between sign and digits is not allowed */
  {
    const char str[] = "- 1";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv = 99;
    hb_always_assert (!hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pv == 99);
  }

  /* "0x" prefix is case-insensitive but only for base 16 */
  {
    const char str[] = "0X10";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, true, 16));
    hb_always_assert (pv == 16);
    hb_always_assert (pp == end);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 16));
    hb_always_assert (pp == libc_end);
  }

  /* A second "0x" is not consumed as a prefix */
  {
    const char str[] = "0x0x10";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv, false, 16));
    hb_always_assert (pv == 0);
    hb_always_assert (pp == str + 3);

    char *libc_end;
    hb_always_assert (pv == (unsigned) strtoul (str, &libc_end, 16));
    hb_always_assert (pp == libc_end);
  }

  /* Null byte stops the scan (end passed at NUL) */
  {
    const char str[] = "12\0F";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    unsigned int pv;
    hb_always_assert (hb_parse_uint (&pp, end, &pv));
    hb_always_assert (pv == 12);
    hb_always_assert (pp == str + 2);
  }
}

/* Tolerate 1 ULP difference. Needed because 32-bit x86 (x87) computes
 * at 80-bit precision and rounds to 64-bit on store, which can differ
 * from a directly-rounded decimal literal by one ULP. */
static inline bool
almost_equal (double a, double b)
{
  return fabs (a - b) <= fabs (a) * 2 * DBL_EPSILON;
}

static void
test_parse_double (void)
{
  {
    const char str[] = ".123";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str);

    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert ((int) roundf (pv * 1000.) == 123);
    hb_always_assert (pp - str == 4);
    hb_always_assert (end - pp == 1);
    hb_always_assert (pv == strtod (str, nullptr));
  }

  {
    const char str[] = "0.123";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;

    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert ((int) roundf (pv * 1000.) == 123);
    hb_always_assert (pp - str == 5);
    hb_always_assert (end - pp == 0);
    hb_always_assert (pv == strtod (str, nullptr));
  }

  {
    const char str[] = "0.123e0";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;

    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert ((int) roundf (pv * 1000.) == 123);
    hb_always_assert (pp - str == 7);
    hb_always_assert (end - pp == 0);
    hb_always_assert (pv == strtod (str, nullptr));
  }

  {
    const char str[] = "123e-3";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;

    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert ((int) roundf (pv * 1000.) == 123);
    hb_always_assert (pp - str == 6);
    hb_always_assert (end - pp == 0);
    hb_always_assert (pv == strtod (str, nullptr));
  }

  {
    const char str[] = ".000123e+3";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;

    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert ((int) roundf (pv * 1000.) == 123);
    hb_always_assert (pp - str == 10);
    hb_always_assert (end - pp == 0);
    hb_always_assert (almost_equal (pv, strtod (str, nullptr)));
  }

  {
    const char str[] = "-.000000123e6";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;

    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert ((int) roundf (pv * 1000.) == -123);
    hb_always_assert (pp - str == 13);
    hb_always_assert (end - pp == 0);
    hb_always_assert (pv == strtod (str, nullptr));
  }

  {
    const char str[] = "-1.23E-1";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;

    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert ((int) roundf (pv * 1000.) == -123);
    hb_always_assert (pp - str == 8);
    hb_always_assert (end - pp == 0);
    hb_always_assert (pv == strtod (str, nullptr));
  }

  {
    const char str[] = "1e512";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (std::isinf (pv) && pv > 0);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1e600";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (std::isinf (pv) && pv > 0);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1e-512";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 0.0);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1e-600";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 0.0);
    hb_always_assert (pv == strtod (str, nullptr));
  }

  {
    const char str[] = "0.9999999999999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.0);
    /* Known 1-ULP divergence: the fraction accumulator keeps only
     * ~15 significant digits, and 16 nines carry up to 1.0 through
     * rounding.  strtod uses arbitrary-precision intermediates and
     * returns the double below 1.0, which is genuinely closer to the
     * true value.  Both are within 1 ULP. */
    hb_always_assert (almost_equal (pv, strtod (str, nullptr)));
  }
  {
    const char str[] = "0.499999999999999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 0.5);
    hb_always_assert (pv == strtod (str, nullptr));
  }

  {
    const char str[] = "1e";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.0);
    hb_always_assert (pp == str + 1);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1e+";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.0);
    hb_always_assert (pp == str + 1);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "-";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 99);
  }
  {
    const char str[] = ".";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 99);
  }
  {
    const char str[] = "1e-";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.0);
    hb_always_assert (pp == str + 1);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "-1e";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == -1.0);
    hb_always_assert (pp == str + 2);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "+";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
  }
  {
    const char str[] = "-.";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
  }

  // But be careful with fractional parts
  {
    const char str[] = "1.";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 1.0);
    hb_always_assert (pp == end);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1.abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.0);
    hb_always_assert (pp == str + 2); // at least "1." was consumed
    hb_always_assert (pv == strtod (str, nullptr));
  }

  {
    const char str[] = "abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 99.0);
  }
  {
    const char str[] = "";
    const char *pp = str;
    const char *end = str;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 99.0);
  }
  {
    const char str[] = " \t 123";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 123.0);
    hb_always_assert (pp == end);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "-0.0";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 0.0 && std::signbit (pv));
    hb_always_assert (std::signbit (pv) == std::signbit (strtod (str, nullptr)));
  }
  {
    const char str[] = "1.5E10";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 1.5e10);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1.5E+10";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 1.5e10);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1e308";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv > 0 && pv <= DBL_MAX);
    hb_always_assert (almost_equal (pv, strtod (str, nullptr)));
  }
  {
    const char str[] = "1e309";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (std::isinf (pv) && pv > 0);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1e-309";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv > 0.0 && pv < DBL_MIN);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "5e-324";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv > 0.0 && pv < DBL_MIN);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1e+5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 100000.0);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1E5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 100000.0);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "123abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 123.0);
    hb_always_assert (pp == str + 3);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "123abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (!hb_parse_double (&pp, end, &pv, true));
  }
  {
    const char str[] = "-1.5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == -1.5);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1.5E-10";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (almost_equal (pv, 1.5e-10));
    hb_always_assert (almost_equal (pv, strtod (str, nullptr)));
  }
  {
    const char str[] = "-.5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == -0.5);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "500";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 500.0);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "-7.5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == -7.5);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "123 ";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (!hb_parse_double (&pp, end, &pv, true));
  }
  {
    const char str[] = " 123";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 123.0);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "+1";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 1.0);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "+1.5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 1.5);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "--1";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 99.0);
  }
  {
    const char str[] = "+-1";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 99.0);
  }
  {
    const char str[] = "0e5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 0.0);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1.5e";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.5);
    hb_always_assert (pp == str + 3);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1.5e+";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.5);
    hb_always_assert (pp == str + 3);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1e9999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (std::isinf (pv) && pv > 0);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "-1e9999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (std::isinf (pv) && pv < 0);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1e-9999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == .0);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "-1e-9999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == -.0);
    hb_always_assert (pv == -strtod (str, nullptr));
  }
  {
    const char str[] = "1e-400";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 0.0);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "007";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 7.0);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "0.0";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 0.0 && !std::signbit (pv));
    hb_always_assert (std::signbit (pv) == std::signbit (strtod (str, nullptr)));
  }
  {
    const char str[] = "NaN";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 99.0);
  }
  {
    const char str[] = "inf";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 99.0);
  }
  {
    const char str[] = "1..5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.0);
    hb_always_assert (pp == str + 2); // Don't consume ending ".5"
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1.5.5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.5);
    hb_always_assert (pp == str + 3); // Don't consume ending ".5"
    hb_always_assert (pv == strtod (str, nullptr));
  }

  {
    const char str[] = "   ";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 99.0);
  }
  {
    const char str[] = " abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 99.0);
  }

  {
    const char str[] = "1.7976931348623157e308";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv >= DBL_MAX || std::isinf (pv));
    hb_always_assert (pp == end);
    /* Known divergence: this literal has 17 significant digits, and our
     * integer accumulator has no sticky bit, so the final product may
     * round up past DBL_MAX and overflow to infinity.  strtod produces
     * DBL_MAX.  Both are accepted here; matching strtod exactly would
     * require sticky-bit rounding on the integer part (see the 20-nines
     * test for the same issue at a smaller scale, where we relax to
     * almost_equal). */
  }
  {
    const char str[] = "99999999999999999999"; // 20 nines
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv >= 1e19 && pv <= 1.1e20);
    hb_always_assert (pp == end);
    hb_always_assert (almost_equal (pv, strtod (str, nullptr)));
  }
  {
    const char str[] = "1.e5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 100000.0);
    hb_always_assert (pp == end);
    hb_always_assert (pv == strtod (str, nullptr));
  }

  /* Exponent with leading zeros */
  {
    const char str[] = "1e005";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 100000.0);
    hb_always_assert (pp == end);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  {
    const char str[] = "1e-005";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (almost_equal (pv, 1e-5));
    hb_always_assert (pp == end);
    hb_always_assert (almost_equal (pv, strtod (str, nullptr)));
  }

  /* Bare -0 (no decimal) preserves the sign, matching strtod */
  {
    const char str[] = "-0";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 0.0 && std::signbit (pv));
    hb_always_assert (pp == end);
    hb_always_assert (std::signbit (pv) == std::signbit (strtod (str, nullptr)));
  }

  /* Long mantissa followed by an exponent */
  {
    const char str[] = "12345678901234567890e-10";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv > 1.234e9 && pv < 1.235e9);
    hb_always_assert (pp == end);
    hb_always_assert (almost_equal (pv, strtod (str, nullptr)));
  }

  /* whole_buffer failure must not touch *pv */
  {
    const char str[] = "123abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 42.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 42.0);
  }

  /* Same for no-match failure */
  {
    const char str[] = "abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 42.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
    hb_always_assert (pp == str);
    hb_always_assert (pv == 42.0);
  }

  /* Exponent followed by trailing content */
  {
    const char str[] = "1e5abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 100000.0);
    hb_always_assert (pp == str + 3);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  /* Same with a sign in the exponent */
  {
    const char str[] = "1e+5x";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 100000.0);
    hb_always_assert (pp == str + 4);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  /* Malformed exponent followed by content: should stop at the 'e' */
  {
    const char str[] = "1ex";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.0);
    hb_always_assert (pp == str + 1);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  /* Zero mantissa with an overflowing exponent must yield zero, not a clamped DBL_MAX. */
  {
    const char str[] = "0e9999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 0.0 && !std::signbit (pv));
    hb_always_assert (pv == strtod (str, nullptr));
  }
  /* Same, but the sign of zero must be preserved. */
  {
    const char str[] = "-0e9999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 0.0 && std::signbit (pv));
    hb_always_assert (pv == strtod (str, nullptr));
    hb_always_assert (std::signbit (pv) == std::signbit (strtod (str, nullptr)));
  }
  /* 1000-digit integer part: value saturates to Infinity, parse still
   * consumes everything and terminates in linear time. */
  {
    char str[1001 + 1];
    memset (str, '9', 1000);
    str[1000] = '\0';
    const char *pp = str;
    const char *end = str + 1000;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (std::isinf (pv) && pv > 0);
    hb_always_assert (pp == end);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  /* 1000-digit fraction: accumulator saturates, sticky/drop handles the
   * rest, result is 1.0 (all nines). */
  {
    char str[2 + 1000 + 1];
    str[0] = '0'; str[1] = '.';
    memset (str + 2, '9', 1000);
    str[2 + 1000] = '\0';
    const char *pp = str;
    const char *end = str + 2 + 1000;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.0);
    hb_always_assert (pp == end);
    hb_always_assert (pv == strtod (str, nullptr));
  }
  /* 1000-digit exponent: exp_overflow fires, result is +infinity */
  {
    char str[2 + 1000 + 1];
    str[0] = '1'; str[1] = 'e';
    memset (str + 2, '9', 1000);
    str[2 + 1000] = '\0';
    const char *pp = str;
    const char *end = str + 2 + 1000;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (std::isinf (pv) && pv > 0);
    hb_always_assert (pv == strtod (str, nullptr));
    hb_always_assert (pp == end);
  }
}

int
main (int argc, char **argv)
{
  test_parse_double ();
  test_parse_int ();
  test_parse_uint ();

  return 0;
}
