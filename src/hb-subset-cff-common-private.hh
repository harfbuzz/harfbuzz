/*
 * Copyright © 2018 Adobe Systems Incorporated.
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
 * Adobe Author(s): Michiharu Ariza
 */

#ifndef HB_SUBSET_CFF_COMMON_PRIVATE_HH
#define HB_SUBSET_CFF_COMMON_PRIVATE_HH

#include "hb-private.hh"

#include "hb-subset-plan.hh"

HB_INTERNAL bool
hb_plan_subset_cff_fdselect (const hb_vector_t<hb_codepoint_t> &glyphs,
                            unsigned int fdCount,
                            const CFF::FDSelect &src, /* IN */
                            unsigned int &subset_fd_count /* OUT */,
                            unsigned int &subst_fdselect_size /* OUT */,
                            unsigned int &subst_fdselect_format /* OUT */,
                            hb_vector_t<hb_codepoint_t> &subst_first_glyphs /* OUT */,
                            CFF::FDMap &fdmap /* OUT */);

HB_INTERNAL bool
hb_serialize_cff_fdselect (hb_serialize_context_t *c,
                          const hb_vector_t<hb_codepoint_t> &glyphs,
                          const CFF::FDSelect &src,
                          unsigned int fd_count,
                          unsigned int fdselect_format,
                          unsigned int size,
                          const hb_vector_t<hb_codepoint_t> &first_glyphs,
                          const CFF::FDMap &fdmap);

#endif /* HB_SUBSET_CFF_COMMON_PRIVATE_HH */
