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

#include "hb-open-type.hh"

/*
 * fvar -- Font Variations
 * https://docs.microsoft.com/en-us/typography/opentype/spec/fvar
 */

#define HB_OT_TAG_fvar HB_TAG('f','v','a','r')


namespace OT {


struct InstanceRecord
{
  inline bool sanitize (hb_sanitize_context_t *c, unsigned int axis_count) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  c->check_array (coordinatesZ.arrayZ, axis_count));
  }

  protected:
  NameID	subfamilyNameID;/* The name ID for entries in the 'name' table
				 * that provide subfamily names for this instance. */
  HBUINT16	flags;		/* Reserved for future use — set to 0. */
  UnsizedArrayOf<Fixed>
		coordinatesZ;	/* The coordinates array for this instance. */
  //NameID	postScriptNameIDX;/*Optional. The name ID for entries in the 'name'
  //				  * table that provide PostScript names for this
  //				  * instance. */

  public:
  DEFINE_SIZE_ARRAY (4, coordinatesZ);
};

struct AxisRecord
{
  enum
  {
    AXIS_FLAG_HIDDEN	= 0x0001,
  };

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  public:
  Tag		axisTag;	/* Tag identifying the design variation for the axis. */
  Fixed		minValue;	/* The minimum coordinate value for the axis. */
  Fixed		defaultValue;	/* The default coordinate value for the axis. */
  Fixed		maxValue;	/* The maximum coordinate value for the axis. */
  HBUINT16	flags;		/* Axis flags. */
  NameID	axisNameID;	/* The name ID for entries in the 'name' table that
				 * provide a display name for this axis. */

  public:
  DEFINE_SIZE_STATIC (20);
};

struct fvar
{
  static const hb_tag_t tableTag	= HB_OT_TAG_fvar;

  inline bool has_data (void) const { return version.to_int (); }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (version.sanitize (c) &&
		  likely (version.major == 1) &&
		  c->check_struct (this) &&
		  axisSize == 20 && /* Assumed in our code. */
		  instanceSize >= axisCount * 4 + 4 &&
		  get_axes ().sanitize (c) &&
		  c->check_range (get_first_instance (), instanceCount, instanceSize));
  }

  inline unsigned int get_axis_count (void) const
  { return axisCount; }

  inline bool get_axis (unsigned int index, hb_ot_var_axis_t *info) const
  {
    if (info)
    {
      const AxisRecord &axis = get_axes ()[index];
      info->tag = axis.axisTag;
      info->name_id =  axis.axisNameID;
      info->default_value = axis.defaultValue / 65536.;
      /* Ensure order, to simplify client math. */
      info->min_value = MIN<float> (info->default_value, axis.minValue / 65536.);
      info->max_value = MAX<float> (info->default_value, axis.maxValue / 65536.);
    }

    return true;
  }

  inline hb_ot_var_axis_flags_t get_axis_flags (unsigned int index) const
  {
    const AxisRecord &axis = get_axes ()[index];
    return (hb_ot_var_axis_flags_t) (unsigned int) axis.flags;
  }

  inline unsigned int get_axis_infos (unsigned int      start_offset,
				      unsigned int     *axes_count /* IN/OUT */,
				      hb_ot_var_axis_t *axes_array /* OUT */) const
  {
    if (axes_count)
    {
      unsigned int count = axisCount;
      start_offset = MIN (start_offset, count);

      count -= start_offset;
      axes_array += start_offset;

      count = MIN (count, *axes_count);
      *axes_count = count;

      for (unsigned int i = 0; i < count; i++)
	get_axis (start_offset + i, axes_array + i);
    }
    return axisCount;
  }

  inline bool find_axis (hb_tag_t tag, unsigned int *index, hb_ot_var_axis_t *info) const
  {
    const AxisRecord *axes = get_axes ();
    unsigned int count = get_axis_count ();
    for (unsigned int i = 0; i < count; i++)
      if (axes[i].axisTag == tag)
      {
        if (index)
	  *index = i;
	return get_axis (i, info);
      }
    if (index)
      *index = HB_OT_VAR_NO_AXIS_INDEX;
    return false;
  }

  inline int normalize_axis_value (unsigned int axis_index, float v) const
  {
    hb_ot_var_axis_t axis;
    if (!get_axis (axis_index, &axis))
      return 0;

    v = MAX (MIN (v, axis.max_value), axis.min_value); /* Clamp. */

    if (v == axis.default_value)
      return 0;
    else if (v < axis.default_value)
      v = (v - axis.default_value) / (axis.default_value - axis.min_value);
    else
      v = (v - axis.default_value) / (axis.max_value - axis.default_value);
    return (int) (v * 16384.f + (v >= 0.f ? .5f : -.5f));
  }

  protected:
  inline hb_array_t<const AxisRecord> get_axes (void) const
  { return hb_array (&(this+firstAxis), axisCount); }

  inline const InstanceRecord * get_first_instance (void) const
  { return &StructAfter<InstanceRecord> (get_axes ()); }

  protected:
  FixedVersion<>version;	/* Version of the fvar table
				 * initially set to 0x00010000u */
  OffsetTo<AxisRecord>
		firstAxis;	/* Offset in bytes from the beginning of the table
				 * to the start of the AxisRecord array. */
  HBUINT16	reserved;	/* This field is permanently reserved. Set to 2. */
  HBUINT16	axisCount;	/* The number of variation axes in the font (the
				 * number of records in the axes array). */
  HBUINT16	axisSize;	/* The size in bytes of each VariationAxisRecord —
				 * set to 20 (0x0014) for this version. */
  HBUINT16	instanceCount;	/* The number of named instances defined in the font
				 * (the number of records in the instances array). */
  HBUINT16	instanceSize;	/* The size in bytes of each InstanceRecord — set
				 * to either axisCount * sizeof(Fixed) + 4, or to
				 * axisCount * sizeof(Fixed) + 6. */

  public:
  DEFINE_SIZE_STATIC (16);
};

} /* namespace OT */


#endif /* HB_OT_VAR_FVAR_TABLE_HH */
