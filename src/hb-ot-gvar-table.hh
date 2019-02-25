/*
 * Copyright © 2018 Adobe Inc.
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
 * Adobe Author(s): Michiharu Ariza
 */

#ifndef HB_OT_GVAR_TABLE_HH
#define HB_OT_GVAR_TABLE_HH

#include "hb-open-type.hh"

/*
 * gvar -- Glyph Variation Table
 * https://docs.microsoft.com/en-us/typography/opentype/spec/gvar
 */
#define HB_OT_TAG_gvar HB_TAG('g','v','a','r')

namespace OT {

struct Tupple
{
  bool sanitize (hb_sanitize_context_t *c, unsigned int axis_count) const
  {
    TRACE_SANITIZE (this);
    return_trace (coords.sanitize (c, axis_count, this));s
  }

  protected:
  UnsizedArrayOf<F2DOT14>
  		coords;

  public:
  DEFINE_SIZE_ARRAY (0, coords);
};

struct TupleVarHeader
{
  HBUINT16	varDataSize;
  HBUINT16	tupleIndex;
  /* Tuple peakTuple */
  /* Tuple intermediateStartTuple */
  /* Tuple intermediateEndTuple */
};

struct GlyphVarData
{
  HBUINT16	tupleVarCount;
  OffsetTo<HBUINT8>
  		data;
  UnsizedArrayOf<TupleVarHeader>
  		tupleVarHeaders;
};

struct gvar
{
  static constexpr hb_tag_t tableTag = HB_OT_TAG_gvar;

  bool has_data () const { return version.to_int (); }

  int get_y_origin (hb_codepoint_t glyph) const
  {
    unsigned int i;
    if (!vertYOrigins.bfind (glyph, &i))
      return defaultVertOriginY;
    return vertYOrigins[i].vertOriginY;
  }

  bool _subset (const hb_subset_plan_t *plan HB_UNUSED,
		const gvar *vorg_table,
		const hb_vector_t<VertOriginMetric> &subset_metrics,
		unsigned int dest_sz,
		void *dest) const
  {
    hb_serialize_context_t c (dest, dest_sz);

    gvar *subset_table = c.start_serialize<gvar> ();
    if (unlikely (!c.extend_min (*subset_table)))
      return false;

    subset_table->version.major.set (1);
    subset_table->version.minor.set (0);

    subset_table->defaultVertOriginY.set (vorg_table->defaultVertOriginY);
    subset_table->vertYOrigins.len.set (subset_metrics.length);

    bool success = true;
    if (subset_metrics.length > 0)
    {
      unsigned int  size = VertOriginMetric::static_size * subset_metrics.length;
      VertOriginMetric  *metrics = c.allocate_size<VertOriginMetric> (size);
      if (likely (metrics != nullptr))
        memcpy (metrics, &subset_metrics[0], size);
      else
        success = false;
    }
    c.end_serialize ();

    return success;
  }

  bool subset (hb_subset_plan_t *plan) const
  {
    hb_blob_t *vorg_blob = hb_sanitize_context_t().reference_table<gvar> (plan->source);
    const gvar *vorg_table = vorg_blob->as<gvar> ();

    /* count the number of glyphs to be included in the subset table */
    hb_vector_t<VertOriginMetric> subset_metrics;
    subset_metrics.init ();


    hb_codepoint_t old_glyph = HB_SET_VALUE_INVALID;
    unsigned int i = 0;
    while (i < vertYOrigins.len
           && plan->glyphset ()->next (&old_glyph))
    {
      while (old_glyph > vertYOrigins[i].glyph)
      {
        i++;
        if (i >= vertYOrigins.len)
          break;
      }

      if (old_glyph == vertYOrigins[i].glyph)
      {
        hb_codepoint_t new_glyph;
        if (plan->new_gid_for_old_gid (old_glyph, &new_glyph))
        {
          VertOriginMetric *metrics = subset_metrics.push ();
          metrics->glyph.set (new_glyph);
          metrics->vertOriginY.set (vertYOrigins[i].vertOriginY);
        }
      }
    }

    /* alloc the new table */
    unsigned int dest_sz = gvar::min_size + VertOriginMetric::static_size * subset_metrics.length;
    void *dest = (void *) malloc (dest_sz);
    if (unlikely (!dest))
    {
      subset_metrics.fini ();
      hb_blob_destroy (vorg_blob);
      return false;
    }

    /* serialize the new table */
    if (!_subset (plan, vorg_table, subset_metrics, dest_sz, dest))
    {
      subset_metrics.fini ();
      free (dest);
      hb_blob_destroy (vorg_blob);
      return false;
    }

    hb_blob_t *result = hb_blob_create ((const char *)dest,
                                        dest_sz,
                                        HB_MEMORY_MODE_READONLY,
                                        dest,
                                        free);
    bool success = plan->add_table (HB_OT_TAG_gvar, result);
    hb_blob_destroy (result);
    subset_metrics.fini ();
    hb_blob_destroy (vorg_blob);
    return success;
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  version.major == 1 &&
                  glyphVarDataArray.sanitize (c));
  }

  protected:
  FixedVersion<>	version;		/* Version of gvar table. Set to 0x00010000u. */
  HBUINT16		axisCount;
  HBUINT16		sharedTupleCount;
  LOffsetTo<Tupple>	sharedTuples;
  HBUINT16		glyphCount;
  HBUINT16		flags;
  LOffsetTo<GlyphVarData>
  			glyphVarDataArray;
  /* OffsetTo<GlyphVariationData> glyphVariationDataOffsets */

  public:
  DEFINE_SIZE_ARRAY(8, vertYOrigins);
};

} /* namespace OT */

#endif /* HB_OT_GVAR_TABLE_HH */
