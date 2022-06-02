/*
 * Copyright © 2022 Behdad Esfahbod
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

#ifndef HB_CPLUSPLUS_HH
#define HB_CPLUSPLUS_HH

#include "hb.h"

HB_BEGIN_DECLS
HB_END_DECLS

#ifdef __cplusplus

#include <functional>
#include <utility>

#if 0
#if !(__cplusplus >= 201103L)
#error "HarfBuzz C++ helpers require C++11"
#endif
#endif

namespace hb {


template <typename T>
struct vtable;

template <typename T>
struct shared_ptr
{
  using element_type = T;

  using v = vtable<T>;

  explicit shared_ptr (T *p = nullptr) : p (p) {}
  shared_ptr (const shared_ptr &o) : p (v::reference (o.p)) {}
  shared_ptr (shared_ptr &&o) : p (o.p) { o.p = nullptr; }
  shared_ptr& operator = (const shared_ptr &o) { if (p != o.p) { destroy (); p = o.p; reference (); } return *this; }
  shared_ptr& operator = (shared_ptr &&o) { destroy (); p = o.p; o.p = nullptr; return *this; }
  ~shared_ptr () { v::destroy (p); }

  T* get() const { return p; }

  void swap (shared_ptr &o) { std::swap (p, o.p); }
  friend void swap (shared_ptr &a, shared_ptr &b) { std::swap (a.p, b.p); }

  operator T * () const { return p; }
  T& operator * () const { return *get (); }
  T* operator -> () const { return get (); }
  operator bool () { return p; }
  bool operator == (const shared_ptr &o) { return p == o.p; }
  bool operator != (const shared_ptr &o) { return p != o.p; }

  static T* get_empty() { return v::get_empty (); }
  void reference() { v::reference (p); }
  void destroy() { v::destroy (p); }
  void set_user_data (hb_user_data_key_t *key,
		      void *value,
		      hb_destroy_func_t destroy,
		      hb_bool_t replace) { v::set_user_data (p, key, value, destroy, replace); } 
  void * get_user_data (hb_user_data_key_t *key) { return v::get_user_data (p, key); }

  private:
  T *p;
};

template<typename T> struct is_shared_ptr : std::false_type {};
template<typename T> struct is_shared_ptr<shared_ptr<T>> : std::true_type {};

template <typename T,
	  T * (*_get_empty) (void),
	  T * (*_reference) (T *),
	  void (*_destroy) (T *),
	  hb_bool_t (*_set_user_data) (T *,
				       hb_user_data_key_t *,
				       void *,
				       hb_destroy_func_t,
				       hb_bool_t),
	  void * (*_get_user_data) (T *,
				    hb_user_data_key_t *)>
struct vtable_t
{
  static constexpr auto get_empty = _get_empty;
  static constexpr auto reference = _reference;
  static constexpr auto destroy = _destroy;
  static constexpr auto set_user_data = _set_user_data;
  static constexpr auto get_user_data = _get_user_data;
};

#define HB_DEFINE_VTABLE(name) \
	template<> \
	struct vtable<hb_##name##_t> \
	     : vtable_t<hb_##name##_t, \
			&hb_##name##_get_empty, \
			&hb_##name##_reference, \
			&hb_##name##_destroy, \
			&hb_##name##_set_user_data, \
			&hb_##name##_get_user_data> {}

HB_DEFINE_VTABLE (buffer);
HB_DEFINE_VTABLE (blob);
HB_DEFINE_VTABLE (face);
HB_DEFINE_VTABLE (font);
HB_DEFINE_VTABLE (font_funcs);
HB_DEFINE_VTABLE (map);
HB_DEFINE_VTABLE (set);
HB_DEFINE_VTABLE (shape_plan);
HB_DEFINE_VTABLE (unicode_funcs);

#undef HB_DEFINE_VTABLE


} // namespace hb

template<typename T>
struct std::hash<hb::shared_ptr<T>>
{
    std::size_t operator()(const hb::shared_ptr<T>& v) const noexcept
    {
        std::size_t h = std::hash<decltype (v.get ())>{}(v.get ());
        return h;
    }
};


#endif /* __cplusplus */

#endif /* HB_CPLUSPLUS_HH */
