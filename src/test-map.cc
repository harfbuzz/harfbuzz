/*
 * Copyright © 2021  Behdad Esfahbod
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
 */

#include "hb.hh"
#include "hb-map.hh"
#include "hb-set.hh"
#include <string>

int
main (int argc, char **argv)
{

  /* Test copy constructor. */
  {
    hb_map_t v1;
    v1.set (1, 2);
    hb_map_t v2 {v1};
    assert (v1.get_population () == 1);
    assert (v2.get_population () == 1);
    assert (v1[1] == 2);
    assert (v2[1] == 2);
  }

  /* Test copy assignment. */
  {
    hb_map_t v1;
    v1.set (1, 2);
    hb_map_t v2 = v1;
    assert (v1.get_population () == 1);
    assert (v2.get_population () == 1);
    assert (v1[1] == 2);
    assert (v2[1] == 2);
  }

  /* Test move constructor. */
  {
    hb_map_t s {};
    s.set (1, 2);
    hb_map_t v (std::move (s));
    assert (s.get_population () == 0);
    assert (v.get_population () == 1);
  }

  /* Test move assignment. */
  {
    hb_map_t s {};
    s.set (1, 2);
    hb_map_t v;
    v = std::move (s);
    assert (s.get_population () == 0);
    assert (v.get_population () == 1);
  }

  /* Test initializing from iterable. */
  {
    hb_map_t s;

    s.set (1, 2);
    s.set (3, 4);

    hb_vector_t<hb_pair_t<hb_codepoint_t, hb_codepoint_t>> v (s);
    hb_map_t v0 (v);
    hb_map_t v1 (s);
    hb_map_t v2 (std::move (s));

    assert (s.get_population () == 0);
    assert (v0.get_population () == 2);
    assert (v1.get_population () == 2);
    assert (v2.get_population () == 2);
  }

  /* Test call fini() twice. */
  {
    hb_map_t s;
    for (int i = 0; i < 16; i++)
      s.set(i, i+1);
    s.fini();
  }

  /* Test initializing from iterator. */
  {
    hb_map_t s;

    s.set (1, 2);
    s.set (3, 4);

    hb_map_t v (hb_iter (s));

    assert (v.get_population () == 2);
  }

  /* Test initializing from initializer list and swapping. */
  {
    using pair_t = hb_pair_t<hb_codepoint_t, hb_codepoint_t>;
    hb_map_t v1 {pair_t{1,2}, pair_t{4,5}};
    hb_map_t v2 {pair_t{3,4}};
    hb_swap (v1, v2);
    assert (v1.get_population () == 1);
    assert (v2.get_population () == 2);
  }

  /* Test class key / value types. */
  {
    hb_hashmap_t<hb_bytes_t, int> m1;
    hb_hashmap_t<int, hb_bytes_t> m2;
    hb_hashmap_t<hb_bytes_t, hb_bytes_t> m3;
    assert (m1.get_population () == 0);
    assert (m2.get_population () == 0);
    assert (m3.get_population () == 0);
  }

  {
    hb_hashmap_t<int, int> m0;
    hb_hashmap_t<std::string, int> m1;
    hb_hashmap_t<int, std::string> m2;
    hb_hashmap_t<std::string, std::string> m3;

    std::string s;
    for (unsigned i = 1; i < 1000; i++)
    {
      s += "x";
      m0.set (i, i);
      m1.set (s, i);
      m2.set (i, s);
      m3.set (s, s);
    }
  }

  /* Test hashing maps. */
  {
    using pair = hb_pair_t<hb_codepoint_t, hb_codepoint_t>;

    hb_hashmap_t<hb_map_t, hb_map_t> m1;

    m1.set (hb_map_t (), hb_map_t {});
    m1.set (hb_map_t (), hb_map_t {pair (1u, 2u)});
    m1.set (hb_map_t {pair (1u, 2u)}, hb_map_t {pair (2u, 3u)});

    assert (m1.get (hb_map_t ()) == hb_map_t {pair (1u, 2u)});
    assert (m1.get (hb_map_t {pair (1u, 2u)}) == hb_map_t {pair (2u, 3u)});
  }

  /* Test hashing sets. */
  {
    hb_hashmap_t<hb_set_t, hb_set_t> m1;

    m1.set (hb_set_t (), hb_set_t ());
    m1.set (hb_set_t (), hb_set_t {1});
    m1.set (hb_set_t {1, 1000}, hb_set_t {2});

    assert (m1.get (hb_set_t ()) == hb_set_t {1});
    assert (m1.get (hb_set_t {1000, 1}) == hb_set_t {2});
  }

  /* Test hashing vectors. */
  {
    using vector_t = hb_vector_t<unsigned>;

    hb_hashmap_t<vector_t, vector_t> m1;

    m1.set (vector_t (), vector_t ());
    m1.set (vector_t (), vector_t {1});
    m1.set (vector_t {1}, vector_t {2});

    assert (m1.get (vector_t ()) == vector_t {1});
    assert (m1.get (vector_t {1}) == vector_t {2});
  }

  /* Test hb::shared_ptr. */
  hb_hash (hb::shared_ptr<hb_set_t> ());
  {
    hb_hashmap_t<hb::shared_ptr<hb_set_t>, hb::shared_ptr<hb_set_t>> m;

    m.get (hb::shared_ptr<hb_set_t> ());
    m.get (hb::shared_ptr<hb_set_t> (hb_set_get_empty ()));
    m.iter ();
    m.keys ();
    m.values ();
    m.iter_ref ();
    m.keys_ref ();
    m.values_ref ();
  }
  /* Test hb::unique_ptr. */
  hb_hash (hb::unique_ptr<hb_set_t> ());
  {
    hb_hashmap_t<hb::unique_ptr<hb_set_t>, hb::unique_ptr<hb_set_t>> m;

    m.get (hb::unique_ptr<hb_set_t> ());
    m.get (hb::unique_ptr<hb_set_t> (hb_set_get_empty ()));
    m.iter_ref ();
    m.keys_ref ();
    m.values_ref ();
  }
  /* Test more complex unique_ptr's. */
  {
    hb_hashmap_t<int, hb::unique_ptr<hb_hashmap_t<int, int>>> m;

    m.get (0);
    const hb::unique_ptr<hb_hashmap_t<int, int>> *v1;
    m.has (0, &v1);
    hb::unique_ptr<hb_hashmap_t<int, int>> *v2;
    m.has (0, &v2);
  }

  return 0;
}
