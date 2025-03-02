#include "hb-aat-var-hvgl-table.hh"

#ifndef HB_NO_VAR_HVF

#include "hb-draw.hh"
#include "hb-geometry.hh"
#include "hb-ot-layout-common.hh"

namespace AAT {

namespace hvgl_impl {


void
PartShape::get_path_at (hb_font_t *font,
		        hb_draw_session_t &draw_session,
		        hb_array_t<const int> coords,
		        hb_set_t *visited,
		        signed *edges_left,
		        signed depth_left) const
{
  // TODO
}


void
PartComposite::get_path_at (hb_font_t *font,
			    hb_draw_session_t &draw_session,
			    hb_array_t<const int> coords,
			    hb_set_t *visited,
			    signed *edges_left,
			    signed depth_left) const
{
  // TODO
}


} // namespace hvgl_impl
} // namespace AAT

#endif
