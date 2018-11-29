/*
 * Copyright © 2017,2018  Google, Inc.
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
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_VECTOR_HH
#define HB_VECTOR_HH

#include "hb.hh"


template <typename Type, unsigned int PreallocedCount=8>
struct hb_vector_t
{
  static_assert ((bool) (unsigned) hb_static_size (Type), "");

  typedef Type ItemType;
  enum { item_size = sizeof (Type) };

  HB_NO_COPY_ASSIGN_TEMPLATE2 (hb_vector_t, Type, PreallocedCount);
  inline hb_vector_t (void) { init (); }
  inline ~hb_vector_t (void) { fini (); }

  unsigned int len;
  private:
  unsigned int allocated; /* == 0 means allocation failed. */
  Type *arrayZ_;
  Type static_array[PreallocedCount];
  public:

  void init (void)
  {
    len = 0;
    allocated = ARRAY_LENGTH (static_array);
    arrayZ_ = nullptr;
  }

  inline void fini (void)
  {
    if (arrayZ_)
      free (arrayZ_);
    arrayZ_ = nullptr;
    allocated = len = 0;
  }
  inline void fini_deep (void)
  {
    Type *array = arrayZ();
    unsigned int count = len;
    for (unsigned int i = 0; i < count; i++)
      array[i].fini ();
    fini ();
  }

  inline Type * arrayZ (void)
  { return arrayZ_ ? arrayZ_ : static_array; }
  inline const Type * arrayZ (void) const
  { return arrayZ_ ? arrayZ_ : static_array; }

  inline Type& operator [] (unsigned int i)
  {
    if (unlikely (i >= len))
      return Crap (Type);
    return arrayZ()[i];
  }
  inline const Type& operator [] (unsigned int i) const
  {
    if (unlikely (i >= len))
      return Null(Type);
    return arrayZ()[i];
  }

  inline hb_array_t<Type> as_array (void)
  { return hb_array (arrayZ(), len); }
  inline hb_array_t<const Type> as_array (void) const
  { return hb_array (arrayZ(), len); }

  inline hb_sorted_array_t<Type> as_sorted_array (void)
  { return hb_sorted_array (arrayZ(), len); }
  inline hb_sorted_array_t<const Type> as_sorted_array (void) const
  { return hb_sorted_array (arrayZ(), len); }

  template <typename T> inline operator  T * (void) { return arrayZ(); }
  template <typename T> inline operator const T * (void) const { return arrayZ(); }

  inline Type * operator  + (unsigned int i) { return arrayZ() + i; }
  inline const Type * operator  + (unsigned int i) const { return arrayZ() + i; }

  inline Type *push (void)
  {
    if (unlikely (!resize (len + 1)))
      return &Crap(Type);
    return &arrayZ()[len - 1];
  }
  inline Type *push (const Type& v)
  {
    Type *p = push ();
    *p = v;
    return p;
  }

  inline bool in_error (void) const { return allocated == 0; }

  /* Allocate for size but don't adjust len. */
  inline bool alloc (unsigned int size)
  {
    if (unlikely (!allocated))
      return false;

    if (likely (size <= allocated))
      return true;

    /* Reallocate */

    unsigned int new_allocated = allocated;
    while (size >= new_allocated)
      new_allocated += (new_allocated >> 1) + 8;

    Type *new_array = nullptr;

    if (!arrayZ_)
    {
      new_array = (Type *) calloc (new_allocated, sizeof (Type));
      if (new_array)
        memcpy (new_array, static_array, len * sizeof (Type));
    }
    else
    {
      bool overflows = (new_allocated < allocated) || hb_unsigned_mul_overflows (new_allocated, sizeof (Type));
      if (likely (!overflows))
        new_array = (Type *) realloc (arrayZ_, new_allocated * sizeof (Type));
    }

    if (unlikely (!new_array))
    {
      allocated = 0;
      return false;
    }

    arrayZ_ = new_array;
    allocated = new_allocated;

    return true;
  }

  inline bool resize (int size_)
  {
    unsigned int size = size_ < 0 ? 0u : (unsigned int) size_;
    if (!alloc (size))
      return false;

    if (size > len)
      memset (arrayZ() + len, 0, (size - len) * sizeof (*arrayZ()));

    len = size;
    return true;
  }

  inline void pop (void)
  {
    if (!len) return;
    len--;
  }

  inline void remove (unsigned int i)
  {
    if (unlikely (i >= len))
      return;
    Type *array = arrayZ();
    memmove (static_cast<void *> (&array[i]),
	     static_cast<void *> (&array[i + 1]),
	     (len - i - 1) * sizeof (Type));
    len--;
  }

  inline void shrink (int size_)
  {
    unsigned int size = size_ < 0 ? 0u : (unsigned int) size_;
     if (size < len)
       len = size;
  }

  template <typename T>
  inline Type *find (T v)
  {
    Type *array = arrayZ();
    for (unsigned int i = 0; i < len; i++)
      if (array[i] == v)
	return &array[i];
    return nullptr;
  }
  template <typename T>
  inline const Type *find (T v) const
  {
    const Type *array = arrayZ();
    for (unsigned int i = 0; i < len; i++)
      if (array[i] == v)
	return &array[i];
    return nullptr;
  }

  inline void qsort (int (*cmp)(const void*, const void*))
  { as_array ().qsort (cmp); }
  inline void qsort (unsigned int start = 0, unsigned int end = (unsigned int) -1)
  { as_array ().qsort (start, end); }

  template <typename T>
  inline Type *lsearch (const T &x, Type *not_found = nullptr)
  { return as_array ().lsearch (x, not_found); }
  template <typename T>
  inline const Type *lsearch (const T &x, const Type *not_found = nullptr) const
  { return as_array ().lsearch (x, not_found); }

  template <typename T>
  inline Type *bsearch (const T &x, Type *not_found = nullptr)
  { return as_sorted_array ().bsearch (x, not_found); }
  template <typename T>
  inline const Type *bsearch (const T &x, const Type *not_found = nullptr) const
  { return as_sorted_array ().bsearch (x, not_found); }
  template <typename T>
  inline bool bfind (const T &x, unsigned int *i = nullptr,
		     hb_bfind_not_found_t not_found = HB_BFIND_NOT_FOUND_DONT_STORE,
		     unsigned int to_store = (unsigned int) -1) const
  { return as_sorted_array ().bfind (x, i, not_found, to_store); }
};


#endif /* HB_VECTOR_HH */
