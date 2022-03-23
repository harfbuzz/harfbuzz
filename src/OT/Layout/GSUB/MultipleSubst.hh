#ifndef OT_LAYOUT_GSUB_MULTIPLESUBST_HH
#define OT_LAYOUT_GSUB_MULTIPLESUBST_HH

#include "Common.hh"
#include "MultipleSubstFormat1.hh"

namespace OT {
namespace Layout {
namespace GSUB {

struct MultipleSubst
{
  protected:
  union {
  HBUINT16              format;         /* Format identifier */
  MultipleSubstFormat1  format1;
  } u;

  public:

  template <typename context_t, typename ...Ts>
  typename context_t::return_t dispatch (context_t *c, Ts&&... ds) const
  {
    TRACE_DISPATCH (this, u.format);
    if (unlikely (!c->may_dispatch (this, &u.format))) return_trace (c->no_dispatch_return_value ());
    switch (u.format) {
    case 1: return_trace (c->dispatch (u.format1, std::forward<Ts> (ds)...));
    default:return_trace (c->default_return_value ());
    }
  }

  bool serialize (hb_serialize_context_t *c,
                  hb_sorted_array_t<const HBGlyphID16> glyphs,
                  hb_array_t<const unsigned int> substitute_len_list,
                  hb_array_t<const HBGlyphID16> substitute_glyphs_list)
  {
    TRACE_SERIALIZE (this);
    if (unlikely (!c->extend_min (u.format))) return_trace (false);
    unsigned int format = 1;
    u.format = format;
    switch (u.format) {
    case 1: return_trace (u.format1.serialize (c, glyphs, substitute_len_list, substitute_glyphs_list));
    default:return_trace (false);
    }
  }
};


}
}
}

#endif /* OT_LAYOUT_GSUB_MULTIPLESUBST_HH */
