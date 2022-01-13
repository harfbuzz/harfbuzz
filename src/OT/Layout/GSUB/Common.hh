#ifndef OT_LAYOUT_GSUB_COMMON
#define OT_LAYOUT_GSUB_COMMON

#include "hb-serialize.hh"

namespace OT {
namespace Layout {
namespace GSUB {

typedef hb_pair_t<hb_codepoint_t, hb_codepoint_t> hb_codepoint_pair_t;

template<typename Iterator>
static void SingleSubst_serialize (hb_serialize_context_t *c,
                                   Iterator it);

}
}
}

#endif
