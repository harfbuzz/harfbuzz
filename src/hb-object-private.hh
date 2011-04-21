/*
 * Copyright (C) 2007  Chris Wilson
 * Copyright (C) 2009,2010  Red Hat, Inc.
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
 * Contributor(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 * Red Hat Author(s): Behdad Esfahbod
 */

#ifndef HB_OBJECT_PRIVATE_HH
#define HB_OBJECT_PRIVATE_HH

#include "hb-private.hh"

HB_BEGIN_DECLS


typedef struct {
  hb_atomic_int_t ref_count;

#define HB_REFERENCE_COUNT_INVALID_VALUE ((hb_atomic_int_t) -1)
#define HB_REFERENCE_COUNT_INVALID {HB_REFERENCE_COUNT_INVALID_VALUE}

  inline void init (int v) { ref_count = v; /* non-atomic is fine */ }
  inline int inc (void) { return hb_atomic_int_fetch_and_add (ref_count,  1); }
  inline int dec (void) { return hb_atomic_int_fetch_and_add (ref_count, -1); }
  inline void set (int v) { return hb_atomic_int_set (ref_count, v); }

  /* XXX
   *
   * One thing I'm not sure.  The following two methods should be declared
   * const.  However, that assumes that hb_atomic_int_get() is const.  I have
   * a vague memory hearing from Chris Wilson or Jeff Muizelaar that atomic get
   * is implemented as a fetch_and_add(0).  In which case it does write to the
   * memory, and hence cannot be called on .rodata section.  But that's how we
   * use it.
   *
   * If that is indeed the case, then perhaps is_invalid() should do a
   * non-protected read of the location.
   */
  inline int get (void) { return hb_atomic_int_get (ref_count); }
  inline bool is_invalid (void) { return get () == HB_REFERENCE_COUNT_INVALID_VALUE; }

} hb_reference_count_t;



/* Debug */

#ifndef HB_DEBUG_OBJECT
#define HB_DEBUG_OBJECT (HB_DEBUG+0)
#endif

static inline void
_hb_trace_object (const void *obj,
		  hb_reference_count_t *ref_count,
		  const char *function)
{
  (void) (HB_DEBUG_OBJECT &&
	  fprintf (stderr, "OBJECT(%p) refcount=%d %s\n",
		   obj,
		   ref_count->get (),
		   function));
}

#define TRACE_OBJECT(obj) _hb_trace_object (obj, &obj->ref_count, __FUNCTION__)



/* Object allocation and lifecycle manamgement macros */

#define HB_OBJECT_IS_INERT(obj) \
    (unlikely ((obj)->ref_count.is_invalid ()))

#define HB_OBJECT_DO_INIT_EXPR(obj) \
    obj->ref_count.init (1)

#define HB_OBJECT_DO_INIT(obj) \
  HB_STMT_START { \
    HB_OBJECT_DO_INIT_EXPR (obj); \
  } HB_STMT_END

#define HB_OBJECT_DO_CREATE(Type, obj) \
  likely (( \
	       (void) ( \
		 ((obj) = (Type *) calloc (1, sizeof (Type))) && \
		 ( \
		  HB_OBJECT_DO_INIT_EXPR (obj), \
		  TRACE_OBJECT (obj), \
		  TRUE \
		 ) \
	       ), \
	       (obj) \
	     ))

#define HB_OBJECT_DO_REFERENCE(obj) \
  HB_STMT_START { \
    int old_count; \
    if (unlikely (!(obj) || HB_OBJECT_IS_INERT (obj))) \
      return obj; \
    TRACE_OBJECT (obj); \
    old_count = obj->ref_count.inc (); \
    assert (old_count > 0); \
    return obj; \
  } HB_STMT_END

#define HB_OBJECT_DO_DESTROY(obj) \
  HB_STMT_START { \
    int old_count; \
    if (unlikely (!(obj) || HB_OBJECT_IS_INERT (obj))) \
      return; \
    TRACE_OBJECT (obj); \
    old_count = obj->ref_count.dec (); \
    assert (old_count > 0); \
    if (old_count != 1) \
      return; \
  } HB_STMT_END


HB_END_DECLS

#endif /* HB_OBJECT_PRIVATE_HH */
