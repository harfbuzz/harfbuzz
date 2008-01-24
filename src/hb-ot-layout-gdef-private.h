/*
 * Copyright (C) 2007,2008  Red Hat, Inc.
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

#ifndef HB_OT_LAYOUT_GDEF_PRIVATE_H
#define HB_OT_LAYOUT_GDEF_PRIVATE_H

#include "hb-ot-layout-private.h"

#include "hb-ot-layout-open-private.h"


#define DEFINE_INDIRECT_GLYPH_ARRAY_LOOKUP(Type, name) \
  inline const Type& name (hb_ot_layout_glyph_t glyph_id) { \
    const Coverage &c = get_coverage (); \
    hb_ot_layout_coverage_t c_index = c.get_coverage (glyph_id); \
    return (*this)[c_index]; \
  }


struct GlyphClassDef : ClassDef {
  static const uint16_t BaseGlyph		= 0x0001u;
  static const uint16_t LigatureGlyph		= 0x0002u;
  static const uint16_t MarkGlyph		= 0x0003u;
  static const uint16_t ComponentGlyph		= 0x0004u;
};

/*
 * Attachment List Table
 */

struct AttachPoint {

  friend struct AttachList;

  private:
  /* countour point indices, in increasing numerical order */
  DEFINE_ARRAY_TYPE (USHORT, pointIndex, pointCount);

  private:
  USHORT	pointCount;		/* Number of attachment points on
					 * this glyph */
  USHORT	pointIndex[];		/* Array of contour point indices--in
					 * increasing numerical order */
};
DEFINE_NULL_ASSERT_SIZE (AttachPoint, 2);

struct AttachList {

  friend struct GDEF;

  private:
  /* const AttachPoint& get_attach_points (hb_ot_layout_glyph_t glyph_id); */
  DEFINE_INDIRECT_GLYPH_ARRAY_LOOKUP (AttachPoint, get_attach_points);

  private:
  /* AttachPoint tables, in Coverage Index order */
  DEFINE_OFFSET_ARRAY_TYPE (AttachPoint, attachPoint, glyphCount);
  DEFINE_ACCESSOR (Coverage, get_coverage, coverage);

 private:
  Offset	coverage;		/* Offset to Coverage table -- from
					 * beginning of AttachList table */
  USHORT	glyphCount;		/* Number of glyphs with attachment
					 * points */
  Offset	attachPoint[];		/* Array of offsets to AttachPoint
					 * tables--from beginning of AttachList
					 * table--in Coverage Index order */
};
DEFINE_NULL_ASSERT_SIZE (AttachList, 4);

/*
 * Ligature Caret Table
 */

struct CaretValueFormat1 {

  friend struct CaretValue;

  private:
  inline int get_caret_value (int ppem) const {
    return /* TODO garbage */ coordinate / ppem;
  }

  private:
  USHORT	caretValueFormat;	/* Format identifier--format = 1 */
  SHORT		coordinate;		/* X or Y value, in design units */
};
ASSERT_SIZE (CaretValueFormat1, 4);

struct CaretValueFormat2 {

  friend struct CaretValue;

  private:
  inline int get_caret_value (int ppem) const {
    return /* TODO garbage */ 0 / ppem;
  }

  private:
  USHORT	caretValueFormat;	/* Format identifier--format = 2 */
  USHORT	caretValuePoint;	/* Contour point index on glyph */
};
ASSERT_SIZE (CaretValueFormat2, 4);

struct CaretValueFormat3 {

  friend struct CaretValue;

  private:
  inline const Device& get_device (void) const {
    if (HB_UNLIKELY (!deviceTable)) return NullDevice;
    return *(const Device*)((const char*)this + deviceTable);
  }

  inline int get_caret_value (int ppem) const {
    return /* TODO garbage */ (coordinate + get_device().get_delta (ppem)) / ppem;
  }

  private:
  USHORT	caretValueFormat;	/* Format identifier--format = 3 */
  SHORT		coordinate;		/* X or Y value, in design units */
  Offset	deviceTable;		/* Offset to Device table for X or Y
					 * value--from beginning of CaretValue
					 * table */
};
ASSERT_SIZE (CaretValueFormat3, 6);

struct CaretValue {
  DEFINE_NON_INSTANTIABLE(CaretValue);

  unsigned int get_size (void) const {
    switch (u.caretValueFormat) {
    case 1: return sizeof (u.format1);
    case 2: return sizeof (u.format2);
    case 3: return sizeof (u.format3);
    default:return sizeof (u.caretValueFormat);
    }
  }

  /* XXX  we need access to a load-contour-point vfunc here */
  int get_caret_value (int ppem) const {
    switch (u.caretValueFormat) {
    case 1: return u.format1.get_caret_value(ppem);
    case 2: return u.format2.get_caret_value(ppem);
    case 3: return u.format3.get_caret_value(ppem);
    default:return 0;
    }
  }

  private:
  union {
  USHORT	caretValueFormat;	/* Format identifier */
  CaretValueFormat1	format1;
  CaretValueFormat2	format2;
  CaretValueFormat3	format3;
  /* FIXME old HarfBuzz code has a format 4 here! */
  } u;
};
DEFINE_NULL (CaretValue, 2);

struct LigGlyph {

  friend struct LigCaretList;

  private:
  /* Caret value tables, in increasing coordinate order */
  DEFINE_OFFSET_ARRAY_TYPE (CaretValue, caretValue, caretCount);
  /* TODO */

  private:
  USHORT	caretCount;		/* Number of CaretValues for this
					 * ligature (components - 1) */
  Offset	caretValue[];		/* Array of offsets to CaretValue
					 * tables--from beginning of LigGlyph
					 * table--in increasing coordinate
					 * order */
};
DEFINE_NULL_ASSERT_SIZE (LigGlyph, 2);

struct LigCaretList {

  friend struct GDEF;

  private:
  /* const LigGlyph& get_lig_glyph (hb_ot_layout_glyph_t glyph_id); */
  DEFINE_INDIRECT_GLYPH_ARRAY_LOOKUP (LigGlyph, get_lig_glyph);

  private:
  /* AttachPoint tables, in Coverage Index order */
  DEFINE_OFFSET_ARRAY_TYPE (LigGlyph, ligGlyph, ligGlyphCount);
  DEFINE_ACCESSOR (Coverage, get_coverage, coverage);

  private:
  Offset	coverage;		/* Offset to Coverage table--from
					 * beginning of LigCaretList table */
  USHORT	ligGlyphCount;		/* Number of ligature glyphs */
  Offset	ligGlyph[];		/* Array of offsets to LigGlyph
					 * tables--from beginning of
					 * LigCaretList table--in Coverage
					 * Index order */
};
DEFINE_NULL_ASSERT_SIZE (LigCaretList, 4);

/*
 * GDEF
 */

struct GDEF {
  static const hb_tag_t Tag		= HB_TAG ('G','D','E','F');

  static const hb_ot_layout_class_t UnclassifiedGlyph	= 0;
  static const hb_ot_layout_class_t BaseGlyph		= 1;
  static const hb_ot_layout_class_t LigatureGlyph	= 2;
  static const hb_ot_layout_class_t MarkGlyph		= 3;
  static const hb_ot_layout_class_t ComponentGlyph	= 4;

  STATIC_DEFINE_GET_FOR_DATA (GDEF);
  /* XXX check version here? */

  DEFINE_ACCESSOR (ClassDef, get_glyph_class_def, glyphClassDef);
  DEFINE_ACCESSOR (AttachList, get_attach_list, attachList);
  DEFINE_ACCESSOR (LigCaretList, get_lig_caret_list, ligCaretList);
  DEFINE_ACCESSOR (ClassDef, get_mark_attach_class_def, markAttachClassDef);

  /* Returns 0 if not found. */
  inline hb_ot_layout_class_t get_glyph_class (hb_ot_layout_glyph_t glyph_id) const {
    return get_glyph_class_def ().get_class (glyph_id);
  }

  /* Returns 0 if not found. */
  inline hb_ot_layout_class_t get_mark_attachment_type (hb_ot_layout_glyph_t glyph_id) const {
    return get_mark_attach_class_def ().get_class (glyph_id);
  }

  /* TODO get_attach and get_lig_caret */

  private:
  Fixed		version;		/* Version of the GDEF table--initially
					 * 0x00010000 */
  Offset	glyphClassDef;		/* Offset to class definition table
					 * for glyph type--from beginning of
					 * GDEF header (may be Null) */
  Offset	attachList;		/* Offset to list of glyphs with
					 * attachment points--from beginning
					 * of GDEF header (may be Null) */
  Offset	ligCaretList;		/* Offset to list of positioning points
					 * for ligature carets--from beginning
					 * of GDEF header (may be Null) */
  Offset	markAttachClassDef;	/* Offset to class definition table for
					 * mark attachment type--from beginning
					 * of GDEF header (may be Null) */
};
DEFINE_NULL_ASSERT_SIZE (GDEF, 12);

#endif /* HB_OT_LAYOUT_GDEF_PRIVATE_H */
