/*
 * Copyright © 2009  Red Hat, Inc.
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
 * Red Hat Author(s): Behdad Esfahbod
 */

#include "hb-private.hh"

#include "hb-shape.h"

#include "hb-buffer-private.hh"

#ifdef HAVE_UNISCRIBE
# include "hb-uniscribe.h"
#endif
#ifdef HAVE_OT
# include "hb-ot-shape.h"
#endif
#include "hb-fallback-shape-private.hh"

typedef hb_bool_t (*hb_shape_func_t) (hb_font_t          *font,
				      hb_buffer_t        *buffer,
				      const hb_feature_t *features,
				      unsigned int        num_features,
				      const char         *shaper_options);

#define HB_SHAPER_IMPLEMENT(name) {#name, hb_##name##_shape}
static const struct hb_shaper_pair_t {
  const char name[16];
  hb_shape_func_t func;
} shapers[] = {
  /* v--- Add new shapers in the right place here */
#ifdef HAVE_UNISCRIBE
  HB_SHAPER_IMPLEMENT (uniscribe),
#endif
#ifdef HAVE_OT
  HB_SHAPER_IMPLEMENT (ot),
#endif
  HB_SHAPER_IMPLEMENT (fallback) /* should be last */
};
#undef HB_SHAPER_IMPLEMENT

static struct static_shaper_list_t
{
  static_shaper_list_t (void)
  {
    char *env = getenv ("HB_SHAPER_LIST");
    shaper_list = NULL;
    if (!env || !*env) {
    fallback:
      ASSERT_STATIC ((ARRAY_LENGTH (shapers) + 1) * sizeof (*shaper_list) <= sizeof (static_buffer));
      shaper_list = (const char **) static_buffer;
      unsigned int i;
      for (i = 0; i < ARRAY_LENGTH (shapers); i++)
        shaper_list[i] = shapers[i].name;
      shaper_list[i] = NULL;
      return;
    }

    unsigned int count = 3; /* initial, fallback, null */
    for (const char *p = env; (p == strchr (p, ',')) && p++; )
      count++;

    unsigned int len = strlen (env);

    if (count > 100 || len > 1000)
      goto fallback;

    len += count * sizeof (*shaper_list) + 1;
    char *buffer = len < sizeof (static_buffer) ? static_buffer : (char *) malloc (len);
    shaper_list = (const char **) buffer;
    buffer += count * sizeof (*shaper_list);
    len -= count * sizeof (*shaper_list);
    strncpy (buffer, env, len);

    count = 0;
    shaper_list[count++] = buffer;
    for (char *p = buffer; (p == strchr (p, ',')) && (*p = '\0', TRUE) && p++; )
      shaper_list[count++] = p;
    shaper_list[count++] = "fallback";
    shaper_list[count] = NULL;
  }
  ~static_shaper_list_t (void)
  {
    if ((char *) shaper_list != static_buffer)
      free (shaper_list);
  }

  const char **shaper_list;
  char static_buffer[32];
} static_shaper_list;

const char **
hb_shape_list_shapers (void)
{
  return static_shaper_list.shaper_list;
}

hb_bool_t
hb_shape_full (hb_font_t           *font,
	       hb_buffer_t         *buffer,
	       const hb_feature_t  *features,
	       unsigned int         num_features,
	       const char          *shaper_options,
	       const char         **shaper_list)
{
  if (likely (!shaper_list))
    shaper_list = static_shaper_list.shaper_list;

  if (likely (!shaper_list)) {
    for (unsigned int i = 0; i < ARRAY_LENGTH (shapers); i++)
      if (likely (shapers[i].func (font, buffer,
				   features, num_features,
				   shaper_options)))
        return TRUE;
  } else {
    while (*shaper_list) {
      for (unsigned int i = 0; i < ARRAY_LENGTH (shapers); i++)
	if (0 == strcmp (*shaper_list, shapers[i].name) &&
	    likely (shapers[i].func (font, buffer,
				     features, num_features,
				     shaper_options)))
	  return TRUE;
      shaper_list++;
    }
  }
  return FALSE;
}

void
hb_shape (hb_font_t           *font,
	  hb_buffer_t         *buffer,
	  const hb_feature_t  *features,
	  unsigned int         num_features)
{
  hb_shape_full (font, buffer, features, num_features, NULL, NULL);
}
