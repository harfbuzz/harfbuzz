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


#endif /* HB_DSALGS_HH */
