
#line 1 "hb-ot-shaper-khmer-machine.rl"
/*
 * Copyright © 2011,2012  Google, Inc.
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

#ifndef HB_OT_SHAPER_KHMER_MACHINE_HH
#define HB_OT_SHAPER_KHMER_MACHINE_HH

#include "hb.hh"

#include "hb-ot-layout.hh"
#include "hb-ot-shaper-indic.hh"

/* buffer var allocations */
#define khmer_category() ot_shaper_var_u8_category() /* khmer_category_t */

using khmer_category_t = unsigned;

#define K_Cat(Cat) khmer_syllable_machine_ex_##Cat

enum khmer_syllable_type_t {
  khmer_consonant_syllable,
  khmer_broken_cluster,
  khmer_non_khmer_cluster,
};


#line 52 "hb-ot-shaper-khmer-machine.hh"
#define khmer_syllable_machine_ex_C 1u
#define khmer_syllable_machine_ex_DOTTEDCIRCLE 11u
#define khmer_syllable_machine_ex_H 4u
#define khmer_syllable_machine_ex_PLACEHOLDER 10u
#define khmer_syllable_machine_ex_Ra 15u
#define khmer_syllable_machine_ex_Robatic 25u
#define khmer_syllable_machine_ex_V 2u
#define khmer_syllable_machine_ex_VAbv 20u
#define khmer_syllable_machine_ex_VBlw 21u
#define khmer_syllable_machine_ex_VPre 22u
#define khmer_syllable_machine_ex_VPst 23u
#define khmer_syllable_machine_ex_Xgroup 26u
#define khmer_syllable_machine_ex_Ygroup 27u
#define khmer_syllable_machine_ex_ZWJ 6u
#define khmer_syllable_machine_ex_ZWNJ 5u


#line 70 "hb-ot-shaper-khmer-machine.hh"
static const unsigned char _khmer_syllable_machine_trans_keys[] = {
	5u, 26u, 5u, 26u, 1u, 15u, 5u, 26u, 5u, 26u, 5u, 26u, 5u, 26u, 5u, 26u, 
	5u, 26u, 5u, 26u, 5u, 26u, 5u, 26u, 1u, 15u, 5u, 26u, 5u, 26u, 5u, 26u, 
	5u, 26u, 5u, 26u, 5u, 26u, 5u, 26u, 1u, 27u, 4u, 27u, 1u, 15u, 4u, 27u, 
	27u, 27u, 4u, 27u, 4u, 27u, 4u, 27u, 4u, 27u, 4u, 27u, 1u, 15u, 4u, 27u, 
	4u, 27u, 27u, 27u, 4u, 27u, 4u, 27u, 4u, 27u, 4u, 27u, 4u, 27u, 5u, 26u, 
	0
};

static const char _khmer_syllable_machine_key_spans[] = {
	22, 22, 15, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 15, 22, 22, 22, 
	22, 22, 22, 22, 27, 24, 15, 24, 
	1, 24, 24, 24, 24, 24, 15, 24, 
	24, 1, 24, 24, 24, 24, 24, 22
};

static const short _khmer_syllable_machine_index_offsets[] = {
	0, 23, 46, 62, 85, 108, 131, 154, 
	177, 200, 223, 246, 269, 285, 308, 331, 
	354, 377, 400, 423, 446, 474, 499, 515, 
	540, 542, 567, 592, 617, 642, 667, 683, 
	708, 733, 735, 760, 785, 810, 835, 860
};

static const char _khmer_syllable_machine_indicies[] = {
	1, 1, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 2, 
	0, 0, 0, 0, 3, 4, 0, 1, 
	1, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 4, 0, 5, 5, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 5, 0, 1, 1, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 2, 0, 0, 
	0, 0, 0, 4, 0, 6, 6, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 2, 0, 7, 7, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 8, 0, 9, 9, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 2, 0, 0, 0, 0, 0, 
	10, 0, 9, 9, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 10, 
	0, 11, 11, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	2, 0, 0, 0, 0, 0, 12, 0, 
	11, 11, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 12, 0, 14, 
	14, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 15, 13, 
	13, 13, 13, 16, 17, 13, 14, 14, 
	18, 18, 18, 18, 18, 18, 18, 18, 
	18, 18, 18, 18, 18, 18, 18, 18, 
	18, 18, 18, 17, 18, 19, 19, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 19, 13, 14, 14, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 15, 13, 13, 13, 
	13, 13, 17, 13, 20, 20, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 
	13, 15, 13, 21, 21, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 
	22, 13, 23, 23, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 
	13, 15, 13, 13, 13, 13, 13, 24, 
	13, 23, 23, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 24, 13, 
	25, 25, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 15, 
	13, 13, 13, 13, 13, 26, 13, 25, 
	25, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 26, 13, 28, 28, 
	27, 29, 30, 30, 27, 27, 27, 3, 
	3, 27, 27, 27, 28, 27, 27, 27, 
	27, 15, 24, 26, 22, 27, 27, 17, 
	19, 27, 32, 33, 33, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 2, 10, 12, 8, 31, 3, 
	4, 5, 31, 28, 28, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 28, 31, 34, 35, 35, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 31, 2, 10, 12, 8, 31, 
	31, 4, 5, 31, 5, 31, 34, 6, 
	6, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 8, 31, 31, 2, 5, 31, 34, 
	7, 7, 31, 31, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 8, 5, 31, 
	34, 36, 36, 31, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	2, 31, 31, 8, 31, 31, 10, 5, 
	31, 34, 37, 37, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 2, 10, 31, 8, 31, 31, 12, 
	5, 31, 32, 35, 35, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 2, 10, 12, 8, 31, 31, 
	4, 5, 31, 39, 39, 38, 38, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 39, 38, 29, 40, 40, 38, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 38, 38, 15, 24, 26, 22, 38, 
	16, 17, 19, 38, 41, 42, 42, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 38, 38, 38, 15, 24, 26, 22, 
	38, 38, 17, 19, 38, 19, 38, 41, 
	20, 20, 38, 38, 38, 38, 38, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 38, 22, 38, 38, 15, 19, 38, 
	41, 21, 21, 38, 38, 38, 38, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 38, 38, 38, 38, 38, 22, 19, 
	38, 41, 43, 43, 38, 38, 38, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 15, 38, 38, 22, 38, 38, 24, 
	19, 38, 41, 44, 44, 38, 38, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 38, 15, 24, 38, 22, 38, 38, 
	26, 19, 38, 29, 42, 42, 38, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 38, 38, 15, 24, 26, 22, 38, 
	38, 17, 19, 38, 14, 14, 45, 45, 
	45, 45, 45, 45, 45, 45, 45, 45, 
	45, 45, 45, 15, 45, 45, 45, 45, 
	45, 17, 45, 0
};

static const char _khmer_syllable_machine_trans_targs[] = {
	20, 1, 25, 29, 23, 24, 4, 5, 
	26, 7, 27, 9, 28, 20, 11, 34, 
	38, 32, 20, 33, 14, 15, 35, 17, 
	36, 19, 37, 20, 21, 30, 39, 20, 
	22, 0, 2, 3, 6, 8, 20, 31, 
	10, 12, 13, 16, 18, 20
};

static const char _khmer_syllable_machine_trans_actions[] = {
	1, 0, 2, 2, 2, 0, 0, 0, 
	2, 0, 2, 0, 2, 3, 0, 2, 
	4, 4, 5, 0, 0, 0, 2, 0, 
	2, 0, 2, 8, 2, 0, 9, 10, 
	0, 0, 0, 0, 0, 0, 11, 4, 
	0, 0, 0, 0, 0, 12
};

static const char _khmer_syllable_machine_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 6, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0
};

static const char _khmer_syllable_machine_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 7, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0
};

static const unsigned char _khmer_syllable_machine_eof_trans[] = {
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 14, 19, 14, 14, 14, 14, 
	14, 14, 14, 14, 0, 32, 32, 32, 
	32, 32, 32, 32, 32, 32, 39, 39, 
	39, 39, 39, 39, 39, 39, 39, 46
};

static const int khmer_syllable_machine_start = 20;
static const int khmer_syllable_machine_first_final = 20;
static const int khmer_syllable_machine_error = -1;

static const int khmer_syllable_machine_en_main = 20;


#line 53 "hb-ot-shaper-khmer-machine.rl"



#line 102 "hb-ot-shaper-khmer-machine.rl"


#define found_syllable(syllable_type) \
  HB_STMT_START { \
    if (0) fprintf (stderr, "syllable %d..%d %s\n", ts, te, #syllable_type); \
    for (unsigned int i = ts; i < te; i++) \
      info[i].syllable() = (syllable_serial << 4) | syllable_type; \
    syllable_serial++; \
    if (unlikely (syllable_serial == 16)) syllable_serial = 1; \
  } HB_STMT_END

inline void
find_syllables_khmer (hb_buffer_t *buffer)
{
  unsigned int p, pe, eof, ts, te, act HB_UNUSED;
  int cs;
  hb_glyph_info_t *info = buffer->info;
  
#line 282 "hb-ot-shaper-khmer-machine.hh"
	{
	cs = khmer_syllable_machine_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 122 "hb-ot-shaper-khmer-machine.rl"


  p = 0;
  pe = eof = buffer->len;

  unsigned int syllable_serial = 1;
  
#line 298 "hb-ot-shaper-khmer-machine.hh"
	{
	int _slen;
	int _trans;
	const unsigned char *_keys;
	const char *_inds;
	if ( p == pe )
		goto _test_eof;
_resume:
	switch ( _khmer_syllable_machine_from_state_actions[cs] ) {
	case 7:
#line 1 "NONE"
	{ts = p;}
	break;
#line 312 "hb-ot-shaper-khmer-machine.hh"
	}

	_keys = _khmer_syllable_machine_trans_keys + (cs<<1);
	_inds = _khmer_syllable_machine_indicies + _khmer_syllable_machine_index_offsets[cs];

	_slen = _khmer_syllable_machine_key_spans[cs];
	_trans = _inds[ _slen > 0 && _keys[0] <=( info[p].khmer_category()) &&
		( info[p].khmer_category()) <= _keys[1] ?
		( info[p].khmer_category()) - _keys[0] : _slen ];

_eof_trans:
	cs = _khmer_syllable_machine_trans_targs[_trans];

	if ( _khmer_syllable_machine_trans_actions[_trans] == 0 )
		goto _again;

	switch ( _khmer_syllable_machine_trans_actions[_trans] ) {
	case 2:
#line 1 "NONE"
	{te = p+1;}
	break;
	case 8:
#line 98 "hb-ot-shaper-khmer-machine.rl"
	{te = p+1;{ found_syllable (khmer_non_khmer_cluster); }}
	break;
	case 10:
#line 96 "hb-ot-shaper-khmer-machine.rl"
	{te = p;p--;{ found_syllable (khmer_consonant_syllable); }}
	break;
	case 11:
#line 97 "hb-ot-shaper-khmer-machine.rl"
	{te = p;p--;{ found_syllable (khmer_broken_cluster); buffer->scratch_flags |= HB_BUFFER_SCRATCH_FLAG_HAS_BROKEN_SYLLABLE; }}
	break;
	case 12:
#line 98 "hb-ot-shaper-khmer-machine.rl"
	{te = p;p--;{ found_syllable (khmer_non_khmer_cluster); }}
	break;
	case 1:
#line 96 "hb-ot-shaper-khmer-machine.rl"
	{{p = ((te))-1;}{ found_syllable (khmer_consonant_syllable); }}
	break;
	case 3:
#line 97 "hb-ot-shaper-khmer-machine.rl"
	{{p = ((te))-1;}{ found_syllable (khmer_broken_cluster); buffer->scratch_flags |= HB_BUFFER_SCRATCH_FLAG_HAS_BROKEN_SYLLABLE; }}
	break;
	case 5:
#line 1 "NONE"
	{	switch( act ) {
	case 2:
	{{p = ((te))-1;} found_syllable (khmer_broken_cluster); buffer->scratch_flags |= HB_BUFFER_SCRATCH_FLAG_HAS_BROKEN_SYLLABLE; }
	break;
	case 3:
	{{p = ((te))-1;} found_syllable (khmer_non_khmer_cluster); }
	break;
	}
	}
	break;
	case 4:
#line 1 "NONE"
	{te = p+1;}
#line 97 "hb-ot-shaper-khmer-machine.rl"
	{act = 2;}
	break;
	case 9:
#line 1 "NONE"
	{te = p+1;}
#line 98 "hb-ot-shaper-khmer-machine.rl"
	{act = 3;}
	break;
#line 382 "hb-ot-shaper-khmer-machine.hh"
	}

_again:
	switch ( _khmer_syllable_machine_to_state_actions[cs] ) {
	case 6:
#line 1 "NONE"
	{ts = 0;}
	break;
#line 391 "hb-ot-shaper-khmer-machine.hh"
	}

	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _khmer_syllable_machine_eof_trans[cs] > 0 ) {
		_trans = _khmer_syllable_machine_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	}

#line 130 "hb-ot-shaper-khmer-machine.rl"

}

#undef found_syllable

#endif /* HB_OT_SHAPER_KHMER_MACHINE_HH */
