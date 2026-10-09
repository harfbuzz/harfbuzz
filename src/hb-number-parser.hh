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

#ifndef HB_NUMBER_PARSER_HH
#define HB_NUMBER_PARSER_HH

#include "hb.hh"

// Works only for n < 512
static inline double
_pow10 (unsigned exponent)
{
  // DBL_MAX is between 10^308 and 10^309
  if (unlikely (exponent > 308)) return HUGE_VAL;

  static const double _powers_of_10[] =
  {
    1.0e+256,
    1.0e+128,
    1.0e+64,
    1.0e+32,
    1.0e+16,
    1.0e+8,
    10000.,
    100.,
    10.
  };
  unsigned mask = 1 << (ARRAY_LENGTH (_powers_of_10) - 1);
  double result = 1;
  for (const double *power = _powers_of_10; mask; ++power, mask >>= 1)
    if (exponent & mask) result *= *power;
  return result;
}

// A variant of strtod that also gets end of buffer in its second argument.
//
// Grammar (with backtracking on partial exponents):
//
//   number  := sign? mantissa exponent?
//   sign    := '+' | '-'
//   mantissa:= digits ('.' digits?)?
//	    | '.' digits
//   exponent:= ('e'|'E') sign? digits
//
// On success, *end_ptr is set just past the longest well-formed prefix.
// On no match, *end_ptr is left at the original p (matching strtod's
// "no conversion performed" behavior, so the caller's pointer is unchanged).
static inline double
hb_strtod (const char *p, const char **end_ptr /* IN/OUT */)
{
  double value = 0;
  double frac = 0;
  double frac_count = 0;
  unsigned exp = 0;
  bool neg = false, exp_neg = false, exp_overflow = false;
  int frac_drop = -1;
  bool frac_sticky = false;
  const unsigned long long MAX_FRACT = 0xFFFFFFFFFFFFFull; /* 2^52-1 */
  const unsigned MAX_EXP = 0x7FFu; /* 2^11-1 */

  const char *p_original = p;
  const char *pe = *end_ptr;

  // Skip leading whitespace
  while (p < pe && ISSPACE (*p))
    p++;

  // Sign
  if (p < pe && (*p == '+' || *p == '-'))
  {
    neg = (*p == '-');
    p++;
  }

  // Mantissa.
  const char *mantissa_end = nullptr;

  if (p < pe && '0' <= *p && *p <= '9')
  {
    // Integer part
    do
    {
      value = value * 10. + (*p - '0');
      p++;
    } while (p < pe && '0' <= *p && *p <= '9');
    mantissa_end = p;

    // Optional fractional part; a trailing '.' with no digits is accepted
    if (p < pe && *p == '.')
    {
      p++;
      while (p < pe && '0' <= *p && *p <= '9')
      {
	if (likely (frac <= MAX_FRACT / 10))
	{
	  frac = frac * 10. + (*p - '0');
	  ++frac_count;
	}
	// Record the tail we couldn't fit for round-to-nearest below
	else if (frac_drop < 0) frac_drop = *p - '0';
	else if (*p != '0') frac_sticky = true;
	p++;
      }
      mantissa_end = p;
    }
  }
  else if (p < pe && *p == '.')
  {
    // Fractional-only form: '.' must be followed by at least one digit
    p++;
    if (p < pe && '0' <= *p && *p <= '9')
    {
      do
      {
	if (likely (frac <= MAX_FRACT / 10))
	{
	  frac = frac * 10. + (*p - '0');
	  ++frac_count;
	}
	else if (frac_drop < 0) frac_drop = *p - '0';
	else if (*p != '0') frac_sticky = true;
	p++;
      } while (p < pe && '0' <= *p && *p <= '9');
      mantissa_end = p;
    }
    else
    {
      *end_ptr = p_original;
      return 0.0;
    }
  }
  else
  {
    // No digits: no subject sequence
    *end_ptr = p_original;
    return 0.0;
  }

  // Exponent. Backtracking is implicit: end_ptr follows mantissa_end,
  // which is only updated when the exponent is well-formed.
  if (p < pe && (*p == 'e' || *p == 'E'))
  {
    p++;
    if (p < pe && (*p == '+' || *p == '-'))
    {
      exp_neg = (*p == '-');
      p++;
    }
    if (p < pe && '0' <= *p && *p <= '9')
    {
      do
      {
	if (likely (exp * 10 + (*p - '0') <= MAX_EXP))
	  exp = exp * 10 + (*p - '0');
	else
	  exp_overflow = true;
	p++;
      } while (p < pe && '0' <= *p && *p <= '9');
      mantissa_end = p;
    }
    // else: exponent malformed; end_ptr stays at end of mantissa
  }

  *end_ptr = mantissa_end;

  // Apply round-to-nearest-even using the dropped tail recorded above
  if (frac_drop > 5 ||
      (frac_drop == 5 && (frac_sticky || (((uint64_t) frac) & 1))))
    frac += 1;
  if (frac_count) value += frac / _pow10 ((unsigned) frac_count);
  if (neg) value *= -1.;

  if (unlikely (exp_overflow))
  {
    if (value == 0) return value;
    if (exp_neg)    return neg ? -0.0 : 0.0;
    else            return neg ? -DBL_MAX : DBL_MAX;
  }

  if (exp)
  {
    if (exp_neg)
    {
      // 10^308 is the largest finite power of ten, so two reductions of 308 suffice.
      if (unlikely (exp > 308))
      {
	value /= 1e308;
	exp -= 308;
	if (unlikely (exp > 308))
	{
	  value /= 1e308;
	  exp -= 308;
	}
      }
      value /= _pow10 (exp);
    }
    else
      value *= _pow10 (exp);
  }

  return value;
}

#endif /* HB_NUMBER_PARSER_HH */
