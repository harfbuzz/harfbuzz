/*
 * Copyright © 2017  Google, Inc.
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

#ifndef HB_OT_VAR_FVAR_TABLE_HH
#define HB_OT_VAR_FVAR_TABLE_HH

#include "hb-open-type-private.hh"
#include "hb-ot-var.h"

namespace OT {


struct InstanceRecord
{
  inline bool sanitize (hb_sanitize_context_t *c, unsigned int axis_count) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  c->check_array (coordinates, coordinates[0].static_size, axis_count));
  }

  protected:
  USHORT	subfamilyNameID;/* The name ID for entries in the 'name' table
				 * that provide subfamily names for this instance. */
  USHORT	reserved;	/* Reserved for future use — set to 0. */
  Fixed		coordinates[VAR];/* The coordinates array for this instance. */
  //USHORT	postScriptNameIDX;/*Optional. The name ID for entries in the 'name'
  //				  * table that provide PostScript names for this
  //				  * instance. */

  public:
  DEFINE_SIZE_ARRAY (4, coordinates);
};

struct AxisRecord
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  protected:
  Tag		axisTag;	/* Tag identifying the design variation for the axis. */
  Fixed		minValue;	/* The minimum coordinate value for the axis. */
  Fixed		defaultValue;	/* The default coordinate value for the axis. */
  Fixed		maxValue;	/* The maximum coordinate value for the axis. */
  USHORT	reserved;	/* Reserved for future use — set to 0. */
  USHORT	axisNameID;	/* The name ID for entries in the 'name' table that
				 * provide a display name for this axis. */

  public:
  DEFINE_SIZE_STATIC (20);
};


/*
 * fvar — Font Variations Table
 */

struct fvar
{
  static const hb_tag_t tableTag	= HB_OT_TAG_fvar;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (version.sanitize (c) &&
		  likely (version.major == 1) /*&&
		  mathConstants.sanitize (c, this) &&
		  mathGlyphInfo.sanitize (c, this) &&
		  mathVariants.sanitize (c, this)*/);
  }

#if 0
  inline hb_position_t get_constant (hb_ot_math_constant_t  constant,
				     hb_font_t		   *font) const
  { return (this+mathConstants).get_value (constant, font); }

  inline const MathGlyphInfo &get_math_glyph_info (void) const
  { return this+mathGlyphInfo; }

  inline const MathVariants &get_math_variants (void) const
  { return this+mathVariants; }
#endif

  protected:
  FixedVersion<>version;	/* Version of the fvar table
				 * initially set to 0x00010000u */
  Offset<>	things;		/* Offset in bytes from the beginning of the table
				 * to the start of the AxisRecord array. */
  USHORT	reserved;	/* This field is permanently reserved. Set to 2. */
  USHORT	axisCount;	/* The number of variation axes in the font (the
				 * number of records in the axes array). */
  USHORT	axisSize;	/* The size in bytes of each VariationAxisRecord —
				 * set to 20 (0x0014) for this version. */
  USHORT	instanceCount;	/* The number of named instances defined in the font
				 * (the number of records in the instances array). */
  USHORT	instanceSize;	/* The size in bytes of each InstanceRecord — set
				 * to either axisCount * sizeof(Fixed) + 4, or to
				 * axisCount * sizeof(Fixed) + 6. */

  public:
  DEFINE_SIZE_STATIC (16);
};

} /* namespace OT */


#endif /* HB_OT_VAR_FVAR_TABLE_HH */
