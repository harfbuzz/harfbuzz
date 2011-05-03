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


/* atomic_int and mutex */

/* We need external help for these */

#ifdef HAVE_GLIB

#include <glib.h>

typedef volatile int hb_atomic_int_t;
#define hb_atomic_int_fetch_and_add(AI, V)	g_atomic_int_exchange_and_add (&(AI), V)
#define hb_atomic_int_get(AI)			g_atomic_int_get (&(AI))
#define hb_atomic_int_set(AI, V)		g_atomic_int_set (&(AI), V)

typedef GStaticMutex hb_mutex_t;
#define HB_MUTEX_INIT			G_STATIC_MUTEX_INIT
#define hb_mutex_init(M)		g_static_mutex_init (&(M))
#define hb_mutex_lock(M)		g_static_mutex_lock (&(M))
#define hb_mutex_trylock(M)		g_static_mutex_trylock (&(M))
#define hb_mutex_unlock(M)		g_static_mutex_unlock (&(M))
#define hb_mutex_free(M)		g_static_mutex_free (&(M))

#else

#ifdef _MSC_VER

#include <intrin.h>

typedef long hb_atomic_int_t;
#define hb_atomic_int_fetch_and_add(AI, V)	_InterlockedExchangeAdd (&(AI), V)
#define hb_atomic_int_get(AI)			(_ReadBarrier (), (AI))
#define hb_atomic_int_set(AI, V)		((void) _InterlockedExchange (&(AI), (V)))

#include <Windows.h>

typedef CRITICAL_SECTION hb_mutex_t;
#define HB_MUTEX_INIT				{ NULL, 0, 0, NULL, NULL, 0 }
#define hb_mutex_init(M)			InitializeCriticalSection (&(M))
#define hb_mutex_lock(M)			EnterCriticalSection (&(M))
#define hb_mutex_trylock(M)			TryEnterCriticalSection (&(M))
#define hb_mutex_unlock(M)			LeaveCriticalSection (&(M))
#define hb_mutex_free(M)			DeleteCriticalSection (&(M))

#else

#warning "Could not find any system to define platform macros, library will NOT be thread-safe"

typedef volatile int hb_atomic_int_t;
#define hb_atomic_int_fetch_and_add(AI, V)	((AI) += (V), (AI) - (V))
#define hb_atomic_int_get(AI)			(AI)
#define hb_atomic_int_set(AI, V)		((void) ((AI) = (V)))

typedef volatile int hb_mutex_t;
#define HB_MUTEX_INIT				0
#define hb_mutex_init(M)			((void) ((M) = 0))
#define hb_mutex_lock(M)			((void) ((M) = 1))
#define hb_mutex_trylock(M)			((M) = 1, 1)
#define hb_mutex_unlock(M)			((void) ((M) = 0))
#define hb_mutex_free(M)			((void) ((M) = 2))

#endif

#endif




/* reference_count */

typedef struct {
  hb_atomic_int_t ref_count;

#define HB_REFERENCE_COUNT_INVALID_VALUE ((hb_atomic_int_t) -1)
#define HB_REFERENCE_COUNT_INVALID {HB_REFERENCE_COUNT_INVALID_VALUE}

  inline void init (int v) { ref_count = v; /* non-atomic is fine */ }
  inline int inc (void) { return hb_atomic_int_fetch_and_add (ref_count,  1); }
  inline int dec (void) { return hb_atomic_int_fetch_and_add (ref_count, -1); }
  inline void set (int v) { hb_atomic_int_set (ref_count, v); }

  inline int get (void) const { return hb_atomic_int_get (ref_count); }
  inline bool is_invalid (void) const { return get () == HB_REFERENCE_COUNT_INVALID_VALUE; }

} hb_reference_count_t;


/* user_data */

/* XXX make this thread-safe, somehow! */

typedef struct {
  void *data;
  hb_destroy_func_t destroy;

  void finish (void) { if (destroy) destroy (data); }
} hb_user_data_t;

struct hb_user_data_array_t {

  hb_map_t<hb_user_data_key_t *, hb_user_data_t> map;

  inline bool set (hb_user_data_key_t *key,
		   void *              data,
		   hb_destroy_func_t   destroy)
  {
    if (!data && !destroy) {
      map.unset (key);
      return true;
    }
    hb_user_data_t user_data = {data, destroy};
    return map.set (key, user_data);
  }

  inline void *get (hb_user_data_key_t *key) {
    hb_user_data_t *user_data = map.get (key);
    return user_data ? user_data->data : NULL;
  }

  void finish (void) { map.finish (); }
};


/* object_header */

typedef struct _hb_object_header_t hb_object_header_t;

struct _hb_object_header_t {
  hb_reference_count_t ref_count;
  hb_user_data_array_t user_data;

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

  inline bool is_inert (void) const {
    return unlikely (ref_count.is_invalid ());
  }

  inline void reference (void) {
    if (unlikely (!this || this->is_inert ()))
      return;
    ref_count.inc ();
  }

  inline bool destroy (void) {
    if (unlikely (!this || this->is_inert ()))
      return false;
    if (ref_count.dec () != 1)
      return false;

    user_data.finish ();

    return true;
  }

  inline bool set_user_data (hb_user_data_key_t *key,
			     void *              data,
			     hb_destroy_func_t   destroy) {
    if (unlikely (!this || this->is_inert ()))
      return false;

    return user_data.set (key, data, destroy);
  }

  inline void *get_user_data (hb_user_data_key_t *key) {
    return user_data.get (key);
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


/* object */

template <typename Type>
static inline void hb_object_trace (const Type *obj, const char *function)
{
  obj->header.trace (function);
}
template <typename Type>
static inline Type *hb_object_create (void)
{
  Type *obj = (Type *) hb_object_header_t::create (sizeof (Type));
  hb_object_trace (obj, HB_FUNC);
  return obj;
}
template <typename Type>
static inline bool hb_object_is_inert (const Type *obj)
{
  return unlikely (obj->header.is_inert ());
}
template <typename Type>
static inline Type *hb_object_reference (Type *obj)
{
  hb_object_trace (obj, HB_FUNC);
  obj->header.reference ();
  return obj;
}
template <typename Type>
static inline bool hb_object_destroy (Type *obj)
{
  hb_object_trace (obj, HB_FUNC);
  return obj->header.destroy ();
}
template <typename Type>
static inline bool hb_object_set_user_data (Type               *obj,
					    hb_user_data_key_t *key,
					    void *              data,
					    hb_destroy_func_t   destroy)
{
  return obj->header.set_user_data (key, data, destroy);
}

template <typename Type>
static inline void *hb_object_get_user_data (Type               *obj,
					     hb_user_data_key_t *key)
{
  return obj->header.get_user_data (key);
}


HB_BEGIN_DECLS


HB_END_DECLS

#endif /* HB_OBJECT_PRIVATE_HH */
