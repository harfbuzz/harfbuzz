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
#ifndef HB_NUMBER_PARSER_HH
#define HB_NUMBER_PARSER_HH

#include "hb.hh"

#include <float.h>

/* Compile time calculating 10^n for n = 2^i */
constexpr double
pow10_of_2i (unsigned int n)
{
  return n == 1 ? 10. : pow10_of_2i (n >> 1) * pow10_of_2i (n >> 1);
}

static const double powers_of_10[] =
{
  pow10_of_2i (0x100),
  pow10_of_2i (0x80),
  pow10_of_2i (0x40),
  pow10_of_2i (0x20),
  pow10_of_2i (0x10),
  pow10_of_2i (0x8),
  pow10_of_2i (0x4),
  pow10_of_2i (0x2),
  pow10_of_2i (0x1),
};

struct float_parser_t
{
  float_parser_t () : part (INT_PART), value (0), neg (false), int_part (0),
		      frac_part (0), frac_count (0), exp_neg (0), exp_part (0),
		      exp_overflow (false), i (0), in_error (false), exp_start (true) {}

  bool is_in_error ()  { return in_error; }
  void set_in_error () { in_error = true; }

  bool consume (char c)
  {
    switch (c)
    {
    case '-':
      if (i == 0) neg = true;
      else if (exp_start) exp_neg = true;
      else in_error = true;
      break;

    case '+':
      if (i != 0 || !exp_start) in_error = true;
      break;

    case '.':
      if (part != INT_PART) in_error = true;
      part = FRAC_PART;
      break;

    case 'e':
    case 'E':
      if (part == EXP_PART) in_error = true;
      part = EXP_PART;
      exp_start = true;
      break;

    default:
      if (c < '0' || c > '9')
	/* Return gracefully as strtod guarantee */
	return false;

      unsigned d = c - '0';

      switch (part) {
      default:
      case INT_PART:
	int_part = (int_part * 10) + d;
	break;

      case FRAC_PART:
	if (likely (frac_part <= MAX_FRACT / 10))
	{
	  frac_part = (frac_part * 10) + d;
	  frac_count++;
	}
	break;

      case EXP_PART:
	if (likely (exp_part * 10 + d <= MAX_EXP))
	  exp_part = (exp_part * 10) + d;
	else
	  exp_overflow = true;
	break;
      }
      break;
    }

    /* if the current input isn't 'e', we have passed exp start already */
    if (c != 'e') exp_start = false;
    return !in_error;
  }

  double end ()
  {
    if (in_error) return .0;
    value = neg ? -int_part : int_part;
    if (frac_count > 0)
    {
      double frac = frac_part / pow10 (frac_count);
      if (neg) frac = -frac;
	value += frac;
    }
    if (unlikely (exp_overflow))
    {
      if (value == .0)
	return value;
      if (exp_neg)
	return neg ? -DBL_MIN : DBL_MIN;
      else
	return neg ? -DBL_MAX : DBL_MAX;
    }
    if (exp_part != 0)
    {
      if (exp_neg)
	value /= pow10 (exp_part);
      else
	value *= pow10 (exp_part);
    }
    return value;
  }

  private:

  const uint64_t MAX_FRACT = 0xFFFFFFFFFFFFFull; /* 1^52-1 */
  const uint32_t MAX_EXP = 0x7FFu; /* 1^11-1 */

  enum Part { INT_PART=0, FRAC_PART, EXP_PART };

  /* Works for x < 512 */
  double
  pow10 (unsigned int x)
  {
    unsigned int mask = 0x100; /* Should be same with the first element  */
    unsigned long result = 1;
    const double *power = powers_of_10;
    for (; mask; ++power, mask >>= 1) if (mask & x) result *= *power;
    return result;
  }

  char part;
  double value;
  bool neg;
  double int_part;
  uint64_t frac_part;
  uint32_t frac_count;
  bool exp_neg;
  uint32_t exp_part;
  bool exp_overflow;
  unsigned int i;
  bool in_error;
  bool exp_start;
};

#endif /* HB_NUMBER_PARSER_HH */
