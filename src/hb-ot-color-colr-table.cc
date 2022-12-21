#include "hb-ot-color-colr-table.hh"

namespace OT {

void PaintColrLayers::paint_glyph (hb_ot_paint_context_t *c) const
{
  const LayerList &paint_offset_lists = c->get_colr_table ()->get_layerList ();
  for (unsigned i = firstLayerIndex; i < firstLayerIndex + numLayers; i++)
  {
    const Paint &paint = paint_offset_lists.get_paint (i);
    c->funcs->push_group (c->data, &c->ctx);
    c->recurse (paint);
    c->funcs->pop_group (c->data, HB_PAINT_COMPOSITE_MODE_SRC_OVER, &c->ctx);
  }
}

void PaintColrGlyph::paint_glyph (hb_ot_paint_context_t *c) const
{
  const COLR *colr_table = c->get_colr_table ();
  const Paint *paint = colr_table->get_base_glyph_paint (gid);

  // TODO apply clipbox
  if (paint)
    c->recurse (*paint);
}

}

/**
 * hb_color_line_get_color_stops:
 * @color_line: a #hb_color_line_t object
 * @start: the index of the first color stop to return
 * @count: (inout) (optional): Input = the maximum number of feature tags to return;
 *     Output = the actual number of feature tags returned (may be zero)
 * @color_stops: (out) (array length=count) (optional): Array of #hb_color_stop_t to populate
 *
 * Fetches a list of color stops from the given color line object.
 *
 * Note that due to variations being applied, the returned color stops
 * may be out of order. It is the callers responsibility to ensure that
 * color stops are sorted by their offset before they are used.
 *
 * Return value: the total number of color stops in @cl
 *
 * Since: REPLACEME
 */
unsigned int
hb_color_line_get_color_stops (hb_color_line_t *color_line,
                               unsigned int start,
                               unsigned int *count,
                               hb_color_stop_t *color_stops)
{
  if (color_line->is_variable)
    return reinterpret_cast<const OT::ColorLine<OT::Variable> *>(color_line->base)->get_color_stops (&color_line->c->ctx, start, count, color_stops, color_line->c->instancer);
  else
    return reinterpret_cast<const OT::ColorLine<OT::NoVariable> *>(color_line->base)->get_color_stops (&color_line->c->ctx, start, count, color_stops, color_line->c->instancer);
}

/**
 * hb_color_line_get_extend:
 * @color_line: a #hb_color_line_t object
 *
 * Fetches the extend mode of the color line object.
 *
 * Since: REPLACEME
 */
hb_paint_extend_t
hb_color_line_get_extend (hb_color_line_t *color_line)
{
  if (color_line->is_variable)
    return reinterpret_cast<const OT::ColorLine<OT::Variable> *>(color_line->base)->get_extend ();
  else
    return reinterpret_cast<const OT::ColorLine<OT::NoVariable> *>(color_line->base)->get_extend ();
}
