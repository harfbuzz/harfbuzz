/*
 * Copyright © 2024  Google, Inc.
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
 * Author(s): Behdad Esfahbod
 */

#ifndef HB_BENCHMARK_HH
#define HB_BENCHMARK_HH

#include <hb-config.hh>

#include <hb.h>
#include <hb-subset.h>

#include <hb-ot.h>
#ifdef HAVE_FREETYPE
#include <hb-ft.h>
#endif
#ifdef HAVE_FONTATIONS
#include <hb-fontations.h>
#endif
#ifdef HAVE_CORETEXT
#include <hb-coretext.h>
#endif

#include <benchmark/benchmark.h>

#include <cassert>
#include <cstdlib>
#include <cstring>


HB_BEGIN_DECLS

static inline hb_face_t *
hb_benchmark_face_create_from_file_or_fail (const char *font_path,
					    unsigned face_index)
{
  return hb_face_create_from_file_or_fail_using (font_path, face_index, nullptr);
}

HB_END_DECLS

#endif /* HB_BENCHMARK_HH */
