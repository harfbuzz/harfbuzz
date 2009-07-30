/*
 * Copyright © 2007 Chris Wilson
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
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
 * Contributor(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#ifndef HB_REFCOUNT_PRIVATE_H
#define HB_REFCOUNT_PRIVATE_H

typedef int hb_atomic_int_t;

/* Encapsulate operations on the object's reference count */
typedef struct {
  hb_atomic_int_t ref_count;
} hb_reference_count_t;

/* XXX add real atomic ops */
#define _hb_reference_count_inc(RC) ((RC).ref_count++)
#define _hb_reference_count_dec_and_test(RC) ((RC).ref_count-- == 1)

#define HB_REFERENCE_COUNT_INIT(RC, VALUE) ((RC).ref_count = (VALUE))

#define HB_REFERENCE_COUNT_GET_VALUE(RC) ((RC).ref_count+0)
#define HB_REFERENCE_COUNT_SET_VALUE(RC, VALUE) ((RC).ref_count = (VALUE), 0)

#define HB_REFERENCE_COUNT_INVALID_VALUE ((hb_atomic_int_t) -1)
#define HB_REFERENCE_COUNT_INVALID {HB_REFERENCE_COUNT_INVALID_VALUE}

#define HB_REFERENCE_COUNT_IS_INVALID(RC) (HB_REFERENCE_COUNT_GET_VALUE (RC) == HB_REFERENCE_COUNT_INVALID_VALUE)

#define HB_REFERENCE_COUNT_HAS_REFERENCE(RC) (HB_REFERENCE_COUNT_GET_VALUE (RC) > 0)

#endif /* HB_REFCOUNT_PRIVATE_H */
