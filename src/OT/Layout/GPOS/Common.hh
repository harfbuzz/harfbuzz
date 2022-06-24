#ifndef OT_LAYOUT_GPOS_COMMON_HH
#define OT_LAYOUT_GPOS_COMMON_HH

namespace OT {
namespace Layout {
namespace GPOS {

template<typename Iterator, typename SrcLookup>
static void SinglePos_serialize (hb_serialize_context_t *c,
                                 const SrcLookup *src,
                                 Iterator it,
                                 const hb_map_t *layout_variation_idx_map);

}
}
}

#endif  // OT_LAYOUT_GPOS_COMMON_HH
