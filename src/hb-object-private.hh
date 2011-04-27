/*
 * Copyright © 2007  Chris Wilson
 * Copyright © 2009,2010  Red Hat, Inc.
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
 * Contributor(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OBJECT_PRIVATE_HH
#define HB_OBJECT_PRIVATE_HH

#include "hb-private.hh"

HB_BEGIN_DECLS


/* Debug */

#ifndef HB_DEBUG_OBJECT
#define HB_DEBUG_OBJECT (HB_DEBUG+0)
#endif


typedef struct _hb_object_header_t hb_object_header_t;

struct _hb_object_header_t {
  hb_reference_count_t ref_count;

#define HB_OBJECT_HEADER_STATIC {HB_REFERENCE_COUNT_INVALID}

  static inline void *create (unsigned int size) {
    hb_object_header_t *obj = (hb_object_header_t *) calloc (1, size);

    if (likely (obj))
      obj->init ();

    return obj;
  }

  inline void init (void) {
    ref_count.init (1);
  }

  inline bool is_inert (void) const { return unlikely (ref_count.is_invalid ()); }

  inline void reference (void) {
    if (unlikely (!this || this->is_inert ()))
      return;
    ref_count.inc ();
  }

  inline bool destroy (void) {
    if (unlikely (!this || this->is_inert ()))
      return false;
    return ref_count.dec () == 1;
  }

  inline void trace (const char *function) const {
    (void) (HB_DEBUG_OBJECT &&
	    fprintf (stderr, "OBJECT(%p) refcount=%d %s\n",
		     this,
		     this ? ref_count.get () : 0,
		     function));
  }

};


HB_END_DECLS


template <typename Type>
static inline Type *hb_object_create () { return (Type *) hb_object_header_t::create (sizeof (Type)); }

template <typename Type>
static inline bool hb_object_is_inert (const Type *obj) { return unlikely (obj->header.is_inert()); }

template <typename Type>
static inline Type *hb_object_reference (Type *obj) { obj->header.reference (); return obj; }

template <typename Type>
static inline bool hb_object_destroy (Type *obj) { return obj->header.destroy (); }

template <typename Type>
static inline void hb_object_trace (const Type *obj) { obj->header.trace (__FUNCTION__); }


HB_BEGIN_DECLS


/* Object allocation and lifecycle manamgement macros */

/* XXX Trace objects.  Got removed in refactoring */
#define HB_TRACE_OBJECT(obj) hb_object_trace (obj)
#define HB_OBJECT_DO_CREATE(Type, obj) likely (obj = hb_object_create<Type> ())
#define HB_OBJECT_IS_INERT(obj) hb_object_is_inert (obj)
#define HB_OBJECT_DO_REFERENCE(obj) return hb_object_reference (obj)
#define HB_OBJECT_DO_DESTROY(obj) if (!hb_object_destroy (obj)) return


HB_END_DECLS

#endif /* HB_OBJECT_PRIVATE_HH */
