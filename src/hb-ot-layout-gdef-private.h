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

#ifndef HB_OT_LAYOUT_GDEF_PRIVATE_H
#define HB_OT_LAYOUT_GDEF_PRIVATE_H

#include "hb-ot-layout-common-private.h"


#define DEFINE_INDIRECT_GLYPH_ARRAY_LOOKUP(Type, array, name) \
  inline const Type& name (hb_codepoint_t glyph) { return this+array[(this+coverage)(glyph)]; }


struct GlyphClassDef : ClassDef
{
  enum {
    BaseGlyph		= 0x0001u,
    LigatureGlyph	= 0x0002u,
    MarkGlyph		= 0x0003u,
    ComponentGlyph	= 0x0004u,
  };
};

/*
 * Attachment List Table
 */

typedef ArrayOf<USHORT> AttachPoint;	/* Array of contour point indices--in
					 * increasing numerical order */
ASSERT_SIZE (AttachPoint, 2);

struct AttachList
{
  /* XXX We need enumeration API here */
  /* const AttachPoint& get_attach_points (hb_codepoint_t glyph); */
  DEFINE_INDIRECT_GLYPH_ARRAY_LOOKUP (AttachPoint, attachPoint, get_attach_points);

 private:
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table -- from
					 * beginning of AttachList table */
  OffsetArrayOf<AttachPoint>
		attachPoint;		/* Array of AttachPoint tables
					 * in Coverage Index order */
};
ASSERT_SIZE (AttachList, 4);

/*
 * Ligature Caret Table
 */

struct CaretValueFormat1
{
  friend struct CaretValue;

  private:
  inline int get_caret_value (hb_ot_layout_t *layout, hb_codepoint_t glyph_id) const
  {
    /* XXX vertical */
    return layout->gpos_info.x_scale * coordinate / 0x10000;
  }

  private:
  USHORT	caretValueFormat;	/* Format identifier--format = 1 */
  SHORT		coordinate;		/* X or Y value, in design units */
};
ASSERT_SIZE (CaretValueFormat1, 4);

struct CaretValueFormat2
{
  friend struct CaretValue;

  private:
  inline int get_caret_value (hb_ot_layout_t *layout, hb_codepoint_t glyph_id) const
  {
    return /* TODO contour point */ 0;
  }

  private:
  USHORT	caretValueFormat;	/* Format identifier--format = 2 */
  USHORT	caretValuePoint;	/* Contour point index on glyph */
};
ASSERT_SIZE (CaretValueFormat2, 4);

struct CaretValueFormat3
{
  friend struct CaretValue;

  inline int get_caret_value (hb_ot_layout_t *layout, hb_codepoint_t glyph_id) const
  {
    /* XXX vertical */
    return layout->gpos_info.x_scale * coordinate / 0x10000 +
	   (this+deviceTable).get_delta (layout->gpos_info.x_ppem) << 6;
  }

  private:
  USHORT	caretValueFormat;	/* Format identifier--format = 3 */
  SHORT		coordinate;		/* X or Y value, in design units */
  OffsetTo<Device>
		deviceTable;		/* Offset to Device table for X or Y
					 * value--from beginning of CaretValue
					 * table */
};
ASSERT_SIZE (CaretValueFormat3, 6);

struct CaretValue
{
  /* XXX  we need access to a load-contour-point vfunc here */
  int get_caret_value (hb_ot_layout_t *layout, hb_codepoint_t glyph_id) const
  {
    switch (u.format) {
    case 1: return u.format1->get_caret_value (layout, glyph_id);
    case 2: return u.format2->get_caret_value (layout, glyph_id);
    case 3: return u.format3->get_caret_value (layout, glyph_id);
    default:return 0;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  CaretValueFormat1	format1[];
  CaretValueFormat2	format2[];
  CaretValueFormat3	format3[];
  } u;
};
ASSERT_SIZE (CaretValue, 2);

struct LigGlyph
{
  friend struct LigCaretList;

  private:
  OffsetArrayOf<CaretValue>
		caret;			/* Offset rrray of CaretValue tables
					 * --from beginning of LigGlyph table
					 * --in increasing coordinate order */
};
ASSERT_SIZE (LigGlyph, 2);

struct LigCaretList
{
  friend struct GDEF;

  private:
  /* const LigGlyph& get_lig_glyph (hb_codepoint_t glyph); */
  DEFINE_INDIRECT_GLYPH_ARRAY_LOOKUP (LigGlyph, ligGlyph, get_lig_glyph);

  private:
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of LigCaretList table */
  OffsetArrayOf<LigGlyph>
		ligGlyph;		/* Array of LigGlyph tables
					 * in Coverage Index order */
};
ASSERT_SIZE (LigCaretList, 4);

/*
 * GDEF
 */

struct GDEF
{
  static const hb_tag_t Tag		= HB_TAG ('G','D','E','F');

  enum {
    UnclassifiedGlyph	= 0,
    BaseGlyph		= 1,
    LigatureGlyph	= 2,
    MarkGlyph		= 3,
    ComponentGlyph	= 4,
  };

  STATIC_DEFINE_GET_FOR_DATA (GDEF);
  /* XXX check version here? */

  inline bool has_glyph_classes () const { return glyphClassDef != 0; }
  inline hb_ot_layout_class_t get_glyph_class (hb_codepoint_t glyph) const { return glyphClassDef(this).get_class (glyph); }

  inline bool has_mark_attachment_types () const { return markAttachClassDef != 0; }
  inline hb_ot_layout_class_t get_mark_attachment_type (hb_codepoint_t glyph) const { return markAttachClassDef(this).get_class (glyph); }

  /* TODO get_attach and get_lig_caret */
  inline bool has_attach_list () const { return attachList != 0; }
  inline bool has_lig_caret_list () const { return ligCaretList != 0; }

  private:
  Fixed		version;		/* Version of the GDEF table--initially
					 * 0x00010000 */
  OffsetTo<ClassDef>
		glyphClassDef;		/* Offset to class definition table
					 * for glyph type--from beginning of
					 * GDEF header (may be Null) */
  OffsetTo<AttachList>
		attachList;		/* Offset to list of glyphs with
					 * attachment points--from beginning
					 * of GDEF header (may be Null) */
  OffsetTo<LigCaretList>
		ligCaretList;		/* Offset to list of positioning points
					 * for ligature carets--from beginning
					 * of GDEF header (may be Null) */
  OffsetTo<ClassDef>
		markAttachClassDef;	/* Offset to class definition table for
					 * mark attachment type--from beginning
					 * of GDEF header (may be Null) */
};
ASSERT_SIZE (GDEF, 12);


#endif /* HB_OT_LAYOUT_GDEF_PRIVATE_H */
