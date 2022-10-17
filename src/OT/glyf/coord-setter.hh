#ifndef OT_GLYF_COORD_SETTER_HH
#define OT_GLYF_COORD_SETTER_HH


#include "../../hb.hh"


namespace OT {
namespace glyf_impl {


struct coord_setter_t
{
  coord_setter_t (hb_font_t *font) :
    font (font),
    old_coords (font->coords, font->num_coords),
    new_coords (old_coords)
  {
    font->coords = new_coords.arrayZ;
    font->num_coords = new_coords.length;
  }

  ~coord_setter_t ()
  {
    font->coords = old_coords.arrayZ;
    font->num_coords = old_coords.length;
  }

  int& operator [] (unsigned idx)
  {
    if (new_coords.length < idx + 1)
    {
      new_coords.resize (idx + 1);
      font->coords = new_coords.arrayZ;
      font->num_coords = new_coords.length;
    }
    return new_coords[idx];
  }

  hb_font_t *font;
  hb_array_t<int> old_coords;
  hb_vector_t<int> new_coords;
};


} /* namespace glyf_impl */
} /* namespace OT */

#endif /* OT_GLYF_COORD_SETTER_HH */
