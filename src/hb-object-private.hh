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


/* user_data */

HB_END_DECLS


template <typename Type, unsigned int StaticSize>
struct hb_static_array_t {

  unsigned int len;
  unsigned int allocated;
  Type *array;
  Type static_array[StaticSize];

  void finish (void) { for (unsigned i = 0; i < len; i++) array[i].finish (); }

  inline Type& operator [] (unsigned int i)
  {
    return array[i];
  }

  inline Type *push (void)
  {
    if (!array) {
      array = static_array;
      allocated = ARRAY_LENGTH (static_array);
    }
    if (likely (len < allocated))
      return &array[len++];
    /* Need to reallocate */
    unsigned int new_allocated = allocated + (allocated >> 1) + 8;
    Type *new_array;
    if (array == static_array) {
      new_array = (Type *) calloc (new_allocated, sizeof (Type));
      if (new_array) {
        memcpy (new_array, array, len * sizeof (Type));
	array = new_array;
      }
    } else {
      bool overflows = new_allocated >= ((unsigned int) -1) / sizeof (Type);
      if (unlikely (overflows))
        new_array = NULL;
      else
	new_array = (Type *) realloc (array, new_allocated * sizeof (Type));
      if (new_array) {
        free (array);
	array = new_array;
      }
    }
    if ((len < allocated))
      return &array[len++];
    else
      return NULL;
  }

  inline void pop (void)
  {
    len--;
    /* TODO: shrink array if needed */
  }
};

template <typename Type>
struct hb_array_t : hb_static_array_t<Type, 2> {};


template <typename Key, typename Value>
struct hb_map_t
{
  struct item_t {
    Key key;
    /* unsigned int hash; */
    Value value;

    void finish (void) { value.finish (); }
  };

  hb_array_t <item_t> items;

  private:

  inline item_t *find (Key key) {
    if (unlikely (!key)) return NULL;
    for (unsigned int i = 0; i < items.len; i++)
      if (key == items[i].key)
	return &items[i];
    return NULL;
  }

  public:

  inline bool set (Key   key,
		   Value &value)
  {
    if (unlikely (!key)) return NULL;
    item_t *item;
    item = find (key);
    if (item)
      item->finish ();
    else
      item = items.push ();
    if (unlikely (!item)) return false;
    item->key = key;
    item->value = value;
    return true;
  }

  inline void unset (Key &key)
  {
    item_t *item;
    item = find (key);
    if (!item) return;

    item->finish ();
    items[items.len - 1] = *item;
    items.pop ();
  }

  inline Value *get (Key key)
  {
    item_t *item = find (key);
    return item ? &item->value : NULL;
  }

  void finish (void) { items.finish (); }
};


HB_BEGIN_DECLS

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
    return map.get (key);
  }

  void finish (void) { map.finish (); }
};



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

template <typename Type>
static inline void hb_object_trace (const Type *obj, const char *function)
{
  obj->header.trace (function);
}
template <typename Type>
static inline Type *hb_object_create ()
{
  Type *obj = (Type *) hb_object_header_t::create (sizeof (Type));
  hb_object_trace (obj, HB_FUNC);
  return obj;
}
template <typename Type>
static inline bool hb_object_is_inert (const Type *obj)
{
  return unlikely (obj->header.is_inert());
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


HB_BEGIN_DECLS


HB_END_DECLS

#endif /* HB_OBJECT_PRIVATE_HH */
