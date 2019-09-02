
/*
 * Copyright © 2019  Ebrahim Byagowi
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
 */

#include "hb-float.hh"
#include "hb-machinery.hh"

#include <locale.h>
#ifdef HAVE_XLOCALE_H
#include <xlocale.h>
#endif

#if defined (HAVE_NEWLOCALE) && defined (HAVE_STRTOD_L)
#define USE_XLOCALE 1
#define HB_LOCALE_T locale_t
#define HB_CREATE_LOCALE(locName) newlocale (LC_ALL_MASK, locName, nullptr)
#define HB_FREE_LOCALE(loc) freelocale (loc)
#elif defined(_MSC_VER)
#define USE_XLOCALE 1
#define HB_LOCALE_T _locale_t
#define HB_CREATE_LOCALE(locName) _create_locale (LC_ALL, locName)
#define HB_FREE_LOCALE(loc) _free_locale (loc)
#define strtod_l(a, b, c) _strtod_l ((a), (b), (c))
#endif

#ifdef USE_XLOCALE

#if HB_USE_ATEXIT
static void free_static_C_locale ();
#endif

static struct hb_C_locale_lazy_loader_t : hb_lazy_loader_t<hb_remove_pointer<HB_LOCALE_T>,
							  hb_C_locale_lazy_loader_t>
{
  static HB_LOCALE_T create ()
  {
    HB_LOCALE_T C_locale = HB_CREATE_LOCALE ("C");

#if HB_USE_ATEXIT
    atexit (free_static_C_locale);
#endif

    return C_locale;
  }
  static void destroy (HB_LOCALE_T p)
  {
    HB_FREE_LOCALE (p);
  }
  static HB_LOCALE_T get_null ()
  {
    return nullptr;
  }
} static_C_locale;

#if HB_USE_ATEXIT
static
void free_static_C_locale ()
{
  static_C_locale.free_instance ();
}
#endif

static HB_LOCALE_T
get_C_locale ()
{
  return static_C_locale.get_unconst ();
}
#endif /* USE_XLOCALE */

bool
parse_float (const char **pp, const char *end, float *pv)
{
  char buf[32];
  unsigned int len = hb_min (ARRAY_LENGTH (buf) - 1, (unsigned int) (end - *pp));
  strncpy (buf, *pp, len);
  buf[len] = '\0';

  char *p = buf;
  char *pend = p;
  float v;

  errno = 0;
#ifdef USE_XLOCALE
  v = strtod_l (p, &pend, get_C_locale ());
#else
  v = strtod (p, &pend);
#endif
  if (errno || p == pend)
    return false;

  *pv = v;
  *pp += pend - p;
  return true;
}
