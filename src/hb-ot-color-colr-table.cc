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

  // TODO apply clipbox
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
  if (cl->is_variable)
    return reinterpret_cast<const OT::ColorLine<OT::Variable> *>(cl->base)->get_color_stops (start, count, color_stops, cl->c->instancer);
  else
    return reinterpret_cast<const OT::ColorLine<OT::NoVariable> *>(cl->base)->get_color_stops (start, count, color_stops, cl->c->instancer);
}

hb_paint_extend_t
hb_color_line_get_extend (hb_color_line_t *cl)
{
  if (cl->is_variable)
    return reinterpret_cast<const OT::ColorLine<OT::Variable> *>(cl->base)->get_extend ();
  else
    return reinterpret_cast<const OT::ColorLine<OT::NoVariable> *>(cl->base)->get_extend ();
}
