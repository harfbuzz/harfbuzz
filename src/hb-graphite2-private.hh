/*
 * Copyright (C) 2011  Martin Hosken
 * Copyright (C) 2011  SIL International
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
 */

#ifndef HB_GRAPHITE2_PRIVATE_HH
#define HB_GRAPHITE2_PRIVATE_HH

#include "hb-private.hh"

#if !HAVE_GRAPHITE2_STATIC

typedef unsigned char   gr_uint8;
typedef gr_uint8        gr_byte;
typedef signed char     gr_int8;
typedef unsigned short  gr_uint16;
typedef short           gr_int16;
typedef unsigned int    gr_uint32;
typedef int             gr_int32;

typedef struct gr_face  gr_face;
typedef struct gr_font  gr_font;
typedef struct gr_feature_ref   gr_feature_ref;
typedef struct gr_feature_val   gr_feature_val;
typedef struct gr_segment   gr_segment;
typedef struct gr_slot      gr_slot;
typedef const void *(*gr_get_table_fn)(const void* appFaceHandle, unsigned int name, size_t *len);
typedef float (*gr_advance_fn)(const void* appFontHandle, gr_uint16 glyphid);
enum gr_encform {
  gr_utf8 = 1/*sizeof(uint8)*/, gr_utf16 = 2/*sizeof(uint16)*/, gr_utf32 = 4/*sizeof(uint32)*/
};

#else
#include <graphite2/Font.h>
#endif


#ifdef _WIN32
#define HB_GR2_LIBRARY "graphite2.dll"
#elif __APPLE__
#define HB_GR2_LIBRARY "libgraphite2.dylib"
#else
#define HB_GR2_LIBRARY "libgraphite2.so"
#endif

#define HB_GRAPHITE2_TAG_SILF HB_TAG('S','i','l','f')

#if !HAVE_GRAPHITE2_STATIC

#define DO_GR2_FUNCS \
    GR2_DO(gr_make_face, gr_face*, (const void* appFaceHandle/*non-NULL*/, gr_get_table_fn getTable, unsigned int faceOptions)) \
    GR2_DO(gr_face_destroy, void, (gr_face *pFace)) \
    GR2_DO(gr_make_font, gr_font*, (float ppm, const gr_face *face)) \
    GR2_DO(gr_make_font_with_advance_fn, gr_font*, (float ppm, const void* appFontHandle, gr_advance_fn getAdvance, const gr_face *face)) \
    GR2_DO(gr_font_destroy, void, (gr_font *pFont)) \
    GR2_DO(gr_face_featureval_for_lang, gr_feature_val*, (const gr_face* pFace, gr_uint32 langname)) \
    GR2_DO(gr_featureval_destroy, void, (gr_feature_val *pfeatures)) \
    GR2_DO(gr_face_find_fref, gr_feature_ref*, (const gr_face* pFace, gr_uint32 featId)) \
    GR2_DO(gr_fref_set_feature_value, int, (const gr_feature_ref* pfeatureref, gr_uint16 val, gr_feature_val* pDest)) \
    GR2_DO(gr_make_seg, gr_segment*, (const gr_font* font, const gr_face* face, gr_uint32 script, const gr_feature_val* pFeats, enum gr_encform enc, const void* pStart, size_t nChars, int dir)) \
    GR2_DO(gr_seg_destroy, void, (const gr_segment* pSeg)) \
    GR2_DO(gr_seg_advance_X, float, (const gr_segment* pSeg/*not NULL*/)) \
    GR2_DO(gr_seg_advance_Y, float, (const gr_segment* pSeg/*not NULL*/)) \
    GR2_DO(gr_seg_n_slots, int, (const gr_segment* pSeg/*not NULL*/)) \
    GR2_DO(gr_seg_first_slot, gr_slot*, (gr_segment* pSeg/*not NULL*/)) \
    GR2_DO(gr_seg_last_slot, gr_slot*, (gr_segment* pSeg/*not NULL*/)) \
    GR2_DO(gr_slot_next_in_segment, gr_slot*, (const gr_slot* p)) \
    GR2_DO(gr_slot_prev_in_segment, gr_slot*, (const gr_slot* p)) \
    GR2_DO(gr_slot_before, int, (const gr_slot* p/*not NULL*/)) \
    GR2_DO(gr_slot_after, int, (const gr_slot* p/*not NULL*/)) \
    GR2_DO(gr_slot_can_insert_before, int, (const gr_slot *p/*not NULL*/)) \
    GR2_DO(gr_slot_gid, unsigned short, (const gr_slot* p/*not NULL*/)) \
    GR2_DO(gr_slot_origin_X, float, (const gr_slot* p)) \
    GR2_DO(gr_slot_origin_Y, float, (const gr_slot* p)) \
    GR2_DO(gr_slot_advance_X, float, (const gr_slot* p, const gr_face* face, const gr_font *font)) \
    GR2_DO(gr_slot_advance_Y, float, (const gr_slot* p, const gr_face* face, const gr_font *font)) \



#define GR2_DO(f, r, x) r ( * f## _p) x;
typedef struct graphite2_funcs_t {
DO_GR2_FUNCS
#undef GR2_DO
} graphite2_funcs_t;

#define gr_make_face(...) grfuncs.gr_make_face_p(__VA_ARGS__)
#define gr_face_destroy(...) grfuncs.gr_face_destroy_p(__VA_ARGS__)
#define gr_make_font(...) grfuncs.gr_make_font_p(__VA_ARGS__)
#define gr_make_font_with_advance_fn(...) grfuncs.gr_make_font_with_advance_fn_p(__VA_ARGS__)
#define gr_font_destroy(...) grfuncs.gr_font_destroy_p(__VA_ARGS__)
#define gr_face_featureval_for_lang(...) grfuncs.gr_face_featureval_for_lang_p(__VA_ARGS__)
#define gr_featureval_destroy(...) grfuncs.gr_featureval_destroy_p(__VA_ARGS__)
#define gr_face_find_fref(...) grfuncs.gr_face_find_fref_p(__VA_ARGS__)
#define gr_fref_set_feature_value(...) grfuncs.gr_fref_set_feature_value_p(__VA_ARGS__)
#define gr_make_seg(...) grfuncs.gr_make_seg_p(__VA_ARGS__)
#define gr_seg_destroy(...) grfuncs.gr_seg_destroy_p(__VA_ARGS__)
#define gr_seg_advance_X(...) grfuncs.gr_seg_advance_X_p(__VA_ARGS__)
#define gr_seg_advance_Y(...) grfuncs.gr_seg_advance_Y_p(__VA_ARGS__)
#define gr_seg_n_slots(...) grfuncs.gr_seg_n_slots_p(__VA_ARGS__)
#define gr_seg_first_slot(...) grfuncs.gr_seg_first_slot_p(__VA_ARGS__)
#define gr_seg_last_slot(...) grfuncs.gr_seg_last_slot_p(__VA_ARGS__)
#define gr_slot_next_in_segment(...) grfuncs.gr_slot_next_in_segment_p(__VA_ARGS__)
#define gr_slot_prev_in_segment(...) grfuncs.gr_slot_prev_in_segment_p(__VA_ARGS__)
#define gr_slot_before(...) grfuncs.gr_slot_before_p(__VA_ARGS__)
#define gr_slot_after(...) grfuncs.gr_slot_after_p(__VA_ARGS__)
#define gr_slot_can_insert_before(...) grfuncs.gr_slot_can_insert_before_p(__VA_ARGS__)
#define gr_slot_gid(...) grfuncs.gr_slot_gid_p(__VA_ARGS__)
#define gr_slot_origin_X(...) grfuncs.gr_slot_origin_X_p(__VA_ARGS__)
#define gr_slot_origin_Y(...) grfuncs.gr_slot_origin_Y_p(__VA_ARGS__)
#define gr_slot_advance_X(...) grfuncs.gr_slot_advance_X_p(__VA_ARGS__)
#define gr_slot_advance_Y(...) grfuncs.gr_slot_advance_Y_p(__VA_ARGS__)

#endif

#endif /* HB_GRAPHITE2_PRIVATE_HH */
