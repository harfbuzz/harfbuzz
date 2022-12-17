#include "hb-ot-color-colr-table.hh"

namespace OT {

void PaintColrLayers::paint_glyph (hb_paint_context_t *c) const
{
  const LayerList &paint_offset_lists = c->get_colr_table ()->get_layerList ();
  for (unsigned i = firstLayerIndex; i < firstLayerIndex + numLayers; i++)
  {
    const Paint &paint = paint_offset_lists.get_paint (i);
    c->funcs->push_group (c->data);
    c->recurse (paint);
    c->funcs->pop_group (c->data, HB_PAINT_COMPOSITE_MODE_SRC_OVER);
  }
}

void PaintColrGlyph::paint_glyph (hb_paint_context_t *c) const
{
  const COLR *colr_table = c->get_colr_table ();
  const Paint *paint = colr_table->get_base_glyph_paint (gid);

  if (paint)
    c->recurse (*paint);
}

}

unsigned int
hb_color_line_get_color_stops (hb_color_line_t *cl,
                               unsigned int start,
                               unsigned int *count,
                               hb_color_stop_t *color_stops)
{
  switch (cl->format)
  {
  case 4:
    return reinterpret_cast<const OT::PaintLinearGradient<OT::NoVariable> *>(cl->base)->get_color_stops (start, count, color_stops);
  case 5:
    return reinterpret_cast<const OT::PaintLinearGradient<OT::Variable> *>(cl->base)->get_color_stops (start, count, color_stops);
  case 6:
    return reinterpret_cast<const OT::PaintRadialGradient<OT::NoVariable> *>(cl->base)->get_color_stops (start, count, color_stops);
  case 7:
    return reinterpret_cast<const OT::PaintRadialGradient<OT::Variable> *>(cl->base)->get_color_stops (start, count, color_stops);
  case 8:
    return reinterpret_cast<const OT::PaintSweepGradient<OT::NoVariable> *>(cl->base)->get_color_stops (start, count, color_stops);
  case 9:
    return reinterpret_cast<const OT::PaintSweepGradient<OT::Variable> *>(cl->base)->get_color_stops (start, count, color_stops);
  default: assert (0);
  }
  return 0;
}

hb_paint_extend_t
hb_color_line_get_extend (hb_color_line_t *cl)
{
  switch (cl->format)
  {
  case 4:
    return reinterpret_cast<const OT::PaintLinearGradient<OT::NoVariable> *>(cl->base)->get_extend ();
  case 5:
    return reinterpret_cast<const OT::PaintLinearGradient<OT::Variable> *>(cl->base)->get_extend ();
  case 6:
    return reinterpret_cast<const OT::PaintRadialGradient<OT::NoVariable> *>(cl->base)->get_extend ();
  case 7:
    return reinterpret_cast<const OT::PaintRadialGradient<OT::Variable> *>(cl->base)->get_extend ();
  case 8:
    return reinterpret_cast<const OT::PaintSweepGradient<OT::NoVariable> *>(cl->base)->get_extend ();
  case 9:
    return reinterpret_cast<const OT::PaintSweepGradient<OT::Variable> *>(cl->base)->get_extend ();
  default: assert (0);
  }
  return HB_PAINT_EXTEND_PAD;
}
