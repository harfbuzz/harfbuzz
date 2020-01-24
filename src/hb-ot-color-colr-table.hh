/*
 * Copyright © 2018  Ebrahim Byagowi
 * Copyright © 2020  Google, Inc.
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
 * Google Author(s): Calder Kitagawa
 */

#ifndef HB_OT_COLOR_COLR_TABLE_HH
#define HB_OT_COLOR_COLR_TABLE_HH

#include "hb-open-type.hh"

/*
 * COLR -- Color
 * https://docs.microsoft.com/en-us/typography/opentype/spec/colr
 */
#define HB_OT_TAG_COLR HB_TAG('C','O','L','R')


namespace OT {


struct LayerRecord
{
  operator hb_ot_color_layer_t () const { return {glyphId, colorIdx}; }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  public:
  HBGlyphID	glyphId;	/* Glyph ID of layer glyph */
  Index		colorIdx;	/* Index value to use with a
				 * selected color palette.
				 * An index value of 0xFFFF
				 * is a special case indicating
				 * that the text foreground
				 * color (defined by a
				 * higher-level client) should
				 * be used and shall not be
				 * treated as actual index
				 * into CPAL ColorRecord array. */
  public:
  DEFINE_SIZE_STATIC (4);
};

struct BaseGlyphRecord
{
  int cmp (hb_codepoint_t g) const
  { return g < glyphId ? -1 : g > glyphId ? 1 : 0; }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)));
  }

  public:
  HBGlyphID	glyphId;	/* Glyph ID of reference glyph */
  HBUINT16	firstLayerIdx;	/* Index (from beginning of
				 * the Layer Records) to the
				 * layer record. There will be
				 * numLayers consecutive entries
				 * for this base glyph. */
  HBUINT16	numLayers;	/* Number of color layers
				 * associated with this glyph */
  public:
  DEFINE_SIZE_STATIC (6);
};

struct COLR
{
  static constexpr hb_tag_t tableTag = HB_OT_TAG_COLR;

  bool has_data () const { return numBaseGlyphs; }

  unsigned int get_glyph_layers (hb_codepoint_t       glyph,
				 unsigned int         start_offset,
				 unsigned int        *count, /* IN/OUT.  May be NULL. */
				 hb_ot_color_layer_t *layers /* OUT.     May be NULL. */) const
  {
    const BaseGlyphRecord &record = (this+baseGlyphsZ).bsearch (numBaseGlyphs, glyph);

    hb_array_t<const LayerRecord> all_layers = (this+layersZ).as_array (numLayers);
    hb_array_t<const LayerRecord> glyph_layers = all_layers.sub_array (record.firstLayerIdx,
								       record.numLayers);
    if (count)
    {
      + glyph_layers.sub_array (start_offset, count)
      | hb_sink (hb_array (layers, *count))
      ;
    }
    return glyph_layers.length;
  }

  struct accelerator_t
  {
    accelerator_t () {}
    ~accelerator_t () { fini (); }

    void init (hb_face_t *face)
    { colr = hb_sanitize_context_t ().reference_table<COLR> (face); }

    void fini () { this->colr.destroy (); }

    bool is_valid () { return colr.get_blob ()->length; }

    void get_related_glyphs (hb_codepoint_t glyph,
                             hb_set_t *related_ids /* OUT */) const
    { colr->get_related_glyphs (glyph, related_ids); }

    private:
    hb_blob_ptr_t<COLR> colr;
  };

  void get_related_glyphs (hb_codepoint_t glyph,
                           hb_set_t *related_ids /* OUT */) const
  {
    const BaseGlyphRecord &record = (this+baseGlyphsZ).bsearch (numBaseGlyphs, glyph);

    hb_array_t<const LayerRecord> all_layers = (this+layersZ).as_array (numLayers);
    hb_array_t<const LayerRecord> glyph_layers = all_layers.sub_array (record.firstLayerIdx,
                                                                       record.numLayers);
    for (unsigned int i = 0; i < glyph_layers.length; i++)
      hb_set_add (related_ids, glyph_layers[i].glyphId);
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  (this+baseGlyphsZ).sanitize (c, numBaseGlyphs) &&
			  (this+layersZ).sanitize (c, numLayers)));
  }

  template<typename Iterator,
           hb_requires (hb_is_iterator (Iterator))>
  bool serialize (hb_subset_context_t *c,
                  unsigned version,
                  Iterator it,
                  hb_array_t<const LayerRecord> all_layers)
  {
    TRACE_SERIALIZE (this);

    if (unlikely (!c->serializer->extend_min (this))) return_trace (false);
    this->version = version;
    numLayers = 0;
    numBaseGlyphs = it.len ();
    baseGlyphsZ = COLR::min_size;
    layersZ = COLR::min_size + numBaseGlyphs * BaseGlyphRecord::min_size;

    if (unlikely (!c->serializer->allocate_size<HBUINT8> ((unsigned int) layersZ - (unsigned int) baseGlyphsZ)))
      return_trace (false);
    hb_sorted_array_t<BaseGlyphRecord> glyph_records = (this+baseGlyphsZ).as_array (numBaseGlyphs);

    unsigned int index = 0;
    for (const hb_item_type<Iterator>& _ : + it.iter ())
    {
      const unsigned int new_gid = _.first;
      const BaseGlyphRecord* record = _.second;
      if (record->firstLayerIdx >= all_layers.length ||
          record->firstLayerIdx + record->numLayers > all_layers.length)
        return_trace (false);

      glyph_records[index].glyphId = new_gid;
      glyph_records[index].numLayers = record->numLayers;
      glyph_records[index].firstLayerIdx = numLayers;
      index += 1;

      hb_array_t<const LayerRecord> layers = all_layers.sub_array (record->firstLayerIdx,
                                                                   record->numLayers);

      for (unsigned int i = 0; i < layers.length; i++)
      {
        hb_codepoint_t new_gid = 0;
        if (unlikely (!c->plan->new_gid_for_old_gid (layers[i].glyphId, &new_gid))) return_trace (false);

        LayerRecord* out_layer = c->serializer->start_embed<LayerRecord> ();
        if (unlikely (!c->serializer->extend_min (out_layer))) return_trace (false);

        out_layer->glyphId = new_gid;
        out_layer->colorIdx = layers[i].colorIdx;
      }
      numLayers += record->numLayers;
    }

    return_trace (true);
  }

  bool subset (hb_subset_context_t *c) const
  {
    TRACE_SUBSET (this);

    auto it =
    + hb_range (1u, c->plan->num_output_glyphs ())  // Skip notdef.
    | hb_map ([&](unsigned int new_gid)
        {
          hb_codepoint_t old_gid = 0;
          if (unlikely (!c->plan->old_gid_for_new_gid (new_gid, &old_gid)))
            return hb_pair_t<hb_codepoint_t, const BaseGlyphRecord*>(new_gid, nullptr);

          const BaseGlyphRecord* record = &(this+baseGlyphsZ).bsearch (numBaseGlyphs, old_gid);
          if (record && (hb_codepoint_t) record->glyphId != old_gid)
            record = nullptr;
          return hb_pair_t<hb_codepoint_t, const BaseGlyphRecord*> (new_gid, record);
        })
    | hb_filter (hb_second)
    ;

    if (unlikely (!it.len ())) return_trace (false);

    COLR *colr_prime = c->serializer->start_embed<COLR> ();

    hb_array_t<const LayerRecord> all_layers = (this+layersZ).as_array (numLayers);

    return_trace (colr_prime->serialize (c, version, it, all_layers));
  }

  protected:
  HBUINT16	version;	/* Table version number (starts at 0). */
  HBUINT16	numBaseGlyphs;	/* Number of Base Glyph Records. */
  LNNOffsetTo<SortedUnsizedArrayOf<BaseGlyphRecord>>
		baseGlyphsZ;	/* Offset to Base Glyph records. */
  LNNOffsetTo<UnsizedArrayOf<LayerRecord>>
		layersZ;	/* Offset to Layer Records. */
  HBUINT16	numLayers;	/* Number of Layer Records. */
  public:
  DEFINE_SIZE_STATIC (14);
};

} /* namespace OT */


#endif /* HB_OT_COLOR_COLR_TABLE_HH */
