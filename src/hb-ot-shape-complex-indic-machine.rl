/*
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_SHAPE_COMPLEX_INDIC_MACHINE_HH
#define HB_OT_SHAPE_COMPLEX_INDIC_MACHINE_HH

#include "hb-private.hh"

HB_BEGIN_DECLS

%%{
  machine indic_syllable_machine;
  alphtype unsigned char;
  write data;
}%%

%%{

# Same order as enum indic_category_t.  Not sure how to avoid duplication.
X    = 0;
C    = 1;
Ra   = 2;
V    = 3;
N    = 4;
H    = 5;
ZWNJ = 6;
ZWJ  = 7;
M    = 8;
SM   = 9;
VD   = 10;
A    = 11;
NBSP = 12;

c = C | Ra;
z = ZWJ|ZWNJ;
matra_group = M N? H?;
syllable_tail = SM? (VD VD?)?;

action matched_syllable {
  matched_syllable (c, last, p);
  last = p;
}

consonant_syllable =	(c.N? (z.H|H.z?))* c.N? A? (H.z? | matra_group*)? syllable_tail %(matched_syllable);
vowel_syllable =	(Ra H)? V N? (z.H.c | ZWJ.c)? matra_group* syllable_tail;
standalone_cluster =	(Ra H)? NBSP N? (z? H c)? matra_group* syllable_tail;
non_indic = X;

syllable =
	  consonant_syllable
	| vowel_syllable
	| standalone_cluster
	| non_indic
	;

main := syllable**;

}%%


#include <stdio.h>
#include <string.h>

static void
find_syllables (hb_ot_shape_context_t *c)
{
  unsigned int p, pe, eof;
  int cs;
  %%{
    write init;
    getkey c->buffer->info[p].indic_category();
  }%%

  p = 0;
  pe = eof = c->buffer->len;

  unsigned int last = 0;
  %%{
    write exec;
  }%%
}

HB_END_DECLS

#endif /* HB_OT_SHAPE_COMPLEX_INDIC_MACHINE_HH */
