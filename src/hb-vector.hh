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


template <typename Type, unsigned int StaticSize=8>
struct hb_vector_t
{
  unsigned int len;
  private:
  unsigned int allocated; /* == 0 means allocation failed. */
  Type *arrayZ_;
  Type static_array[StaticSize];
  public:

  void init (void)
  {
    len = 0;
    allocated = ARRAY_LENGTH (static_array);
    arrayZ_ = nullptr;
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
  {
    ::qsort (arrayZ(), len, sizeof (Type), cmp);
  }

  inline void qsort (void)
  {
    ::qsort (arrayZ(), len, sizeof (Type), Type::cmp);
  }

  inline void qsort (unsigned int start, unsigned int end)
  {
    ::qsort (arrayZ() + start, end - start, sizeof (Type), Type::cmp);
  }

  template <typename T>
  inline Type *lsearch (const T &x)
  {
    Type *array = arrayZ();
    for (unsigned int i = 0; i < len; i++)
      if (0 == array[i].cmp (&x))
	return &array[i];
    return nullptr;
  }
  template <typename T>
  inline const Type *lsearch (const T &x) const
  {
    const Type *array = arrayZ();
    for (unsigned int i = 0; i < len; i++)
      if (0 == array[i].cmp (&x))
	return &array[i];
    return nullptr;
  }

  template <typename T>
  inline Type *bsearch (const T &x)
  {
    unsigned int i;
    return bfind (x, &i) ? &arrayZ()[i] : nullptr;
  }
  template <typename T>
  inline const Type *bsearch (const T &x) const
  {
    unsigned int i;
    return bfind (x, &i) ? &arrayZ()[i] : nullptr;
  }
  template <typename T>
  inline bool bfind (const T &x, unsigned int *i) const
  {
    int min = 0, max = (int) this->len - 1;
    const Type *array = this->arrayZ();
    while (min <= max)
    {
      int mid = (min + max) / 2;
      int c = array[mid].cmp (&x);
      if (c < 0)
        max = mid - 1;
      else if (c > 0)
        min = mid + 1;
      else
      {
        *i = mid;
	return true;
      }
    }
    if (max < 0 || (max < (int) this->len && array[max].cmp (&x) > 0))
      max++;
    *i = max;
    return false;
  }

  inline void fini_deep (void)
  {
    Type *array = arrayZ();
    unsigned int count = len;
    for (unsigned int i = 0; i < count; i++)
      array[i].fini ();
    fini ();
  }

  inline void fini (void)
  {
    if (arrayZ_)
      free (arrayZ_);
    arrayZ_ = nullptr;
    allocated = len = 0;
  }
};


#endif /* HB_VECTOR_HH */
