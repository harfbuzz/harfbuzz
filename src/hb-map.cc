/*
 * Copyright © 2018  Google, Inc.
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

#include "hb-map-private.hh"


/* Public API */


/**
 * hb_map_create: (Xconstructor)
 *
 * Return value: (transfer full):
 *
 * Since: REPLACEME
 **/
hb_map_t *
hb_map_create (void)
{
  hb_map_t *map;

  if (!(map = hb_object_create<hb_map_t> ()))
    return hb_map_get_empty ();

  map->init_shallow ();

  return map;
}

static const hb_map_t _hb_map_nil = {
  HB_OBJECT_HEADER_STATIC,
  true, /* in_error */
  0, /* population */
  0, /* occupancy */
  0, /* mask */
  0, /* prime */
  nullptr /* items */
};

/**
 * hb_map_get_empty:
 *
 * Return value: (transfer full):
 *
 * Since: REPLACEME
 **/
hb_map_t *
hb_map_get_empty (void)
{
  return const_cast<hb_map_t *> (&_hb_map_nil);
}

/**
 * hb_map_reference: (skip)
 * @map: a map.
 *
 * Return value: (transfer full):
 *
 * Since: REPLACEME
 **/
hb_map_t *
hb_map_reference (hb_map_t *map)
{
  return hb_object_reference (map);
}

/**
 * hb_map_destroy: (skip)
 * @map: a map.
 *
 * Since: REPLACEME
 **/
void
hb_map_destroy (hb_map_t *map)
{
  if (!hb_object_destroy (map)) return;

  map->fini_shallow ();

  free (map);
}

/**
 * hb_map_set_user_data: (skip)
 * @map: a map.
 * @key:
 * @data:
 * @destroy:
 * @replace:
 *
 * Return value:
 *
 * Since: REPLACEME
 **/
hb_bool_t
hb_map_set_user_data (hb_map_t           *map,
		      hb_user_data_key_t *key,
		      void *              data,
		      hb_destroy_func_t   destroy,
		      hb_bool_t           replace)
{
  return hb_object_set_user_data (map, key, data, destroy, replace);
}

/**
 * hb_map_get_user_data: (skip)
 * @map: a map.
 * @key:
 *
 * Return value: (transfer none):
 *
 * Since: REPLACEME
 **/
void *
hb_map_get_user_data (hb_map_t           *map,
		      hb_user_data_key_t *key)
{
  return hb_object_get_user_data (map, key);
}


/**
 * hb_map_allocation_successful:
 * @map: a map.
 *
 *
 *
 * Return value:
 *
 * Since: REPLACEME
 **/
hb_bool_t
hb_map_allocation_successful (const hb_map_t  *map)
{
  return !map->in_error;
}


/**
 * hb_map_set:
 * @map: a map.
 * @key:
 * @value:
 *
 *
 *
 * Return value:
 *
 * Since: REPLACEME
 **/
void
hb_map_set (hb_map_t       *map,
	    hb_codepoint_t  key,
	    hb_codepoint_t  value)
{
  map->set (key, value);
}

/**
 * hb_map_get:
 * @map: a map.
 * @key:
 *
 *
 *
 * Since: REPLACEME
 **/
hb_codepoint_t
hb_map_get (const hb_map_t *map,
	    hb_codepoint_t  key)
{
  return map->get (key);
}

/**
 * hb_map_del:
 * @map: a map.
 * @codepoint:
 *
 *
 *
 * Since: REPLACEME
 **/
void
hb_map_del (hb_map_t       *map,
	    hb_codepoint_t  key)
{
  map->del (key);
}

/**
 * hb_map_has:
 * @map: a map.
 * @codepoint:
 *
 *
 *
 * Since: REPLACEME
 **/
bool
hb_map_has (const hb_map_t *map,
	    hb_codepoint_t  key)
{
  return map->has (key);
}


/* Following comment and table copied from glib. */
/* Each table size has an associated prime modulo (the first prime
 * lower than the table size) used to find the initial bucket. Probing
 * then works modulo 2^n. The prime modulo is necessary to get a
 * good distribution with poor hash functions.
 */
static const unsigned int prime_mod [] =
{
  1,          /* For 1 << 0 */
  2,
  3,
  7,
  13,
  31,
  61,
  127,
  251,
  509,
  1021,
  2039,
  4093,
  8191,
  16381,
  32749,
  65521,      /* For 1 << 16 */
  131071,
  262139,
  524287,
  1048573,
  2097143,
  4194301,
  8388593,
  16777213,
  33554393,
  67108859,
  134217689,
  268435399,
  536870909,
  1073741789,
  2147483647  /* For 1 << 31 */
};

unsigned int
hb_map_t::prime_for (unsigned int shift)
{
  if (unlikely (shift >= ARRAY_LENGTH (prime_mod)))
    return prime_mod[ARRAY_LENGTH (prime_mod) -1];

  return prime_mod[shift];
}
