/*
 * Copyright © 2011,2012  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_HMTX_TABLE_HH
#define HB_OT_HMTX_TABLE_HH

#include "hb-open-type-private.hh"
#include "hb-ot-os2-table.hh"
#include "hb-ot-var-hvar-table.hh"


namespace OT {


/*
 * hmtx -- The Horizontal Metrics Table
 * vmtx -- The Vertical Metrics Table
 */

#define HB_OT_TAG_hmtx HB_TAG('h','m','t','x')
#define HB_OT_TAG_vmtx HB_TAG('v','m','t','x')


struct LongMetric
{
  UFWORD	advance; /* Advance width/height. */
  FWORD		lsb; /* Leading (left/top) side bearing. */
  public:
  DEFINE_SIZE_STATIC (4);
};

struct hmtxvmtx
{
  static const hb_tag_t hmtxTag	= HB_OT_TAG_hmtx;
  static const hb_tag_t vmtxTag	= HB_OT_TAG_vmtx;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    /* We don't check for anything specific here.  The users of the
     * struct do all the hard work... */
    return_trace (true);
  }

  struct accelerator_t
  {
    const hmtxvmtx *table;
    hb_blob_t *blob;

    const HVARVVAR *var;
    hb_blob_t *var_blob;

    inline void init (hb_face_t *face,
		      hb_tag_t _hea_tag,
		      hb_tag_t _mtx_tag,
		      hb_tag_t _var_tag,
		      hb_tag_t os2_tag,
		      unsigned int default_advance = 0)
    {
      this->default_advance = default_advance ? default_advance : face->get_upem ();

      bool got_font_extents = false;
      if (os2_tag)
      {
	hb_blob_t *os2_blob = Sanitizer<os2>::sanitize (face->reference_table (os2_tag));
	const os2 *os2_table = Sanitizer<os2>::lock_instance (os2_blob);
#define USE_TYPO_METRICS (1u<<7)
	if (0 != (os2_table->fsSelection & USE_TYPO_METRICS))
	{
	  this->ascender = os2_table->sTypoAscender;
	  this->descender = os2_table->sTypoDescender;
	  this->line_gap = os2_table->sTypoLineGap;
	  got_font_extents = (this->ascender | this->descender) != 0;
	}
	hb_blob_destroy (os2_blob);
      }

      hb_blob_t *_hea_blob = Sanitizer<_hea>::sanitize (face->reference_table (_hea_tag));
      const _hea *_hea_table = Sanitizer<_hea>::lock_instance (_hea_blob);
      this->num_advances = _hea_table->numberOfLongMetrics;
      if (!got_font_extents)
      {
	this->ascender = _hea_table->ascender;
	this->descender = _hea_table->descender;
	this->line_gap = _hea_table->lineGap;
	got_font_extents = (this->ascender | this->descender) != 0;
      }
      hb_blob_destroy (_hea_blob);

      this->has_font_extents = got_font_extents;

      this->blob = Sanitizer<hmtxvmtx>::sanitize (face->reference_table (_mtx_tag));

      /* Cap num_metrics() and num_advances() based on table length. */
      unsigned int len = hb_blob_get_length (this->blob);
      if (unlikely (this->num_advances * 4 > len))
	this->num_advances = len / 4;
      this->num_metrics = this->num_advances + (len - 4 * this->num_advances) / 2;

      /* We MUST set num_metrics to zero if num_advances is zero.
       * Our get_advance() depends on that. */
      if (unlikely (!this->num_advances))
      {
	this->num_metrics = this->num_advances = 0;
	hb_blob_destroy (this->blob);
	this->blob = hb_blob_get_empty ();
      }
      this->table = Sanitizer<hmtxvmtx>::lock_instance (this->blob);

      this->var_blob = Sanitizer<HVARVVAR>::sanitize (face->reference_table (_var_tag));
      this->var = Sanitizer<HVARVVAR>::lock_instance (this->var_blob);
    }

    inline void fini (void)
    {
      hb_blob_destroy (this->blob);
      hb_blob_destroy (this->var_blob);
    }

    inline unsigned int get_advance (hb_codepoint_t  glyph,
				     hb_font_t      *font) const
    {
      if (unlikely (glyph >= this->num_metrics))
      {
	/* If this->num_metrics is zero, it means we don't have the metrics table
	 * for this direction: return default advance.  Otherwise, it means that the
	 * glyph index is out of bound: return zero. */
	if (this->num_metrics)
	  return 0;
	else
	  return this->default_advance;
      }

      return this->table->longMetric[MIN (glyph, (uint32_t) this->num_advances - 1)].advance
	   + this->var->get_advance_var (glyph, font->coords, font->num_coords); // TODO Optimize?!
    }

    public:
    bool has_font_extents;
    unsigned short ascender;
    unsigned short descender;
    unsigned short line_gap;
    private:
    unsigned int num_metrics;
    unsigned int num_advances;
    unsigned int default_advance;
  };

  protected:
  LongMetric	longMetric[VAR];	/* Paired advance width and leading
					 * bearing values for each glyph. The
					 * value numOfHMetrics comes from
					 * the 'hhea' table. If the font is
					 * monospaced, only one entry need
					 * be in the array, but that entry is
					 * required. The last entry applies to
					 * all subsequent glyphs. */
  FWORD		leadingBearingX[VAR];	/* Here the advance is assumed
					 * to be the same as the advance
					 * for the last entry above. The
					 * number of entries in this array is
					 * derived from numGlyphs (from 'maxp'
					 * table) minus numberOfLongMetrics.
					 * This generally is used with a run
					 * of monospaced glyphs (e.g., Kanji
					 * fonts or Courier fonts). Only one
					 * run is allowed and it must be at
					 * the end. This allows a monospaced
					 * font to vary the side bearing
					 * values for each glyph. */
  public:
  DEFINE_SIZE_ARRAY2 (0, longMetric, leadingBearingX);
};

struct hmtx : hmtxvmtx {
  static const hb_tag_t tableTag	= HB_OT_TAG_hmtx;
};
struct vmtx : hmtxvmtx {
  static const hb_tag_t tableTag	= HB_OT_TAG_vmtx;
};

} /* namespace OT */


#endif /* HB_OT_HMTX_TABLE_HH */
