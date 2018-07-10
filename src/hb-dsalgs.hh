/*
 * Copyright © 2017  Google, Inc.
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

#ifndef HB_DSALGS_HH
#define HB_DSALGS_HH

#include "hb-private.hh"


static inline void *
hb_bsearch_r (const void *key, const void *base,
	      size_t nmemb, size_t size,
	      int (*compar)(const void *_key, const void *_item, void *_arg),
	      void *arg)
{
  int min = 0, max = (int) nmemb - 1;
  while (min <= max)
  {
    int mid = (min + max) / 2;
    const void *p = (const void *) (((const char *) base) + (mid * size));
    int c = compar (key, p, arg);
    if (c < 0)
      max = mid - 1;
    else if (c > 0)
      min = mid + 1;
    else
      return (void *) p;
  }
  return nullptr;
}



/* From https://github.com/noporpoise/sort_r */

/* Isaac Turner 29 April 2014 Public Domain */

/*

hb_sort_r function to be exported.

Parameters:
  base is the array to be sorted
  nel is the number of elements in the array
  width is the size in bytes of each element of the array
  compar is the comparison function
  arg is a pointer to be passed to the comparison function

void hb_sort_r(void *base, size_t nel, size_t width,
               int (*compar)(const void *_a, const void *_b, void *_arg),
               void *arg);
*/


/* swap a, b iff a>b */
/* __restrict is same as restrict but better support on old machines */
static int sort_r_cmpswap(char *__restrict a, char *__restrict b, size_t w,
			  int (*compar)(const void *_a, const void *_b,
					void *_arg),
			  void *arg)
{
  char tmp, *end = a+w;
  if(compar(a, b, arg) > 0) {
    for(; a < end; a++, b++) { tmp = *a; *a = *b; *b = tmp; }
    return 1;
  }
  return 0;
}

/* Note: quicksort is not stable, equivalent values may be swapped */
static inline void sort_r_simple(void *base, size_t nel, size_t w,
				 int (*compar)(const void *_a, const void *_b,
					       void *_arg),
				 void *arg)
{
  char *b = (char *)base, *end = b + nel*w;
  if(nel < 7) {
    /* Insertion sort for arbitrarily small inputs */
    char *pi, *pj;
    for(pi = b+w; pi < end; pi += w) {
      for(pj = pi; pj > b && sort_r_cmpswap(pj-w,pj,w,compar,arg); pj -= w) {}
    }
  }
  else
  {
    /* nel > 6; Quicksort */

    /* Use median of first, middle and last items as pivot */
    char *x, *y, *xend, ch;
    char *pl, *pr;
    char *last = b+w*(nel-1), *tmp;
    char *l[3];
    l[0] = b;
    l[1] = b+w*(nel/2);
    l[2] = last;

    if(compar(l[0],l[1],arg) > 0) { tmp=l[0]; l[0]=l[1]; l[1]=tmp; }
    if(compar(l[1],l[2],arg) > 0) {
      tmp=l[1]; l[1]=l[2]; l[2]=tmp; /* swap(l[1],l[2]) */
      if(compar(l[0],l[1],arg) > 0) { tmp=l[0]; l[0]=l[1]; l[1]=tmp; }
    }

    /* swap l[id], l[2] to put pivot as last element */
    for(x = l[1], y = last, xend = x+w; x<xend; x++, y++) {
      ch = *x; *x = *y; *y = ch;
    }

    pl = b;
    pr = last;

    while(pl < pr) {
      for(; pl < pr; pl += w) {
        if(sort_r_cmpswap(pl, pr, w, compar, arg)) {
          pr -= w; /* pivot now at pl */
          break;
        }
      }
      for(; pl < pr; pr -= w) {
        if(sort_r_cmpswap(pl, pr, w, compar, arg)) {
          pl += w; /* pivot now at pr */
          break;
        }
      }
    }

    sort_r_simple(b, (pl-b)/w, w, compar, arg);
    sort_r_simple(pl+w, (end-(pl+w))/w, w, compar, arg);
  }
}

static inline void hb_sort_r(void *base, size_t nel, size_t width,
			     int (*compar)(const void *_a, const void *_b, void *_arg),
			     void *arg)
{
    sort_r_simple(base, nel, width, compar, arg);
}


template <typename T, typename T2> static inline void
hb_stable_sort (T *array, unsigned int len, int(*compar)(const T *, const T *), T2 *array2)
{
  for (unsigned int i = 1; i < len; i++)
  {
    unsigned int j = i;
    while (j && compar (&array[j - 1], &array[i]) > 0)
      j--;
    if (i == j)
      continue;
    /* Move item i to occupy place for item j, shift what's in between. */
    {
      T t = array[i];
      memmove (&array[j + 1], &array[j], (i - j) * sizeof (T));
      array[j] = t;
    }
    if (array2)
    {
      T2 t = array2[i];
      memmove (&array2[j + 1], &array2[j], (i - j) * sizeof (T2));
      array2[j] = t;
    }
  }
}

template <typename T> static inline void
hb_stable_sort (T *array, unsigned int len, int(*compar)(const T *, const T *))
{
  hb_stable_sort (array, len, compar, (int *) nullptr);
}

static inline hb_bool_t
hb_codepoint_parse (const char *s, unsigned int len, int base, hb_codepoint_t *out)
{
  /* Pain because we don't know whether s is nul-terminated. */
  char buf[64];
  len = MIN (ARRAY_LENGTH (buf) - 1, len);
  strncpy (buf, s, len);
  buf[len] = '\0';

  char *end;
  errno = 0;
  unsigned long v = strtoul (buf, &end, base);
  if (errno) return false;
  if (*end) return false;
  *out = v;
  return true;
}


#define HB_VECTOR_INIT {0, 0, false, nullptr}
template <typename Type, unsigned int StaticSize=8>
struct hb_vector_t
{
  unsigned int len;
  unsigned int allocated;
  bool successful;
  Type *arrayZ;
  Type static_array[StaticSize];

  void init (void)
  {
    len = 0;
    allocated = ARRAY_LENGTH (static_array);
    successful = true;
    arrayZ = static_array;
  }

  inline Type& operator [] (unsigned int i)
  {
    if (unlikely (i >= len))
      return Crap (Type);
    return arrayZ[i];
  }
  inline const Type& operator [] (unsigned int i) const
  {
    if (unlikely (i >= len))
      return Null(Type);
    return arrayZ[i];
  }

  inline Type *push (void)
  {
    if (unlikely (!resize (len + 1)))
      return &Crap(Type);
    return &arrayZ[len - 1];
  }
  inline Type *push (const Type& v)
  {
    Type *p = push ();
    *p = v;
    return p;
  }

  /* Allocate for size but don't adjust len. */
  inline bool alloc (unsigned int size)
  {
    if (unlikely (!successful))
      return false;

    if (likely (size <= allocated))
      return true;

    /* Reallocate */

    unsigned int new_allocated = allocated;
    while (size >= new_allocated)
      new_allocated += (new_allocated >> 1) + 8;

    Type *new_array = nullptr;

    if (arrayZ == static_array)
    {
      new_array = (Type *) calloc (new_allocated, sizeof (Type));
      if (new_array)
        memcpy (new_array, arrayZ, len * sizeof (Type));
    }
    else
    {
      bool overflows = (new_allocated < allocated) || hb_unsigned_mul_overflows (new_allocated, sizeof (Type));
      if (likely (!overflows))
        new_array = (Type *) realloc (arrayZ, new_allocated * sizeof (Type));
    }

    if (unlikely (!new_array))
    {
      successful = false;
      return false;
    }

    arrayZ = new_array;
    allocated = new_allocated;

    return true;
  }

  inline bool resize (int size_)
  {
    unsigned int size = size_ < 0 ? 0u : (unsigned int) size_;
    if (!alloc (size))
      return false;

    if (size > len)
      memset (arrayZ + len, 0, (size - len) * sizeof (*arrayZ));

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
     memmove (static_cast<void *> (&arrayZ[i]),
	      static_cast<void *> (&arrayZ[i + 1]),
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
  inline Type *find (T v) {
    for (unsigned int i = 0; i < len; i++)
      if (arrayZ[i] == v)
	return &arrayZ[i];
    return nullptr;
  }
  template <typename T>
  inline const Type *find (T v) const {
    for (unsigned int i = 0; i < len; i++)
      if (arrayZ[i] == v)
	return &arrayZ[i];
    return nullptr;
  }

  inline void qsort (int (*cmp)(const void*, const void*))
  {
    ::qsort (arrayZ, len, sizeof (Type), cmp);
  }

  inline void qsort (void)
  {
    ::qsort (arrayZ, len, sizeof (Type), Type::cmp);
  }

  inline void qsort (unsigned int start, unsigned int end)
  {
    ::qsort (arrayZ + start, end - start, sizeof (Type), Type::cmp);
  }

  template <typename T>
  inline Type *lsearch (const T &x)
  {
    for (unsigned int i = 0; i < len; i++)
      if (0 == this->arrayZ[i].cmp (&x))
	return &arrayZ[i];
    return nullptr;
  }

  template <typename T>
  inline Type *bsearch (const T &x)
  {
    unsigned int i;
    return bfind (x, &i) ? &arrayZ[i] : nullptr;
  }
  template <typename T>
  inline const Type *bsearch (const T &x) const
  {
    unsigned int i;
    return bfind (x, &i) ? &arrayZ[i] : nullptr;
  }
  template <typename T>
  inline bool bfind (const T &x, unsigned int *i) const
  {
    int min = 0, max = (int) this->len - 1;
    while (min <= max)
    {
      int mid = (min + max) / 2;
      int c = this->arrayZ[mid].cmp (&x);
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
    if (max < 0 || (max < (int) this->len && this->arrayZ[max].cmp (&x) > 0))
      max++;
    *i = max;
    return false;
  }

  inline void fini (void)
  {
    if (arrayZ != static_array)
      free (arrayZ);
    arrayZ = nullptr;
    allocated = len = 0;
  }
};

template <typename Type>
struct hb_auto_t : Type
{
  hb_auto_t (void) { Type::init (); }
  ~hb_auto_t (void) { Type::fini (); }
  private: /* Hide */
  void init (void) {}
  void fini (void) {}
};


#define HB_LOCKABLE_SET_INIT {HB_VECTOR_INIT}
template <typename item_t, typename lock_t>
struct hb_lockable_set_t
{
  hb_vector_t <item_t, 1> items;

  inline void init (void) { items.init (); }

  template <typename T>
  inline item_t *replace_or_insert (T v, lock_t &l, bool replace)
  {
    l.lock ();
    item_t *item = items.find (v);
    if (item) {
      if (replace) {
	item_t old = *item;
	*item = v;
	l.unlock ();
	old.fini ();
      }
      else {
        item = nullptr;
	l.unlock ();
      }
    } else {
      item = items.push (v);
      l.unlock ();
    }
    return item;
  }

  template <typename T>
  inline void remove (T v, lock_t &l)
  {
    l.lock ();
    item_t *item = items.find (v);
    if (item) {
      item_t old = *item;
      *item = items[items.len - 1];
      items.pop ();
      l.unlock ();
      old.fini ();
    } else {
      l.unlock ();
    }
  }

  template <typename T>
  inline bool find (T v, item_t *i, lock_t &l)
  {
    l.lock ();
    item_t *item = items.find (v);
    if (item)
      *i = *item;
    l.unlock ();
    return !!item;
  }

  template <typename T>
  inline item_t *find_or_insert (T v, lock_t &l)
  {
    l.lock ();
    item_t *item = items.find (v);
    if (!item) {
      item = items.push (v);
    }
    l.unlock ();
    return item;
  }

  inline void fini (lock_t &l)
  {
    if (!items.len) {
      /* No need for locking. */
      items.fini ();
      return;
    }
    l.lock ();
    while (items.len) {
      item_t old = items[items.len - 1];
	items.pop ();
	l.unlock ();
	old.fini ();
	l.lock ();
    }
    items.fini ();
    l.unlock ();
  }

};


#endif /* HB_DSALGS_HH */
