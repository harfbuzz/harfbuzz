#ifndef OT_GLYF_GLYF_HELPERS_HH
#define OT_GLYF_GLYF_HELPERS_HH


#include "../../hb-open-type.hh"
#include "../../hb-subset-plan.hh"

#include "loca.hh"


namespace OT {
namespace glyf_impl {


template<typename IteratorIn, typename TypeOut,
	 hb_requires (hb_is_source_of (IteratorIn, unsigned int))>
static void
_write_loca (IteratorIn&& it,
	     const hb_sorted_vector_t<hb_pair_t<hb_codepoint_t, hb_codepoint_t>> new_to_old_gid_list,
	     bool short_offsets,
	     TypeOut *dest,
	     unsigned num_offsets)
{
  unsigned num_glyphs = num_offsets - 1;
  unsigned right_shift = short_offsets ? 1 : 0;
  unsigned offset = 0;
  *dest++ = 0;
  for (hb_codepoint_t i = 0, j = 0; i < num_glyphs; i++)
  {
    if (i != new_to_old_gid_list[j].first)
    {
      DEBUG_MSG (SUBSET, nullptr, "loca entry empty offset %u", offset);
      *dest++ = offset >> right_shift;
      continue;
    }

    j++;
    unsigned padded_size = *it++;
    offset += padded_size;
    DEBUG_MSG (SUBSET, nullptr, "loca entry offset %u padded-size %u", offset, padded_size);
    *dest++ = offset >> right_shift;
  }
}

static bool
_add_head_and_set_loca_version (hb_subset_plan_t *plan, bool use_short_loca)
{
  hb_blob_t *head_blob = hb_sanitize_context_t ().reference_table<head> (plan->source);
  hb_blob_t *head_prime_blob = hb_blob_copy_writable_or_fail (head_blob);
  hb_blob_destroy (head_blob);

  if (unlikely (!head_prime_blob))
    return false;

  head *head_prime = (head *) hb_blob_get_data_writable (head_prime_blob, nullptr);
  head_prime->indexToLocFormat = use_short_loca ? 0 : 1;
  if (plan->normalized_coords)
  {
    head_prime->xMin = plan->head_maxp_info.xMin;
    head_prime->xMax = plan->head_maxp_info.xMax;
    head_prime->yMin = plan->head_maxp_info.yMin;
    head_prime->yMax = plan->head_maxp_info.yMax;

    unsigned orig_flag = head_prime->flags;
    if (plan->head_maxp_info.allXMinIsLsb)
      orig_flag |= 1 << 1;
    else
      orig_flag &= ~(1 << 1);
    head_prime->flags = orig_flag;
  }
  bool success = plan->add_table (HB_OT_TAG_head, head_prime_blob);

  hb_blob_destroy (head_prime_blob);
  return success;
}

template<typename Iterator,
	 hb_requires (hb_is_source_of (Iterator, unsigned int))>
static bool
_add_loca_and_head (hb_subset_plan_t * plan,
		    Iterator padded_offsets,
		    const hb_sorted_vector_t<hb_pair_t<hb_codepoint_t, hb_codepoint_t>> new_to_old_gid_list,
		    unsigned num_glyphs,
		    bool use_short_loca)
{
  unsigned num_offsets = num_glyphs + 1;
  unsigned entry_size = use_short_loca ? 2 : 4;
  char *loca_prime_data = (char *) hb_malloc (entry_size * num_offsets);

  if (unlikely (!loca_prime_data)) return false;

  DEBUG_MSG (SUBSET, nullptr, "loca entry_size %u num_offsets %u size %u",
	     entry_size, num_offsets, entry_size * num_offsets);

  if (use_short_loca)
    _write_loca (padded_offsets, new_to_old_gid_list, true, (HBUINT16 *) loca_prime_data, num_offsets);
  else
    _write_loca (padded_offsets, new_to_old_gid_list, false, (HBUINT32 *) loca_prime_data, num_offsets);

  hb_blob_t *loca_blob = hb_blob_create (loca_prime_data,
					 entry_size * num_offsets,
					 HB_MEMORY_MODE_WRITABLE,
					 loca_prime_data,
					 hb_free);

  bool result = plan->add_table (HB_OT_TAG_loca, loca_blob)
	     && _add_head_and_set_loca_version (plan, use_short_loca);

  hb_blob_destroy (loca_blob);
  return result;
}


} /* namespace glyf_impl */
} /* namespace OT */


#endif /* OT_GLYF_GLYF_HELPERS_HH */
