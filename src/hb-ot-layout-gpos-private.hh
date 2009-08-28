/*
 * Copyright (C) 2007,2008,2009  Red Hat, Inc.
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
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
 * Red Hat Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_LAYOUT_GPOS_PRIVATE_HH
#define HB_OT_LAYOUT_GPOS_PRIVATE_HH

#include "hb-ot-layout-gsubgpos-private.hh"

#define HB_OT_LAYOUT_GPOS_NO_LAST ((unsigned int) -1)

/* Shared Tables: ValueRecord, Anchor Table, and MarkArray */

typedef SHORT Value;

typedef Value ValueRecord[0];
ASSERT_SIZE (ValueRecord, 0);

struct ValueFormat : USHORT
{
  enum
  {
    xPlacement	= 0x0001,	/* Includes horizontal adjustment for placement */
    yPlacement	= 0x0002,	/* Includes vertical adjustment for placement */
    xAdvance	= 0x0004,	/* Includes horizontal adjustment for advance */
    yAdvance	= 0x0008,	/* Includes vertical adjustment for advance */
    xPlaDevice	= 0x0010,	/* Includes horizontal Device table for placement */
    yPlaDevice	= 0x0020,	/* Includes vertical Device table for placement */
    xAdvDevice	= 0x0040,	/* Includes horizontal Device table for advance */
    yAdvDevice	= 0x0080,	/* Includes vertical Device table for advance */
    ignored	= 0x0F00,	/* Was used in TrueType Open for MM fonts */
    reserved	= 0xF000 	/* For future use */
  };

  inline unsigned int get_len () const
  { return _hb_popcount32 ((unsigned int) *this); }
  inline unsigned int get_size () const
  { return get_len () * sizeof (Value); }

  void apply_value (hb_ot_layout_context_t *context,
		    const char          *base,
		    const Value         *values,
		    hb_internal_glyph_position_t *glyph_pos) const
  {
    unsigned int x_ppem, y_ppem;
    hb_16dot16_t x_scale, y_scale;
    unsigned int format = *this;

    if (!format)
      return;

    /* All fields are options.  Only those available advance the value
     * pointer. */
#if 0
struct ValueRecord {
  SHORT		xPlacement;		/* Horizontal adjustment for
					 * placement--in design units */
  SHORT		yPlacement;		/* Vertical adjustment for
					 * placement--in design units */
  SHORT		xAdvance;		/* Horizontal adjustment for
					 * advance--in design units (only used
					 * for horizontal writing) */
  SHORT		yAdvance;		/* Vertical adjustment for advance--in
					 * design units (only used for vertical
					 * writing) */
  Offset	xPlaDevice;		/* Offset to Device table for
					 * horizontal placement--measured from
					 * beginning of PosTable (may be NULL) */
  Offset	yPlaDevice;		/* Offset to Device table for vertical
					 * placement--measured from beginning
					 * of PosTable (may be NULL) */
  Offset	xAdvDevice;		/* Offset to Device table for
					 * horizontal advance--measured from
					 * beginning of PosTable (may be NULL) */
  Offset	yAdvDevice;		/* Offset to Device table for vertical
					 * advance--measured from beginning of
					 * PosTable (may be NULL) */
};
#endif

    x_scale = context->font->x_scale;
    y_scale = context->font->y_scale;
    /* design units -> fractional pixel */
    if (format & xPlacement)
      glyph_pos->x_pos += _hb_16dot16_mul_trunc (x_scale, *(SHORT*)values++);
    if (format & yPlacement)
      glyph_pos->y_pos += _hb_16dot16_mul_trunc (y_scale, *(SHORT*)values++);
    if (format & xAdvance)
      glyph_pos->x_advance += _hb_16dot16_mul_trunc (x_scale, *(SHORT*)values++);
    if (format & yAdvance)
      glyph_pos->y_advance += _hb_16dot16_mul_trunc (y_scale, *(SHORT*)values++);

    x_ppem = context->font->x_ppem;
    y_ppem = context->font->y_ppem;
    /* pixel -> fractional pixel */
    if (format & xPlaDevice) {
      if (x_ppem)
	glyph_pos->x_pos += (base+*(OffsetTo<Device>*)values++).get_delta (x_ppem) << 6;
      else
        values++;
    }
    if (format & yPlaDevice) {
      if (y_ppem)
	glyph_pos->y_pos += (base+*(OffsetTo<Device>*)values++).get_delta (y_ppem) << 6;
      else
        values++;
    }
    if (format & xAdvDevice) {
      if (x_ppem)
	glyph_pos->x_advance += (base+*(OffsetTo<Device>*)values++).get_delta (x_ppem) << 6;
      else
        values++;
    }
    if (format & yAdvDevice) {
      if (y_ppem)
	glyph_pos->y_advance += (base+*(OffsetTo<Device>*)values++).get_delta (y_ppem) << 6;
      else
        values++;
    }
  }
};
ASSERT_SIZE (ValueFormat, 2);


struct AnchorFormat1
{
  friend struct Anchor;

  private:
  inline void get_anchor (hb_ot_layout_context_t *context, hb_codepoint_t glyph_id,
			  hb_position_t *x, hb_position_t *y) const
  {
      *x = _hb_16dot16_mul_trunc (context->font->x_scale, xCoordinate);
      *y = _hb_16dot16_mul_trunc (context->font->y_scale, yCoordinate);
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF ();
  }

  private:
  USHORT	format;			/* Format identifier--format = 1 */
  SHORT		xCoordinate;		/* Horizontal value--in design units */
  SHORT		yCoordinate;		/* Vertical value--in design units */
};
ASSERT_SIZE (AnchorFormat1, 6);

struct AnchorFormat2
{
  friend struct Anchor;

  private:
  inline void get_anchor (hb_ot_layout_context_t *context, hb_codepoint_t glyph_id,
			  hb_position_t *x, hb_position_t *y) const
  {
      /* TODO Contour */
      *x = _hb_16dot16_mul_trunc (context->font->x_scale, xCoordinate);
      *y = _hb_16dot16_mul_trunc (context->font->y_scale, yCoordinate);
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF ();
  }

  private:
  USHORT	format;			/* Format identifier--format = 2 */
  SHORT		xCoordinate;		/* Horizontal value--in design units */
  SHORT		yCoordinate;		/* Vertical value--in design units */
  USHORT	anchorPoint;		/* Index to glyph contour point */
};
ASSERT_SIZE (AnchorFormat2, 8);

struct AnchorFormat3
{
  friend struct Anchor;

  private:
  inline void get_anchor (hb_ot_layout_context_t *context, hb_codepoint_t glyph_id,
			  hb_position_t *x, hb_position_t *y) const
  {
      *x = _hb_16dot16_mul_trunc (context->font->x_scale, xCoordinate);
      *y = _hb_16dot16_mul_trunc (context->font->y_scale, yCoordinate);

      if (context->font->x_ppem)
	*x += (this+xDeviceTable).get_delta (context->font->x_ppem) << 6;
      if (context->font->y_ppem)
	*y += (this+yDeviceTable).get_delta (context->font->y_ppem) << 6;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF () && SANITIZE_THIS2 (xDeviceTable, yDeviceTable);
  }

  private:
  USHORT	format;			/* Format identifier--format = 3 */
  SHORT		xCoordinate;		/* Horizontal value--in design units */
  SHORT		yCoordinate;		/* Vertical value--in design units */
  OffsetTo<Device>
		xDeviceTable;		/* Offset to Device table for X
					 * coordinate-- from beginning of
					 * Anchor table (may be NULL) */
  OffsetTo<Device>
		yDeviceTable;		/* Offset to Device table for Y
					 * coordinate-- from beginning of
					 * Anchor table (may be NULL) */
};
ASSERT_SIZE (AnchorFormat3, 10);

struct Anchor
{
  inline void get_anchor (hb_ot_layout_context_t *context, hb_codepoint_t glyph_id,
			  hb_position_t *x, hb_position_t *y) const
  {
    *x = *y = 0;
    switch (u.format) {
    case 1: u.format1->get_anchor (context, glyph_id, x, y); return;
    case 2: u.format2->get_anchor (context, glyph_id, x, y); return;
    case 3: u.format3->get_anchor (context, glyph_id, x, y); return;
    default:						     return;
    }
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!SANITIZE (u.format)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (SANITIZE_ARG);
    case 2: return u.format2->sanitize (SANITIZE_ARG);
    case 3: return u.format3->sanitize (SANITIZE_ARG);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  AnchorFormat1		format1[];
  AnchorFormat2		format2[];
  AnchorFormat3		format3[];
  } u;
};
ASSERT_SIZE (Anchor, 2);


struct AnchorMatrix
{
  inline const Anchor& get_anchor (unsigned int row, unsigned int col, unsigned int cols) const {
    if (HB_UNLIKELY (row >= rows || col >= cols)) return Null(Anchor);
    return this+matrix[row * cols + col];
  }

  inline bool sanitize (SANITIZE_ARG_DEF, unsigned int cols) {
    TRACE_SANITIZE ();
    if (!SANITIZE_SELF ()) return false;
    unsigned int count = rows * cols;
    if (!SANITIZE_ARRAY (matrix, sizeof (matrix[0]), count)) return false;
    for (unsigned int i = 0; i < count; i++)
      if (!SANITIZE_THIS (matrix[i])) return false;
    return true;
  }

  USHORT	rows;			/* Number of rows */
  private:
  OffsetTo<Anchor>
		matrix[];		/* Matrix of offsets to Anchor tables--
					 * from beginning of AnchorMatrix table */
};
ASSERT_SIZE (AnchorMatrix, 2);


struct MarkRecord
{
  friend struct MarkArray;

  inline bool sanitize (SANITIZE_ARG_DEF, const void *base) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF () && SANITIZE_BASE (markAnchor, base);
  }

  private:
  USHORT	klass;			/* Class defined for this mark */
  OffsetTo<Anchor>
		markAnchor;		/* Offset to Anchor table--from
					 * beginning of MarkArray table */
};
ASSERT_SIZE (MarkRecord, 4);

struct MarkArray
{
  inline bool apply (APPLY_ARG_DEF,
		     unsigned int mark_index, unsigned int glyph_index,
		     const AnchorMatrix &anchors, unsigned int class_count,
		     unsigned int glyph_pos) const
  {
    TRACE_APPLY ();
    const MarkRecord &record = markRecord[mark_index];
    unsigned int mark_class = record.klass;

    const Anchor& mark_anchor = this + record.markAnchor;
    const Anchor& glyph_anchor = anchors.get_anchor (glyph_index, mark_class, class_count);

    hb_position_t mark_x, mark_y, base_x, base_y;

    mark_anchor.get_anchor (context, IN_CURGLYPH (), &mark_x, &mark_y);
    glyph_anchor.get_anchor (context, IN_GLYPH (glyph_pos), &base_x, &base_y);

    hb_internal_glyph_position_t *o = POSITION (buffer->in_pos);
    o->x_pos     = base_x - mark_x;
    o->y_pos     = base_y - mark_y;
    o->x_advance = 0;
    o->y_advance = 0;
    o->back      = buffer->in_pos - glyph_pos;

    buffer->in_pos++;
    return true;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_THIS (markRecord);
  }

  private:
  ArrayOf<MarkRecord>
		markRecord;	/* Array of MarkRecords--in Coverage order */
};
ASSERT_SIZE (MarkArray, 2);


/* Lookups */

struct SinglePosFormat1
{
  friend struct SinglePos;

  private:
  inline bool apply (APPLY_ARG_DEF) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (HB_LIKELY (index == NOT_COVERED))
      return false;

    valueFormat.apply_value (context, CONST_CHARP(this), values, CURPOSITION ());

    buffer->in_pos++;
    return true;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF () && SANITIZE_THIS (coverage) &&
	   SANITIZE_MEM (values, valueFormat.get_size ());
  }

  private:
  USHORT	format;			/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of subtable */
  ValueFormat	valueFormat;		/* Defines the types of data in the
					 * ValueRecord */
  ValueRecord	values;			/* Defines positioning
					 * value(s)--applied to all glyphs in
					 * the Coverage table */
};
ASSERT_SIZE (SinglePosFormat1, 6);

struct SinglePosFormat2
{
  friend struct SinglePos;

  private:
  inline bool apply (APPLY_ARG_DEF) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (HB_LIKELY (index == NOT_COVERED))
      return false;

    if (HB_LIKELY (index >= valueCount))
      return false;

    valueFormat.apply_value (context, CONST_CHARP(this),
			     values + index * valueFormat.get_len (),
			     CURPOSITION ());

    buffer->in_pos++;
    return true;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF () && SANITIZE_THIS (coverage) &&
	   SANITIZE_MEM (values, valueFormat.get_size () * valueCount);
  }

  private:
  USHORT	format;			/* Format identifier--format = 2 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of subtable */
  ValueFormat	valueFormat;		/* Defines the types of data in the
					 * ValueRecord */
  USHORT	valueCount;		/* Number of ValueRecords */
  ValueRecord	values;			/* Array of ValueRecords--positioning
					 * values applied to glyphs */
};
ASSERT_SIZE (SinglePosFormat2, 8);

struct SinglePos
{
  friend struct PosLookupSubTable;

  private:
  inline bool apply (APPLY_ARG_DEF) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1->apply (APPLY_ARG);
    case 2: return u.format2->apply (APPLY_ARG);
    default:return false;
    }
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!SANITIZE (u.format)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (SANITIZE_ARG);
    case 2: return u.format2->sanitize (SANITIZE_ARG);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  SinglePosFormat1	format1[];
  SinglePosFormat2	format2[];
  } u;
};
ASSERT_SIZE (SinglePos, 2);


struct PairValueRecord
{
  friend struct PairPosFormat1;

  private:
  GlyphID	secondGlyph;		/* GlyphID of second glyph in the
					 * pair--first glyph is listed in the
					 * Coverage table */
  ValueRecord	values;			/* Positioning data for the first glyph
					 * followed by for second glyph */
};
ASSERT_SIZE (PairValueRecord, 2);

struct PairSet
{
  friend struct PairPosFormat1;

  inline bool sanitize (SANITIZE_ARG_DEF, unsigned int format_len) {
    TRACE_SANITIZE ();
    if (!SANITIZE_SELF ()) return false;
    unsigned int count = (1 + format_len) * len;
    return SANITIZE_MEM (array, sizeof (array[0]) * count);
  }

  private:
  USHORT	len;			/* Number of PairValueRecords */
  PairValueRecord
		array[];		/* Array of PairValueRecords--ordered
					 * by GlyphID of the second glyph */
};
ASSERT_SIZE (PairSet, 2);

struct PairPosFormat1
{
  friend struct PairPos;

  private:
  inline bool apply (APPLY_ARG_DEF) const
  {
    TRACE_APPLY ();
    unsigned int end = MIN (buffer->in_length, buffer->in_pos + context_length);
    if (HB_UNLIKELY (buffer->in_pos + 2 > end))
      return false;

    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (HB_LIKELY (index == NOT_COVERED))
      return false;

    unsigned int j = buffer->in_pos + 1;
    while (_hb_ot_layout_skip_mark (context->face, IN_INFO (j), lookup_flag, NULL))
    {
      if (HB_UNLIKELY (j == end))
	return false;
      j++;
    }

    const PairSet &pair_set = this+pairSet[index];

    unsigned int len1 = valueFormat1.get_len ();
    unsigned int len2 = valueFormat2.get_len ();
    unsigned int record_len = 1 + len1 + len2;

    unsigned int count = pair_set.len;
    const PairValueRecord *record = pair_set.array;
    for (unsigned int i = 0; i < count; i++)
    {
      if (IN_GLYPH (j) == record->secondGlyph)
      {
	valueFormat1.apply_value (context, CONST_CHARP(this), record->values, CURPOSITION ());
	valueFormat2.apply_value (context, CONST_CHARP(this), record->values + len1, POSITION (j));
	if (len2)
	  j++;
	buffer->in_pos = j;
	return true;
      }
      record += record_len;
    }

    return false;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF () && SANITIZE_THIS (coverage) &&
	   pairSet.sanitize (SANITIZE_ARG, CONST_CHARP(this),
			     valueFormat1.get_len () + valueFormat2.get_len ());
  }

  private:
  USHORT	format;			/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of subtable */
  ValueFormat	valueFormat1;		/* Defines the types of data in
					 * ValueRecord1--for the first glyph
					 * in the pair--may be zero (0) */
  ValueFormat	valueFormat2;		/* Defines the types of data in
					 * ValueRecord2--for the second glyph
					 * in the pair--may be zero (0) */
  OffsetArrayOf<PairSet>
		pairSet;		/* Array of PairSet tables
					 * ordered by Coverage Index */
};
ASSERT_SIZE (PairPosFormat1, 10);

struct PairPosFormat2
{
  friend struct PairPos;

  private:
  inline bool apply (APPLY_ARG_DEF) const
  {
    TRACE_APPLY ();
    unsigned int end = MIN (buffer->in_length, buffer->in_pos + context_length);
    if (HB_UNLIKELY (buffer->in_pos + 2 > end))
      return false;

    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (HB_LIKELY (index == NOT_COVERED))
      return false;

    unsigned int j = buffer->in_pos + 1;
    while (_hb_ot_layout_skip_mark (context->face, IN_INFO (j), lookup_flag, NULL))
    {
      if (HB_UNLIKELY (j == end))
	return false;
      j++;
    }

    unsigned int len1 = valueFormat1.get_len ();
    unsigned int len2 = valueFormat2.get_len ();
    unsigned int record_len = len1 + len2;

    unsigned int klass1 = (this+classDef1) (IN_CURGLYPH ());
    unsigned int klass2 = (this+classDef2) (IN_GLYPH (j));
    if (HB_UNLIKELY (klass1 >= class1Count || klass2 >= class2Count))
      return false;

    const Value *v = values + record_len * (klass1 * class2Count + klass2);
    valueFormat1.apply_value (context, CONST_CHARP(this), v, CURPOSITION ());
    valueFormat2.apply_value (context, CONST_CHARP(this), v + len1, POSITION (j));

    if (len2)
      j++;
    buffer->in_pos = j;

    return true;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!(SANITIZE_SELF () && SANITIZE_THIS (coverage) &&
	  SANITIZE_THIS2 (classDef1, classDef2))) return false;

    unsigned int record_size =valueFormat1.get_size () + valueFormat2.get_size ();
    unsigned int len = class1Count * class2Count;
    return SANITIZE_ARRAY (values, record_size, len);
  }

  private:
  USHORT	format;			/* Format identifier--format = 2 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of subtable */
  ValueFormat	valueFormat1;		/* ValueRecord definition--for the
					 * first glyph of the pair--may be zero
					 * (0) */
  ValueFormat	valueFormat2;		/* ValueRecord definition--for the
					 * second glyph of the pair--may be
					 * zero (0) */
  OffsetTo<ClassDef>
		classDef1;		/* Offset to ClassDef table--from
					 * beginning of PairPos subtable--for
					 * the first glyph of the pair */
  OffsetTo<ClassDef>
		classDef2;		/* Offset to ClassDef table--from
					 * beginning of PairPos subtable--for
					 * the second glyph of the pair */
  USHORT	class1Count;		/* Number of classes in ClassDef1
					 * table--includes Class0 */
  USHORT	class2Count;		/* Number of classes in ClassDef2
					 * table--includes Class0 */
  ValueRecord	values;			/* Matrix of value pairs:
					 * class1-major, class2-minor,
					 * Each entry has value1 and value2 */
};
ASSERT_SIZE (PairPosFormat2, 16);

struct PairPos
{
  friend struct PosLookupSubTable;

  private:
  inline bool apply (APPLY_ARG_DEF) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1->apply (APPLY_ARG);
    case 2: return u.format2->apply (APPLY_ARG);
    default:return false;
    }
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!SANITIZE (u.format)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (SANITIZE_ARG);
    case 2: return u.format2->sanitize (SANITIZE_ARG);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  PairPosFormat1	format1[];
  PairPosFormat2	format2[];
  } u;
};
ASSERT_SIZE (PairPos, 2);


struct EntryExitRecord
{
  inline bool sanitize (SANITIZE_ARG_DEF, const void *base) {
    TRACE_SANITIZE ();
    return SANITIZE_BASE2 (entryAnchor, exitAnchor, base);
  }

  OffsetTo<Anchor>
		entryAnchor;		/* Offset to EntryAnchor table--from
					 * beginning of CursivePos
					 * subtable--may be NULL */
  OffsetTo<Anchor>
		exitAnchor;		/* Offset to ExitAnchor table--from
					 * beginning of CursivePos
					 * subtable--may be NULL */
};
ASSERT_SIZE (EntryExitRecord, 4);

struct CursivePosFormat1
{
  friend struct CursivePos;

  private:
  inline bool apply (APPLY_ARG_DEF) const
  {
    TRACE_APPLY ();
    /* Now comes the messiest part of the whole OpenType
       specification.  At first glance, cursive connections seem easy
       to understand, but there are pitfalls!  The reason is that
       the specs don't mention how to compute the advance values
       resp. glyph offsets.  I was told it would be an omission, to
       be fixed in the next OpenType version...  Again many thanks to
       Andrei Burago <andreib@microsoft.com> for clarifications.

       Consider the following example:

			|  xadv1    |
			 +---------+
			 |         |
		   +-----+--+ 1    |
		   |     | .|      |
		   |    0+--+------+
		   |   2    |
		   |        |
		  0+--------+
		  |  xadv2   |

	 glyph1: advance width = 12
		 anchor point = (3,1)

	 glyph2: advance width = 11
		 anchor point = (9,4)

	 LSB is 1 for both glyphs (so the boxes drawn above are glyph
	 bboxes).  Writing direction is R2L; `0' denotes the glyph's
	 coordinate origin.

       Now the surprising part: The advance width of the *left* glyph
       (resp. of the *bottom* glyph) will be modified, no matter
       whether the writing direction is L2R or R2L (resp. T2B or
       B2T)!  This assymetry is caused by the fact that the glyph's
       coordinate origin is always the lower left corner for all
       writing directions.

       Continuing the above example, we can compute the new
       (horizontal) advance width of glyph2 as

	 9 - 3 = 6  ,

       and the new vertical offset of glyph2 as

	 1 - 4 = -3  .


       Vertical writing direction is far more complicated:

       a) Assuming that we recompute the advance height of the lower glyph:

				    --
			 +---------+
		--       |         |
		   +-----+--+ 1    | yadv1
		   |     | .|      |
	     yadv2 |    0+--+------+        -- BSB1  --
		   |   2    |       --      --        y_offset
		   |        |
     BSB2 --      0+--------+                        --
	  --    --

	 glyph1: advance height = 6
		 anchor point = (3,1)

	 glyph2: advance height = 7
		 anchor point = (9,4)

	 TSB is 1 for both glyphs; writing direction is T2B.


	   BSB1     = yadv1 - (TSB1 + ymax1)
	   BSB2     = yadv2 - (TSB2 + ymax2)
	   y_offset = y2 - y1

	 vertical advance width of glyph2
	   = y_offset + BSB2 - BSB1
	   = (y2 - y1) + (yadv2 - (TSB2 + ymax2)) - (yadv1 - (TSB1 + ymax1))
	   = y2 - y1 + yadv2 - TSB2 - ymax2 - (yadv1 - TSB1 - ymax1)
	   = y2 - y1 + yadv2 - TSB2 - ymax2 - yadv1 + TSB1 + ymax1


       b) Assuming that we recompute the advance height of the upper glyph:

				    --      --
			 +---------+        -- TSB1
	  --    --       |         |
     TSB2 --       +-----+--+ 1    | yadv1   ymax1
		   |     | .|      |
	     yadv2 |    0+--+------+        --       --
      ymax2        |   2    |       --                y_offset
		   |        |
	  --      0+--------+                        --
		--

	 glyph1: advance height = 6
		 anchor point = (3,1)

	 glyph2: advance height = 7
		 anchor point = (9,4)

	 TSB is 1 for both glyphs; writing direction is T2B.

	 y_offset = y2 - y1

	 vertical advance width of glyph2
	   = TSB1 + ymax1 + y_offset - (TSB2 + ymax2)
	   = TSB1 + ymax1 + y2 - y1 - TSB2 - ymax2


       Comparing a) with b) shows that b) is easier to compute.  I'll wait
       for a reply from Andrei to see what should really be implemented...

       Since horizontal advance widths or vertical advance heights
       can be used alone but not together, no ambiguity occurs.        */

    struct hb_ot_layout_context_t::info_t::gpos_t *gpi = &context->info.gpos;
    hb_codepoint_t last_pos = gpi->last;
    gpi->last = HB_OT_LAYOUT_GPOS_NO_LAST;

    /* We don't handle mark glyphs here. */
    if (property == HB_OT_LAYOUT_GLYPH_CLASS_MARK)
      return false;

    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (HB_LIKELY (index == NOT_COVERED))
      return false;

    const EntryExitRecord &record = entryExitRecord[index];

    if (last_pos == HB_OT_LAYOUT_GPOS_NO_LAST || !record.entryAnchor)
      goto end;

    hb_position_t entry_x, entry_y;
    (this+record.entryAnchor).get_anchor (context, IN_CURGLYPH (), &entry_x, &entry_y);

    /* TODO vertical */

    if (buffer->direction == HB_DIRECTION_RTL)
    {
      POSITION (buffer->in_pos)->x_advance   = entry_x - gpi->anchor_x;
      POSITION (buffer->in_pos)->new_advance = TRUE;
    }
    else
    {
      POSITION (last_pos)->x_advance   = gpi->anchor_x - entry_x;
      POSITION (last_pos)->new_advance = TRUE;
    }

    if  (lookup_flag & LookupFlag::RightToLeft)
    {
      POSITION (last_pos)->cursive_chain = last_pos - buffer->in_pos;
      POSITION (last_pos)->y_pos = entry_y - gpi->anchor_y;
    }
    else
    {
      POSITION (buffer->in_pos)->cursive_chain = buffer->in_pos - last_pos;
      POSITION (buffer->in_pos)->y_pos = gpi->anchor_y - entry_y;
    }

  end:
    if (record.exitAnchor)
    {
      gpi->last = buffer->in_pos;
      (this+record.exitAnchor).get_anchor (context, IN_CURGLYPH (), &gpi->anchor_x, &gpi->anchor_y);
    }

    buffer->in_pos++;
    return true;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_THIS2 (coverage, entryExitRecord);
  }

  private:
  USHORT	format;			/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of subtable */
  ArrayOf<EntryExitRecord>
		entryExitRecord;	/* Array of EntryExit records--in
					 * Coverage Index order */
};
ASSERT_SIZE (CursivePosFormat1, 6);

struct CursivePos
{
  friend struct PosLookupSubTable;

  private:
  inline bool apply (APPLY_ARG_DEF) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1->apply (APPLY_ARG);
    default:return false;
    }
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!SANITIZE (u.format)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (SANITIZE_ARG);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  CursivePosFormat1	format1[];
  } u;
};
ASSERT_SIZE (CursivePos, 2);


typedef AnchorMatrix BaseArray;		/* base-major--
					 * in order of BaseCoverage Index--,
					 * mark-minor--
					 * ordered by class--zero-based. */
ASSERT_SIZE (BaseArray, 2);

struct MarkBasePosFormat1
{
  friend struct MarkBasePos;

  private:
  inline bool apply (APPLY_ARG_DEF) const
  {
    TRACE_APPLY ();
    unsigned int mark_index = (this+markCoverage) (IN_CURGLYPH ());
    if (HB_LIKELY (mark_index == NOT_COVERED))
      return false;

    /* now we search backwards for a non-mark glyph */
    unsigned int j = buffer->in_pos;
    do
    {
      if (HB_UNLIKELY (!j))
	return false;
      j--;
    } while (_hb_ot_layout_skip_mark (context->face, IN_INFO (j), LookupFlag::IgnoreMarks, &property));

#if 0
    /* The following assertion is too strong. */
    if (!(property & HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH))
      return false;
#endif

    unsigned int base_index = (this+baseCoverage) (IN_GLYPH (j));
    if (base_index == NOT_COVERED)
      return false;

    return (this+markArray).apply (APPLY_ARG, mark_index, base_index, this+baseArray, classCount, j);
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF () && SANITIZE_THIS2 (markCoverage, baseCoverage) &&
	   SANITIZE_THIS (markArray) && baseArray.sanitize (SANITIZE_ARG, CONST_CHARP(this), classCount);
  }

  private:
  USHORT	format;			/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		markCoverage;		/* Offset to MarkCoverage table--from
					 * beginning of MarkBasePos subtable */
  OffsetTo<Coverage>
		baseCoverage;		/* Offset to BaseCoverage table--from
					 * beginning of MarkBasePos subtable */
  USHORT	classCount;		/* Number of classes defined for marks */
  OffsetTo<MarkArray>
		markArray;		/* Offset to MarkArray table--from
					 * beginning of MarkBasePos subtable */
  OffsetTo<BaseArray>
		baseArray;		/* Offset to BaseArray table--from
					 * beginning of MarkBasePos subtable */
};
ASSERT_SIZE (MarkBasePosFormat1, 12);

struct MarkBasePos
{
  friend struct PosLookupSubTable;

  private:
  inline bool apply (APPLY_ARG_DEF) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1->apply (APPLY_ARG);
    default:return false;
    }
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!SANITIZE (u.format)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (SANITIZE_ARG);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  MarkBasePosFormat1	format1[];
  } u;
};
ASSERT_SIZE (MarkBasePos, 2);


typedef AnchorMatrix LigatureAttach;	/* component-major--
					 * in order of writing direction--,
					 * mark-minor--
					 * ordered by class--zero-based. */
ASSERT_SIZE (LigatureAttach, 2);

typedef OffsetListOf<LigatureAttach> LigatureArray;
					/* Array of LigatureAttach
					 * tables ordered by
					 * LigatureCoverage Index */
ASSERT_SIZE (LigatureArray, 2);

struct MarkLigPosFormat1
{
  friend struct MarkLigPos;

  private:
  inline bool apply (APPLY_ARG_DEF) const
  {
    TRACE_APPLY ();
    unsigned int mark_index = (this+markCoverage) (IN_CURGLYPH ());
    if (HB_LIKELY (mark_index == NOT_COVERED))
      return false;

    /* now we search backwards for a non-mark glyph */
    unsigned int j = buffer->in_pos;
    do
    {
      if (HB_UNLIKELY (!j))
	return false;
      j--;
    } while (_hb_ot_layout_skip_mark (context->face, IN_INFO (j), LookupFlag::IgnoreMarks, &property));

#if 0
    /* The following assertion is too strong. */
    if (!(property & HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE))
      return false;
#endif

    unsigned int lig_index = (this+ligatureCoverage) (IN_GLYPH (j));
    if (lig_index == NOT_COVERED)
      return false;

    const LigatureArray& lig_array = this+ligatureArray;
    const LigatureAttach& lig_attach = lig_array[lig_index];

    /* Find component to attach to */
    unsigned int comp_count = lig_attach.rows;
    if (HB_UNLIKELY (!comp_count))
      return false;
    unsigned int comp_index;
    /* We must now check whether the ligature ID of the current mark glyph
     * is identical to the ligature ID of the found ligature.  If yes, we
     * can directly use the component index.  If not, we attach the mark
     * glyph to the last component of the ligature. */
    if (IN_LIGID (j) == IN_LIGID (buffer->in_pos))
    {
      comp_index = IN_COMPONENT (buffer->in_pos);
      if (comp_index >= comp_count)
	comp_index = comp_count - 1;
    }
    else
      comp_index = comp_count - 1;

    return (this+markArray).apply (APPLY_ARG, mark_index, comp_index, lig_attach, classCount, j);
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF () &&
	   SANITIZE_THIS2 (markCoverage, ligatureCoverage) &&
	   SANITIZE_THIS (markArray) && ligatureArray.sanitize (SANITIZE_ARG, CONST_CHARP(this), classCount);
  }

  private:
  USHORT	format;			/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		markCoverage;		/* Offset to Mark Coverage table--from
					 * beginning of MarkLigPos subtable */
  OffsetTo<Coverage>
		ligatureCoverage;	/* Offset to Ligature Coverage
					 * table--from beginning of MarkLigPos
					 * subtable */
  USHORT	classCount;		/* Number of defined mark classes */
  OffsetTo<MarkArray>
		markArray;		/* Offset to MarkArray table--from
					 * beginning of MarkLigPos subtable */
  OffsetTo<LigatureArray>
		ligatureArray;		/* Offset to LigatureArray table--from
					 * beginning of MarkLigPos subtable */
};
ASSERT_SIZE (MarkLigPosFormat1, 12);

struct MarkLigPos
{
  friend struct PosLookupSubTable;

  private:
  inline bool apply (APPLY_ARG_DEF) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1->apply (APPLY_ARG);
    default:return false;
    }
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!SANITIZE (u.format)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (SANITIZE_ARG);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  MarkLigPosFormat1	format1[];
  } u;
};
ASSERT_SIZE (MarkLigPos, 2);


typedef AnchorMatrix Mark2Array;	/* mark2-major--
					 * in order of Mark2Coverage Index--,
					 * mark1-minor--
					 * ordered by class--zero-based. */
ASSERT_SIZE (Mark2Array, 2);

struct MarkMarkPosFormat1
{
  friend struct MarkMarkPos;

  private:
  inline bool apply (APPLY_ARG_DEF) const
  {
    TRACE_APPLY ();
    unsigned int mark1_index = (this+mark1Coverage) (IN_CURGLYPH ());
    if (HB_LIKELY (mark1_index == NOT_COVERED))
      return false;

    /* now we search backwards for a suitable mark glyph until a non-mark glyph */
    unsigned int j = buffer->in_pos;
    do
    {
      if (HB_UNLIKELY (!j))
	return false;
      j--;
    } while (_hb_ot_layout_skip_mark (context->face, IN_INFO (j), lookup_flag, &property));

    if (!(property & HB_OT_LAYOUT_GLYPH_CLASS_MARK))
      return false;

    /* Two marks match only if they belong to the same base, or same component
     * of the same ligature. */
    if (IN_LIGID (j) != IN_LIGID (buffer->in_pos) ||
        IN_COMPONENT (j) != IN_COMPONENT (buffer->in_pos))
      return false;

    unsigned int mark2_index = (this+mark2Coverage) (IN_GLYPH (j));
    if (mark2_index == NOT_COVERED)
      return false;

    return (this+mark1Array).apply (APPLY_ARG, mark1_index, mark2_index, this+mark2Array, classCount, j);
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF () && SANITIZE_THIS2 (mark1Coverage, mark2Coverage) &&
	   SANITIZE_THIS (mark1Array) && mark2Array.sanitize (SANITIZE_ARG, CONST_CHARP(this), classCount);
  }

  private:
  USHORT	format;			/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		mark1Coverage;		/* Offset to Combining Mark1 Coverage
					 * table--from beginning of MarkMarkPos
					 * subtable */
  OffsetTo<Coverage>
		mark2Coverage;		/* Offset to Combining Mark2 Coverage
					 * table--from beginning of MarkMarkPos
					 * subtable */
  USHORT	classCount;		/* Number of defined mark classes */
  OffsetTo<MarkArray>
		mark1Array;		/* Offset to Mark1Array table--from
					 * beginning of MarkMarkPos subtable */
  OffsetTo<Mark2Array>
		mark2Array;		/* Offset to Mark2Array table--from
					 * beginning of MarkMarkPos subtable */
};
ASSERT_SIZE (MarkMarkPosFormat1, 12);

struct MarkMarkPos
{
  friend struct PosLookupSubTable;

  private:
  inline bool apply (APPLY_ARG_DEF) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1->apply (APPLY_ARG);
    default:return false;
    }
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!SANITIZE (u.format)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (SANITIZE_ARG);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  MarkMarkPosFormat1	format1[];
  } u;
};
ASSERT_SIZE (MarkMarkPos, 2);


static inline bool position_lookup (APPLY_ARG_DEF, unsigned int lookup_index);

struct ContextPos : Context
{
  friend struct PosLookupSubTable;

  private:
  inline bool apply (APPLY_ARG_DEF) const
  {
    TRACE_APPLY ();
    return Context::apply (APPLY_ARG, position_lookup);
  }
};
ASSERT_SIZE (ContextPos, 2);

struct ChainContextPos : ChainContext
{
  friend struct PosLookupSubTable;

  private:
  inline bool apply (APPLY_ARG_DEF) const
  {
    TRACE_APPLY ();
    return ChainContext::apply (APPLY_ARG, position_lookup);
  }
};
ASSERT_SIZE (ChainContextPos, 2);


struct ExtensionPos : Extension
{
  friend struct PosLookupSubTable;

  private:
  inline const struct PosLookupSubTable& get_subtable (void) const
  { return CONST_CAST (PosLookupSubTable, Extension::get_subtable (), 0); }

  inline bool apply (APPLY_ARG_DEF) const;

  inline bool sanitize (SANITIZE_ARG_DEF);
};
ASSERT_SIZE (ExtensionPos, 2);



/*
 * PosLookup
 */


struct PosLookupSubTable
{
  friend struct PosLookup;

  enum {
    Single		= 1,
    Pair		= 2,
    Cursive		= 3,
    MarkBase		= 4,
    MarkLig		= 5,
    MarkMark		= 6,
    Context		= 7,
    ChainContext	= 8,
    Extension		= 9
  };

  inline bool apply (APPLY_ARG_DEF, unsigned int lookup_type) const
  {
    TRACE_APPLY ();
    switch (lookup_type) {
    case Single:		return u.single->apply (APPLY_ARG);
    case Pair:			return u.pair->apply (APPLY_ARG);
    case Cursive:		return u.cursive->apply (APPLY_ARG);
    case MarkBase:		return u.markBase->apply (APPLY_ARG);
    case MarkLig:		return u.markLig->apply (APPLY_ARG);
    case MarkMark:		return u.markMark->apply (APPLY_ARG);
    case Context:		return u.context->apply (APPLY_ARG);
    case ChainContext:		return u.chainContext->apply (APPLY_ARG);
    case Extension:		return u.extension->apply (APPLY_ARG);
    default:return false;
    }
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!SANITIZE (u.format)) return false;
    switch (u.format) {
    case Single:		return u.single->sanitize (SANITIZE_ARG);
    case Pair:			return u.pair->sanitize (SANITIZE_ARG);
    case Cursive:		return u.cursive->sanitize (SANITIZE_ARG);
    case MarkBase:		return u.markBase->sanitize (SANITIZE_ARG);
    case MarkLig:		return u.markLig->sanitize (SANITIZE_ARG);
    case MarkMark:		return u.markMark->sanitize (SANITIZE_ARG);
    case Context:		return u.context->sanitize (SANITIZE_ARG);
    case ChainContext:		return u.chainContext->sanitize (SANITIZE_ARG);
    case Extension:		return u.extension->sanitize (SANITIZE_ARG);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;
  SinglePos		single[];
  PairPos		pair[];
  CursivePos		cursive[];
  MarkBasePos		markBase[];
  MarkLigPos		markLig[];
  MarkMarkPos		markMark[];
  ContextPos		context[];
  ChainContextPos	chainContext[];
  ExtensionPos		extension[];
  } u;
};
ASSERT_SIZE (PosLookupSubTable, 2);


struct PosLookup : Lookup
{
  inline const PosLookupSubTable& get_subtable (unsigned int i) const
  { return (const PosLookupSubTable&) Lookup::get_subtable (i); }

  /* Like get_type(), but looks through extension lookups.
   * Never returns Extension */
  inline unsigned int get_effective_type (void) const
  {
    unsigned int type = get_type ();

    if (HB_UNLIKELY (type == PosLookupSubTable::Extension))
    {
      unsigned int count = get_subtable_count ();
      type = get_subtable(0).u.extension->get_type ();
      /* The spec says all subtables should have the same type.
       * This is specially important if one has a reverse type! */
      for (unsigned int i = 1; i < count; i++)
        if (get_subtable(i).u.extension->get_type () != type)
	  return 0;
    }

    return type;
  }

  inline bool apply_once (hb_ot_layout_context_t *context,
			  hb_buffer_t    *buffer,
			  unsigned int    context_length,
			  unsigned int    nesting_level_left) const
  {
    unsigned int lookup_type = get_type ();
    unsigned int lookup_flag = get_flag ();
    unsigned int property;

    if (!_hb_ot_layout_check_glyph_property (context->face, IN_CURINFO (), lookup_flag, &property))
      return false;

    for (unsigned int i = 0; i < get_subtable_count (); i++)
      if (get_subtable (i).apply (APPLY_ARG_INIT, lookup_type))
	return true;

    return false;
  }

   inline bool apply_string (hb_ot_layout_context_t *context,
			     hb_buffer_t *buffer,
			     hb_mask_t    mask) const
  {
    bool ret = false;

    if (HB_UNLIKELY (!buffer->in_length))
      return false;

    context->info.gpos.last = HB_OT_LAYOUT_GPOS_NO_LAST; /* no last valid glyph for cursive pos. */

    buffer->in_pos = 0;
    while (buffer->in_pos < buffer->in_length)
    {
      bool done;
      if (~IN_MASK (buffer->in_pos) & mask)
      {
	  done = apply_once (context, buffer, NO_CONTEXT, MAX_NESTING_LEVEL);
	  ret |= done;
      }
      else
      {
          done = false;
	  /* Contrary to properties defined in GDEF, user-defined properties
	     will always stop a possible cursive positioning.                */
	  context->info.gpos.last = HB_OT_LAYOUT_GPOS_NO_LAST;
      }

      if (!done)
	buffer->in_pos++;
    }

    return ret;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!Lookup::sanitize (SANITIZE_ARG)) return false;
    OffsetArrayOf<PosLookupSubTable> &list = (OffsetArrayOf<PosLookupSubTable> &) subTable;
    return SANITIZE_THIS (list);
  }
};
ASSERT_SIZE (PosLookup, 6);

typedef OffsetListOf<PosLookup> PosLookupList;
ASSERT_SIZE (PosLookupList, 2);

/*
 * GPOS
 */

struct GPOS : GSUBGPOS
{
  static const hb_tag_t Tag	= HB_OT_TAG_GPOS;

  static inline const GPOS& get_for_data (const char *data)
  { return (const GPOS&) GSUBGPOS::get_for_data (data); }

  inline const PosLookup& get_lookup (unsigned int i) const
  { return (const PosLookup&) GSUBGPOS::get_lookup (i); }

  inline bool position_lookup (hb_ot_layout_context_t *context,
			       hb_buffer_t  *buffer,
			       unsigned int  lookup_index,
			       hb_mask_t     mask) const
  { return get_lookup (lookup_index).apply_string (context, buffer, mask); }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!GSUBGPOS::sanitize (SANITIZE_ARG)) return false;
    OffsetTo<PosLookupList> &list = CAST(OffsetTo<PosLookupList>, lookupList, 0);
    return SANITIZE_THIS (list);
  }
};
ASSERT_SIZE (GPOS, 10);


/* Out-of-class implementation for methods recursing */

inline bool ExtensionPos::apply (APPLY_ARG_DEF) const
{
  TRACE_APPLY ();
  unsigned int lookup_type = get_type ();

  if (HB_UNLIKELY (lookup_type == PosLookupSubTable::Extension))
    return false;

  return get_subtable ().apply (APPLY_ARG, lookup_type);
}

inline bool ExtensionPos::sanitize (SANITIZE_ARG_DEF)
{
  TRACE_SANITIZE ();
  return Extension::sanitize (SANITIZE_ARG) &&
	 (&(Extension::get_subtable ()) == &Null(LookupSubTable) ||
	  get_type () == PosLookupSubTable::Extension ||
	  DECONST_CAST (PosLookupSubTable, get_subtable (), 0).sanitize (SANITIZE_ARG));
}

static inline bool position_lookup (APPLY_ARG_DEF, unsigned int lookup_index)
{
  const GPOS &gpos = *(context->face->ot_layout.gpos);
  const PosLookup &l = gpos.get_lookup (lookup_index);

  if (HB_UNLIKELY (nesting_level_left == 0))
    return false;
  nesting_level_left--;

  if (HB_UNLIKELY (context_length < 1))
    return false;

  return l.apply_once (context, buffer, context_length, nesting_level_left);
}


#endif /* HB_OT_LAYOUT_GPOS_PRIVATE_HH */
