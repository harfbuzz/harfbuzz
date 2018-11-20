/*
 * Copyright © 2018  Ebrahim Byagowi
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

#ifndef HB_OT_H_IN
#error "Include <hb-ot.h> instead."
#endif

#ifndef HB_OT_METRICS_H
#define HB_OT_METRICS_H

#include "hb.h"
#include "hb-ot-name.h"

HB_BEGIN_DECLS


/**
 * hb_ot_metrics_t:
 *
 * From https://docs.microsoft.com/en-us/typography/opentype/spec/mvar#value-tags
 *
 * Since: REPLACEME
 **/
typedef enum {
  HB_OT_METRICS_HORIZONTAL_ASCENDER		= HB_TAG ('h','a','s','c'), /* OS/2.sTypoAscender */
  HB_OT_METRICS_HORIZONTAL_DESCENDER		= HB_TAG ('h','d','s','c'), /* OS/2.sTypoDescender */
  HB_OT_METRICS_HORIZONTAL_LINE_GAP		= HB_TAG ('h','l','g','p'), /* OS/2.sTypoLineGap */
  HB_OT_METRICS_HORIZONTAL_CLIPPING_ASCENT	= HB_TAG ('h','c','l','a'), /* OS/2.usWinAscent */
  HB_OT_METRICS_HORIZONTAL_CLIPPING_DESCENT	= HB_TAG ('h','c','l','d'), /* OS/2.usWinDescent */
  HB_OT_METRICS_VERTICAL_ASCENDER		= HB_TAG ('v','a','s','c'), /* vhea.ascent */
  HB_OT_METRICS_VERTICAL_DESCENDER		= HB_TAG ('v','d','s','c'), /* vhea.descent */
  HB_OT_METRICS_VERTICAL_LINE_GAP		= HB_TAG ('v','l','g','p'), /* vhea.lineGap */
  HB_OT_METRICS_HORIZONTAL_CARET_RISE		= HB_TAG ('h','c','r','s'), /* hhea.caretSlopeRise */
  HB_OT_METRICS_HORIZONTAL_CARET_RUN		= HB_TAG ('h','c','r','n'), /* hhea.caretSlopeRun */
  HB_OT_METRICS_HORIZONTAL_CARET_OFFSET		= HB_TAG ('h','c','o','f'), /* hhea.caretOffset */
  HB_OT_METRICS_VERTICAL_CARET_RISE		= HB_TAG ('v','c','r','s'), /* vhea.caretSlopeRise */
  HB_OT_METRICS_VERTICAL_CARET_RUN		= HB_TAG ('v','c','r','n'), /* vhea.caretSlopeRun */
  HB_OT_METRICS_VERTICAL_CARET_OFFSET		= HB_TAG ('v','c','o','f'), /* vhea.caretOffset */
  HB_OT_METRICS_X_HEIGHT			= HB_TAG ('x','h','g','t'), /* OS/2.sxHeight */
  HB_OT_METRICS_CAP_HEIGHT			= HB_TAG ('c','p','h','t'), /* OS/2.sCapHeight */
  HB_OT_METRICS_SUBSCRIPT_EM_X_SIZE		= HB_TAG ('s','b','x','s'), /* OS/2.ySubscriptXSize */
  HB_OT_METRICS_SUBSCRIPT_EM_Y_SIZE		= HB_TAG ('s','b','y','s'), /* OS/2.ySubscriptYSize */
  HB_OT_METRICS_SUBSCRIPT_EM_X_OFFSET		= HB_TAG ('s','b','x','o'), /* OS/2.ySubscriptXOffset */
  HB_OT_METRICS_SUBSCRIPT_EM_Y_OFFSET		= HB_TAG ('s','b','y','o'), /* OS/2.ySubscriptYOffset */
  HB_OT_METRICS_SUPERSCRIPT_EM_X_SIZE		= HB_TAG ('s','p','x','s'), /* OS/2.ySuperscriptXSize */
  HB_OT_METRICS_SUPERSCRIPT_EM_Y_SIZE		= HB_TAG ('s','p','y','s'), /* OS/2.ySuperscriptYSize */
  HB_OT_METRICS_SUPERSCRIPT_EM_X_OFFSET		= HB_TAG ('s','p','x','o'), /* OS/2.ySuperscriptXOffset */
  HB_OT_METRICS_SUPERSCRIPT_EM_Y_OFFSET		= HB_TAG ('s','p','y','o'), /* OS/2.ySuperscriptYOffset */
  HB_OT_METRICS_STRIKEOUT_SIZE			= HB_TAG ('s','t','r','s'), /* OS/2.yStrikeoutSize */
  HB_OT_METRICS_STRIKEOUT_OFFSET		= HB_TAG ('s','t','r','o'), /* OS/2.yStrikeoutPosition */
  HB_OT_METRICS_UNDERLINE_SIZE			= HB_TAG ('u','n','d','s'), /* post.underlineThickness */
  HB_OT_METRICS_UNDERLINE_OFFSET		= HB_TAG ('u','n','d','o'), /* post.underlinePosition */
  HB_OT_METRICS_GASP_RANGE_0			= HB_TAG ('g','s','p','0'), /* gasp.gaspRange[0].rangeMaxPPEM */
  HB_OT_METRICS_GASP_RANGE_1			= HB_TAG ('g','s','p','1'), /* gasp.gaspRange[1].rangeMaxPPEM */
  HB_OT_METRICS_GASP_RANGE_2			= HB_TAG ('g','s','p','2'), /* gasp.gaspRange[2].rangeMaxPPEM */
  HB_OT_METRICS_GASP_RANGE_3			= HB_TAG ('g','s','p','3'), /* gasp.gaspRange[3].rangeMaxPPEM */
  HB_OT_METRICS_GASP_RANGE_4			= HB_TAG ('g','s','p','4'), /* gasp.gaspRange[4].rangeMaxPPEM */
  HB_OT_METRICS_GASP_RANGE_5			= HB_TAG ('g','s','p','5'), /* gasp.gaspRange[5].rangeMaxPPEM */
  HB_OT_METRICS_GASP_RANGE_6			= HB_TAG ('g','s','p','6'), /* gasp.gaspRange[6].rangeMaxPPEM */
  HB_OT_METRICS_GASP_RANGE_7			= HB_TAG ('g','s','p','7'), /* gasp.gaspRange[7].rangeMaxPPEM */
  HB_OT_METRICS_GASP_RANGE_8			= HB_TAG ('g','s','p','8'), /* gasp.gaspRange[8].rangeMaxPPEM */
  HB_OT_METRICS_GASP_RANGE_9			= HB_TAG ('g','s','p','9'), /* gasp.gaspRange[9].rangeMaxPPEM */
} hb_ot_metrics_t;

HB_EXTERN hb_bool_t
hb_ot_metrics_get (hb_font_t       *font,
		   hb_ot_metrics_t  metrics_tag,
		   hb_position_t   *position);

HB_END_DECLS

#endif /* HB_OT_METRICS_H */
