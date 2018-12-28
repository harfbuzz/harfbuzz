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

#ifndef HB_ITER_HH
#define HB_ITER_HH

#include "hb.hh"
#include "hb-dsalgs.hh" // for hb_addressof
#include "hb-meta.hh"
#include "hb-null.hh"


/* Unified iterator object.
 *
 * The goal of this template is to make the same iterator interface
 * available to all types, and make it very easy and compact to use.
 * hb_iter_tator objects are small, light-weight, objects that can be
 * copied by value.  If the collection / object being iterated on
 * is writable, then the iterator returns lvalues, otherwise it
 * returns rvalues.
 */


/*
 * Base classes for iterators.
 */

/* Base class for all iterators. */
template <typename Iter, typename Item = typename Iter::__item_type__>
struct hb_iter_t
{
  typedef Iter iter_t;
  typedef Item item_t;
  enum { item_size = hb_static_size (Item) };
  enum { is_iter = true };

  private:
  /* https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern */
  const iter_t* thiz () const { return static_cast<const iter_t *> (this); }
        iter_t* thiz ()       { return static_cast<      iter_t *> (this); }
  public:

  /* Operators. */
  iter_t iter () const { return *thiz(); }
  explicit_operator bool () const { return thiz()->__more__ (); }
  unsigned len () const { return thiz()->__len__ (); }
  /* TODO enable_if item_t is reference type only. */
  typename hb_remove_reference (item_t)* operator -> () const { return hb_addressof (*thiz()); }
  item_t operator * () const { return thiz()->__item__ (); }
  item_t operator [] (unsigned i) const { return thiz()->__item_at__ (i); }
  iter_t& operator += (unsigned count) { thiz()->__forward__ (count); return *thiz(); }
  iter_t& operator ++ () { thiz()->__next__ (); return *thiz(); }
  iter_t& operator -= (unsigned count) { thiz()->__rewind__ (count); return *thiz(); }
  iter_t& operator -- () { thiz()->__prev__ (); return *thiz(); }
  iter_t operator + (unsigned count) const { iter_t c (*thiz()); c += count; return c; }
  friend iter_t operator + (unsigned count, const iter_t &it) { return it + count; }
  iter_t operator ++ (int) { iter_t c (*thiz()); ++*thiz(); return c; }
  iter_t operator - (unsigned count) const { iter_t c (*thiz()); c -= count; return c; }
  iter_t operator -- (int) { iter_t c (*thiz()); --*thiz(); return c; }
  constexpr bool is_random_access () const { return thiz()->__random_access__ (); }

  protected:
  hb_iter_t () {}
  hb_iter_t (const hb_iter_t &o HB_UNUSED) {}
  void operator = (const hb_iter_t &o HB_UNUSED) {}
};

#define HB_ITER_USING(Name) \
  using typename Name::iter_t; \
  using typename Name::item_t; \
  using Name::item_size; \
  using Name::is_iter; \
  using Name::iter; \
  using Name::operator bool; \
  using Name::len; \
  using Name::operator ->; \
  using Name::operator *; \
  using Name::operator []; \
  using Name::operator +=; \
  using Name::operator ++; \
  using Name::operator -=; \
  using Name::operator --; \
  using Name::operator +; \
  using Name::operator -; \
  using Name::is_random_access; \
  static_assert (true, "")

/* Base class for sorted iterators.  Does not enforce anything.
 * Just for class taxonomy and requirements. */
template <typename Iter, typename Item = typename Iter::__item_type__>
struct hb_sorted_iter_t : hb_iter_t<Iter, Item>
{
  protected:
  hb_sorted_iter_t () {}
  hb_sorted_iter_t (const hb_sorted_iter_t &o) : hb_iter_t<Iter, Item> (o) {}
  void operator = (const hb_sorted_iter_t &o HB_UNUSED) {}
};

/* Mixin to fill in what the subclass doesn't provide. */
template <typename iter_t, typename item_t = typename iter_t::__item_type__>
struct hb_iter_mixin_t
{
  private:
  /* https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern */
  const iter_t* thiz () const { return static_cast<const iter_t *> (this); }
        iter_t* thiz ()       { return static_cast<      iter_t *> (this); }
  public:

  /* Access: Implement __item__(), or __item_at__() if random-access. */
  item_t __item__ () const { return (*thiz())[0]; }
  item_t __item_at__ (unsigned i) const { return *(*thiz() + i); }

  /* Termination: Implement __more__(), or __len__() if random-access. */
  bool __more__ () const { return thiz()->len (); }
  unsigned __len__ () const
  { iter_t c (*thiz()); unsigned l = 0; while (c) { c++; l++; }; return l; }

  /* Advancing: Implement __next__(), or __forward__() if random-access. */
  void __next__ () { *thiz() += 1; }
  void __forward__ (unsigned n) { while (n--) ++*thiz(); }

  /* Rewinding: Implement __prev__() or __rewind__() if bidirectional. */
  void __prev__ () { *thiz() -= 1; }
  void __rewind__ (unsigned n) { while (n--) --*thiz(); }

  /* Random access: Implement if __item_at__(), __len__(), __forward__() are. */
  constexpr bool __random_access__ () const { return false; }

  protected:
  hb_iter_mixin_t () {}
  hb_iter_mixin_t (const hb_iter_mixin_t &o HB_UNUSED) {}
  void operator = (const hb_iter_mixin_t &o HB_UNUSED) {}
};

/*
 * Meta-programming predicates.
 */

/* hb_is_iterable() */

template<typename T, typename B>
struct _hb_is_iterable
{ enum { value = false }; };
template<typename T>
struct _hb_is_iterable<T, hb_bool_tt<true || sizeof (hb_declval<T> ().iter ())> >
{ enum { value = true }; };

template<typename T>
struct hb_is_iterable { enum { value = _hb_is_iterable<T, hb_true_t>::value }; };
#define hb_is_iterable(Iterable) hb_is_iterable<Iterable>::value


/* hb_is_iterator() */

/* The following SFINAE fails to match template parameters to hb_iter_t<>.
 * As such, just check for member is_iter being there. */
# if 0
template<typename T = void> char
_hb_is_iterator (T *) {};
template<typename Iter, typename Item> int
_hb_is_iterator (hb_iter_t<Iter, Item> *) {};
static_assert (sizeof (char) != sizeof (int), "");

template<typename T>
struct hb_is_iterator { enum {
  value = sizeof (int) == sizeof (_hb_is_iterator (hb_declval<T*> ()))
}; };
#endif

template<typename T, typename B>
struct _hb_is_iterator
{ enum { value = false }; };
template<typename T>
struct _hb_is_iterator<T, hb_bool_tt<true || sizeof (T::is_iter)> >
{ enum { value = true }; };

template<typename T>
struct hb_is_iterator { enum { value = _hb_is_iterator<T, hb_true_t>::value }; };
#define hb_is_iterator(Iterator) hb_is_iterator<Iterator>::value


/*
 * Algorithms operating on iterators or iteratables.
 */

template <typename C, typename V> inline void
hb_fill (const C& c, const V &v)
{
  for (typename C::iter_t i (c); i; i++)
    hb_assign (*i, v);
}

template <typename S, typename D> inline bool
hb_copy (hb_iter_t<D> &id, hb_iter_t<S> &is)
{
  for (; id && is; ++id, ++is)
    *id = *is;
  return !is;
}


#endif /* HB_ITER_HH */
