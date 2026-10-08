
#line 1 "hb-number-parser.rl"
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


#line 35 "hb-number-parser.hh"
static const unsigned char _double_parser_trans_keys[] = {
	0u, 0u, 46u, 57u, 48u, 57u, 43u, 57u, 48u, 57u, 43u, 57u, 48u, 101u, 48u, 57u, 
	46u, 101u, 0
};

static const char _double_parser_key_spans[] = {
	0, 12, 10, 15, 10, 15, 54, 10, 
	56
};

static const unsigned char _double_parser_index_offsets[] = {
	0, 0, 13, 24, 40, 51, 67, 122, 
	133
};

static const char _double_parser_indicies[] = {
	0, 1, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 1, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	1, 5, 4, 6, 4, 4, 7, 7, 
	7, 7, 7, 7, 7, 7, 7, 7, 
	4, 7, 7, 7, 7, 7, 7, 7, 
	7, 7, 7, 4, 8, 1, 9, 0, 
	1, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 1, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 11, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 11, 10, 7, 7, 7, 7, 7, 
	7, 7, 7, 7, 7, 10, 12, 10, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 11, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 11, 10, 0
};

static const char _double_parser_trans_targs[] = {
	2, 0, 8, 6, 5, 4, 4, 7, 
	1, 1, 5, 3, 6
};

static const char _double_parser_trans_actions[] = {
	0, 0, 1, 2, 3, 0, 4, 5, 
	0, 8, 9, 0, 10
};

static const char _double_parser_to_state_actions[] = {
	0, 0, 0, 0, 0, 6, 0, 0, 
	0
};

static const char _double_parser_from_state_actions[] = {
	0, 0, 0, 0, 0, 7, 0, 0, 
	0
};

static const char _double_parser_eof_trans[] = {
	0, 0, 0, 5, 5, 0, 11, 11, 
	11
};

static const int double_parser_start = 5;
static const int double_parser_first_final = 5;
static const int double_parser_error = 0;

static const int double_parser_en_main = 5;


#line 76 "hb-number-parser.rl"


/* Works only for n < 512 */
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

/* a variant of strtod that also gets end of buffer in its second argument */
static inline double
strtod_rl (const char *p, const char **end_ptr /* IN/OUT */)
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
  while (p < pe && ISSPACE (*p))
    p++;
  const char *p_start = p;

  int cs;
  const char *ts = p;
  const char *te = p;
  int act = 0;
  const char *eof = pe;
  bool matched = false;
  (void) act;
  
#line 167 "hb-number-parser.hh"
	{
	cs = double_parser_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 175 "hb-number-parser.hh"
	{
	int _slen;
	int _trans;
	const unsigned char *_keys;
	const char *_inds;
	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	switch ( _double_parser_from_state_actions[cs] ) {
	case 7:
#line 1 "NONE"
	{ts = p;}
	break;
#line 191 "hb-number-parser.hh"
	}

	_keys = _double_parser_trans_keys + (cs<<1);
	_inds = _double_parser_indicies + _double_parser_index_offsets[cs];

	_slen = _double_parser_key_spans[cs];
	_trans = _inds[ _slen > 0 && _keys[0] <=(*p) &&
		(*p) <= _keys[1] ?
		(*p) - _keys[0] : _slen ];

_eof_trans:
	cs = _double_parser_trans_targs[_trans];

	if ( _double_parser_trans_actions[_trans] == 0 )
		goto _again;

	switch ( _double_parser_trans_actions[_trans] ) {
	case 8:
#line 37 "hb-number-parser.rl"
	{ neg = true; }
	break;
	case 4:
#line 38 "hb-number-parser.rl"
	{ exp_neg = true; }
	break;
	case 5:
#line 52 "hb-number-parser.rl"
	{
	if (likely (exp * 10 + ((*p) - '0') <= MAX_EXP))
	  exp = exp * 10 + ((*p) - '0');
	else
	  exp_overflow = true;
}
	break;
	case 10:
#line 1 "NONE"
	{te = p+1;}
	break;
	case 9:
#line 61 "hb-number-parser.rl"
	{te = p;p--;{
	matched = true;
	{p++; goto _out; }
}}
	break;
	case 3:
#line 61 "hb-number-parser.rl"
	{{p = ((te))-1;}{
	matched = true;
	{p++; goto _out; }
}}
	break;
	case 1:
#line 1 "NONE"
	{te = p+1;}
#line 40 "hb-number-parser.rl"
	{
	value = value * 10. + ((*p) - '0');
}
	break;
	case 2:
#line 1 "NONE"
	{te = p+1;}
#line 43 "hb-number-parser.rl"
	{
	if (likely (frac <= MAX_FRACT / 10))
	{
	  frac = frac * 10. + ((*p) - '0');
	  ++frac_count;
	}
	// Record the tail we couldn't fit for round-to-nearest below
	else if (frac_drop < 0) frac_drop = (*p) - '0'; else if ((*p) != '0') frac_sticky = true;
}
	break;
#line 266 "hb-number-parser.hh"
	}

_again:
	switch ( _double_parser_to_state_actions[cs] ) {
	case 6:
#line 1 "NONE"
	{ts = 0;}
	break;
#line 275 "hb-number-parser.hh"
	}

	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _double_parser_eof_trans[cs] > 0 ) {
		_trans = _double_parser_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	_out: {}
	}

#line 134 "hb-number-parser.rl"


  // end_ptr = end of match on success, else the original p
  *end_ptr = (matched && ts == p_start) ? p : p_original;

  // Apply round-to-nearest-even using the dropped tail recorded above
  if (frac_drop > 5 ||
      (frac_drop == 5 && (frac_sticky || (((uint64_t) frac) & 1))))
    frac += 1;
  if (frac_count) value += frac / _pow10 (frac_count);
  if (neg) value *= -1.;

  if (unlikely (exp_overflow))
  {
    if (value == 0) return value;
    if (exp_neg)    return neg ? -DBL_MIN : DBL_MIN;
    else            return neg ? -DBL_MAX : DBL_MAX;
  }

  if (exp)
  {
    if (exp_neg) value /= _pow10 (exp);
    else         value *= _pow10 (exp);
  }

  return value;
}

#endif /* HB_NUMBER_PARSER_HH */
