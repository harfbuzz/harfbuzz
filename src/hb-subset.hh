/*
 * Copyright © 2018  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Google Author(s): Garret Rieger, Roderick Sheeter
 */

#ifndef HB_SUBSET_HH
#define HB_SUBSET_HH


#include "hb.hh"

#include "hb-subset.h"

#include "hb-machinery.hh"
#include "hb-serialize.hh"
#include "hb-subset-input.hh"
#include "hb-subset-plan.hh"

struct hb_subset_context_t :
       hb_dispatch_context_t<hb_subset_context_t, bool, HB_DEBUG_SUBSET>
{
  const char *get_name () { return "SUBSET"; }
  static return_t default_return_value () { return true; }

  private:
  template <typename T, typename ...Ts> auto
  _dispatch (const T &obj, hb_priority<1>, Ts&&... ds) HB_AUTO_RETURN
  ( obj.subset (this, std::forward<Ts> (ds)...) )
  template <typename T, typename ...Ts> auto
  _dispatch (const T &obj, hb_priority<0>, Ts&&... ds) HB_AUTO_RETURN
  ( obj.dispatch (this, std::forward<Ts> (ds)...) )
  public:
  template <typename T, typename ...Ts> auto
  dispatch (const T &obj, Ts&&... ds) HB_AUTO_RETURN
  ( _dispatch (obj, hb_prioritize, std::forward<Ts> (ds)...) )

  hb_blob_t *source_blob;
  hb_subset_plan_t *plan;
  hb_serialize_context_t *serializer;
  hb_tag_t table_tag;

  hb_subset_context_t (hb_blob_t *source_blob_,
		       hb_subset_plan_t *plan_,
		       hb_serialize_context_t *serializer_,
		       hb_tag_t table_tag_) :
		        source_blob (source_blob_),
			plan (plan_),
			serializer (serializer_),
			table_tag (table_tag_) {}
};

template<typename TableType>
static bool
_hb_subset_table_try (const TableType *table,
		      hb_vector_t<char>* buf,
		      hb_subset_context_t* c /* OUT */)
{
  c->serializer->start_serialize ();
  if (c->serializer->in_error ()) return false;

  bool needed = table->subset (c);
  if (!c->serializer->ran_out_of_room ())
  {
    c->serializer->end_serialize ();
    return needed;
  }

  unsigned buf_size = buf->allocated;
  buf_size = buf_size * 2 + 16;




  DEBUG_MSG (SUBSET, nullptr, "OT::%c%c%c%c ran out of room; reallocating to %u bytes.",
             HB_UNTAG (c->table_tag), buf_size);

  if (unlikely (buf_size > c->source_blob->length * 256 ||
		!buf->alloc_exact (buf_size)))
  {
    DEBUG_MSG (SUBSET, nullptr, "OT::%c%c%c%c failed to reallocate %u bytes.",
               HB_UNTAG (c->table_tag), buf_size);
    return needed;
  }

  c->serializer->reset (buf->arrayZ, buf->allocated);
  return _hb_subset_table_try (table, buf, c);
}

static HB_UNUSED unsigned
_hb_subset_estimate_table_size (hb_subset_plan_t *plan,
				unsigned table_len,
				hb_tag_t table_tag)
{
  unsigned src_glyphs = plan->source->get_num_glyphs ();
  unsigned dst_glyphs = plan->glyphset ()->get_population ();

  unsigned bulk = 8192;
  /* Tables that we want to allocate same space as the source table. For GSUB/GPOS it's
   * because those are expensive to subset, so giving them more room is fine. */
  bool same_size = table_tag == HB_TAG('G','S','U','B') ||
		   table_tag == HB_TAG('G','P','O','S') ||
		   table_tag == HB_TAG('G','D','E','F') ||
		   table_tag == HB_TAG('n','a','m','e');

  if (plan->flags & HB_SUBSET_FLAGS_RETAIN_GIDS)
  {
    if (table_tag == HB_TAG('C','F','F',' '))
    {
      /* Add some extra room for the CFF charset. */
      bulk += src_glyphs * 16;
    }
    else if (table_tag == HB_TAG('C','F','F','2'))
    {
      /* Just extra CharString offsets. */
      bulk += src_glyphs * 4;
    }
  }

  if (unlikely (!src_glyphs) || same_size)
    return bulk + table_len;

  return bulk + (unsigned) (table_len * sqrt ((double) dst_glyphs / src_glyphs));
}


#endif /* HB_SUBSET_HH */
