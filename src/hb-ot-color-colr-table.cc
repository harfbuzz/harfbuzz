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
