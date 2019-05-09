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
template <bool b> struct hb_bool_tt { static constexpr bool value = b; };
typedef hb_bool_tt<true> hb_true_t;
typedef hb_bool_tt<false> hb_false_t;


/* Basic type SFINAE. */

template <bool B, typename T = void> struct hb_enable_if {};
template <typename T>                struct hb_enable_if<true, T> { typedef T type; };
#define hb_enable_if(Cond) typename hb_enable_if<(Cond)>::type* = nullptr
/* Concepts/Requires alias: */
#define hb_requires(Cond) hb_enable_if((Cond))

template <typename T, typename T2> struct hb_is_same : hb_false_t {};
template <typename T>              struct hb_is_same<T, T> : hb_true_t {};
#define hb_is_same(T, T2) hb_is_same<T, T2>::value

/* Function overloading SFINAE and priority. */

#define HB_RETURN(Ret, E) -> hb_head_tt<Ret, decltype ((E))> { return (E); }
#define HB_AUTO_RETURN(E) -> decltype ((E)) { return (E); }
#define HB_VOID_RETURN(E) -> hb_void_tt<decltype ((E))> { (E); }

template <unsigned Pri> struct hb_priority : hb_priority<Pri - 1> {};
template <>             struct hb_priority<0> {};
#define hb_prioritize hb_priority<16> ()

#define HB_FUNCOBJ(x) static_const x HB_UNUSED


template <typename T> struct hb_match_identity { typedef T type; };
template <typename T> using hb_type_identity = typename hb_match_identity<T>::type;

struct
{
  template <typename T>
  T* operator () (T& arg) const
  {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
    /* https://en.cppreference.com/w/cpp/memory/addressof */
    return reinterpret_cast<T*> (
	     &const_cast<char&> (
		reinterpret_cast<const volatile char&> (arg)));
#pragma GCC diagnostic pop
  }
}
HB_FUNCOBJ (hb_addressof);

template <typename T> static inline T hb_declval ();
#define hb_declval(T) (hb_declval<T> ())

template <typename T> struct hb_match_const		{ typedef T type; static constexpr bool value = false; };
template <typename T> struct hb_match_const<const T>	{ typedef T type; static constexpr bool value = true;  };
template <typename T> using hb_remove_const = typename hb_match_const<T>::type;
#define hb_is_const(T) hb_match_const<T>::value
template <typename T> struct hb_match_reference		{ typedef T type; static constexpr bool value = false; };
template <typename T> struct hb_match_reference<T &>	{ typedef T type; static constexpr bool value = true;  };
template <typename T> struct hb_match_reference<T &&>	{ typedef T type; static constexpr bool value = true;  };
template <typename T> using hb_remove_reference = typename hb_match_reference<T>::type;
#define hb_is_reference(T) hb_match_reference<T>::value
template <typename T> struct hb_match_pointer		{ typedef T type; static constexpr bool value = false; };
template <typename T> struct hb_match_pointer<T *>	{ typedef T type; static constexpr bool value = true;  };
template <typename T> using hb_remove_pointer = typename hb_match_pointer<T>::type;
#define hb_is_pointer(T) hb_match_pointer<T>::value

/* TODO Add feature-parity to std::decay. */
template <typename T> using hb_decay = hb_remove_const<hb_remove_reference<T>>;

#define hb_is_cr_convertible(From, To) \
	( \
	  hb_is_same (hb_decay<From>, hb_decay<To>) && \
	  ( \
	    hb_is_const (From) <= hb_is_const (To) && \
	    hb_is_reference (From) >= hb_is_reference (To) \
	  ) || ( \
	    hb_is_const (To) && hb_is_reference (To) \
	  ) \
	)



template<bool B, class T, class F>
struct _hb_conditional { typedef T type; };
template<class T, class F>
struct _hb_conditional<false, T, F> { typedef F type; };
template<bool B, class T, class F>
using hb_conditional = typename _hb_conditional<B, T, F>::type;


template <typename From, typename To>
struct hb_is_convertible
{
  private:
  static constexpr bool   from_void = hb_is_same (void, hb_decay<From>);
  static constexpr bool     to_void = hb_is_same (void, hb_decay<To>  );
  static constexpr bool either_void = from_void || to_void;
  static constexpr bool   both_void = from_void && to_void;

  static hb_true_t impl2 (hb_conditional<to_void, int, To>);

  template <typename T>
  static auto impl (hb_priority<1>) HB_AUTO_RETURN ( impl2 (hb_declval (T)) )
  template <typename T>
  static hb_false_t impl (hb_priority<0>);
  public:
  static constexpr bool value = both_void ||
		       (!either_void &&
			decltype (impl<hb_conditional<from_void, int, From>> (hb_prioritize))::value);
};


#define hb_is_convertible(From,To) hb_is_convertible<From, To>::value


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
}
HB_FUNCOBJ (hb_deref);

struct
{
  template <typename T> auto
  operator () (T&& v) const HB_AUTO_RETURN (hb_forward<T> (v))

  template <typename T> auto
  operator () (T& v) const HB_AUTO_RETURN (hb_addressof (v))
}
HB_FUNCOBJ (hb_ref);

template <typename T>
struct hb_reference_wrapper
{
  hb_reference_wrapper (T v) : v (v) {}
  bool operator == (const hb_reference_wrapper& o) const { return v == o.v; }
  bool operator != (const hb_reference_wrapper& o) const { return v != o.v; }
  operator T () const { return v; }
  T get () const { return v; }
  T v;
};
template <typename T>
struct hb_reference_wrapper<T&>
{
  hb_reference_wrapper (T& v) : v (hb_addressof (v)) {}
  bool operator == (const hb_reference_wrapper& o) const { return v == o.v; }
  bool operator != (const hb_reference_wrapper& o) const { return v != o.v; }
  operator T& () const { return *v; }
  T& get () const { return *v; }
  T* v;
};


template <typename T> struct hb_is_signed;
template <> struct hb_is_signed<char>			{ static constexpr bool value = CHAR_MIN < 0;	};
template <> struct hb_is_signed<signed char>		{ static constexpr bool value = true;		};
template <> struct hb_is_signed<unsigned char>		{ static constexpr bool value = false;		};
template <> struct hb_is_signed<signed short>		{ static constexpr bool value = true;		};
template <> struct hb_is_signed<unsigned short>		{ static constexpr bool value = false;		};
template <> struct hb_is_signed<signed int>		{ static constexpr bool value = true;		};
template <> struct hb_is_signed<unsigned int>		{ static constexpr bool value = false;		};
template <> struct hb_is_signed<signed long>		{ static constexpr bool value = true;		};
template <> struct hb_is_signed<unsigned long>		{ static constexpr bool value = false;		};
template <> struct hb_is_signed<signed long long>	{ static constexpr bool value = true;		};
template <> struct hb_is_signed<unsigned long long>	{ static constexpr bool value = false;		};
#define hb_is_signed(T) hb_is_signed<T>::value

template <typename T> struct hb_int_min;
template <> struct hb_int_min<char>			{ static constexpr char			value = CHAR_MIN;	};
template <> struct hb_int_min<signed char>		{ static constexpr signed char		value = SCHAR_MIN;	};
template <> struct hb_int_min<unsigned char>		{ static constexpr unsigned char	value = 0;		};
template <> struct hb_int_min<signed short>		{ static constexpr signed short		value = SHRT_MIN;	};
template <> struct hb_int_min<unsigned short>		{ static constexpr unsigned short	value = 0;		};
template <> struct hb_int_min<signed int>		{ static constexpr signed int		value = INT_MIN;	};
template <> struct hb_int_min<unsigned int>		{ static constexpr unsigned int		value = 0;		};
template <> struct hb_int_min<signed long>		{ static constexpr signed long		value = LONG_MIN;	};
template <> struct hb_int_min<unsigned long>		{ static constexpr unsigned long	value = 0;		};
template <> struct hb_int_min<signed long long>		{ static constexpr signed long long	value = LLONG_MIN;	};
template <> struct hb_int_min<unsigned long long>	{ static constexpr unsigned long long	value = 0;		};
#define hb_int_min(T) hb_int_min<T>::value
template <typename T> struct hb_int_max;
template <> struct hb_int_max<char>			{ static constexpr char			value = CHAR_MAX;	};
template <> struct hb_int_max<signed char>		{ static constexpr signed char		value = SCHAR_MAX;	};
template <> struct hb_int_max<unsigned char>		{ static constexpr unsigned char	value = UCHAR_MAX;	};
template <> struct hb_int_max<signed short>		{ static constexpr signed short		value = SHRT_MAX;	};
template <> struct hb_int_max<unsigned short>		{ static constexpr unsigned short	value = USHRT_MAX;	};
template <> struct hb_int_max<signed int>		{ static constexpr signed int		value = INT_MAX;	};
template <> struct hb_int_max<unsigned int>		{ static constexpr unsigned int		value = UINT_MAX;	};
template <> struct hb_int_max<signed long>		{ static constexpr signed long		value = LONG_MAX;	};
template <> struct hb_int_max<unsigned long>		{ static constexpr unsigned long	value = ULONG_MAX;	};
template <> struct hb_int_max<signed long long>		{ static constexpr signed long long	value = LLONG_MAX;	};
template <> struct hb_int_max<unsigned long long>	{ static constexpr unsigned long long	value = ULLONG_MAX;	};
#define hb_int_max(T) hb_int_max<T>::value

template <bool is_signed> struct hb_signedness_int;
template <> struct hb_signedness_int<false> { typedef unsigned int value; };
template <> struct hb_signedness_int<true>  { typedef   signed int value; };
#define hb_signedness_int(T) hb_signedness_int<T>::value

template <typename T> struct hb_is_integer		{ static constexpr bool value = false;};
template <> struct hb_is_integer<char> 			{ static constexpr bool value = true; };
template <> struct hb_is_integer<signed char> 		{ static constexpr bool value = true; };
template <> struct hb_is_integer<unsigned char> 	{ static constexpr bool value = true; };
template <> struct hb_is_integer<signed short> 		{ static constexpr bool value = true; };
template <> struct hb_is_integer<unsigned short> 	{ static constexpr bool value = true; };
template <> struct hb_is_integer<signed int> 		{ static constexpr bool value = true; };
template <> struct hb_is_integer<unsigned int> 		{ static constexpr bool value = true; };
template <> struct hb_is_integer<signed long> 		{ static constexpr bool value = true; };
template <> struct hb_is_integer<unsigned long> 	{ static constexpr bool value = true; };
template <> struct hb_is_integer<signed long long> 	{ static constexpr bool value = true; };
template <> struct hb_is_integer<unsigned long long> 	{ static constexpr bool value = true; };
#define hb_is_integer(T) hb_is_integer<T>::value


#endif /* HB_META_HH */
