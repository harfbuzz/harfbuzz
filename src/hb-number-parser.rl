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

%%{

machine double_parser;
alphtype unsigned char;
write data;

action see_neg { neg = true; }
action see_exp_neg { exp_neg = true; }

action add_int  { value = value * 10. + (fc - '0'); }
action add_frac { frac = frac * 10. + (fc - '0'); ++frac_count; }
action add_exp  { exp = exp * 10 + (fc - '0'); }

num = [0-9]+;

main := (
	(
		(('+'|'-'@see_neg)? num @add_int) ('.' num @add_frac)?
		|
		(('+'|'-'@see_neg)? '.' num @add_frac)
	)
	(('e'|'E') (('+'|'-'@see_exp_neg)? num @add_exp))?
);

}%%

constexpr double _pow2 (double x) { return x * x; }
constexpr double _pow10_of_2i (unsigned int n)
{ return n == 1 ? 10. : _pow2 (_pow10_of_2i (n >> 1)); }

static const double _powers_of_10[] =
{
  _pow10_of_2i (0x100),
  _pow10_of_2i (0x80),
  _pow10_of_2i (0x40),
  _pow10_of_2i (0x20),
  _pow10_of_2i (0x10),
  _pow10_of_2i (0x8),
  _pow10_of_2i (0x4),
  _pow10_of_2i (0x2),
  _pow10_of_2i (0x1),
};

/* Works only for n < 512 */
inline double
_pow10 (unsigned int exponent)
{
  unsigned int mask = 0x100; /* Should be same with the first element  */
  double result = 1;
  for (const double *power = _powers_of_10; mask; ++power, mask >>= 1)
    if (exponent & mask) result *= *power;
  return result;
}

inline double
strtod_rl (const char *buf, char **end_ptr)
{
  const char *p, *pe;
  double value = 0;
  double frac = 0;
  double frac_count = 0;
  unsigned int exp = 0;
  bool neg = false, exp_neg = false;
  p = buf;
  pe = p + strlen (p);

  while (p < pe && ISSPACE (*p))
    p++;

  int cs;
  %%{
    write init;
    write exec;
  }%%

  *end_ptr = (char *) p;

  if (frac_count) value += frac / _pow10 (frac_count);
  if (neg) value *= -1.;

  if (exp)
  {
    if (exp_neg)
      value /= _pow10 (exp);
    else
      value *= _pow10 (exp);
  }

  return value;
}

#endif /* HB_NUMBER_PARSER_HH */
