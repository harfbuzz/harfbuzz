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

#include "hb.hh"

#include "hb-ot-hhea-table.hh"
#include "hb-ot-os2-table.hh"
#include "hb-ot-post-table.hh"

#include "hb-ot-face.hh"

/**
 * hb_ot_metrics_get:
 *
 * Returns: Whether found the requested metrics,
 * Since: REPLACEME
 **/
hb_bool_t
hb_ot_metrics_get (hb_font_t       *font,
		   hb_ot_metrics_t  metrics_tag,
		   hb_position_t   *position)
{
#define GET_METRICS(TABLE, ATTR, DIR) \
  if (font->face->table.TABLE->has_data ()) \
  { \
    if (position) *position = font->em_scale_##DIR (font->face->table.TABLE->ATTR); \
    return true; \
  } \
  else return false

  switch (metrics_tag)
  {
  case HB_OT_METRICS_HORIZONTAL_ASCENDER:		GET_METRICS (OS2, sTypoAscender, y);
  case HB_OT_METRICS_HORIZONTAL_DESCENDER:		GET_METRICS (OS2, sTypoDescender, y);
  case HB_OT_METRICS_HORIZONTAL_LINE_GAP:		GET_METRICS (OS2, sTypoLineGap, y);
  case HB_OT_METRICS_HORIZONTAL_CLIPPING_ASCENT:	GET_METRICS (OS2, usWinAscent, y);
  case HB_OT_METRICS_HORIZONTAL_CLIPPING_DESCENT:	GET_METRICS (OS2, usWinDescent, y);
  case HB_OT_METRICS_VERTICAL_ASCENDER:			GET_METRICS (vhea, ascender, x);
  case HB_OT_METRICS_VERTICAL_DESCENDER:		GET_METRICS (vhea, descender, x);
  case HB_OT_METRICS_VERTICAL_LINE_GAP:			GET_METRICS (vhea, lineGap, x);
  case HB_OT_METRICS_HORIZONTAL_CARET_RISE:		GET_METRICS (hhea, caretSlopeRise, y);
  case HB_OT_METRICS_HORIZONTAL_CARET_RUN:		GET_METRICS (hhea, caretSlopeRun, y);
  case HB_OT_METRICS_HORIZONTAL_CARET_OFFSET:		GET_METRICS (hhea, caretOffset, y);
  case HB_OT_METRICS_VERTICAL_CARET_RISE:		GET_METRICS (vhea, caretSlopeRise, x);
  case HB_OT_METRICS_VERTICAL_CARET_RUN:		GET_METRICS (vhea, caretSlopeRun, x);
  case HB_OT_METRICS_VERTICAL_CARET_OFFSET:		GET_METRICS (hhea, caretOffset, x);
//   case HB_OT_METRICS_X_HEIGHT:				HB_TAG ('x','h','g','t'), /* OS/2.sxHeight */
//   case HB_OT_METRICS_CAP_HEIGHT:			HB_TAG ('c','p','h','t'), /* OS/2.sCapHeight */
  case HB_OT_METRICS_SUBSCRIPT_EM_X_SIZE:		GET_METRICS (OS2, ySubscriptXSize, x);
  case HB_OT_METRICS_SUBSCRIPT_EM_Y_SIZE:		GET_METRICS (OS2, ySubscriptYSize, y);
  case HB_OT_METRICS_SUBSCRIPT_EM_X_OFFSET:		GET_METRICS (OS2, ySubscriptXOffset, x);
  case HB_OT_METRICS_SUBSCRIPT_EM_Y_OFFSET:		GET_METRICS (OS2, ySubscriptYOffset, x);
  case HB_OT_METRICS_SUPERSCRIPT_EM_X_SIZE:		GET_METRICS (OS2, ySuperscriptXSize, x);
  case HB_OT_METRICS_SUPERSCRIPT_EM_Y_SIZE:		GET_METRICS (OS2, ySuperscriptYSize, y);
  case HB_OT_METRICS_SUPERSCRIPT_EM_X_OFFSET:		GET_METRICS (OS2, ySuperscriptXOffset, x);
  case HB_OT_METRICS_SUPERSCRIPT_EM_Y_OFFSET:		GET_METRICS (OS2, ySuperscriptYOffset, y);
  case HB_OT_METRICS_STRIKEOUT_SIZE:			GET_METRICS (OS2, yStrikeoutSize, y);
  case HB_OT_METRICS_STRIKEOUT_OFFSET:			GET_METRICS (OS2, yStrikeoutPosition, y);
  case HB_OT_METRICS_UNDERLINE_SIZE:			GET_METRICS (post->table, underlineThickness, y);
  case HB_OT_METRICS_UNDERLINE_OFFSET:			GET_METRICS (post->table, underlinePosition, y);
//   case HB_OT_METRICS_GASP_RANGE_0:			HB_TAG ('g','s','p','0'), /* gasp.gaspRange[0].rangeMaxPPEM */
//   case HB_OT_METRICS_GASP_RANGE_1:			HB_TAG ('g','s','p','1'), /* gasp.gaspRange[1].rangeMaxPPEM */
//   case HB_OT_METRICS_GASP_RANGE_2:			HB_TAG ('g','s','p','2'), /* gasp.gaspRange[2].rangeMaxPPEM */
//   case HB_OT_METRICS_GASP_RANGE_3:			HB_TAG ('g','s','p','3'), /* gasp.gaspRange[3].rangeMaxPPEM */
//   case HB_OT_METRICS_GASP_RANGE_4:			HB_TAG ('g','s','p','4'), /* gasp.gaspRange[4].rangeMaxPPEM */
//   case HB_OT_METRICS_GASP_RANGE_5:			HB_TAG ('g','s','p','5'), /* gasp.gaspRange[5].rangeMaxPPEM */
//   case HB_OT_METRICS_GASP_RANGE_6:			HB_TAG ('g','s','p','6'), /* gasp.gaspRange[6].rangeMaxPPEM */
//   case HB_OT_METRICS_GASP_RANGE_7:			HB_TAG ('g','s','p','7'), /* gasp.gaspRange[7].rangeMaxPPEM */
//   case HB_OT_METRICS_GASP_RANGE_8:			HB_TAG ('g','s','p','8'), /* gasp.gaspRange[8].rangeMaxPPEM */
//   case HB_OT_METRICS_GASP_RANGE_9:			HB_TAG ('g','s','p','9'), /* gasp.gaspRange[9].rangeMaxPPEM */
  default: return false;
  }
#undef GET_METRICS
}
