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

#ifndef HB_META_HH
#define HB_META_HH

#include "hb.hh"


/*
 * C++ template meta-programming & fundamentals used with them.
 */

/* Void!  For when we need a expression-type of void. */
struct hb_void_t { typedef void value; };

/* Void meta-function ala std::void_t
 * https://en.cppreference.com/w/cpp/types/void_t */
template<typename... Ts> struct _hb_void_tt { typedef void type; };
template<typename... Ts> using hb_void_tt = typename _hb_void_tt<Ts...>::type;

template<typename Head, typename... Ts> struct _hb_head_tt { typedef Head type; };
template<typename... Ts> using hb_head_tt = typename _hb_head_tt<Ts...>::type;

/* Bool!  For when we need to evaluate type-dependent expressions
 * in a template argument. */
template <bool b> struct hb_bool_tt { enum { value = b }; };
typedef hb_bool_tt<true> hb_true_t;
typedef hb_bool_tt<false> hb_false_t;


/* Function overloading SFINAE and priority. */

#define HB_RETURN(Ret, E) -> hb_head_tt<Ret, decltype ((E))> { return (E); }
#define HB_AUTO_RETURN(E) -> decltype ((E)) { return (E); }
#define HB_VOID_RETURN(E) -> hb_void_tt<decltype ((E))> { (E); }

template <unsigned Pri> struct hb_priority : hb_priority<Pri - 1> {};
template <>             struct hb_priority<0> {};
#define hb_prioritize hb_priority<16> ()

#define HB_FUNCOBJ(x) static_const x HB_UNUSED


struct
{
  template <typename T>
  T* operator () (const T& arg) const
  {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
    /* https://en.cppreference.com/w/cpp/memory/addressof */
    return reinterpret_cast<T*> (
	     &const_cast<char&> (
		reinterpret_cast<const volatile char&> (arg)));
#pragma GCC diagnostic pop
  }
} HB_FUNCOBJ (hb_addressof);

template <typename T> static inline T hb_declval ();
#define hb_declval(T) (hb_declval<T> ())

template <typename T> struct hb_match_const { typedef T type; enum { value = false }; };
template <typename T> struct hb_match_const<const T> { typedef T type; enum { value = true }; };
template <typename T> using hb_remove_const = typename hb_match_const<T>::type;
#define hb_is_const(T) hb_match_const<T>::value
template <typename T> struct hb_match_reference { typedef T type; enum { value = false }; };
template <typename T> struct hb_match_reference<T &> { typedef T type; enum { value = true }; };
template <typename T> using hb_remove_reference = typename hb_match_reference<T>::type;
#define hb_is_reference(T) hb_match_reference<T>::value
template <typename T> struct hb_match_pointer { typedef T type; enum { value = false }; };
template <typename T> struct hb_match_pointer<T *> { typedef T type; enum { value = true }; };
template <typename T> using hb_remove_pointer = typename hb_match_pointer<T>::type;
#define hb_is_pointer(T) hb_match_pointer<T>::value


/* std::move and std::forward */

template <typename T>
static hb_remove_reference<T>&& hb_move (T&& t) { return (hb_remove_reference<T>&&) (t); }

template <typename T>
static T&& hb_forward (hb_remove_reference<T>& t) { return (T&&) t; }
template <typename T>
static T&& hb_forward (hb_remove_reference<T>&& t) { return (T&&) t; }

struct
{
  template <typename T> auto
  operator () (T&& v) const HB_AUTO_RETURN (hb_forward<T> (v))

  template <typename T> auto
  operator () (T *v) const HB_AUTO_RETURN (*v)

} HB_FUNCOBJ (hb_deref_pointer);


template<bool B, typename T = void> struct hb_enable_if {};
template<typename T>                struct hb_enable_if<true, T> { typedef T type; };
#define hb_enable_if(Cond) typename hb_enable_if<(Cond)>::type* = nullptr

template <typename T, typename T2> struct hb_is_same : hb_false_t {};
template <typename T>              struct hb_is_same<T, T> : hb_true_t {};
#define hb_is_same(T, T2) hb_is_same<T, T2>::value

template <typename T> struct hb_is_signed;
/* https://github.com/harfbuzz/harfbuzz/issues/1535 */
template <> struct hb_is_signed<int8_t> { enum { value = true }; };
template <> struct hb_is_signed<int16_t> { enum { value = true }; };
template <> struct hb_is_signed<int32_t> { enum { value = true }; };
template <> struct hb_is_signed<int64_t> { enum { value = true }; };
template <> struct hb_is_signed<uint8_t> { enum { value = false }; };
template <> struct hb_is_signed<uint16_t> { enum { value = false }; };
template <> struct hb_is_signed<uint32_t> { enum { value = false }; };
template <> struct hb_is_signed<uint64_t> { enum { value = false }; };
#define hb_is_signed(T) hb_is_signed<T>::value

template <bool is_signed> struct hb_signedness_int;
template <> struct hb_signedness_int<false> { typedef unsigned int value; };
template <> struct hb_signedness_int<true>  { typedef   signed int value; };
#define hb_signedness_int(T) hb_signedness_int<T>::value

template <typename T> struct hb_is_integer { enum { value = false }; };
template <> struct hb_is_integer<char> { enum { value = true }; };
template <> struct hb_is_integer<signed char> { enum { value = true }; };
template <> struct hb_is_integer<unsigned char> { enum { value = true }; };
template <> struct hb_is_integer<signed short> { enum { value = true }; };
template <> struct hb_is_integer<unsigned short> { enum { value = true }; };
template <> struct hb_is_integer<signed int> { enum { value = true }; };
template <> struct hb_is_integer<unsigned int> { enum { value = true }; };
template <> struct hb_is_integer<signed long> { enum { value = true }; };
template <> struct hb_is_integer<unsigned long> { enum { value = true }; };
template <> struct hb_is_integer<signed long long> { enum { value = true }; };
template <> struct hb_is_integer<unsigned long long> { enum { value = true }; };
#define hb_is_integer(T) hb_is_integer<T>::value


#endif /* HB_META_HH */
