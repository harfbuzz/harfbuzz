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

#include "hb-ot-gasp-table.hh"
#include "hb-ot-os2-table.hh"
#include "hb-ot-post-table.hh"
#include "hb-ot-hmtx-table.hh"

#include "hb-ot-face.hh"

#if 0
static bool
_get_gasp (hb_font_t *font, unsigned int i, hb_position_t *result)
{
  const OT::GaspRange& range = font->face->table.gasp->get_gasp_range (i);
  if (&range == &Null (OT::GaspRange)) return false;
  if (result) *result = font->em_scale_y (range.rangeMaxPPEM);
  return true;
}
#endif

/**
 * hb_ot_metrics_get:
 * @font:
 * @metrics_tag:
 * @position:    (out) (optional):
 *
 * Returns: Whether found the requested metrics
 *
 * Since: REPLACEME
 **/
hb_bool_t
hb_ot_metrics_get (hb_font_t       *font,
		   hb_ot_metrics_t  metrics_tag,
		   hb_position_t   *position     /* OUT.  May be NULL. */)
{
  switch (metrics_tag)
  {
#define GET_METRIC(TABLE, ATTR, DIR) \
  (font->face->table.TABLE->has_data () && \
    (position && (*position = font->em_scale_##DIR (font->face->table.TABLE->ATTR)), true))
#define GET_X_METRIC(TABLE, ATTR) GET_METRIC (TABLE, ATTR, x)
#define GET_Y_METRIC(TABLE, ATTR) GET_METRIC (TABLE, ATTR, y)

  case HB_OT_METRICS_HORIZONTAL_ASCENDER:         return GET_Y_METRIC (hmtx, ascender);
  case HB_OT_METRICS_HORIZONTAL_DESCENDER:        return GET_Y_METRIC (hmtx, descender);
  case HB_OT_METRICS_HORIZONTAL_LINE_GAP:         return GET_Y_METRIC (hmtx, line_gap);
  case HB_OT_METRICS_HORIZONTAL_CLIPPING_ASCENT:  return GET_Y_METRIC (OS2, usWinAscent);
  case HB_OT_METRICS_HORIZONTAL_CLIPPING_DESCENT: return GET_Y_METRIC (OS2, usWinDescent);
  case HB_OT_METRICS_VERTICAL_ASCENDER:           return GET_X_METRIC (vmtx, ascender);
  case HB_OT_METRICS_VERTICAL_DESCENDER:          return GET_X_METRIC (vmtx, descender);
  case HB_OT_METRICS_VERTICAL_LINE_GAP:           return GET_X_METRIC (vmtx, line_gap);
  case HB_OT_METRICS_HORIZONTAL_CARET_RISE:       return GET_Y_METRIC (hhea, caretSlopeRise);
  case HB_OT_METRICS_HORIZONTAL_CARET_RUN:        return GET_Y_METRIC (hhea, caretSlopeRun);
  case HB_OT_METRICS_HORIZONTAL_CARET_OFFSET:     return GET_Y_METRIC (hhea, caretOffset);
  case HB_OT_METRICS_VERTICAL_CARET_RISE:         return GET_X_METRIC (vhea, caretSlopeRise);
  case HB_OT_METRICS_VERTICAL_CARET_RUN:          return GET_X_METRIC (vhea, caretSlopeRun);
  case HB_OT_METRICS_VERTICAL_CARET_OFFSET:       return GET_X_METRIC (vhea, caretOffset);
  case HB_OT_METRICS_X_HEIGHT:                    return GET_Y_METRIC (OS2->get_v2 ().thiz (), sxHeight);
  case HB_OT_METRICS_CAP_HEIGHT:                  return GET_Y_METRIC (OS2->get_v2 ().thiz (), sCapHeight);
  case HB_OT_METRICS_SUBSCRIPT_EM_X_SIZE:         return GET_X_METRIC (OS2, ySubscriptXSize);
  case HB_OT_METRICS_SUBSCRIPT_EM_Y_SIZE:         return GET_Y_METRIC (OS2, ySubscriptYSize);
  case HB_OT_METRICS_SUBSCRIPT_EM_X_OFFSET:       return GET_X_METRIC (OS2, ySubscriptXOffset);
  case HB_OT_METRICS_SUBSCRIPT_EM_Y_OFFSET:       return GET_Y_METRIC (OS2, ySubscriptYOffset);
  case HB_OT_METRICS_SUPERSCRIPT_EM_X_SIZE:       return GET_X_METRIC (OS2, ySuperscriptXSize);
  case HB_OT_METRICS_SUPERSCRIPT_EM_Y_SIZE:       return GET_Y_METRIC (OS2, ySuperscriptYSize);
  case HB_OT_METRICS_SUPERSCRIPT_EM_X_OFFSET:     return GET_X_METRIC (OS2, ySuperscriptXOffset);
  case HB_OT_METRICS_SUPERSCRIPT_EM_Y_OFFSET:     return GET_Y_METRIC (OS2, ySuperscriptYOffset);
  case HB_OT_METRICS_STRIKEOUT_SIZE:              return GET_Y_METRIC (OS2, yStrikeoutSize);
  case HB_OT_METRICS_STRIKEOUT_OFFSET:            return GET_Y_METRIC (OS2, yStrikeoutPosition);
  case HB_OT_METRICS_UNDERLINE_SIZE:              return GET_Y_METRIC (post->table, underlineThickness);
  case HB_OT_METRICS_UNDERLINE_OFFSET:            return GET_Y_METRIC (post->table, underlinePosition);
#undef GET_METRIC
#undef GET_X_METRIC
#undef GET_Y_METRIC
#if 0
  case HB_OT_METRICS_GASP_RANGE_0:                return _get_gasp (font, 0, position);
  case HB_OT_METRICS_GASP_RANGE_1:                return _get_gasp (font, 1, position);
  case HB_OT_METRICS_GASP_RANGE_2:                return _get_gasp (font, 2, position);
  case HB_OT_METRICS_GASP_RANGE_3:                return _get_gasp (font, 3, position);
  case HB_OT_METRICS_GASP_RANGE_4:                return _get_gasp (font, 4, position);
  case HB_OT_METRICS_GASP_RANGE_5:                return _get_gasp (font, 5, position);
  case HB_OT_METRICS_GASP_RANGE_6:                return _get_gasp (font, 6, position);
  case HB_OT_METRICS_GASP_RANGE_7:                return _get_gasp (font, 7, position);
  case HB_OT_METRICS_GASP_RANGE_8:                return _get_gasp (font, 8, position);
  case HB_OT_METRICS_GASP_RANGE_9:                return _get_gasp (font, 9, position);
#endif
  default:                                        return false;
  }
}
