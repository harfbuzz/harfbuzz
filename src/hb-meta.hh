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


template <typename T> static inline T*
hb_addressof (const T& arg)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
  /* https://en.cppreference.com/w/cpp/memory/addressof */
  return reinterpret_cast<T*>(
	   &const_cast<char&>(
	      reinterpret_cast<const volatile char&>(arg)));
#pragma GCC diagnostic pop
}

template <typename T> static inline T hb_declval ();
#define hb_declval(T) (hb_declval<T> ())

template <typename T> struct hb_match_const { typedef T type; enum { value = false }; };
template <typename T> struct hb_match_const<const T> { typedef T type; enum { value = true }; };
#define hb_remove_const(T) typename hb_match_const<T>::type
#define hb_is_const(T) hb_match_const<T>::value
template <typename T> struct hb_match_reference { typedef T type; enum { value = false }; };
template <typename T> struct hb_match_reference<T &> { typedef T type; enum { value = true }; };
#define hb_remove_reference(T) typename hb_match_reference<T>::type
#define hb_is_reference(T) hb_match_reference<T>::value
template <typename T> struct hb_match_pointer { typedef T type; enum { value = false }; };
template <typename T> struct hb_match_pointer<T *> { typedef T type; enum { value = true }; };
#define hb_remove_pointer(T) typename hb_match_pointer<T>::type
#define hb_is_pointer(T) hb_match_pointer<T>::value

static const struct
{
  template <typename T>
  T operator () (T v) const { return v; }
  template <typename T>
  T& operator () (T *v) const { return *v; }
} hb_deref_pointer HB_UNUSED;


/* std::move and std::forward */

template <typename T>
hb_remove_reference (T)&& hb_move (T&& t) { return (hb_remove_reference (T)&&) (t); }

template <typename T>
T&& hb_forward (hb_remove_reference (T)& t) { return (T&&) t; }
template <typename T>
T&& hb_forward (hb_remove_reference (T)&& t) { return (T&&) t; }


/* Void!  For when we need a expression-type of void. */
struct hb_void_t { typedef void value; };

/* Bool!  For when we need to evaluate type-dependent expressions
 * in a template argument. */
template <bool b> struct hb_bool_tt { enum { value = b }; };
typedef hb_bool_tt<true> hb_true_t;
typedef hb_bool_tt<false> hb_false_t;


template<bool B, typename T = void>
struct hb_enable_if {};

template<typename T>
struct hb_enable_if<true, T> { typedef T type; };

#define hb_enable_if(Cond) typename hb_enable_if<(Cond)>::type* = nullptr


/*
 * Meta-functions.
 */

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
