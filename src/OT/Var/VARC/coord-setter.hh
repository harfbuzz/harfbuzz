#ifndef OT_VAR_VARC_COORD_SETTER_HH
#define OT_VAR_VARC_COORD_SETTER_HH


#include "../../../hb.hh"


namespace OT {
//namespace Var {


struct coord_setter_t
{
  coord_setter_t (hb_array_t<const int> coords_)
  {
    length = coords_.length;
    if (length <= ARRAY_LENGTH (static_coords))
      hb_memcpy (static_coords, coords_.arrayZ, length * sizeof (int));
    else
      dynamic_coords.extend (coords_);
  }

  int& operator [] (unsigned idx)
  {
    if (unlikely (idx >= HB_VAR_COMPOSITE_MAX_AXES))
      return Crap(int);

    if (length <= ARRAY_LENGTH (static_coords))
    {
      if (idx < ARRAY_LENGTH (static_coords))
      {
        while (length <= idx)
	  static_coords[length++] = 0;
	return static_coords[idx];
      }
      else
      {
        dynamic_coords.extend (hb_array (static_coords, length));
      }
    }

    if (dynamic_coords.length <= idx &&
	unlikely (!dynamic_coords.resize (idx + 1)))
      return Crap(int);
    return dynamic_coords.arrayZ[idx];
  }

  hb_array_t<int> get_coords ()
  { return dynamic_coords ? dynamic_coords.as_array () : hb_array (static_coords, length); }

  unsigned length;
  int static_coords[32];
  hb_vector_t<int> dynamic_coords;
};


//} // namespace Var

} // namespace OT

#endif /* OT_VAR_VARC_COORD_SETTER_HH */
