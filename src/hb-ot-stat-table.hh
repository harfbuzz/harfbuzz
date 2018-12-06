/*
 * Copyright © 2018  Ebrahim Byagowi
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
 */

#ifndef HB_OT_STAT_TABLE_HH
#define HB_OT_STAT_TABLE_HH

#include "hb-open-type.hh"
#include "hb-ot-layout-common.hh"

#include "hb-ot-style.h"
#include "hb-ot-var.h"

#include "hb-aat-fdsc-table.hh"
#include "hb-ot-os2-table.hh"
#include "hb-ot-head-table.hh"
#include "hb-ot-stat-table.hh"

/*
 * STAT -- Style Attributes
 * https://docs.microsoft.com/en-us/typography/opentype/spec/stat
 */
#define HB_OT_TAG_STAT HB_TAG('S','T','A','T')


namespace OT {


struct AxisValue;

struct AxisValueFormat1
{
  friend struct AxisValue;

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)));
  }

  protected:
  HBUINT16	format;		/* Format identifier — set to 1. */
  HBUINT16	axisIndex;	/* Zero-base index into the axis record array
				 * identifying the axis of design variation
				 * to which the axis value record applies.
				 * Must be less than designAxisCount. */
  HBUINT16	flags;		/* Flags — see hb_ot_style_flag_t */
  NameID	valueNameID;	/* The name ID for entries in the 'name' table
				 * that provide a display string for this
				 * attribute value. */
  Fixed		value;		/* A numeric value for this attribute value. */
  public:
  DEFINE_SIZE_STATIC (12);
};

struct AxisValueFormat2
{
  friend struct AxisValue;

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)));
  }

  protected:
  HBUINT16	format;		/* Format identifier — set to 2. */
  HBUINT16	axisIndex;	/* Zero-base index into the axis record array
				 * identifying the axis of design variation
				 * to which the axis value record applies.
				 * Must be less than designAxisCount. */
  HBUINT16	flags;		/* Flags — see hb_ot_style_flag_t */
  NameID	valueNameID;	/* The name ID for entries in the 'name' table
				 * that provide a display string for this
				 * attribute value. */
  Fixed		nominalValue;	/* A numeric value for this attribute value. */
  Fixed		rangeMinValue;	/* The minimum value for a range associated
				 * with the specified name ID. */
  Fixed		rangeMaxValue;	/* The maximum value for a range associated
				 * with the specified name ID. */
  public:
  DEFINE_SIZE_STATIC (20);
};

struct AxisValueFormat3
{
  friend struct AxisValue;

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)));
  }

  protected:
  HBUINT16	format;		/* Format identifier — set to 3. */
  HBUINT16	axisIndex;	/* Zero-base index into the axis record array
				 * identifying the axis of design variation
				 * to which the axis value record applies.
				 * Must be less than designAxisCount. */
  HBUINT16	flags;		/* Flags — see hb_ot_style_flag_t */
  NameID	valueNameID;	/* The name ID for entries in the 'name' table
				 * that provide a display string for this
				 * attribute value. */
  Fixed		value;		/* A numeric value for this attribute value. */
  Fixed		linkedValue;	/* The numeric value for a style-linked mapping
				 * from this value. */
  public:
  DEFINE_SIZE_STATIC (16);
};

struct AxisValueRecord
{
  friend struct AxisValue;

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)));
  }

  protected:
  HBUINT16	axisIndex;	/* Zero-base index into the axis record array
				 * identifying the axis to which this value
				 * applies. Must be less than designAxisCount. */
  Fixed		value;		/* A numeric value for this attribute value. */
  public:
  DEFINE_SIZE_STATIC (6);
};

struct AxisValueFormat4
{
  friend struct AxisValue;

  const AxisValueRecord &get_axis_value (unsigned int i) const
  { return axisValuesZ.as_array (axisCount)[i]; }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)));
  }

  protected:
  HBUINT16	format;		/* Format identifier — set to 4. */
  HBUINT16	axisCount;	/* The total number of axes contributing to
				 * this axis-values combination. */
  HBUINT16	flags;		/* Flags — see hb_ot_style_flag_t */
  NameID	valueNameID;	/* The name ID for entries in the 'name' table
				 * that provide a display string for this
				 * attribute value. */
  UnsizedArrayOf<AxisValueRecord>
		axisValuesZ;	/* Array of AxisValue records that provide the
				 * combination of axis values, one for each
				 * contributing axis. */
  public:
  DEFINE_SIZE_ARRAY (8, axisValuesZ);
};

struct AxisValue
{
  unsigned int get_count () const
  {
    switch (u.format)
    {
    case 1:
    case 2:
    case 3: return 1;
    case 4: return u.format4.axisCount;
    default:return 0;
    }
  }

  unsigned int get_axis_index (unsigned int value_array_index) const
  {
    switch (u.format)
    {
    case 1: return u.format1.axisIndex;
    case 2: return u.format2.axisIndex;
    case 3: return u.format3.axisIndex;
    case 4: return u.format4.get_axis_value (value_array_index).axisIndex;
    default:return (unsigned int) -1;
    }
  }

  unsigned int get_flags () const
  {
    switch (u.format)
    {
    case 1: return u.format1.flags;
    case 2: return u.format2.flags;
    case 3: return u.format3.flags;
    case 4: return u.format4.flags;
    default:return 0;
    }
  }

  unsigned int get_value_name_id () const
  {
    switch (u.format)
    {
    case 1: return u.format1.valueNameID;
    case 2: return u.format2.valueNameID;
    case 3: return u.format3.valueNameID;
    case 4: return u.format4.valueNameID;
    default:return HB_OT_NAME_ID_INVALID;
    }
  }

  float get_value (unsigned int value_array_index) const
  {
    switch (u.format)
    {
    case 1: return u.format1.value.to_float ();
    case 2: return u.format2.nominalValue.to_float ();
    case 3: return u.format3.value.to_float ();
    case 4: return u.format4.get_axis_value (value_array_index).value.to_float ();
    default:return HB_OT_NAME_ID_INVALID;
    }
  }

  float get_min_value () const
  { return u.format == 2 ? u.format2.rangeMinValue.to_float () : NAN; }

  float get_max_value () const
  { return u.format == 2 ? u.format2.rangeMaxValue.to_float () : NAN; }

  float get_linked_value () const
  { return u.format == 3 ? u.format3.linkedValue.to_float () : NAN; }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (c->check_struct (this)))
      return_trace (false);

    switch (u.format)
    {
    case 1: return_trace (likely (u.format1.sanitize (c)));
    case 2: return_trace (likely (u.format2.sanitize (c)));
    case 3: return_trace (likely (u.format3.sanitize (c)));
    case 4: return_trace (likely (u.format4.sanitize (c)));
    default:return_trace (true);
    }
  }

  protected:
  union
  {
  HBUINT16		format;
  AxisValueFormat1	format1;
  AxisValueFormat2	format2;
  AxisValueFormat3	format3;
  AxisValueFormat4	format4;
  } u;
  public:
  DEFINE_SIZE_UNION (2, format);
};

struct StatAxisRecord
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)));
  }

  public:
  Tag		tag;		/* A tag identifying the axis of design variation. */
  NameID	nameID;		/* The name ID for entries in the 'name' table that
				 * provide a display string for this axis. */
  HBUINT16	ordering;	/* A value that applications can use to determine
				 * primary sorting of face names, or for ordering
				 * of descriptors when composing family or face names. */
  public:
  DEFINE_SIZE_STATIC (8);
};

struct STAT
{
  enum { tableTag = HB_OT_TAG_STAT };

  struct accelerator_t
  {
    void init (hb_face_t *face)
    {
      this->table = hb_sanitize_context_t().reference_table<STAT> (face);
      this->styles.init ();
      this->styles.alloc (table->axisValueCount + 5);

      /* is it OK to use hb_set_t for this? */
      hb_set_t *axes = hb_set_create ();

      for (unsigned int value_index = 0; value_index < table->axisValuesZ; value_index++)
      {
	const AxisValue &axis_value = this->table+(this->table+this->table->axisValuesZ).as_array (this->table->axisValueCount)[value_index];
	for (unsigned int value_array_index = 0; value_array_index < axis_value.get_count (); value_array_index++)
	{
	  unsigned int axis_index = axis_value.get_axis_index (value_array_index);
	  if (axis_index >= table->designAxisCount) continue;
	  const StatAxisRecord& axis_record = (this+table->designAxesZ).as_array (table->axisValueCount)[axis_index];

	  hb_ot_style_info_t &entry = *this->styles.push ();
	  entry.axis_index = axis_index;
	  entry.value_index = value_index;

	  hb_tag_t tag = axis_record.tag;
	  hb_set_add (axes, tag);
	  entry.tag = tag;
	  entry.axis_name_id = axis_record.nameID;
	  entry.axis_ordering = axis_record.ordering;

	  entry.flags = (hb_ot_style_flag_t) axis_value.get_flags ();
	  entry.value_name_id = axis_value.get_value_name_id ();
	  entry.value = axis_value.get_value (value_array_index);
	  entry.min_value = axis_value.get_min_value ();
	  entry.max_value = axis_value.get_max_value ();
	  entry.linked_value = axis_value.get_linked_value ();
	}
      }

#define SET_DEFAULT_VALUES_TO_INFO(entry) \
  entry.axis_index = (unsigned int) -1; \
  entry.value_index = (unsigned int) -1; \
  entry.axis_name_id = HB_OT_NAME_ID_INVALID; \
  entry.axis_ordering = (unsigned int) -1; \
  entry.value_name_id = HB_OT_NAME_ID_INVALID; \
  entry.flags = (hb_ot_style_flag_t) 0; \
  entry.min_value = NAN; \
  entry.max_value = NAN; \
  entry.linked_value = NAN

      {
	unsigned int start_offset = 0;
	while (true)
	{
	  unsigned int count = 4;
	  hb_ot_var_axis_info_t info[4];
	  hb_ot_var_get_axis_infos (face, start_offset, &count, info);
	  if (count == 0) break;
	  start_offset += count;
	  for (unsigned int i = 0; i < count; i++)
	  {
	    /* Already covered */
	    if (hb_set_has (axes, info[i].tag)) continue;

	    hb_ot_style_info_t &entry = *this->styles.push ();
	    SET_DEFAULT_VALUES_TO_INFO (entry);
	    /* entry.axis_index = info[i].axis_index;? */

	    hb_tag_t tag = info[i].tag;
	    hb_set_add (axes, tag);
	    entry.tag = tag;
	    entry.axis_name_id = info[i].name_id;

	    entry.value = info[i].default_value;
	    entry.linked_value = info[i].default_value;
	    entry.min_value = info[i].min_value;
	    entry.max_value = info[i].max_value;
	  }
	}
      }

      {
	const LArrayOf<AAT::FontDescriptor> &descriptors = face->table.fdsc->descriptors;
	for (unsigned int i = 0; i < descriptors.len; i++)
	{
	  const AAT::FontDescriptor &descriptor = descriptors[i];
	  // ban non-alphabetic as it is not of Fixed value
	  if (descriptor.tag == HB_TAG ('n','a','l','f')) continue;
	  /* Already covered */
	  if (hb_set_has (axes, descriptor.tag)) continue;

	  hb_ot_style_info_t &entry = *this->styles.push ();
	  SET_DEFAULT_VALUES_TO_INFO (entry);

	  hb_tag_t tag = descriptor.tag;
	  hb_set_add (axes, tag);
	  entry.tag = tag;
	  float value = descriptor.get_value ();
	  if (tag == HB_OT_TAG_VAR_AXIS_WIDTH) value *= 100.f;
	  if (tag == HB_OT_TAG_VAR_AXIS_WEIGHT) value *= 400.f;
	  entry.value = tag;
	}
      }

      {
	if (!hb_set_has (axes, HB_OT_TAG_VAR_AXIS_ITALIC))
	{
	  hb_ot_style_info_t &entry = *this->styles.push ();
	  SET_DEFAULT_VALUES_TO_INFO (entry);
	  entry.tag = HB_OT_TAG_VAR_AXIS_ITALIC;
	  entry.value = face->table.OS2->is_italic () || face->table.head->is_italic () ? 1.f : 0.f;
	}

	if (!hb_set_has (axes, HB_OT_TAG_VAR_AXIS_OPTICAL_SIZE))
	{
	  hb_ot_style_info_t &entry = *this->styles.push ();
	  SET_DEFAULT_VALUES_TO_INFO (entry);
	  entry.tag = HB_OT_TAG_VAR_AXIS_OPTICAL_SIZE;

	  const OS2V5Tail &os2v5 = face->table.OS2->v5 ();
	  if (os2v5.has_optical_size ())
	  {
	    entry.min_value = os2v5.usLowerOpticalPointSize;
	    entry.max_value = os2v5.usUpperOpticalPointSize;
	    entry.value = (entry.min_value + entry.max_value) / 2;
	  }
	  else entry.value = 12.f;
	}

	if (!hb_set_has (axes, HB_OT_TAG_VAR_AXIS_SLANT))
	{
	  hb_ot_style_info_t &entry = *this->styles.push ();
	  SET_DEFAULT_VALUES_TO_INFO (entry);
	  entry.tag = HB_OT_TAG_VAR_AXIS_SLANT;

	  /* XXX Is 6.0 correct? Apple Chancery uses it for its `slnt` */
	  entry.value = face->table.OS2->is_oblique () ? 6.f : 0.f;
	}

	if (!hb_set_has (axes, HB_OT_TAG_VAR_AXIS_WIDTH))
	{
	  hb_ot_style_info_t &entry = *this->styles.push ();
	  SET_DEFAULT_VALUES_TO_INFO (entry);
	  entry.tag = HB_OT_TAG_VAR_AXIS_WIDTH;
	  entry.value = face->table.OS2->has_data () ? face->table.OS2->get_width () :
			(face->table.head->is_condensed () ? 75.f : 100.f);
	}

	if (!hb_set_has (axes, HB_OT_TAG_VAR_AXIS_WIDTH))
	{
	  hb_ot_style_info_t &entry = *this->styles.push ();
	  SET_DEFAULT_VALUES_TO_INFO (entry);
	  entry.tag = HB_OT_TAG_VAR_AXIS_WIDTH;
	  entry.value = face->table.OS2->has_data () ? face->table.OS2->usWidthClass :
			(face->table.head->is_bold () ? 700.f : 400.f);
	}
      }

      hb_set_destroy (axes);

      /* Maybe sort it now to make it bsearch-able? */
#undef SET_DEFAULT_VALUES_TO_INFO
    }

    void fini ()
    {
      this->styles.fini ();
      this->table.destroy ();
    }

    hb_blob_ptr_t<STAT> table;
    hb_vector_t<hb_ot_style_info_t> styles;
  };

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  majorVersion == 1 &&
			  minorVersion > 0 &&
			  designAxesZ.sanitize (c, this, designAxisCount) &&
			  axisValuesZ.sanitize (c, this, axisValueCount, &(this+axisValuesZ))));
  }

  protected:
  HBUINT16	majorVersion;	/* Major version number of the style attributes
				 * table — set to 1. */
  HBUINT16	minorVersion;	/* Minor version number of the style attributes
				 * table — set to 2. */
  /* XXX we should consider this also */
  HBUINT16	designAxisSize;	/* The size in bytes of each axis record. */
  HBUINT16	designAxisCount;/* The number of design axis records. In a
				 * font with an 'fvar' table, this value must be
				 * greater than or equal to the axisCount value
				 * in the 'fvar' table. In all fonts, must
				 * be greater than zero if axisValueCount
				 * is greater than zero. */
  LOffsetTo<UnsizedArrayOf<StatAxisRecord>, false>
		designAxesZ;	/* Offset in bytes from the beginning of
				 * the STAT table to the start of the design
				 * axes array. If designAxisCount is zero,
				 * set to zero; if designAxisCount is greater
				 * than zero, must be greater than zero. */
  HBUINT16	axisValueCount;	/* The number of axis value tables. */
  LOffsetTo<UnsizedArrayOf<OffsetTo<AxisValue> >, false>
		axisValuesZ;	/* Offset in bytes from the beginning of
				 * the STAT table to the start of the design
				 * axes value offsets array. If axisValueCount
				 * is zero, set to zero; if axisValueCount is
				 * greater than zero, must be greater than zero. */
  NameID	elidedFallbackNameID;
				/* Name ID used as fallback when projection of
				 * names into a particular font model produces
				 * a subfamily name containing only elidable
				 * elements. */
  public:
  DEFINE_SIZE_STATIC (20);
};

struct STAT_accelerator_t : STAT::accelerator_t {};

} /* namespace OT */


#endif /* HB_OT_STAT_TABLE_HH */
