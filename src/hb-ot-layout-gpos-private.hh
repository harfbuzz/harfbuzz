/*
 * Copyright (C) 2007,2008,2009,2010  Red Hat, Inc.
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
 * Red Hat Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_LAYOUT_GPOS_PRIVATE_HH
#define HB_OT_LAYOUT_GPOS_PRIVATE_HH

#include "hb-ot-layout-gsubgpos-private.hh"


#undef BUFFER
#define BUFFER context->buffer


#define HB_OT_LAYOUT_GPOS_NO_LAST ((unsigned int) -1)

/* Shared Tables: ValueRecord, Anchor Table, and MarkArray */

typedef USHORT Value;

typedef Value ValueRecord[VAR0];
ASSERT_SIZE_VAR (ValueRecord, 0, Value);

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
    reserved	= 0xF000,	/* For future use */

    devices	= 0x00F0	/* Mask for having any Device table */
  };

/* All fields are options.  Only those available advance the value pointer. */
#if 0
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
#endif

  inline unsigned int get_len () const
  { return _hb_popcount32 ((unsigned int) *this); }
  inline unsigned int get_size () const
  { return get_len () * Value::static_size; }

  void apply_value (hb_ot_layout_context_t       *layout,
		    const char                   *base,
		    const Value                  *values,
		    hb_internal_glyph_position_t *glyph_pos) const
  {
    unsigned int x_ppem, y_ppem;
    hb_16dot16_t x_scale, y_scale;
    unsigned int format = *this;

    if (!format) return;

    x_scale = layout->font->x_scale;
    y_scale = layout->font->y_scale;
    /* design units -> fractional pixel */
    if (format & xPlacement) glyph_pos->x_offset  += _hb_16dot16_mul_round (x_scale, get_short (values++));
    if (format & yPlacement) glyph_pos->y_offset  += _hb_16dot16_mul_round (y_scale, get_short (values++));
    if (format & xAdvance)   glyph_pos->x_advance += _hb_16dot16_mul_round (x_scale, get_short (values++));
    if (format & yAdvance)   glyph_pos->y_advance += _hb_16dot16_mul_round (y_scale, get_short (values++));

    if (!has_device ()) return;

    x_ppem = layout->font->x_ppem;
    y_ppem = layout->font->y_ppem;

    if (!x_ppem && !y_ppem) return;

    /* pixel -> fractional pixel */
    if (format & xPlaDevice) {
      if (x_ppem) glyph_pos->x_offset  += (base + get_device (values++)).get_delta (x_ppem) << 16; else values++;
    }
    if (format & yPlaDevice) {
      if (y_ppem) glyph_pos->y_offset  += (base + get_device (values++)).get_delta (y_ppem) << 16; else values++;
    }
    if (format & xAdvDevice) {
      if (x_ppem) glyph_pos->x_advance += (base + get_device (values++)).get_delta (x_ppem) << 16; else values++;
    }
    if (format & yAdvDevice) {
      if (y_ppem) glyph_pos->y_advance += (base + get_device (values++)).get_delta (y_ppem) << 16; else values++;
    }
  }

  private:
  inline bool sanitize_value_devices (hb_sanitize_context_t *context, void *base, Value *values) {
    unsigned int format = *this;

    if (format & xPlacement) values++;
    if (format & yPlacement) values++;
    if (format & xAdvance)   values++;
    if (format & yAdvance)   values++;

    if ((format & xPlaDevice) && !get_device (values++).sanitize (context, base)) return false;
    if ((format & yPlaDevice) && !get_device (values++).sanitize (context, base)) return false;
    if ((format & xAdvDevice) && !get_device (values++).sanitize (context, base)) return false;
    if ((format & yAdvDevice) && !get_device (values++).sanitize (context, base)) return false;

    return true;
  }

  static inline OffsetTo<Device>& get_device (Value* value)
  { return *CastP<OffsetTo<Device> > (value); }
  static inline const OffsetTo<Device>& get_device (const Value* value)
  { return *CastP<OffsetTo<Device> > (value); }

  static inline const SHORT& get_short (const Value* value)
  { return *CastP<SHORT> (value); }

  public:

  inline bool has_device () const {
    unsigned int format = *this;
    return (format & devices) != 0;
  }

  inline bool sanitize_value (hb_sanitize_context_t *context, void *base, Value *values) {
    TRACE_SANITIZE ();
    return context->check_range (values, get_size ())
	&& (!has_device () || sanitize_value_devices (context, base, values));
  }

  inline bool sanitize_values (hb_sanitize_context_t *context, void *base, Value *values, unsigned int count) {
    TRACE_SANITIZE ();
    unsigned int len = get_len ();

    if (!context->check_array (values, get_size (), count)) return false;

    if (!has_device ()) return true;

    for (unsigned int i = 0; i < count; i++) {
      if (!sanitize_value_devices (context, base, values))
        return false;
      values += len;
    }

    return true;
  }

  /* Just sanitize referenced Device tables.  Doesn't check the values themselves. */
  inline bool sanitize_values_stride_unsafe (hb_sanitize_context_t *context, void *base, Value *values, unsigned int count, unsigned int stride) {
    TRACE_SANITIZE ();

    if (!has_device ()) return true;

    for (unsigned int i = 0; i < count; i++) {
      if (!sanitize_value_devices (context, base, values))
        return false;
      values += stride;
    }

    return true;
  }
};
ASSERT_SIZE (ValueFormat, 2);


struct AnchorFormat1
{
  friend struct Anchor;

  private:
  inline void get_anchor (hb_ot_layout_context_t *layout, hb_codepoint_t glyph_id HB_UNUSED,
			  hb_position_t *x, hb_position_t *y) const
  {
      *x = _hb_16dot16_mul_round (layout->font->x_scale, xCoordinate);
      *y = _hb_16dot16_mul_round (layout->font->y_scale, yCoordinate);
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return context->check_struct (this);
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
  inline void get_anchor (hb_ot_layout_context_t *layout, hb_codepoint_t glyph_id,
			  hb_position_t *x, hb_position_t *y) const
  {
      unsigned int x_ppem = layout->font->x_ppem;
      unsigned int y_ppem = layout->font->y_ppem;
      hb_position_t cx, cy;
      hb_bool_t ret;

      if (x_ppem || y_ppem)
	ret = hb_font_get_contour_point (layout->font, layout->face, anchorPoint, glyph_id, &cx, &cy);
      *x = x_ppem && ret ? cx : _hb_16dot16_mul_round (layout->font->x_scale, xCoordinate);
      *y = y_ppem && ret ? cy : _hb_16dot16_mul_round (layout->font->y_scale, yCoordinate);
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return context->check_struct (this);
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
  inline void get_anchor (hb_ot_layout_context_t *layout, hb_codepoint_t glyph_id HB_UNUSED,
			  hb_position_t *x, hb_position_t *y) const
  {
      *x = _hb_16dot16_mul_round (layout->font->x_scale, xCoordinate);
      *y = _hb_16dot16_mul_round (layout->font->y_scale, yCoordinate);

      /* pixel -> fractional pixel */
      if (layout->font->x_ppem)
	*x += (this+xDeviceTable).get_delta (layout->font->x_ppem) << 16;
      if (layout->font->y_ppem)
	*y += (this+yDeviceTable).get_delta (layout->font->y_ppem) << 16;
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return context->check_struct (this)
	&& xDeviceTable.sanitize (context, this)
	&& yDeviceTable.sanitize (context, this);
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
  inline void get_anchor (hb_ot_layout_context_t *layout, hb_codepoint_t glyph_id,
			  hb_position_t *x, hb_position_t *y) const
  {
    *x = *y = 0;
    switch (u.format) {
    case 1: u.format1->get_anchor (layout, glyph_id, x, y); return;
    case 2: u.format2->get_anchor (layout, glyph_id, x, y); return;
    case 3: u.format3->get_anchor (layout, glyph_id, x, y); return;
    default:						    return;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (context)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (context);
    case 2: return u.format2->sanitize (context);
    case 3: return u.format3->sanitize (context);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  AnchorFormat1		format1[VAR];
  AnchorFormat2		format2[VAR];
  AnchorFormat3		format3[VAR];
  } u;
};


struct AnchorMatrix
{
  inline const Anchor& get_anchor (unsigned int row, unsigned int col, unsigned int cols) const {
    if (unlikely (row >= rows || col >= cols)) return Null(Anchor);
    return this+matrix[row * cols + col];
  }

  inline bool sanitize (hb_sanitize_context_t *context, unsigned int cols) {
    TRACE_SANITIZE ();
    if (!context->check_struct (this)) return false;
    if (unlikely (cols >= ((unsigned int) -1) / rows)) return false;
    unsigned int count = rows * cols;
    if (!context->check_array (matrix, matrix[0].static_size, count)) return false;
    for (unsigned int i = 0; i < count; i++)
      if (!matrix[i].sanitize (context, this)) return false;
    return true;
  }

  USHORT	rows;			/* Number of rows */
  private:
  OffsetTo<Anchor>
		matrix[VAR];		/* Matrix of offsets to Anchor tables--
					 * from beginning of AnchorMatrix table */
};
ASSERT_SIZE_VAR (AnchorMatrix, 2, OffsetTo<Anchor>);


struct MarkRecord
{
  friend struct MarkArray;

  inline bool sanitize (hb_sanitize_context_t *context, void *base) {
    TRACE_SANITIZE ();
    return context->check_struct (this)
	&& markAnchor.sanitize (context, base);
  }

  DEFINE_SIZE_STATIC (4);

  private:
  USHORT	klass;			/* Class defined for this mark */
  OffsetTo<Anchor>
		markAnchor;		/* Offset to Anchor table--from
					 * beginning of MarkArray table */
};

struct MarkArray
{
  inline bool apply (hb_apply_context_t *context,
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

    mark_anchor.get_anchor (context->layout, IN_CURGLYPH (), &mark_x, &mark_y);
    glyph_anchor.get_anchor (context->layout, IN_GLYPH (glyph_pos), &base_x, &base_y);

    hb_internal_glyph_position_t *o = POSITION (context->buffer->in_pos);
    o->x_advance = 0;
    o->y_advance = 0;
    o->x_offset  = base_x - mark_x;
    o->y_offset  = base_y - mark_y;
    o->back      = context->buffer->in_pos - glyph_pos;

    context->buffer->in_pos++;
    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return markRecord.sanitize (context, this);
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
  inline bool apply (hb_apply_context_t *context) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (likely (index == NOT_COVERED))
      return false;

    valueFormat.apply_value (context->layout, CharP(this), values, CURPOSITION ());

    context->buffer->in_pos++;
    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return context->check_struct (this)
	&& coverage.sanitize (context, this)
	&& valueFormat.sanitize_value (context, CharP(this), values);
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
ASSERT_SIZE_VAR (SinglePosFormat1, 6, ValueRecord);

struct SinglePosFormat2
{
  friend struct SinglePos;

  private:
  inline bool apply (hb_apply_context_t *context) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (likely (index == NOT_COVERED))
      return false;

    if (likely (index >= valueCount))
      return false;

    valueFormat.apply_value (context->layout, CharP(this),
			     &values[index * valueFormat.get_len ()],
			     CURPOSITION ());

    context->buffer->in_pos++;
    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return context->check_struct (this)
	&& coverage.sanitize (context, this)
	&& valueFormat.sanitize_values (context, CharP(this), values, valueCount);
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
ASSERT_SIZE_VAR (SinglePosFormat2, 8, ValueRecord);

struct SinglePos
{
  friend struct PosLookupSubTable;

  private:
  inline bool apply (hb_apply_context_t *context) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1->apply (context);
    case 2: return u.format2->apply (context);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (context)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (context);
    case 2: return u.format2->sanitize (context);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  SinglePosFormat1	format1[VAR];
  SinglePosFormat2	format2[VAR];
  } u;
};


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
ASSERT_SIZE_VAR (PairValueRecord, 2, ValueRecord);

struct PairSet
{
  friend struct PairPosFormat1;

  /* Note: Doesn't sanitize the Device entries in the ValueRecord */
  inline bool sanitize (hb_sanitize_context_t *context, unsigned int format_len) {
    TRACE_SANITIZE ();
    if (!context->check_struct (this)) return false;
    unsigned int count = (1 + format_len) * len;
    return context->check_array (array, USHORT::static_size, count);
  }

  private:
  USHORT	len;			/* Number of PairValueRecords */
  PairValueRecord
		array[VAR];		/* Array of PairValueRecords--ordered
					 * by GlyphID of the second glyph */
};
ASSERT_SIZE_VAR (PairSet, 2, PairValueRecord);

struct PairPosFormat1
{
  friend struct PairPos;

  private:
  inline bool apply (hb_apply_context_t *context) const
  {
    TRACE_APPLY ();
    unsigned int end = MIN (context->buffer->in_length, context->buffer->in_pos + context->context_length);
    if (unlikely (context->buffer->in_pos + 2 > end))
      return false;

    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (likely (index == NOT_COVERED))
      return false;

    unsigned int j = context->buffer->in_pos + 1;
    while (_hb_ot_layout_skip_mark (context->layout->face, IN_INFO (j), context->lookup_flag, NULL))
    {
      if (unlikely (j == end))
	return false;
      j++;
    }

    unsigned int len1 = valueFormat1.get_len ();
    unsigned int len2 = valueFormat2.get_len ();
    unsigned int record_size = USHORT::static_size * (1 + len1 + len2);

    const PairSet &pair_set = this+pairSet[index];
    unsigned int count = pair_set.len;
    const PairValueRecord *record = pair_set.array;
    for (unsigned int i = 0; i < count; i++)
    {
      if (IN_GLYPH (j) == record->secondGlyph)
      {
	valueFormat1.apply_value (context->layout, CharP(this), &record->values[0], CURPOSITION ());
	valueFormat2.apply_value (context->layout, CharP(this), &record->values[len1], POSITION (j));
	if (len2)
	  j++;
	context->buffer->in_pos = j;
	return true;
      }
      record = &StructAtOffset<PairValueRecord> (*record, record_size);
    }

    return false;
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();

    unsigned int len1 = valueFormat1.get_len ();
    unsigned int len2 = valueFormat2.get_len ();

    if (!(context->check_struct (this)
       && coverage.sanitize (context, this)
       && likely (pairSet.sanitize (context, CharP(this), len1 + len2)))) return false;

    if (!(valueFormat1.has_device () || valueFormat2.has_device ())) return true;

    unsigned int stride = 1 + len1 + len2;
    unsigned int count1 = pairSet.len;
    for (unsigned int i = 0; i < count1; i++)
    {
      PairSet &pair_set = const_cast<PairSet &> (this+pairSet[i]); /* XXX clean this up */

      unsigned int count2 = pair_set.len;
      PairValueRecord *record = pair_set.array;
      if (!(valueFormat1.sanitize_values_stride_unsafe (context, CharP(this), &record->values[0], count2, stride) &&
	    valueFormat2.sanitize_values_stride_unsafe (context, CharP(this), &record->values[len1], count2, stride)))
        return false;
    }

    return true;
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
  inline bool apply (hb_apply_context_t *context) const
  {
    TRACE_APPLY ();
    unsigned int end = MIN (context->buffer->in_length, context->buffer->in_pos + context->context_length);
    if (unlikely (context->buffer->in_pos + 2 > end))
      return false;

    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (likely (index == NOT_COVERED))
      return false;

    unsigned int j = context->buffer->in_pos + 1;
    while (_hb_ot_layout_skip_mark (context->layout->face, IN_INFO (j), context->lookup_flag, NULL))
    {
      if (unlikely (j == end))
	return false;
      j++;
    }

    unsigned int len1 = valueFormat1.get_len ();
    unsigned int len2 = valueFormat2.get_len ();
    unsigned int record_len = len1 + len2;

    unsigned int klass1 = (this+classDef1) (IN_CURGLYPH ());
    unsigned int klass2 = (this+classDef2) (IN_GLYPH (j));
    if (unlikely (klass1 >= class1Count || klass2 >= class2Count))
      return false;

    const Value *v = &values[record_len * (klass1 * class2Count + klass2)];
    valueFormat1.apply_value (context->layout, CharP(this), v, CURPOSITION ());
    valueFormat2.apply_value (context->layout, CharP(this), v + len1, POSITION (j));

    if (len2)
      j++;
    context->buffer->in_pos = j;

    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (!(context->check_struct (this)
       && coverage.sanitize (context, this)
       && classDef1.sanitize (context, this)
       && classDef2.sanitize (context, this))) return false;

    unsigned int len1 = valueFormat1.get_len ();
    unsigned int len2 = valueFormat2.get_len ();
    unsigned int stride = len1 + len2;
    unsigned int record_size = valueFormat1.get_size () + valueFormat2.get_size ();
    unsigned int count = (unsigned int) class1Count * (unsigned int) class2Count;
    return context->check_array (values, record_size, count) &&
	   valueFormat1.sanitize_values_stride_unsafe (context, CharP(this), &values[0], count, stride) &&
	   valueFormat2.sanitize_values_stride_unsafe (context, CharP(this), &values[len1], count, stride);
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
ASSERT_SIZE_VAR (PairPosFormat2, 16, ValueRecord);

struct PairPos
{
  friend struct PosLookupSubTable;

  private:
  inline bool apply (hb_apply_context_t *context) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1->apply (context);
    case 2: return u.format2->apply (context);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (context)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (context);
    case 2: return u.format2->sanitize (context);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  PairPosFormat1	format1[VAR];
  PairPosFormat2	format2[VAR];
  } u;
};


struct EntryExitRecord
{
  inline bool sanitize (hb_sanitize_context_t *context, void *base) {
    TRACE_SANITIZE ();
    return entryAnchor.sanitize (context, base)
	&& exitAnchor.sanitize (context, base);
  }

  DEFINE_SIZE_STATIC (4);

  OffsetTo<Anchor>
		entryAnchor;		/* Offset to EntryAnchor table--from
					 * beginning of CursivePos
					 * subtable--may be NULL */
  OffsetTo<Anchor>
		exitAnchor;		/* Offset to ExitAnchor table--from
					 * beginning of CursivePos
					 * subtable--may be NULL */
};

struct CursivePosFormat1
{
  friend struct CursivePos;

  private:
  inline bool apply (hb_apply_context_t *context) const
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

    struct hb_ot_layout_context_t::info_t::gpos_t *gpi = &context->layout->info.gpos;
    hb_codepoint_t last_pos = gpi->last;
    gpi->last = HB_OT_LAYOUT_GPOS_NO_LAST;

    /* We don't handle mark glyphs here. */
    if (context->property == HB_OT_LAYOUT_GLYPH_CLASS_MARK)
      return false;

    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (likely (index == NOT_COVERED))
      return false;

    const EntryExitRecord &record = entryExitRecord[index];

    if (last_pos == HB_OT_LAYOUT_GPOS_NO_LAST || !record.entryAnchor)
      goto end;

    hb_position_t entry_x, entry_y;
    (this+record.entryAnchor).get_anchor (context->layout, IN_CURGLYPH (), &entry_x, &entry_y);

    /* TODO vertical */

    if (context->buffer->direction == HB_DIRECTION_RTL)
    {
      /* advance is absolute, not relative */
      POSITION (context->buffer->in_pos)->x_advance = entry_x - gpi->anchor_x;
    }
    else
    {
      /* advance is absolute, not relative */
      POSITION (last_pos)->x_advance = gpi->anchor_x - entry_x;
    }

    if  (context->lookup_flag & LookupFlag::RightToLeft)
    {
      POSITION (last_pos)->cursive_chain = last_pos - context->buffer->in_pos;
      POSITION (last_pos)->y_offset = entry_y - gpi->anchor_y;
    }
    else
    {
      POSITION (context->buffer->in_pos)->cursive_chain = context->buffer->in_pos - last_pos;
      POSITION (context->buffer->in_pos)->y_offset = gpi->anchor_y - entry_y;
    }

  end:
    if (record.exitAnchor)
    {
      gpi->last = context->buffer->in_pos;
      (this+record.exitAnchor).get_anchor (context->layout, IN_CURGLYPH (), &gpi->anchor_x, &gpi->anchor_y);
    }

    context->buffer->in_pos++;
    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return coverage.sanitize (context, this)
	&& entryExitRecord.sanitize (context, this);
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
  inline bool apply (hb_apply_context_t *context) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1->apply (context);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (context)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (context);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  CursivePosFormat1	format1[VAR];
  } u;
};


typedef AnchorMatrix BaseArray;		/* base-major--
					 * in order of BaseCoverage Index--,
					 * mark-minor--
					 * ordered by class--zero-based. */

struct MarkBasePosFormat1
{
  friend struct MarkBasePos;

  private:
  inline bool apply (hb_apply_context_t *context) const
  {
    TRACE_APPLY ();
    unsigned int mark_index = (this+markCoverage) (IN_CURGLYPH ());
    if (likely (mark_index == NOT_COVERED))
      return false;

    /* now we search backwards for a non-mark glyph */
    unsigned int property;
    unsigned int j = context->buffer->in_pos;
    do
    {
      if (unlikely (!j))
	return false;
      j--;
    } while (_hb_ot_layout_skip_mark (context->layout->face, IN_INFO (j), LookupFlag::IgnoreMarks, &property));

#if 0
    /* The following assertion is too strong. */
    if (!(property & HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH))
      return false;
#endif

    unsigned int base_index = (this+baseCoverage) (IN_GLYPH (j));
    if (base_index == NOT_COVERED)
      return false;

    return (this+markArray).apply (context, mark_index, base_index, this+baseArray, classCount, j);
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return context->check_struct (this)
        && markCoverage.sanitize (context, this)
	&& baseCoverage.sanitize (context, this)
	&& markArray.sanitize (context, this)
	&& likely (baseArray.sanitize (context, CharP(this), (unsigned int) classCount));
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
  inline bool apply (hb_apply_context_t *context) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1->apply (context);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (context)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (context);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  MarkBasePosFormat1	format1[VAR];
  } u;
};


typedef AnchorMatrix LigatureAttach;	/* component-major--
					 * in order of writing direction--,
					 * mark-minor--
					 * ordered by class--zero-based. */

typedef OffsetListOf<LigatureAttach> LigatureArray;
					/* Array of LigatureAttach
					 * tables ordered by
					 * LigatureCoverage Index */

struct MarkLigPosFormat1
{
  friend struct MarkLigPos;

  private:
  inline bool apply (hb_apply_context_t *context) const
  {
    TRACE_APPLY ();
    unsigned int mark_index = (this+markCoverage) (IN_CURGLYPH ());
    if (likely (mark_index == NOT_COVERED))
      return false;

    /* now we search backwards for a non-mark glyph */
    unsigned int property;
    unsigned int j = context->buffer->in_pos;
    do
    {
      if (unlikely (!j))
	return false;
      j--;
    } while (_hb_ot_layout_skip_mark (context->layout->face, IN_INFO (j), LookupFlag::IgnoreMarks, &property));

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
    if (unlikely (!comp_count))
      return false;
    unsigned int comp_index;
    /* We must now check whether the ligature ID of the current mark glyph
     * is identical to the ligature ID of the found ligature.  If yes, we
     * can directly use the component index.  If not, we attach the mark
     * glyph to the last component of the ligature. */
    if (IN_LIGID (j) && IN_LIGID (j) == IN_LIGID (context->buffer->in_pos) && IN_COMPONENT (context->buffer->in_pos))
    {
      comp_index = IN_COMPONENT (context->buffer->in_pos) - 1;
      if (comp_index >= comp_count)
	comp_index = comp_count - 1;
    }
    else
      comp_index = comp_count - 1;

    return (this+markArray).apply (context, mark_index, comp_index, lig_attach, classCount, j);
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return context->check_struct (this)
        && markCoverage.sanitize (context, this)
	&& ligatureCoverage.sanitize (context, this)
	&& markArray.sanitize (context, this)
	&& likely (ligatureArray.sanitize (context, CharP(this), (unsigned int) classCount));
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
  inline bool apply (hb_apply_context_t *context) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1->apply (context);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (context)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (context);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  MarkLigPosFormat1	format1[VAR];
  } u;
};


typedef AnchorMatrix Mark2Array;	/* mark2-major--
					 * in order of Mark2Coverage Index--,
					 * mark1-minor--
					 * ordered by class--zero-based. */

struct MarkMarkPosFormat1
{
  friend struct MarkMarkPos;

  private:
  inline bool apply (hb_apply_context_t *context) const
  {
    TRACE_APPLY ();
    unsigned int mark1_index = (this+mark1Coverage) (IN_CURGLYPH ());
    if (likely (mark1_index == NOT_COVERED))
      return false;

    /* now we search backwards for a suitable mark glyph until a non-mark glyph */
    unsigned int property;
    unsigned int j = context->buffer->in_pos;
    do
    {
      if (unlikely (!j))
	return false;
      j--;
    } while (_hb_ot_layout_skip_mark (context->layout->face, IN_INFO (j), context->lookup_flag, &property));

    if (!(property & HB_OT_LAYOUT_GLYPH_CLASS_MARK))
      return false;

    /* Two marks match only if they belong to the same base, or same component
     * of the same ligature.  That is, the component numbers must match, and
     * if those are non-zero, the ligid number should also match. */
    if ((IN_COMPONENT (j) != IN_COMPONENT (context->buffer->in_pos)) ||
	(IN_COMPONENT (j) && IN_LIGID (j) != IN_LIGID (context->buffer->in_pos)))
      return false;

    unsigned int mark2_index = (this+mark2Coverage) (IN_GLYPH (j));
    if (mark2_index == NOT_COVERED)
      return false;

    return (this+mark1Array).apply (context, mark1_index, mark2_index, this+mark2Array, classCount, j);
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return context->check_struct (this)
	&& mark1Coverage.sanitize (context, this)
	&& mark2Coverage.sanitize (context, this)
	&& mark1Array.sanitize (context, this)
	&& likely (mark2Array.sanitize (context, CharP(this), (unsigned int) classCount));
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
  inline bool apply (hb_apply_context_t *context) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1->apply (context);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (context)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (context);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  MarkMarkPosFormat1	format1[VAR];
  } u;
};


static inline bool position_lookup (hb_apply_context_t *context, unsigned int lookup_index);

struct ContextPos : Context
{
  friend struct PosLookupSubTable;

  private:
  inline bool apply (hb_apply_context_t *context) const
  {
    TRACE_APPLY ();
    return Context::apply (context, position_lookup);
  }
};

struct ChainContextPos : ChainContext
{
  friend struct PosLookupSubTable;

  private:
  inline bool apply (hb_apply_context_t *context) const
  {
    TRACE_APPLY ();
    return ChainContext::apply (context, position_lookup);
  }
};


struct ExtensionPos : Extension
{
  friend struct PosLookupSubTable;

  private:
  inline const struct PosLookupSubTable& get_subtable (void) const
  {
    unsigned int offset = get_offset ();
    if (unlikely (!offset)) return Null(PosLookupSubTable);
    return StructAtOffset<PosLookupSubTable> (*this, offset);
  }

  inline bool apply (hb_apply_context_t *context) const;

  inline bool sanitize (hb_sanitize_context_t *context);
};



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

  inline bool apply (hb_apply_context_t *context, unsigned int lookup_type) const
  {
    TRACE_APPLY ();
    switch (lookup_type) {
    case Single:		return u.single->apply (context);
    case Pair:			return u.pair->apply (context);
    case Cursive:		return u.cursive->apply (context);
    case MarkBase:		return u.markBase->apply (context);
    case MarkLig:		return u.markLig->apply (context);
    case MarkMark:		return u.markMark->apply (context);
    case Context:		return u.context->apply (context);
    case ChainContext:		return u.chainContext->apply (context);
    case Extension:		return u.extension->apply (context);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (context)) return false;
    switch (u.format) {
    case Single:		return u.single->sanitize (context);
    case Pair:			return u.pair->sanitize (context);
    case Cursive:		return u.cursive->sanitize (context);
    case MarkBase:		return u.markBase->sanitize (context);
    case MarkLig:		return u.markLig->sanitize (context);
    case MarkMark:		return u.markMark->sanitize (context);
    case Context:		return u.context->sanitize (context);
    case ChainContext:		return u.chainContext->sanitize (context);
    case Extension:		return u.extension->sanitize (context);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;
  SinglePos		single[VAR];
  PairPos		pair[VAR];
  CursivePos		cursive[VAR];
  MarkBasePos		markBase[VAR];
  MarkLigPos		markLig[VAR];
  MarkMarkPos		markMark[VAR];
  ContextPos		context[VAR];
  ChainContextPos	chainContext[VAR];
  ExtensionPos		extension[VAR];
  } u;
};


struct PosLookup : Lookup
{
  inline const PosLookupSubTable& get_subtable (unsigned int i) const
  { return this+CastR<OffsetArrayOf<PosLookupSubTable> > (subTable)[i]; }

  inline bool apply_once (hb_ot_layout_context_t *layout,
			  hb_buffer_t    *buffer,
			  unsigned int    context_length,
			  unsigned int    nesting_level_left) const
  {
    unsigned int lookup_type = get_type ();
    hb_apply_context_t context[1] = {{0}};

    context->layout = layout;
    context->buffer = buffer;
    context->context_length = context_length;
    context->nesting_level_left = nesting_level_left;
    context->lookup_flag = get_flag ();

    if (!_hb_ot_layout_check_glyph_property (context->layout->face, IN_CURINFO (), context->lookup_flag, &context->property))
      return false;

    for (unsigned int i = 0; i < get_subtable_count (); i++)
      if (get_subtable (i).apply (context, lookup_type))
	return true;

    return false;
  }

   inline bool apply_string (hb_ot_layout_context_t *layout,
			     hb_buffer_t *buffer,
			     hb_mask_t    mask) const
  {
#undef BUFFER
#define BUFFER buffer
    bool ret = false;

    if (unlikely (!buffer->in_length))
      return false;

    layout->info.gpos.last = HB_OT_LAYOUT_GPOS_NO_LAST; /* no last valid glyph for cursive pos. */

    buffer->in_pos = 0;
    while (buffer->in_pos < buffer->in_length)
    {
      bool done;
      if (~IN_MASK (buffer->in_pos) & mask)
      {
	  done = apply_once (layout, buffer, NO_CONTEXT, MAX_NESTING_LEVEL);
	  ret |= done;
      }
      else
      {
          done = false;
	  /* Contrary to properties defined in GDEF, user-defined properties
	     will always stop a possible cursive positioning.                */
	  layout->info.gpos.last = HB_OT_LAYOUT_GPOS_NO_LAST;
      }

      if (!done)
	buffer->in_pos++;
    }

    return ret;
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (unlikely (!Lookup::sanitize (context))) return false;
    OffsetArrayOf<PosLookupSubTable> &list = CastR<OffsetArrayOf<PosLookupSubTable> > (subTable);
    return list.sanitize (context, this);
  }
};

typedef OffsetListOf<PosLookup> PosLookupList;
ASSERT_SIZE (PosLookupList, 2);

/*
 * GPOS
 */

struct GPOS : GSUBGPOS
{
  static const hb_tag_t Tag	= HB_OT_TAG_GPOS;

  inline const PosLookup& get_lookup (unsigned int i) const
  { return CastR<PosLookup> (GSUBGPOS::get_lookup (i)); }

  inline bool position_lookup (hb_ot_layout_context_t *layout,
			       hb_buffer_t  *buffer,
			       unsigned int  lookup_index,
			       hb_mask_t     mask) const
  { return get_lookup (lookup_index).apply_string (layout, buffer, mask); }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (unlikely (!GSUBGPOS::sanitize (context))) return false;
    OffsetTo<PosLookupList> &list = CastR<OffsetTo<PosLookupList> > (lookupList);
    return list.sanitize (context, this);
  }
};
ASSERT_SIZE (GPOS, 10);


/* Out-of-class implementation for methods recursing */

inline bool ExtensionPos::apply (hb_apply_context_t *context) const
{
  TRACE_APPLY ();
  return get_subtable ().apply (context, get_type ());
}

inline bool ExtensionPos::sanitize (hb_sanitize_context_t *context)
{
  TRACE_SANITIZE ();
  if (unlikely (!Extension::sanitize (context))) return false;
  unsigned int offset = get_offset ();
  if (unlikely (!offset)) return true;
  return StructAtOffset<PosLookupSubTable> (*this, offset).sanitize (context);
}

static inline bool position_lookup (hb_apply_context_t *context, unsigned int lookup_index)
{
  const GPOS &gpos = *(context->layout->face->ot_layout.gpos);
  const PosLookup &l = gpos.get_lookup (lookup_index);

  if (unlikely (context->nesting_level_left == 0))
    return false;

  if (unlikely (context->context_length < 1))
    return false;

  return l.apply_once (context->layout, context->buffer, context->context_length, context->nesting_level_left - 1);
}


#endif /* HB_OT_LAYOUT_GPOS_PRIVATE_HH */
