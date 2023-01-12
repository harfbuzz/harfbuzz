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
 * Google Author(s): Garret Rieger, Roderick Sheeter
 */

#ifndef HB_SUBSET_INPUT_HH
#define HB_SUBSET_INPUT_HH


#include "hb.hh"

#include "hb-subset.h"
#include "hb-map.hh"
#include "hb-set.hh"
#include "hb-cplusplus.hh"
#include "hb-font.hh"

struct hb_ot_name_record_ids_t
{
  hb_ot_name_record_ids_t () = default;
  hb_ot_name_record_ids_t (unsigned platform_id_,
                           unsigned encoding_id_,
                           unsigned language_id_,
                           unsigned name_id_)
      :platform_id (platform_id_),
      encoding_id (encoding_id_),
      language_id (language_id_),
      name_id (name_id_) {}

  bool operator != (const hb_ot_name_record_ids_t o) const
  { return !(*this == o); }

  inline bool operator == (const hb_ot_name_record_ids_t& o) const
  {
    return platform_id == o.platform_id &&
           encoding_id == o.encoding_id &&
           language_id == o.language_id &&
           name_id == o.name_id;
  }

  inline uint32_t hash () const
  {
    uint32_t current = 0;
    current = current * 31 + hb_hash (platform_id);
    current = current * 31 + hb_hash (encoding_id);
    current = current * 31 + hb_hash (language_id);
    current = current * 31 + hb_hash (name_id);
    return current;
  }

  unsigned platform_id;
  unsigned encoding_id;
  unsigned language_id;
  unsigned name_id;
};

typedef struct hb_ot_name_record_ids_t hb_ot_name_record_ids_t;


HB_MARK_AS_FLAG_T (hb_subset_flags_t);

struct hb_subset_input_t
{
  hb_subset_input_t ()
  {
    for (auto& set : sets_iter ())
      set = hb::shared_ptr<hb_set_t> (hb_set_create ());

    if (in_error ())
      return;

    flags = HB_SUBSET_FLAGS_DEFAULT;

    hb_set_add_range (sets.name_ids, 0, 6);
    hb_set_add (sets.name_languages, 0x0409);

    hb_tag_t default_drop_tables[] = {
      // Layout disabled by default
      HB_TAG ('m', 'o', 'r', 'x'),
      HB_TAG ('m', 'o', 'r', 't'),
      HB_TAG ('k', 'e', 'r', 'x'),
      HB_TAG ('k', 'e', 'r', 'n'),

      // Copied from fontTools:
      HB_TAG ('B', 'A', 'S', 'E'),
      HB_TAG ('J', 'S', 'T', 'F'),
      HB_TAG ('D', 'S', 'I', 'G'),
      HB_TAG ('E', 'B', 'D', 'T'),
      HB_TAG ('E', 'B', 'L', 'C'),
      HB_TAG ('E', 'B', 'S', 'C'),
      HB_TAG ('S', 'V', 'G', ' '),
      HB_TAG ('P', 'C', 'L', 'T'),
      HB_TAG ('L', 'T', 'S', 'H'),
      // Graphite tables
      HB_TAG ('F', 'e', 'a', 't'),
      HB_TAG ('G', 'l', 'a', 't'),
      HB_TAG ('G', 'l', 'o', 'c'),
      HB_TAG ('S', 'i', 'l', 'f'),
      HB_TAG ('S', 'i', 'l', 'l'),
    };
    sets.drop_tables->add_array (default_drop_tables, ARRAY_LENGTH (default_drop_tables));

    hb_tag_t default_no_subset_tables[] = {
      HB_TAG ('a', 'v', 'a', 'r'),
      HB_TAG ('g', 'a', 's', 'p'),
      HB_TAG ('c', 'v', 't', ' '),
      HB_TAG ('f', 'p', 'g', 'm'),
      HB_TAG ('p', 'r', 'e', 'p'),
      HB_TAG ('V', 'D', 'M', 'X'),
      HB_TAG ('D', 'S', 'I', 'G'),
      HB_TAG ('M', 'V', 'A', 'R'),
      HB_TAG ('c', 'v', 'a', 'r'),
    };
    sets.no_subset_tables->add_array (default_no_subset_tables,
					   ARRAY_LENGTH (default_no_subset_tables));

    //copied from _layout_features_groups in fonttools
    hb_tag_t default_layout_features[] = {
      // default shaper
      // common
      HB_TAG ('r', 'v', 'r', 'n'),
      HB_TAG ('c', 'c', 'm', 'p'),
      HB_TAG ('l', 'i', 'g', 'a'),
      HB_TAG ('l', 'o', 'c', 'l'),
      HB_TAG ('m', 'a', 'r', 'k'),
      HB_TAG ('m', 'k', 'm', 'k'),
      HB_TAG ('r', 'l', 'i', 'g'),

      //fractions
      HB_TAG ('f', 'r', 'a', 'c'),
      HB_TAG ('n', 'u', 'm', 'r'),
      HB_TAG ('d', 'n', 'o', 'm'),

      //horizontal
      HB_TAG ('c', 'a', 'l', 't'),
      HB_TAG ('c', 'l', 'i', 'g'),
      HB_TAG ('c', 'u', 'r', 's'),
      HB_TAG ('k', 'e', 'r', 'n'),
      HB_TAG ('r', 'c', 'l', 't'),

      //vertical
      HB_TAG ('v', 'a', 'l', 't'),
      HB_TAG ('v', 'e', 'r', 't'),
      HB_TAG ('v', 'k', 'r', 'n'),
      HB_TAG ('v', 'p', 'a', 'l'),
      HB_TAG ('v', 'r', 't', '2'),

      //ltr
      HB_TAG ('l', 't', 'r', 'a'),
      HB_TAG ('l', 't', 'r', 'm'),

      //rtl
      HB_TAG ('r', 't', 'l', 'a'),
      HB_TAG ('r', 't', 'l', 'm'),

      //random
      HB_TAG ('r', 'a', 'n', 'd'),

      //justify
      HB_TAG ('j', 'a', 'l', 't'), // HarfBuzz doesn't use; others might

      //private
      HB_TAG ('H', 'a', 'r', 'f'),
      HB_TAG ('H', 'A', 'R', 'F'),
      HB_TAG ('B', 'u', 'z', 'z'),
      HB_TAG ('B', 'U', 'Z', 'Z'),

      //shapers

      //arabic
      HB_TAG ('i', 'n', 'i', 't'),
      HB_TAG ('m', 'e', 'd', 'i'),
      HB_TAG ('f', 'i', 'n', 'a'),
      HB_TAG ('i', 's', 'o', 'l'),
      HB_TAG ('m', 'e', 'd', '2'),
      HB_TAG ('f', 'i', 'n', '2'),
      HB_TAG ('f', 'i', 'n', '3'),
      HB_TAG ('c', 's', 'w', 'h'),
      HB_TAG ('m', 's', 'e', 't'),
      HB_TAG ('s', 't', 'c', 'h'),

      //hangul
      HB_TAG ('l', 'j', 'm', 'o'),
      HB_TAG ('v', 'j', 'm', 'o'),
      HB_TAG ('t', 'j', 'm', 'o'),

      //tibetan
      HB_TAG ('a', 'b', 'v', 's'),
      HB_TAG ('b', 'l', 'w', 's'),
      HB_TAG ('a', 'b', 'v', 'm'),
      HB_TAG ('b', 'l', 'w', 'm'),

      //indic
      HB_TAG ('n', 'u', 'k', 't'),
      HB_TAG ('a', 'k', 'h', 'n'),
      HB_TAG ('r', 'p', 'h', 'f'),
      HB_TAG ('r', 'k', 'r', 'f'),
      HB_TAG ('p', 'r', 'e', 'f'),
      HB_TAG ('b', 'l', 'w', 'f'),
      HB_TAG ('h', 'a', 'l', 'f'),
      HB_TAG ('a', 'b', 'v', 'f'),
      HB_TAG ('p', 's', 't', 'f'),
      HB_TAG ('c', 'f', 'a', 'r'),
      HB_TAG ('v', 'a', 't', 'u'),
      HB_TAG ('c', 'j', 'c', 't'),
      HB_TAG ('i', 'n', 'i', 't'),
      HB_TAG ('p', 'r', 'e', 's'),
      HB_TAG ('a', 'b', 'v', 's'),
      HB_TAG ('b', 'l', 'w', 's'),
      HB_TAG ('p', 's', 't', 's'),
      HB_TAG ('h', 'a', 'l', 'n'),
      HB_TAG ('d', 'i', 's', 't'),
      HB_TAG ('a', 'b', 'v', 'm'),
      HB_TAG ('b', 'l', 'w', 'm'),
    };

    sets.layout_features->add_array (default_layout_features, ARRAY_LENGTH (default_layout_features));

    sets.layout_scripts->invert (); // Default to all scripts.
  }

  ~hb_subset_input_t ()
  {
#ifdef HB_EXPERIMENTAL_API
    for (auto _ : name_table_overrides)
      _.second.fini ();
#endif
  }

  hb_object_header_t header;

  struct sets_t {
    hb::shared_ptr<hb_set_t> glyphs;
    hb::shared_ptr<hb_set_t> unicodes;
    hb::shared_ptr<hb_set_t> no_subset_tables;
    hb::shared_ptr<hb_set_t> drop_tables;
    hb::shared_ptr<hb_set_t> name_ids;
    hb::shared_ptr<hb_set_t> name_languages;
    hb::shared_ptr<hb_set_t> layout_features;
    hb::shared_ptr<hb_set_t> layout_scripts;
  };

  union {
    sets_t sets;
    hb::shared_ptr<hb_set_t> set_ptrs[sizeof (sets_t) / sizeof (hb_set_t*)];
  };

  unsigned flags;
  bool attach_accelerator_data = false;

  // If set loca format will always be the long version.
  bool force_long_loca = false;

  hb_hashmap_t<hb_tag_t, float> axes_location;
#ifdef HB_EXPERIMENTAL_API
  hb_hashmap_t<hb_ot_name_record_ids_t, hb_bytes_t> name_table_overrides;
#endif

  inline unsigned num_sets () const
  {
    return sizeof (set_ptrs) / sizeof (hb_set_t*);
  }

  inline hb_array_t<hb::shared_ptr<hb_set_t>> sets_iter ()
  {
    return hb_array (set_ptrs);
  }

  bool in_error () const
  {
    for (unsigned i = 0; i < num_sets (); i++)
    {
      if (unlikely (set_ptrs[i]->in_error ()))
        return true;
    }

    return axes_location.in_error ()
#ifdef HB_EXPERIMENTAL_API
	|| name_table_overrides.in_error ()
#endif
	;
  }
};


#endif /* HB_SUBSET_INPUT_HH */
