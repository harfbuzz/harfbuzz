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
#include "hb-algs.hh"

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
 * Filters or Quotient Filters. These digests do all their work using bitwise
 * operations, so they can be easily vectorized. Combined with a gather
 * operation, or just multiple fetches in a row (which should parallelize) when
 * gather is not available.  This will allow us to skip over 8 or 16 glyphs at
 * a time.
 *
 * For fast fonts, like simple Latin fonts, like Roboto, the majority of time
 * is spent in binary searching in the Coverage table of kern and liga lookups.
 * We can, again, use vector gather and comparison operations to implement a
 * 9ary or 17ary search instead of binary search, which will reduce search
 * depth by 3x / 4x respectively.  It's important to keep in mind that a
 * 16-at-a-time 17ary search is /not/ in any way 17 times faster.  Only 4 times
 * faster at best since the number of search steps compared to binary search is
 * log(17)/log(2) ~= 4.  That should be taken into account while assessing
 * various designs.
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
 *   Also do note that for the search usecase, doubling the bitwidth from 256
 *   to 512, as discussed, only has hard max benefit cap of less than 30%
 *   speedup (log(17) / log(9)).  Must be implemented and measured carefully.
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
 * Another snag I hit is that AVX2 only has signed comparisons, not unsigned.
 * So we add a shift to convert uint16_t numbers to int16_t before comparing.
 *
 * Also note that the order of arguments to _mm256_set_epi32() and family
 * is opposite of what I originally assumed.  Docs are correct, just not
 * what you assume.
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

static inline bool
hb_simd_ksearch_glyphid_range (unsigned *pos, /* Out */
			       hb_codepoint_t k,
			       const void *base,
			       size_t length,
			       size_t stride)
{
  if (unlikely (k & ~0xFFFF))
  {
    *pos = length;
    return false;
  }

  *pos = 0;

#define HB_2TIMES(x) (x), (x)
#define HB_4TIMES(x) HB_2TIMES(x), HB_2TIMES (x)
#define HB_8TIMES(x) HB_4TIMES(x), HB_4TIMES (x)
#define HB_16TIMES(x) HB_8TIMES (x), HB_8TIMES (x)

  /* Find deptch of search tree. */
  static const unsigned steps[] = {1, 9, 81, 729, 6561, 59049};
  unsigned rank = 1;
  while (rank < ARRAY_LENGTH (steps) && length >= steps[rank])
    rank++;

  static const __m256i _1x8 = _mm256_set_epi32 (HB_8TIMES (1));
  static const __m256i stridex8 = _mm256_set_epi32 (HB_8TIMES (stride));
  static const __m256i __1x8 = _mm256_set_epi32 (HB_8TIMES (-1));
  static const __m256i _12345678 = _mm256_set_epi32 (8, 7, 6, 5, 4, 3, 2, 1);
  static const __m256i __32768x16 = _mm256_set_epi16 (HB_16TIMES (-32768));

  /* Set up key vector. */
  const __m256i K = _mm256_add_epi16 (_mm256_set_epi16 (HB_16TIMES ((signed) k - 32768)), _1x8);

  while (rank)
  {
    unsigned step = steps[--rank];

    /* Load multiple ranges to test against. */
    const unsigned limit = stride * length;
    const __m256i limits = _mm256_set_epi32 (HB_8TIMES (limit));
    const unsigned pitch = stride * step;
    const __m256i pitches = _mm256_set_epi32 (HB_8TIMES (pitch));
    const __m256i offsets = _mm256_sub_epi32 (_mm256_mullo_epi32 (pitches, _12345678), stridex8);
    const __m256i mask = _mm256_cmpgt_epi32 (limits, offsets);

    /* The actual load... */
    __m256i V = _mm256_mask_i32gather_epi32 (__1x8, (const int *) base, offsets, mask, 1);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    static const __m256i bswap16_shuffle = _mm256_set_epi16 (0x0E0F,0x0C0D,0x0A0B,0x0809,
							     0x0607,0x0405,0x0203,0x0001,
							     0x0E0F,0x0C0D,0x0A0B,0x0809,
							     0x0607,0x0405,0x0203,0x0001);
      V = _mm256_shuffle_epi8 (V, bswap16_shuffle);
#endif
    V = _mm256_add_epi16 (V, __32768x16);

    /* Compare and locate. */
    unsigned answer = hb_ctz (~_mm256_movemask_epi8 (_mm256_cmpgt_epi16 (K, V))) >> 1;
    bool found = answer & 1;
    answer = (answer + 1) >> 1;
    unsigned move = step * answer;
    *pos += move;
    if (found)
    {
      *pos -= 1;
      return true;
    }
    length -= move;
    base = (const void *) ((const char *) base + stride * move);
  }
  return false;
}


#elif !defined(HB_NO_SIMD)
#define HB_NO_SIMD
#endif

#endif /* HB_SIMD_HH */
