/*
 * Copyright © 2019  Ebrahim Byagowi
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


int
main (int argc, char **argv)
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
  }

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
  }

  {
    const char str[] = ".123";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str);

    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert ((int) roundf (pv * 1000.) == 123);
    hb_always_assert (pp - str == 4);
    hb_always_assert (end - pp == 1);
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
  }

  {
    const char str[] = "1e512";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (isinf (pv) && pv > 0);
  }
  {
    const char str[] = "1e600";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (isinf (pv) && pv > 0);
  }
  {
    const char str[] = "1e-512";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 0.0);
  }
  {
    const char str[] = "1e-600";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 0.0);
  }

  {
    const char str[] = "0.9999999999999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.0);
  }
  {
    const char str[] = "0.499999999999999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 0.5);
  }

  {
    const char str[] = "1e";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.0);
    hb_always_assert (pp == str + 1);
  }
  {
    const char str[] = "1e+";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.0);
    hb_always_assert (pp == str + 1);
  }
  {
    const char str[] = "-";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
  }
  {
    const char str[] = ".";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
  }
  {
    const char str[] = "1e-";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.0);
    hb_always_assert (pp == str + 1);
  }
  {
    const char str[] = "-1e";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == -1.0);
    hb_always_assert (pp == str + 2);
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
  }
  {
    const char str[] = "1.abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.0);
    hb_always_assert (pp == str + 2); // at least "1." was consumed
  }

  {
    const char str[] = "abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
  }
  {
    const char str[] = "";
    const char *pp = str;
    const char *end = str;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
  }
  {
    const char str[] = " \t 123";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 123.0);
    hb_always_assert (pp == end);
  }
  {
    const char str[] = "-0.0";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 0.0 && signbit (pv));
  }
  {
    const char str[] = "1.5E10";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 1.5e10);
  }
  {
    const char str[] = "1.5E+10";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 1.5e10);
  }
  {
    const char str[] = "1e308";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv > 0 && pv <= DBL_MAX);
  }
  {
    const char str[] = "1e309";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (isinf (pv) && pv > 0);
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
  }
  {
    const char str[] = "1E5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 100000.0);
  }
  {
    const char str[] = "123abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 123.0);
    hb_always_assert (pp == str + 3);
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
  }
  {
    const char str[] = "1.5E-10";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 1.5e-10);
  }
  {
    const char str[] = "-.5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == -0.5);
  }
  {
    const char str[] = "500";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 500.0);
  }
  {
    const char str[] = "-7.5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == -7.5);
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
  }
  {
    const char str[] = "+1";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 1.0);
  }
  {
    const char str[] = "+1.5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 1.5);
  }
  {
    const char str[] = "--1";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
  }
  {
    const char str[] = "+-1";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
  }
  {
    const char str[] = "0e5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 0.0);
  }
  {
    const char str[] = "1.5e";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.5);
    hb_always_assert (pp == str + 3);
  }
  {
    const char str[] = "1.5e+";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.5);
    hb_always_assert (pp == str + 3);
  }
  {
    const char str[] = "1e9999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == DBL_MAX);
  }
  {
    const char str[] = "-1e9999999";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == -DBL_MAX);
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
  }
  {
    const char str[] = "007";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 7.0);
  }
  {
    const char str[] = "0.0";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv == 0.0 && !signbit (pv));
  }
  {
    const char str[] = "NaN";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
  }
  {
    const char str[] = "inf";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
  }
  {
    const char str[] = "1..5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.0);
    hb_always_assert (pp == str + 2); // Don't consume ending ".5"
  }
  {
    const char str[] = "1.5.5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 1.5);
    hb_always_assert (pp == str + 3); // Don't consume ending ".5"
  }

  {
    const char str[] = "   ";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
  }
  {
    const char str[] = " abc";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv = 99.0;
    hb_always_assert (!hb_parse_double (&pp, end, &pv));
    hb_always_assert (pp == str);
  }

  {
    const char str[] = "1.7976931348623157e308";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv >= DBL_MAX || isinf (pv));
    hb_always_assert (pp == end);
  }
  {
    const char str[] = "99999999999999999999"; // 20 nines
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv, true));
    hb_always_assert (pv >= 1e19 && pv <= 1.1e20);
    hb_always_assert (pp == end);
  }
  {
    const char str[] = "1.e5";
    const char *pp = str;
    const char *end = str + ARRAY_LENGTH (str) - 1;
    double pv;
    hb_always_assert (hb_parse_double (&pp, end, &pv));
    hb_always_assert (pv == 100000.0);
    hb_always_assert (pp == end);
  }

  return 0;
}
