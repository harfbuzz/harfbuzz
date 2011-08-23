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

#include "hb-private.hh"
#include "hb-buffer-private.hh"
#include "hb-font-private.hh"
#include "hb-ot-tag.h"
#include <graphite2/Font.h>
#include <graphite2/Segment.h>
#include "hb-graphite2.h"
#include <stdlib.h>
#include <string.h>

HB_BEGIN_DECLS

typedef struct hbgr_tablelist_t {
    hb_blob_t   *blob;
    struct hbgr_tablelist_t *next;
    unsigned int tag;
} hbgr_tablelist_t;

typedef struct hbgr_face_t {
    gr_face             *grface;
    hbgr_tablelist_t    *tlist;
    hb_destroy_func_t   destroy;
    hb_reference_table_func_t get_table;
    hb_face_t           *face;
    void                *data;
} hbgr_face_t;

typedef struct hbgr_font_t {
    gr_font             *grfont;
    hb_face_t           *face;
    hb_font_t           *font;
    hb_destroy_func_t   destroy;
    hb_font_funcs_t     *klass;
    void                *data;
} hbgr_font_t;

typedef struct hbgr_cluster_t {
    unsigned int base_char;
    unsigned int num_chars;
    unsigned int base_glyph;
    unsigned int num_glyphs;
} hbgr_cluster_t;

static const void *hbgr_get_table(const void *face, unsigned int tag, size_t *len);
static float hbgr_get_advance(const void *font, unsigned short gid);
static void hbgr_face_destroy(void *data);
static hb_blob_t *hbgr_face_get_table(hb_face_t *face, hb_tag_t tag, void *data);
static void hbgr_font_destroy(void *data);
/* static hb_codepoint_t hbgr_font_get_glyph (hb_font_t *font, hb_face_t *face, const void *user_data, hb_codepoint_t unicode, hb_codepoint_t variation_selector);
static void hbgr_font_get_glyph_advance (hb_font_t *font, hb_face_t *face, const void *user_data, hb_codepoint_t glyph, hb_position_t *x_advance, hb_position_t *y_advance);
static void hbgr_font_get_glyph_extents (hb_font_t *font, hb_face_t *face, const void *user_data, hb_codepoint_t glyph, hb_glyph_extents_t *extents);
static hb_bool_t hbgr_font_get_contour_point (hb_font_t *font, hb_face_t *face, const void *user_data, unsigned int point_index, hb_codepoint_t glyph, hb_position_t *x, hb_position_t *y);
static hb_position_t hbgr_font_get_kerning (hb_font_t *font, hb_face_t *face, const void *user_data, hb_codepoint_t first_glyph, hb_codepoint_t second_glyph);
*/

static const void *hbgr_get_table(const void *data, unsigned int tag, size_t *len)
{
    const void *res;
    hbgr_tablelist_t *pl, *p;
    hbgr_face_t *face = (hbgr_face_t *)data;
    hbgr_tablelist_t *tlist = face->tlist;

    for (p = tlist; p; p = p->next)
        if (p->tag == tag)
            return hb_blob_get_data(p->blob, (unsigned int *)len);
        else
            pl = p;

    hb_blob_t *blob = face->get_table(face->face, tag, face->data);
    if (pl->blob)
    {
        p = (hbgr_tablelist_t *)malloc(sizeof(hbgr_tablelist_t));
        if (!p)
        {
            hb_blob_destroy(blob);
            return NULL;
        }
        p->next = NULL;
        pl->next = p;
        pl = p;
    }
    pl->blob = blob;
    pl->tag = tag;
    return hb_blob_get_data(blob, (unsigned int *)len);
}

static float hbgr_get_advance(const void *dat, unsigned short gid)
{
    hbgr_font_t *font = (hbgr_font_t *)dat;
    return (float)(font->klass->get.glyph_h_advance(font->font, font->data, gid, font->klass->user_data.glyph_h_advance));
}

static void hbgr_face_destroy(void *data)
{
    hbgr_face_t *fcomp = (hbgr_face_t *)data;
    hbgr_tablelist_t *tlist = fcomp->tlist;
    while (tlist)
    {
        hbgr_tablelist_t *old = tlist;
        hb_blob_destroy(tlist->blob);
        tlist = tlist->next;
        free(old);
    }
    fcomp->destroy(fcomp->data);
    gr_face_destroy(fcomp->grface);
}

static hb_blob_t *hbgr_face_get_table(hb_face_t *face, hb_tag_t tag, void *data)
{
    hbgr_face_t *fcomp = (hbgr_face_t *)data;
    return fcomp->get_table(fcomp->face, tag, fcomp->data);
}

static void hbgr_font_destroy(void *data)
{
    hbgr_font_t *fcomp = (hbgr_font_t *)data;
    fcomp->destroy(fcomp->data);
    gr_font_destroy(fcomp->grfont);
}

static hb_bool_t hbgr_font_get_glyph (hb_font_t *font,
        void *font_data,
        hb_codepoint_t unicode,
        hb_codepoint_t variation_selector,
        hb_codepoint_t *glyph,
        void *user_data)
{
    return ((hbgr_font_t *)font)->klass->get.glyph(((hbgr_font_t *)font)->font, font_data, unicode, variation_selector, glyph, user_data);
}

static hb_bool_t hbgr_font_get_glyph_h_advance (hb_font_t *font,
        void *font_data,
        hb_codepoint_t glyph,
        void *user_data)
{
    return ((hbgr_font_t *)font)->klass->get.glyph_h_advance(((hbgr_font_t *)font)->font, font_data, glyph, user_data);
}

static hb_position_t hbgr_font_get_glyph_v_advance (hb_font_t *font,
        void *font_data,
        hb_codepoint_t glyph,
        void *user_data)
{
    return ((hbgr_font_t *)font)->klass->get.glyph_v_advance(((hbgr_font_t *)font)->font, font_data, glyph, user_data);
}

static hb_bool_t hbgr_font_get_glyph_h_origin (hb_font_t *font,
        void *font_data,
        hb_codepoint_t glyph,
        hb_position_t *x,
        hb_position_t *y,
        void *user_data)
{
    return ((hbgr_font_t *)font)->klass->get.glyph_h_origin(((hbgr_font_t *)font)->font, font_data, glyph, x, y, user_data);
}

static hb_bool_t hbgr_font_get_glyph_v_origin (hb_font_t *font,
        void *font_data,
        hb_codepoint_t glyph,
        hb_position_t *x,
        hb_position_t *y,
        void *user_data)
{
    return ((hbgr_font_t *)font)->klass->get.glyph_v_origin(((hbgr_font_t *)font)->font, font_data, glyph, x, y, user_data);
}

static hb_position_t hbgr_font_get_glyph_h_kerning (hb_font_t *font,
        void *font_data,
        hb_codepoint_t first_glyph,
        hb_codepoint_t second_glyph,
        void *user_data)
{
    return ((hbgr_font_t *)font)->klass->get.glyph_h_kerning(((hbgr_font_t *)font)->font, font_data, first_glyph, second_glyph, user_data);
}

static hb_position_t hbgr_font_get_glyph_v_kerning (hb_font_t *font,
        void *font_data,
        hb_codepoint_t first_glyph,
        hb_codepoint_t second_glyph,
        void *user_data)
{
    return ((hbgr_font_t *)font)->klass->get.glyph_v_kerning(((hbgr_font_t *)font)->font, font_data, first_glyph, second_glyph, user_data);
}

static hb_bool_t hbgr_font_get_glyph_extents (hb_font_t *font,
        void *font_data,
        hb_codepoint_t glyph,
        hb_glyph_extents_t *extents,
        void *user_data)
{
    return ((hbgr_font_t *)font)->klass->get.glyph_extents(((hbgr_font_t *)font)->font, font_data, glyph, extents, user_data);
}

static hb_bool_t hbgr_font_get_glyph_contour_point (hb_font_t *font,
        void *font_data,
        hb_codepoint_t glyph,
        unsigned int point_index,
        hb_position_t *x,
        hb_position_t *y,
        void *user_data)
{
    return ((hbgr_font_t *)font)->klass->get.glyph_contour_point(((hbgr_font_t *)font)->font, font_data, glyph, point_index, x, y, user_data);
}

hb_font_funcs_t hbgr_klass = {
    HB_OBJECT_HEADER_STATIC,
    TRUE,
    {
        hbgr_font_get_glyph,
        hbgr_font_get_glyph_h_advance,
        hbgr_font_get_glyph_v_advance,
        hbgr_font_get_glyph_h_origin,
        hbgr_font_get_glyph_v_origin,
        hbgr_font_get_glyph_h_kerning,
        hbgr_font_get_glyph_v_kerning,
        hbgr_font_get_glyph_extents,
        hbgr_font_get_glyph_contour_point
    }
};

// returns 0 = success, 1 = fallback
static int hbgr_make_fonts(hb_font_t *font, hb_face_t *face)
{
    // check face for having already added gr_face (based on destroy fn)
    if (face->destroy != &hbgr_face_destroy)
    {
        hb_blob_t *silf_blob;
        silf_blob = hb_face_reference_table (font->face, HB_GRAPHITE_TAG_Silf);
        if (!hb_blob_get_length(silf_blob))     // return as early as possible if no Silf table
        {
            hb_blob_destroy(silf_blob);
            return 1;
        }
        hbgr_tablelist_t *flist = (hbgr_tablelist_t *)malloc(sizeof(hbgr_tablelist_t));
        hbgr_face_t *fcomp = (hbgr_face_t *)malloc(sizeof(hbgr_face_t));
        if (!flist || !fcomp) return 1;
        flist->next = NULL;
        flist->tag = 0;
        flist->blob = NULL;
        fcomp->tlist = flist;
        fcomp->get_table = face->reference_table;
        fcomp->destroy = face->destroy;
        fcomp->data = face->user_data;
        fcomp->grface = gr_make_face(fcomp, &hbgr_get_table, gr_face_preloadGlyphs);
        if (fcomp->grface)
        {
            face->user_data = fcomp;
            face->reference_table = &hbgr_face_get_table;
            face->destroy = &hbgr_face_destroy;
        }
        else
        {
            free(flist);
            free(fcomp);
            return 1;
        }
    }

    // check font for having already added gr_font (based on destroy fn)
    if (font->destroy != &hbgr_font_destroy)
    {
        hbgr_font_t *fcomp = (hbgr_font_t *)malloc(sizeof(hbgr_font_t));
        if (fcomp)
        {
            int scale;
            gr_face *grface = ((hbgr_face_t *)(face->user_data))->grface;
            hb_font_get_scale(font, &scale, NULL);
            fcomp->face = face;
            fcomp->font = font;
            fcomp->destroy = font->destroy;
            fcomp->klass = font->klass;
            fcomp->data = font->user_data;
            fcomp->grfont = gr_make_font_with_advance_fn(scale, fcomp, &hbgr_get_advance, grface);
            if (fcomp->grfont)
            {
                font->destroy = &hbgr_font_destroy;
                font->klass = &hbgr_klass;
                font->user_data = fcomp;
                return 0;
            }
        }
        // something went wrong, back out of the whole thing
        hbgr_face_t *facecomp = (hbgr_face_t *)(face->user_data);
        free(facecomp->tlist);
        face->reference_table = facecomp->get_table;
        face->destroy = facecomp->destroy;
        face->user_data = facecomp->data;
        free(facecomp);
        if (fcomp) free(fcomp);
        return 1;
    // create gr_font and put into struct along with stuff in font
    // replace font component with struct and fns
    }
    return 0;
}

hb_bool_t hb_graphite_shape (hb_font_t *font,
			hb_buffer_t  *buffer,
			const hb_feature_t *features,
			unsigned int  num_features,
            const char * const *shaper_options)
{
    hb_face_t *face = font->face;
    gr_face *grface = NULL;
    gr_font *grfont = NULL;
    gr_segment *seg = NULL;
    unsigned int *text = NULL;
    hbgr_cluster_t *clusters = NULL;
    unsigned short *gids = NULL;
    unsigned int charlen;
    hb_glyph_info_t *bufferi = hb_buffer_get_glyph_infos(buffer, &charlen);

    int success = 0;
    unsigned int *p;
    const gr_slot *is;
    unsigned int glyphlen;
    unsigned short *pg;
    unsigned int ci = 0, ic = 0;
    float curradvx = 0., curradvy = 0.;

    if (!charlen || hbgr_make_fonts(font, face)) return 0;
    grface = ((hbgr_face_t *)face->user_data)->grface;
    grfont = ((hbgr_font_t *)font->user_data)->grfont;

    const char *lang = hb_language_to_string(hb_buffer_get_language(buffer));
    gr_feature_val *feats = gr_face_featureval_for_lang(grface, lang ? hb_tag_from_string(lang) : 0);

    while (num_features--)
    {
        const gr_feature_ref *fref = gr_face_find_fref(grface, features->tag);
        if (fref)
            gr_fref_set_feature_value(fref, features->value, feats);
        features++;
    }
    text = (unsigned int *)malloc((charlen + 1) * sizeof(unsigned int));
    if (!text) goto dieout;
    p = text;
    for (unsigned int i = 0; i < charlen; ++i)
        *p++ = bufferi++->codepoint;
    *p = 0;

    hb_tag_t    script_tag_1, script_tag_2;
    hb_ot_tags_from_script(hb_buffer_get_script(buffer), &script_tag_1, &script_tag_2);
    seg = gr_make_seg(grfont, grface, script_tag_2 == HB_TAG_NONE ? script_tag_1 : script_tag_2,
            feats, gr_utf32, text, charlen, hb_buffer_get_direction(buffer) == HB_DIRECTION_RTL ? 3 : 0);
    if (!seg) goto dieout;
    glyphlen = gr_seg_n_slots(seg);
    clusters = (hbgr_cluster_t *)malloc(charlen * sizeof(hbgr_cluster_t));
    if (!glyphlen || !clusters) goto dieout;
    memset(clusters, 0, charlen * sizeof(hbgr_cluster_t));
    gids = (unsigned short *)malloc(glyphlen * sizeof(unsigned short));
    if (!gids) goto dieout;
    pg = gids;
    for (is = gr_seg_first_slot(seg), ic = 0; is; is = gr_slot_next_in_segment(is), ic++)
    {
        unsigned int before = gr_slot_before(is);
        unsigned int after = gr_slot_after(is);
        *pg = gr_slot_gid(is);
        *pg++ = hb_be_uint16(*pg);        // insane: swap bytes so be16 can swap them back
        while (clusters[ci].base_char > before && ci)
        {
            clusters[ci-1].num_chars += clusters[ci].num_chars;
            clusters[ci-1].num_glyphs += clusters[ci].num_glyphs;
            --ci;
        }

        if (gr_slot_can_insert_before(is) && clusters[ci].num_chars && before >= clusters[ci].base_char + clusters[ci].num_chars)
        {
            hbgr_cluster_t *c = clusters + ci + 1;
            c->base_char = clusters[ci].base_char + clusters[ci].num_chars;
            c->num_chars = before - c->base_char;
            c->base_glyph = ic;
            c->num_glyphs = 0;
            ++ci;
        }
        ++clusters[ci].num_glyphs;

        if (clusters[ci].base_char + clusters[ci].num_chars < after + 1)
            clusters[ci].num_chars = after + 1 - clusters[ci].base_char;
    }

    buffer->clear_output();
    for (unsigned int i = 0; i <= ci; ++i)
        buffer->replace_glyphs_be16(clusters[i].num_chars, clusters[i].num_glyphs, gids + clusters[i].base_glyph);
    buffer->swap_buffers();

    hb_glyph_position_t *pPos;
    for (pPos = hb_buffer_get_glyph_positions(buffer, NULL), is = gr_seg_first_slot(seg);
            is; pPos++, is = gr_slot_next_in_segment(is))
    {
        pPos->x_offset = gr_slot_origin_X(is) - curradvx;
        pPos->y_offset = gr_slot_origin_Y(is) - curradvy;
        pPos->x_advance = gr_slot_advance_X(is, grface, grfont);
        pPos->y_advance = gr_slot_advance_Y(is, grface, grfont);
//        if (pPos->x_advance < 0 && gr_slot_attached_to(is))
//            pPos->x_advance = 0;
        curradvx += pPos->x_advance;
        curradvy += pPos->y_advance;
    }
    pPos[-1].x_advance += gr_seg_advance_X(seg) - curradvx;
    success = 1;

dieout:
    if (gids) free(gids);
    if (clusters) free(clusters);
    if (seg) gr_seg_destroy(seg);
    if (text) free(text);
    return success;
}

int hb_graphite2_feature_check(const hb_feature_t *feats, unsigned int num_feats)
{
    while (num_feats--)
    {
        if (feats[num_feats].tag == HB_TAG(' ', 'R', 'N', 'D'))
            return (feats[num_feats].value == 1);
    }
    return 1;
}

HB_END_DECLS
