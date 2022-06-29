#ifndef OT_LAYOUT_GPOS_MARKBASEPOS_HH
#define OT_LAYOUT_GPOS_MARKBASEPOS_HH

#include "MarkBasePosFormat1.hh"

namespace OT {
namespace Layout {
namespace GPOS_impl {

struct MarkBasePos
{
  protected:
  union {
  HBUINT16              format;         /* Format identifier */
  MarkBasePosFormat1    format1;
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
};

}
}
}

#endif /* OT_LAYOUT_GPOS_MARKBASEPOS_HH */
