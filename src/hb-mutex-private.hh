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

#ifndef HB_MUTEX_PRIVATE_HH
#define HB_MUTEX_PRIVATE_HH

#include "hb-private.hh"

HB_BEGIN_DECLS


/* mutex */

/* We need external help for these */

#ifdef HAVE_GLIB

#include <glib.h>

typedef volatile int hb_atomic_int_t;
#define hb_atomic_int_fetch_and_add(AI, V)	g_atomic_int_exchange_and_add (&(AI), V)
#define hb_atomic_int_get(AI)			g_atomic_int_get (&(AI))
#define hb_atomic_int_set(AI, V)		g_atomic_int_set (&(AI), V)

typedef GStaticMutex hb_mutex_t;
#define HB_MUTEX_INIT			G_STATIC_MUTEX_INIT
#define hb_mutex_init(M)		g_static_mutex_init (M)
#define hb_mutex_lock(M)		g_static_mutex_lock (M)
#define hb_mutex_trylock(M)		g_static_mutex_trylock (M)
#define hb_mutex_unlock(M)		g_static_mutex_unlock (M)
#define hb_mutex_free(M)		g_static_mutex_free (M)


#elif defined(_MSC_VER)

#include <intrin.h>

typedef long hb_atomic_int_t;
#define hb_atomic_int_fetch_and_add(AI, V)	_InterlockedExchangeAdd (&(AI), V)
#define hb_atomic_int_get(AI)			(_ReadBarrier (), (AI))
#define hb_atomic_int_set(AI, V)		((void) _InterlockedExchange (&(AI), (V)))

#include <Windows.h>

typedef CRITICAL_SECTION hb_mutex_t;
#define HB_MUTEX_INIT				{ NULL, 0, 0, NULL, NULL, 0 }
#define hb_mutex_init(M)			InitializeCriticalSection (M)
#define hb_mutex_lock(M)			EnterCriticalSection (M)
#define hb_mutex_trylock(M)			TryEnterCriticalSection (M)
#define hb_mutex_unlock(M)			LeaveCriticalSection (M)
#define hb_mutex_free(M)			DeleteCriticalSection (M)


#else

#warning "Could not find any system to define platform macros, library will NOT be thread-safe"

typedef volatile int hb_atomic_int_t;
#define hb_atomic_int_fetch_and_add(AI, V)	((AI) += (V), (AI) - (V))
#define hb_atomic_int_get(AI)			(AI)
#define hb_atomic_int_set(AI, V)		((void) ((AI) = (V)))

typedef volatile int hb_mutex_t;
#define HB_MUTEX_INIT				0
#define hb_mutex_init(M)			((void) (*(M) = 0))
#define hb_mutex_lock(M)			((void) (*(M) = 1))
#define hb_mutex_trylock(M)			(*(M) = 1, 1)
#define hb_mutex_unlock(M)			((void) (*(M) = 0))
#define hb_mutex_free(M)			((void) (*(M) = 2))


#endif


struct hb_static_mutex_t : hb_mutex_t
{
  hb_static_mutex_t (void) {
    hb_mutex_init (this);
  }
};


HB_END_DECLS


/* XXX If the finish() callbacks of items in the set recursively try to
 * modify the set, deadlock occurs.  This needs fixing in set proper in
 * fact. */

template <typename item_t>
struct hb_threadsafe_set_t
{
  hb_set_t <item_t> set;
  hb_static_mutex_t mutex;

  template <typename T>
  inline item_t *insert (T v)
  {
    hb_mutex_lock (&mutex);
    item_t *item = set.insert (v);
    hb_mutex_unlock (&mutex);
    return item;
  }

  template <typename T>
  inline void remove (T v)
  {
    hb_mutex_lock (&mutex);
    set.remove (v);
    hb_mutex_unlock (&mutex);
  }

  template <typename T>
  inline item_t *find (T v)
  {
    hb_mutex_lock (&mutex);
    item_t *item = set.find (v);
    hb_mutex_unlock (&mutex);
    return item;
  }

  template <typename T>
  inline item_t *find_or_insert (T v) {
    hb_mutex_lock (&mutex);
    item_t *item = set.find_or_insert (v);
    hb_mutex_unlock (&mutex);
    return item;
  }

  void finish (void) {
    hb_mutex_lock (&mutex);
    set.finish ();
    hb_mutex_unlock (&mutex);
  }

};


HB_BEGIN_DECLS

HB_END_DECLS

#endif /* HB_MUTEX_PRIVATE_HH */
