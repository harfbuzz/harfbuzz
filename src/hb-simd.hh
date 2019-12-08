/*
 * Copyright © 2019  Facebook, Inc.
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
 * Facebook Author(s): Behdad Esfahbod
 */

#ifndef HB_SIMD_HH
#define HB_SIMD_HH

#include "hb.hh"
#include "hb-meta.hh"

/*
 * = MOTIVATION
 *
 * We hit a performance plateau with HarfBuzz many many years ago.  The last
 * *major* speedup was when hb_set_digest_t was added back in 2012.  Since then
 * we have shaved off a lot or extra work, but none of that matters ince
 * shaping time is dominated by two hot spots, which no tweaks whatsoever could
 * speed up any more.  SIMD is the only chance to gain a major speedup over
 * that, and that is what this file is about.
 *
 * For heavy fonts, ie. those having dozens, or over a hundred, lookups (Amiri,
 * IranNastaliq, NotoNastaliqUrdu, heavy Indic fonts, etc), the bottleneck is
 * just looping over lookups and for each lookup over the buffer contents and
 * testing wheater current lookup applies to current glyph; normally that would
 * be a binary search in the Coverage table, but that's where the
 * hb_set_digest_t speedup came from: hb_set_digest_t are narrow (3 or 4
 * integers) structures that implement approximate matching, similar to Bloom
 * Filters or Quotient Filters. These digests do all bitwise operations, so
 * they can be easily vectorized. Combined with a gather operation, or just
 * multiple fetches in a row (which should parallelize) when gather is not
 * available.  This will allow us to * skip over 8 or 16 glyphs at a time.
 *
 * For fast fonts, like simple Latin fonts, like Roboto, the majority of time
 * is spent in binary searching in the Coverage table of kern and liga lookups.
 * We can, again, use vector gather and comparison operations to implement a
 * 8ary or 16ary search instead of binary search.
 *
 * The rest of this files adds facilities to implement those, and possibly
 * more.
 *
 *
 * = INTRO to VECTOR EXTENSIONS
 *
 * A great series of short articles to get started with vector extensions is:
 *
 *   https://www.codingame.com/playgrounds/283/sse-avx-vectorization/
 *
 * If analyzing existing vector implementation to improve performance, the
 * following comes handy since it has instruction latencies, throughputs, and
 * micro-operation breakdown:
 *
 *   https://www.agner.org/optimize/instruction_tables.pdf
 *
 * For x86 programming the following intrinsics guide and cheatsheet are
 * indispensible:
 *
 *   https://software.intel.com/sites/landingpage/IntrinsicsGuide/
 *   https://db.in.tum.de/~finis/x86-intrin-cheatsheet-v2.2.pdf
 *
 *
 * = WHICH EXTENSIONS TO TARGET
 *
 * There are four vector extensions that matter for integer work like we need:
 *
 * On x86 / x86_64 (Intel / AMD), those are (in order of introduction):
 *
 * - SSE2	128 bits
 * - AVX2	256 bits
 * - AVX-512	512 bits
 *
 * On ARM there seems to be just:
 *
 * - NEON	128 bits
 *
 * Intel also provides an implementation of the NEON intrinsics API backed by SSE:
 *
 *   https://github.com/intel/ARM_NEON_2_x86_SSE
 *
 * So it's possible we can skip implementing SSE2 separately.
 *
 * To see which extensions are available on your machine you can, eg.:
 *
 * $ gcc -march=native -dM -E - < /dev/null | egrep "SSE|AVX" | sort
 * #define __AVX__ 1
 * #define __AVX2__ 1
 * #define __SSE__ 1
 * #define __SSE2__ 1
 * #define __SSE2_MATH__ 1
 * #define __SSE3__ 1
 * #define __SSE4_1__ 1
 * #define __SSE4_2__ 1
 * #define __SSE_MATH__ 1
 * #define __SSSE3__ 1
 *
 * If no arch is set, my Fedora defaults to enabling SSE2 but not any AVX:
 *
 * $ gcc  -dM -E - < /dev/null | egrep "SSE|AVX" | sort
 * #define __SSE__ 1
 * #define __SSE2__ 1
 * #define __SSE2_MATH__ 1
 * #define __SSE_MATH__ 1
 *
 * Given that, we should explore an SSE2 target at some point for sure
 * (possibly via the NEON path).  But initially, targetting AVX2 makes most
 * sense, for the following reasons:
 *
 * - The number of the integer operations we do on each vector is very few.
 *   This means that how fast we can load data into the vector registers is of
 *   paramount importance on the resulting performance.  AVX2 is the first
 *   extension to add a "gather" operation.  We will use that to load 8 or 16
 *   glyphs from non-consecutive memory addresses into one vector.
 *
 * - AVX-512 is practically the same as AVX2 but extended to 512 bits instead
 *   of 256.  However, it's not widely available on lower-power CPUs.  For
 *   example, my 2019 ThinkPad Yoga X1 does *not* support it.  We should
 *   definitely explore that, but not initially.  Also, it is possible that the
 *   extra memory load that puts will defeat the speedup we can gain from it.
 *   Must be implemented and measured carefully.
 */

/* DESIGN
 *
 * There are a few complexities to using AVX2 though, which we need to
 * overcome.  The main one that sticks out is:
 *
 * While the 256 bit integer register (called __m256i) can be used as 16
 * uint16_t values, there /doens't/ exist a 16bit version of the gather
 * operation.  The closest there is gathers 8 uint32_t values
 * (_mm_[mask_]i32gather_epi32()).  I believe this is because there are only
 * eight ports to fetch from memory concurrently.
 *
 * Whatever the reason, this is wasteful.  OpenType glyph indices are 16bit
 * numbers, so in theory we should be able to process 16 at a time, if there
 * was a 16bit gather opperator.
 *
 * This is also problematic for loading GlyphID values in that we should make
 * sure the extra two bytes being loaded by the 32bit gather operator does
 * not cause invalid memory access, before we throw those extra bytes away.
 *
 * There are a couple of different approaches we can take to mitigate these:
 *
 * Making two gather calls sequentically and swizzling the results together
 * might work just fine.  Specially since the second one will be fulfilled
 * straight from the L1 cachelines.
 *
 *
 * PREFETCH
 *
 * We should use prefetch instructions to request load of the next batch of
 * hb_glyph_info_t's while we process the current batch.
 */


/*
 * Choose intrinsics set to use.
 */


#if !defined(HB_NO_SIMD) && (defined(HB_SIMD_AVX2) || defined(__AVX2__))

#pragma GCC target("avx2")

#include <x86intrin.h>

/* TODO: Test -mvzeroupper. */

static __m256i x HB_UNUSED;




#elif !defined(HB_NO_SIMD)
#define HB_NO_SIMD
#endif


/*
 * Use to implement faster specializations.
 */

#ifndef HB_NO_SIMD

#if 0
static inline bool
hb_simd_bsearch_glyphid_range (unsigned *pos, /* Out */
			       hb_codepoint_t k,
			       const void *base,
			       size_t length,
			       size_t stride)
{
}
#endif

#endif

#endif /* HB_SIMD_HH */
