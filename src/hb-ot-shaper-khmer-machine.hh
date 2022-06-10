
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
#define khmer_category() indic_category() /* khmer_category_t */

using khmer_category_t = ot_category_t;

#define K_Cat(Cat) khmer_syllable_machine_ex_##Cat

enum khmer_syllable_type_t {
  khmer_consonant_syllable,
  khmer_broken_cluster,
  khmer_non_khmer_cluster,
};


#line 52 "hb-ot-shaper-khmer-machine.hh"
#define khmer_syllable_machine_ex_C 1u
#define khmer_syllable_machine_ex_Coeng 24u
#define khmer_syllable_machine_ex_DOTTEDCIRCLE 11u
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
	5u, 26u, 5u, 26u, 5u, 26u, 5u, 26u, 5u, 26u, 1u, 15u, 5u, 26u, 5u, 26u, 
	5u, 26u, 5u, 26u, 5u, 26u, 5u, 26u, 5u, 26u, 5u, 26u, 1u, 15u, 5u, 26u, 
	5u, 26u, 5u, 26u, 5u, 26u, 5u, 26u, 1u, 27u, 5u, 27u, 5u, 27u, 5u, 27u, 
	5u, 27u, 27u, 27u, 5u, 27u, 5u, 27u, 5u, 27u, 1u, 15u, 5u, 26u, 5u, 27u, 
	5u, 27u, 5u, 27u, 27u, 27u, 5u, 27u, 5u, 27u, 1u, 15u, 5u, 27u, 5u, 27u, 
	0
};

static const char _khmer_syllable_machine_key_spans[] = {
	22, 22, 22, 22, 22, 15, 22, 22, 
	22, 22, 22, 22, 22, 22, 15, 22, 
	22, 22, 22, 22, 27, 23, 23, 23, 
	23, 1, 23, 23, 23, 15, 22, 23, 
	23, 23, 1, 23, 23, 15, 23, 23
};

static const short _khmer_syllable_machine_index_offsets[] = {
	0, 23, 46, 69, 92, 115, 131, 154, 
	177, 200, 223, 246, 269, 292, 315, 331, 
	354, 377, 400, 423, 446, 474, 498, 522, 
	546, 570, 572, 596, 620, 644, 660, 683, 
	707, 731, 755, 757, 781, 805, 821, 845
};

static const char _khmer_syllable_machine_indicies[] = {
	1, 1, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 2, 
	0, 0, 0, 0, 3, 4, 0, 1, 
	1, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 4, 0, 1, 1, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 2, 0, 0, 
	0, 0, 0, 4, 0, 5, 5, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 2, 0, 6, 6, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 7, 0, 8, 8, 0, 0, 0, 
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
	13, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 15, 13, 14, 14, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 17, 16, 16, 
	16, 16, 16, 15, 16, 18, 18, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 17, 16, 19, 19, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 20, 16, 21, 21, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 21, 16, 22, 22, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 17, 16, 16, 16, 16, 16, 
	23, 16, 22, 22, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 23, 
	16, 24, 24, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	17, 16, 16, 16, 16, 16, 25, 16, 
	24, 24, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 25, 16, 14, 
	14, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 17, 16, 
	16, 16, 16, 26, 15, 16, 28, 28, 
	27, 27, 29, 29, 27, 27, 27, 3, 
	3, 27, 27, 27, 28, 27, 27, 27, 
	27, 17, 23, 25, 20, 30, 27, 15, 
	21, 27, 32, 32, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 2, 10, 12, 7, 33, 3, 4, 
	8, 31, 34, 34, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 2, 10, 12, 7, 35, 31, 4, 
	8, 31, 5, 5, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 31, 31, 7, 35, 31, 2, 
	8, 31, 6, 6, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 35, 31, 7, 
	8, 31, 8, 31, 36, 36, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 31, 2, 31, 31, 7, 35, 
	31, 10, 8, 31, 37, 37, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 31, 2, 10, 31, 7, 35, 
	31, 12, 8, 31, 34, 34, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 31, 2, 10, 12, 7, 33, 
	31, 4, 8, 31, 28, 28, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 28, 31, 14, 14, 38, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 38, 38, 17, 38, 38, 38, 38, 
	38, 15, 38, 40, 40, 39, 39, 39, 
	39, 39, 39, 39, 39, 39, 39, 39, 
	39, 39, 17, 23, 25, 20, 41, 39, 
	15, 21, 39, 18, 18, 39, 39, 39, 
	39, 39, 39, 39, 39, 39, 39, 39, 
	39, 39, 39, 39, 39, 20, 41, 39, 
	17, 21, 39, 19, 19, 39, 39, 39, 
	39, 39, 39, 39, 39, 39, 39, 39, 
	39, 39, 39, 39, 39, 39, 41, 39, 
	20, 21, 39, 21, 39, 42, 42, 39, 
	39, 39, 39, 39, 39, 39, 39, 39, 
	39, 39, 39, 39, 17, 39, 39, 20, 
	41, 39, 23, 21, 39, 43, 43, 39, 
	39, 39, 39, 39, 39, 39, 39, 39, 
	39, 39, 39, 39, 17, 23, 39, 20, 
	41, 39, 25, 21, 39, 44, 44, 39, 
	39, 39, 39, 39, 39, 39, 39, 39, 
	39, 39, 39, 44, 39, 45, 45, 39, 
	39, 39, 39, 39, 39, 39, 39, 39, 
	39, 39, 39, 39, 17, 23, 25, 20, 
	30, 26, 15, 21, 39, 40, 40, 39, 
	39, 39, 39, 39, 39, 39, 39, 39, 
	39, 39, 39, 39, 17, 23, 25, 20, 
	30, 39, 15, 21, 39, 0
};

static const char _khmer_syllable_machine_trans_targs[] = {
	20, 1, 23, 28, 22, 3, 4, 24, 
	25, 7, 26, 9, 27, 20, 10, 31, 
	20, 32, 12, 13, 33, 34, 16, 35, 
	18, 36, 39, 20, 21, 30, 37, 20, 
	0, 29, 2, 5, 6, 8, 20, 20, 
	11, 14, 15, 17, 38, 19
};

static const char _khmer_syllable_machine_trans_actions[] = {
	1, 0, 2, 2, 2, 0, 0, 2, 
	0, 0, 2, 0, 2, 3, 0, 4, 
	5, 2, 0, 0, 2, 0, 0, 2, 
	0, 2, 4, 8, 2, 9, 0, 10, 
	0, 0, 0, 0, 0, 0, 11, 12, 
	0, 0, 0, 0, 4, 0
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
	1, 1, 14, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 0, 32, 32, 32, 
	32, 32, 32, 32, 32, 32, 39, 40, 
	40, 40, 40, 40, 40, 40, 40, 40
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

static void
find_syllables_khmer (hb_buffer_t *buffer)
{
  unsigned int p, pe, eof, ts, te, act HB_UNUSED;
  int cs;
  hb_glyph_info_t *info = buffer->info;
  
#line 280 "hb-ot-shaper-khmer-machine.hh"
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
  
#line 296 "hb-ot-shaper-khmer-machine.hh"
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
#line 310 "hb-ot-shaper-khmer-machine.hh"
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
	case 12:
#line 97 "hb-ot-shaper-khmer-machine.rl"
	{te = p;p--;{ found_syllable (khmer_broken_cluster); buffer->scratch_flags |= HB_BUFFER_SCRATCH_FLAG_HAS_BROKEN_SYLLABLE; }}
	break;
	case 11:
#line 98 "hb-ot-shaper-khmer-machine.rl"
	{te = p;p--;{ found_syllable (khmer_non_khmer_cluster); }}
	break;
	case 1:
#line 96 "hb-ot-shaper-khmer-machine.rl"
	{{p = ((te))-1;}{ found_syllable (khmer_consonant_syllable); }}
	break;
	case 5:
#line 97 "hb-ot-shaper-khmer-machine.rl"
	{{p = ((te))-1;}{ found_syllable (khmer_broken_cluster); buffer->scratch_flags |= HB_BUFFER_SCRATCH_FLAG_HAS_BROKEN_SYLLABLE; }}
	break;
	case 3:
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
#line 380 "hb-ot-shaper-khmer-machine.hh"
	}

_again:
	switch ( _khmer_syllable_machine_to_state_actions[cs] ) {
	case 6:
#line 1 "NONE"
	{ts = 0;}
	break;
#line 389 "hb-ot-shaper-khmer-machine.hh"
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
