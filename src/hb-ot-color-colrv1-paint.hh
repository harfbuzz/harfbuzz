/*
 * Copyright © 2022 Matthias Clasen
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
 */

#ifndef HB_OT_COLR_COLRV1_PAINT_HH
#define HB_OT_COLR_COLRV1_PAINT_HH

#include "hb-open-type.hh"
#include "hb-ot-layout-common.hh"
#include "hb-ot-color-colr-table.hh"

/*
 * COLR -- Color
 * https://docs.microsoft.com/en-us/typography/opentype/spec/colr
 */
namespace OT {

struct hb_colrv1_paint_context_t :
       hb_dispatch_context_t<hb_colrv1_paint_context_t>
{
  template <typename T>
  return_t dispatch (const T &obj)
  {
    obj.paint (this);
    return hb_empty_t ();
  }

  const COLR* get_colr_table () const
  { return reinterpret_cast<const COLR *> (base); }

  public:
  const void *base;
  hb_paint_funcs_t *funcs;
  void *paint_data;

  hb_colrv1_paint_context_t (const void *base_, hb_paint_funcs_t *funcs_, void *paint_data_)
                             base (base_), funcs (funcs_), paint_data (paint_data_)
  {}

  void push_transform (float xx, float yx,
                       float xy, float yy,
                       float x0, float y0)
  {
    funcs->push_transform (paint_data, xx, yx, xy, yy, x0, y0);
  }

  void pop_transform ()
  {
    funcs->pop_transform (paint_data);
  }

  void push_clip (hb_codepoint_t gid)
  {
    funcs->push_clip (paint_data, gid);
  }

  void pop_clip ()
  {
    funcs->pop_clip (paint_data);
  }

  void solid (unsigned int color_index)
  {
    funcs->solid (paint_data, color_index);
  }

  void linear_gradient (hb_color_line *color_line,
                        float x0, float y0,
                        float x1, float y1,
                        float x2, float y2)
  {
    funcs->linear_gradient (paint_data,
                            color_line, x0, y0, x1, y1, x2, y2);
  }

  void radial_gradient (hb_color_line *color_line,
                        float x0, float y0, float r0,
                        float x1, float y1, float r1)
  {
    funcs->radial_gradient (paint_data,
                            color_line, x0, y0, r0, x1, y1, r1);
  }

  void sweep_gradient (hb_color_line *color_line,
                       float x0, float y0,
                       float start_angle, float end_angle)'
  {
    funcs->sweep_gradient (paint_data,
                           color_line, x0, y0, start_angle, end_angle);
  }

  void push_group ()
  {
    funcs->push_group (paint_data);
  }

  void pop_group_and_composite (hb_paint_composite_mode_t mode)
  {
    funcs->pop_group_and_composite (paint_data, mode);
  }
}

HB_INTERNAL void PaintColrLayers::paint (hb_colrv1_paint_context_t *c) const
{
  const COLR *colr_table = c->get_colr_table ();
  const LayerList &paint_offset_lists = colr_table->get_layerList ();
  for (unsigned i = firstLayerIndex; i < firstLayerIndex + numLayers; i++)
  {
    const Paint &paint = std::addressof (paint_offset_lists) + paint_offset_lists[i];
    c->push_group ();
    paint.dispatch (c);
    c->pop_group_and_composite (HB_PAINT_COMPOSITE_MODE_OVER);
  }
}

HB_INTERNAL void PaintGlyph::paint (hb_colrv1_paint_context_t *c) const
{
  c->push_clip (gid);
  (this+paint).dispatch (c);
  c->pop_clip ();
}

HB_INTERNAL void PaintColrGlyph::paint (hb_colrv1_paint_context_t *c) const
{
  const COLR *colr_table = c->get_colr_table ();
  const BaseGlyphPaintRecord* baseglyph_paintrecord = colr_table->get_base_glyph_paintrecord (gid);
  if (!baseglyph_paintrecord) return;

  c->push_clip (gid);
  const BaseGlyphList &baseglyph_list = colr_table->get_baseglyphList ();
  (&baseglyph_list+baseglyph_paintrecord->paint).dispatch (c);
  c->pop_clip ();
}

template <template<typename> class Var>
HB_INTERNAL void PaintTransform<Var>::paint (hb_colrv1_paint_context_t* c) const
{
  c->push_transform (transform.xx, transform.yx,
                     transform.xy, transform.yy,
                     transform.dx, transform.dy);
  (this+src).dispatch (c);
  c->pop_transform ();
}

HB_INTERNAL void PaintTranslate::paint (hb_colrv1_paint_context_t* c) const
{
  c->push_transform (0, 0, 0, 0, dx, dy);
  (this+src).dispatch (c);
  c->pop_transform ();
}

HB_INTERNAL void PaintScale::paint (hb_colrv1_paint_context_t* c) const
{
  c->push_transform (scaleX, 0, 0, scaleY, 0, 0);
  (this+src).dispatch (c);
  c->pop_transform ();
}

HB_INTERNAL void PaintScaleAroundCenter::paint (hb_colrv1_paint_context_t* c) const
{
  c->push_transform (0, 0, 0, 0, - centerX, - centerY);
  c->push_transform (scaleX, 0, 0, scaleY, 0, 0);
  c->push_transform (0, 0, 0, 0, centerX, centerY);
  (this+src).dispatch (c);
  c->pop_transform ();
  c->pop_transform ();
  c->pop_transform ();
}

HB_INTERNAL void PaintScaleUniform::paint (hb_colrv1_paint_context_t* c) const
{
  c->push_transform (scale, 0, 0, scale, 0, 0);
  (this+src).dispatch (c);
  c->pop_transform ();
}

HB_INTERNAL void PaintScaleUniformAroundCenter::paint (hb_colrv1_paint_context_t* c) const
{
  c->push_transform (0, 0, 0, 0, - centerX, - centerY);
  c->push_transform (scale, 0, 0, scale, 0, 0);
  c->push_transform (0, 0, 0, 0, centerX, centerY);
  (this+src).dispatch (c);
  c->pop_transform ();
  c->pop_transform ();
  c->pop_transform ();
}

HB_INTERNAL void PaintRotate::paint (hb_colrv1_paint_context_t* c) const
{
  c->push_transform (cos (angle), sin (angle),
                     - sin (angle), cos (angle),
                     0, 0);
  (this+src).dispatch (c);
  c->pop_transform ();
}

HB_INTERNAL void PaintRotateAroundCenter::paint (hb_colrv1_paint_context_t* c) const
{
  c->push_transform (0, 0, 0, 0, - centerX, - centerY);
  c->push_transform (cos (angle), sin (angle),
                     - sin (angle), cos (angle),
                     0, 0);
  c->push_transform (0, 0, 0, 0, centerX, centerY);
  (this+src).dispatch (c);
  c->pop_transform ();
  c->pop_transform ();
  c->pop_transform ();
}

HB_INTERNAL void PaintSkew::paint (hb_colrv1_paint_context_t* c) const
{
  c->push_transform (1, tan (ySkewAngle),
                     - tan (xSkewAngle), 1,
                     0, 0);
  (this+src).dispatch (c);
  c->pop_transform ();
}

HB_INTERNAL void PaintSkewAroundCenter::paint (hb_colrv1_paint_context_t* c) const
{
  c->push_transform (0, 0, 0, 0, - centerX, - centerY);
  c->push_transform (1, tan (ySkewAngle),
                     - tan (xSkewAngle), 1,
                     0, 0);
  c->push_transform (0, 0, 0, 0, centerX, centerY);
  (this+src).dispatch (c);
  c->pop_transform ();
  c->pop_transform ();
  c->pop_transform ();
}

HB_INTERNAL void PaintComposite::paint (hb_colrv1_paint_context_t* c) const
{
  c->push_group ();
  (this+src).dispatch (c);
  c->push_group ();
  (this+backdrop).dispatch (c);
  c->pop_group_and_composite (mode);
  c->pop_group_and_composite (HB_PAINT_COMPOSITE_MODE_OVER);
}

HB_INTERNAL void PaintSolid::paint (hb_colrv1_paint_context_t *c) const
{
  c->solid (paletteIndex);
}

HB_INTERNAL void PaintLinearGradient::paint (hb_colrv1_paint_context_t *c) const
{
  c->linear_gradient (color_line, x0, y0, x1, y1, x2, y2);
}

HB_INTERNAL void PaintRadialGradient::paint (hb_colrv1_paint_context_t *c) const
{
  c->radial_gradient (color_line, x0, y0, radius0, x1, y1, radius1);
}

HB_INTERNAL void PaintSweepGradient::paint (hb_colrv1_paint_context_t *c) const
{
  c->sweep_gradient (color_line, centerX, centerY, startAngle, endAngle);
}

} /* namespace OT */


#endif /* HB_OT_COLR_COLRV1_CLOSURE_HH */
