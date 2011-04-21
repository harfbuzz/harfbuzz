/*
 * Copyright © 2011  Google, Inc.
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

#ifndef HB_TEST_H
#define HB_TEST_H

#include <hb.h>
#include <hb-glib.h>

#include <glib.h>

#include <stdlib.h>

HB_BEGIN_DECLS

/* Just in case */
#undef G_DISABLE_ASSERT


/* Bugzilla helpers */

static inline void
hb_test_bug (const char *uri_base, unsigned int number)
{
  char *s = g_strdup_printf ("%u", number);

  g_test_bug_base (uri_base);
  g_test_bug (s);

  g_free (s);
}

static inline void
hb_test_bug_freedesktop (unsigned int number)
{
  hb_test_bug ("http://bugs.freedesktop.org/", number);
}

static inline void
hb_test_bug_gnome (unsigned int number)
{
  hb_test_bug ("http://bugzilla.gnome.org/", number);
}

static inline void
hb_test_bug_mozilla (unsigned int number)
{
  hb_test_bug ("http://bugzilla.mozilla.org/", number);
}

static inline void
hb_test_bug_redhat (unsigned int number)
{
  hb_test_bug ("http://bugzilla.redhat.com/", number);
}


/* Misc */

/* This is too ugly to be public API, but quite handy. */
#define HB_TAG_CHAR4(s)   (HB_TAG(((const char *) s)[0], \
				  ((const char *) s)[1], \
				  ((const char *) s)[2], \
				  ((const char *) s)[3]))
HB_END_DECLS

#endif /* HB_TEST_H */
