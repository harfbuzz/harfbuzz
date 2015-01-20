/*
 * Copyright © 2012  Google, Inc.
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

#ifndef HB_SET_PRIVATE_HH
#define HB_SET_PRIVATE_HH

#include "hb-private.hh"
#include "hb-object-private.hh"


/*
 * The set digests here implement various "filters" that support
 * "approximate member query".  Conceptually these are like Bloom
 * Filter and Quotient Filter, however, much smaller, faster, and
 * designed to fit the requirements of our uses for glyph coverage
 * queries.  As a result, our filters have much higher.
 */

template <typename mask_t, unsigned int shift>
struct hb_set_digest_lowest_bits_t
{
  ASSERT_POD ();

  static const unsigned int mask_bytes = sizeof (mask_t);
  static const unsigned int mask_bits = sizeof (mask_t) * 8;
  static const unsigned int num_bits = 0
				     + (mask_bytes >= 1 ? 3 : 0)
				     + (mask_bytes >= 2 ? 1 : 0)
				     + (mask_bytes >= 4 ? 1 : 0)
				     + (mask_bytes >= 8 ? 1 : 0)
				     + (mask_bytes >= 16? 1 : 0)
				     + 0;

  ASSERT_STATIC (shift < sizeof (hb_codepoint_t) * 8);
  ASSERT_STATIC (shift + num_bits <= sizeof (hb_codepoint_t) * 8);

  inline void init (void) {
    mask = 0;
  }

  inline void add (hb_codepoint_t g) {
    mask |= mask_for (g);
  }

  inline void add_range (hb_codepoint_t a, hb_codepoint_t b) {
    if ((b >> shift) - (a >> shift) >= mask_bits - 1)
      mask = (mask_t) -1;
    else {
      mask_t ma = mask_for (a);
      mask_t mb = mask_for (b);
      mask |= mb + (mb - ma) - (mb < ma);
    }
  }

  inline bool may_have (hb_codepoint_t g) const {
    return !!(mask & mask_for (g));
  }

  private:

  static inline mask_t mask_for (hb_codepoint_t g) {
    return ((mask_t) 1) << ((g >> shift) & (mask_bits - 1));
  }
  mask_t mask;
};

template <typename head_t, typename tail_t>
struct hb_set_digest_combiner_t
{
  ASSERT_POD ();

  inline void init (void) {
    head.init ();
    tail.init ();
  }

  inline void add (hb_codepoint_t g) {
    head.add (g);
    tail.add (g);
  }

  inline void add_range (hb_codepoint_t a, hb_codepoint_t b) {
    head.add_range (a, b);
    tail.add_range (a, b);
  }

  inline bool may_have (hb_codepoint_t g) const {
    return head.may_have (g) && tail.may_have (g);
  }

  private:
  head_t head;
  tail_t tail;
};


/*
 * hb_set_digest_t
 *
 * This is a combination of digests that performs "best".
 * There is not much science to this: it's a result of intuition
 * and testing.
 */
typedef hb_set_digest_combiner_t
<
  hb_set_digest_lowest_bits_t<unsigned long, 4>,
  hb_set_digest_combiner_t
  <
    hb_set_digest_lowest_bits_t<unsigned long, 0>,
    hb_set_digest_lowest_bits_t<unsigned long, 9>
  >
> hb_set_digest_t;



/*
 * hb_set_t
 */

/* Compute pointer offset */
#define PTR_TO_OFFSET(b,p)	((intptr_t) (p) - (intptr_t) (b))
/* Given base address, offset and type, return a pointer */
#define OFFSET_TO_PTR(b,o,t)  ((t *) ((intptr_t) (b) + (o)))
/* Given a structure, offset member and type, return pointer */
#define OFFSET_MEMBER(s,m,t)    OFFSET_TO_PTR(s,(s)->m,t)
#define HB_SET_LEAVES(c)        OFFSET_MEMBER(c,leaves_offset,intptr_t)
#define HB_SET_LEAF(c, i)       (OFFSET_TO_PTR(HB_SET_LEAVES(c), \
                                 HB_SET_LEAVES(c)[i], \
                                 hb_codepoint_leaf))
#define HB_SET_NUMBERS(c)	    OFFSET_MEMBER(c,numbers_offset,int32_t)

#define HB_IS_ZERO_OR_POWER_OF_TWO(x) (!((x) & ((x)-1)))

typedef struct _hb_codepoint_set {
  int num;	                  /* size of leaves and numbers arrays */
  intptr_t leaves_offset;     /* array of offsets to leaves */
  intptr_t numbers_offset;    /* array of offsets to numbers */
} hb_codepoint_set;

typedef struct _hb_codepoint_leaf {
    hb_codepoint_t	map[256/32];
} hb_codepoint_leaf;

static hb_codepoint_set *
hb_codepoint_set_init ()
{
  hb_codepoint_set * hcs;
  hcs =  (hb_codepoint_set *) malloc (sizeof (hb_codepoint_set));
  if (!hcs)
    return NULL;

  hcs->num = 0;
  hcs->leaves_offset = 0;
  hcs->numbers_offset = 0;

  return hcs;
}

struct hb_set_t
{
  hb_object_header_t header;
  ASSERT_POD ();
  bool in_error;

  inline void init (void) {
    hb_object_init (this);
    clear ();

    hcs = hb_codepoint_set_init ();
  }

  inline void fini (void) {
  }

  inline void clear (void) {
    if (unlikely (hb_object_is_inert (this)))
      return;
    in_error = false;
    memset (elts, 0, sizeof elts);

    if (hcs) {
      int i;
      for (i = 0; i < hcs->num; i++)
        memset (HB_SET_LEAF (hcs, i), 0, sizeof (hb_codepoint_leaf));
    }
  }

  // Search for the leaf containing the specified num.
  // Return it's index if it exists, otherwise return negative of where it
  // should be inserted + 1
  inline int find_leaf_forward (int start, int num) const {
    int *numbers = HB_SET_NUMBERS (hcs);
    int page;
    int low = start;
    int mid;
    int high = hcs->num - 1;

    if (!numbers)
      return -1;

    while (low <= high)
    {
      mid = (low + high) >> 1;
      page = numbers[mid];
      if (page == num)
        return mid;
      if (page < num)
        low = mid + 1;
      else
        high = mid - 1;
    }
    if (high < 0 || (high < hcs->num && numbers[high] < num))
      high++;

    return -(high + 1);
  }

  // Locate the leaf containing the specified char.
  // Return it's index or negative (same as find_forward) if it doesn't
  inline int find_leaf_pos (hb_codepoint_t g) const
  {
    return find_leaf_forward (0, g >> 8);
  }

  inline hb_codepoint_leaf * find_leaf (hb_codepoint_t g) const
  {
    int	pos = find_leaf_pos (g);
    if (pos >= 0)
      return HB_SET_LEAF (hcs, pos);

    return 0;
  }


  inline hb_codepoint_leaf * find_leaf_create (hb_codepoint_t g)
  {
    int pos;
    hb_codepoint_leaf *leaf;

    pos = find_leaf_pos (g);
    if (pos >= 0)
      return HB_SET_LEAF (hcs, pos);

    leaf = (hb_codepoint_leaf *) calloc (1, sizeof (hb_codepoint_leaf));
    if (!leaf)
      return NULL;

    pos = -pos - 1;
    if (!put_leaf (g, leaf, pos)) {
      free (leaf);
      return NULL;
    }

    return leaf;
  }

  inline hb_bool_t put_leaf (hb_codepoint_t g, hb_codepoint_leaf *leaf, int pos)
  {
    intptr_t *leaves = HB_SET_LEAVES (hcs);
    int *numbers = HB_SET_NUMBERS (hcs);

    g >>= 8;
    if (g >= 0x10000)
      return false;

    if (HB_IS_ZERO_OR_POWER_OF_TWO (hcs->num))
    {
      if (!hcs->num)
      {
        unsigned int alloced = 8;
        leaves = (intptr_t *) malloc (alloced * sizeof (*leaves));
        numbers = (int *) malloc (alloced * sizeof (*numbers));
      } else {
        unsigned int alloced = hcs->num;
	    intptr_t *new_leaves, distance;

	    alloced *= 2;
	    new_leaves = (intptr_t *) realloc (leaves, alloced * sizeof (*leaves));
	    numbers = (int *) realloc (numbers, alloced * sizeof (*numbers));

	    distance = (intptr_t) new_leaves - (intptr_t) leaves;
	    if (new_leaves && distance)
	    {
          int i;
          for (i = 0; i < hcs->num; i++)
            new_leaves[i] -= distance;
        }
        leaves = new_leaves;
      }

      if (!leaves || !numbers)
        return false;

      hcs->leaves_offset = PTR_TO_OFFSET (hcs, leaves);
      hcs->numbers_offset = PTR_TO_OFFSET (hcs, numbers);
    }

    memmove (leaves + pos + 1, leaves + pos,
	     (hcs->num - pos) * sizeof (*leaves));
    memmove (numbers + pos + 1, numbers + pos,
	     (hcs->num - pos) * sizeof (*numbers));
    numbers[pos] = (hb_codepoint_t) g;
    leaves[pos] = PTR_TO_OFFSET (leaves, leaf);
    hcs->num++;
    return true;
  }

  inline bool is_empty (void) const {
    for (unsigned int i = 0; i < hcs->num; i++)
      if (HB_SET_LEAF (hcs, i))
        return false;
    return true;
  }

  inline void add (hb_codepoint_t g) {
    if (unlikely (in_error)) return;
    if (unlikely (g == INVALID)) return;
    if (unlikely (g > MAX_G)) return;

    hb_codepoint_leaf *leaf;
    hb_codepoint_t *b;

    if (hcs == NULL)
      return;

    leaf = find_leaf_create (g);
    if (!leaf)
      return;
    b = &leaf->map[(g & 0xff) >> 5];
    *b |= (1 << (g & 0x1f));
  }

  inline void add_range (hb_codepoint_t a, hb_codepoint_t b) {
    if (unlikely (in_error)) return;
    /* TODO Speedup */
    for (unsigned int i = a; i < b + 1; i++)
      add (i);
  }

  inline void del (hb_codepoint_t g) {
    hb_codepoint_leaf *leaf;
    hb_codepoint_t	*b;

    if (hcs == NULL)
      return;

    leaf = find_leaf (g);
    if (!leaf)
      return;

    b = &leaf->map[(g & 0xff) >> 5];
    *b &= ~(1 << (g & 0x1f));
  }

  inline void del_range (hb_codepoint_t a, hb_codepoint_t b) {
    if (unlikely (in_error)) return;
    /* TODO Speedup */
    for (unsigned int i = a; i < b + 1; i++)
      del (i);
  }

  inline bool has (hb_codepoint_t g) const {
    hb_codepoint_leaf *leaf;

    if (!hcs)
      return false;

    leaf = find_leaf (g);
    if (!leaf)
      return false;

    return (leaf->map[(g & 0xff) >> 5] & (1 << (g & 0x1f))) != 0;
  }

  inline bool intersects (hb_codepoint_t first,
			  hb_codepoint_t last) const {
    if (unlikely (first > MAX_G)) return false;
    if (unlikely (last  > MAX_G)) last = MAX_G;
    unsigned int end = last + 1;
    for (hb_codepoint_t i = first; i < end; i++)
      if (has (i))
        return true;
    return false;
  }

  inline bool is_equal (const hb_set_t *other) const {
    for (unsigned int i = 0; i < ELTS; i++)
      if (elts[i] != other->elts[i])
        return false;
    return true;
  }

  inline void set (const hb_set_t *other) {
    if (unlikely (in_error)) return;
    for (unsigned int i = 0; i < ELTS; i++)
      elts[i] = other->elts[i];
  }

  inline void union_ (const hb_set_t *other) {
    if (unlikely (in_error)) return;
    for (unsigned int i = 0; i < ELTS; i++)
      elts[i] |= other->elts[i];
  }

  inline void intersect (const hb_set_t *other) {
    if (unlikely (in_error)) return;
    for (unsigned int i = 0; i < ELTS; i++)
      elts[i] &= other->elts[i];
  }

  inline void subtract (const hb_set_t *other) {
    if (unlikely (in_error)) return;
    for (unsigned int i = 0; i < ELTS; i++)
      elts[i] &= ~other->elts[i];
  }

  inline void symmetric_difference (const hb_set_t *other) {
    if (unlikely (in_error)) return;
    for (unsigned int i = 0; i < ELTS; i++)
      elts[i] ^= other->elts[i];
  }

  inline void invert (void) {
    // REMOVE?
  }

  inline bool next (hb_codepoint_t *codepoint) const {
    if (unlikely (*codepoint == INVALID)) {
      hb_codepoint_t i = get_min ();
      if (i != INVALID) {
        *codepoint = i;
        return true;
      } else {
        *codepoint = INVALID;
        return false;
      }
    }
    for (hb_codepoint_t i = *codepoint + 1; i < MAX_G + 1; i++)
      if (has (i)) {
        *codepoint = i;
        return true;
      }
    *codepoint = INVALID;
    return false;
  }

  inline bool next_range (hb_codepoint_t *first, hb_codepoint_t *last) const {
    hb_codepoint_t i;

    i = *last;
    if (!next (&i))
    {
      *last = *first = INVALID;
      return false;
    }

    *last = *first = i;
    while (next (&i) && i == *last + 1)
      (*last)++;

    return true;
  }

  inline unsigned int get_population (void) const {
    unsigned int count = 0;
    for (unsigned int i = 0; i < hcs->num; i++) {
      int j = 256/32;
      while (j--)
        count += _hb_popcount32 (HB_SET_LEAF (hcs, i)->map[j]);
    }
    return count;
  }

  inline hb_codepoint_t get_min (void) const {
    int map_size = 256/32;
    for (unsigned int i = 0; i < hcs->num; i++)
      for (unsigned int j = 0; j < map_size; j++)
        if (HB_SET_LEAF (hcs, i)->map[j])
          for (unsigned k = 0; k < BITS; k++)
            if (HB_SET_LEAF (hcs,i)->map[j] & (1 << k))
              return i * BITS + k;
    return INVALID;
  }

  inline hb_codepoint_t get_max (void) const {
    int map_size = 256/32;
    for (unsigned int i = hcs->num; i; i--)
      for (unsigned int j = map_size; j; j--)
        if (HB_SET_LEAF (hcs, i - 1)->map[j - 1])
          for (unsigned k = BITS; k; k--)
            if (HB_SET_LEAF (hcs, i - 1)->map[j - 1] & (1 << k))
              return (i - 1) * BITS + (k);
    return INVALID;
  }

  typedef uint32_t elt_t;
  static const unsigned int MAX_G = 65536 - 1; /* XXX Fix this... */
  static const unsigned int SHIFT = 5;
  static const unsigned int BITS = (1 << SHIFT);
  static const unsigned int MASK = BITS - 1;
  static const unsigned int ELTS = (MAX_G + 1) / BITS;
  static  const hb_codepoint_t INVALID = HB_SET_VALUE_INVALID;

  elt_t elts[ELTS]; /* XXX 8kb */

  ASSERT_STATIC (sizeof (elt_t) * 8 == BITS);
  ASSERT_STATIC (sizeof (elt_t) * 8 * ELTS > MAX_G);

  hb_codepoint_set * hcs;
};



#endif /* HB_SET_PRIVATE_HH */
