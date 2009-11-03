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

#ifndef HB_OT_LAYOUT_GDEF_PRIVATE_HH
#define HB_OT_LAYOUT_GDEF_PRIVATE_HH

#include "hb-ot-layout-common-private.hh"

#include "hb-font-private.h"


/*
 * Attachment List Table
 */

typedef ArrayOf<USHORT> AttachPoint;	/* Array of contour point indices--in
					 * increasing numerical order */
ASSERT_SIZE (AttachPoint, 2);

struct AttachList
{
  inline bool get_attach_points (hb_codepoint_t glyph_id,
				 unsigned int *point_count /* IN/OUT */,
				 unsigned int *point_array /* OUT */) const
  {
    unsigned int index = (this+coverage) (glyph_id);
    if (index == NOT_COVERED)
    {
      *point_count = 0;
      return false;
    }
    const AttachPoint &points = this+attachPoint[index];

    unsigned int count = MIN (points.len, *point_count);
    for (unsigned int i = 0; i < count; i++)
      point_array[i] = points[i];

    *point_count = points.len;

    return true;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_THIS2 (coverage, attachPoint);
  }

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
  inline int get_caret_value (hb_ot_layout_context_t *context, hb_codepoint_t glyph_id) const
  {
    /* TODO vertical */
    return context->font->x_scale * coordinate / 0x10000;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF ();
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
  inline int get_caret_value (hb_ot_layout_context_t *context, hb_codepoint_t glyph_id) const
  {
    return /* TODO contour point */ 0;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF ();
  }

  private:
  USHORT	caretValueFormat;	/* Format identifier--format = 2 */
  USHORT	caretValuePoint;	/* Contour point index on glyph */
};
ASSERT_SIZE (CaretValueFormat2, 4);

struct CaretValueFormat3
{
  friend struct CaretValue;

  inline int get_caret_value (hb_ot_layout_context_t *context, hb_codepoint_t glyph_id) const
  {
    /* TODO vertical */
    return context->font->x_scale * coordinate / 0x10000 +
	   ((this+deviceTable).get_delta (context->font->x_ppem) << 6);
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF () && SANITIZE_THIS (deviceTable);
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
  inline int get_caret_value (hb_ot_layout_context_t *context, hb_codepoint_t glyph_id) const
  {
    switch (u.format) {
    case 1: return u.format1->get_caret_value (context, glyph_id);
    case 2: return u.format2->get_caret_value (context, glyph_id);
    case 3: return u.format3->get_caret_value (context, glyph_id);
    default:return 0;
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
  CaretValueFormat1	format1[VAR];
  CaretValueFormat2	format2[VAR];
  CaretValueFormat3	format3[VAR];
  } u;
};

struct LigGlyph
{
  inline void get_lig_carets (hb_ot_layout_context_t *context,
			      hb_codepoint_t glyph_id,
			      unsigned int *caret_count /* IN/OUT */,
			      int *caret_array /* OUT */) const
  {

    unsigned int count = MIN (carets.len, *caret_count);
    for (unsigned int i = 0; i < count; i++)
      caret_array[i] = (this+carets[i]).get_caret_value (context, glyph_id);

    *caret_count = carets.len;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_THIS (carets);
  }

  private:
  OffsetArrayOf<CaretValue>
		carets;			/* Offset array of CaretValue tables
					 * --from beginning of LigGlyph table
					 * --in increasing coordinate order */
};
ASSERT_SIZE (LigGlyph, 2);

struct LigCaretList
{
  inline bool get_lig_carets (hb_ot_layout_context_t *context,
			      hb_codepoint_t glyph_id,
			      unsigned int *caret_count /* IN/OUT */,
			      int *caret_array /* OUT */) const
  {
    unsigned int index = (this+coverage) (glyph_id);
    if (index == NOT_COVERED)
    {
      *caret_count = 0;
      return false;
    }
    const LigGlyph &lig_glyph = this+ligGlyph[index];
    lig_glyph.get_lig_carets (context, glyph_id, caret_count, caret_array);
    return true;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_THIS2 (coverage, ligGlyph);
  }

  private:
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of LigCaretList table */
  OffsetArrayOf<LigGlyph>
		ligGlyph;		/* Array of LigGlyph tables
					 * in Coverage Index order */
};
ASSERT_SIZE (LigCaretList, 4);


struct MarkGlyphSetsFormat1
{
  inline bool covers (unsigned int set_index, hb_codepoint_t glyph_id) const
  { return (this+coverage[set_index]).get_coverage (glyph_id) != NOT_COVERED; }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_THIS (coverage);
  }

  private:
  USHORT	format;			/* Format identifier--format = 1 */
  LongOffsetArrayOf<Coverage>
		coverage;		/* Array of long offsets to mark set
					 * coverage tables */
};
ASSERT_SIZE (MarkGlyphSetsFormat1, 4);

struct MarkGlyphSets
{
  inline bool covers (unsigned int set_index, hb_codepoint_t glyph_id) const
  {
    switch (u.format) {
    case 1: return u.format1->covers (set_index, glyph_id);
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
  MarkGlyphSetsFormat1	format1[VAR];
  } u;
};


/*
 * GDEF
 */

struct GDEF
{
  static const hb_tag_t Tag	= HB_OT_TAG_GDEF;

  enum {
    UnclassifiedGlyph	= 0,
    BaseGlyph		= 1,
    LigatureGlyph	= 2,
    MarkGlyph		= 3,
    ComponentGlyph	= 4
  };

  STATIC_DEFINE_GET_FOR_DATA_CHECK_MAJOR_VERSION (GDEF, 1, 1);

  inline bool has_glyph_classes () const { return glyphClassDef != 0; }
  inline hb_ot_layout_class_t get_glyph_class (hb_codepoint_t glyph) const
  { return (this+glyphClassDef).get_class (glyph); }

  inline bool has_mark_attachment_types () const { return markAttachClassDef != 0; }
  inline hb_ot_layout_class_t get_mark_attachment_type (hb_codepoint_t glyph) const
  { return (this+markAttachClassDef).get_class (glyph); }

  inline bool has_attach_points () const { return attachList != 0; }
  inline bool get_attach_points (hb_codepoint_t glyph_id,
				 unsigned int *point_count /* IN/OUT */,
				 unsigned int *point_array /* OUT */) const
  { return (this+attachList).get_attach_points (glyph_id, point_count, point_array); }

  inline bool has_lig_carets () const { return ligCaretList != 0; }
  inline bool get_lig_carets (hb_ot_layout_context_t *context,
			      hb_codepoint_t glyph_id,
			      unsigned int *caret_count /* IN/OUT */,
			      int *caret_array /* OUT */) const
  { return (this+ligCaretList).get_lig_carets (context, glyph_id, caret_count, caret_array); }

  inline bool has_mark_sets () const { return version >= 0x00010002 && markGlyphSetsDef[0] != 0; }
  inline bool mark_set_covers (unsigned int set_index, hb_codepoint_t glyph_id) const
  { return version >= 0x00010002 && (this+markGlyphSetsDef[0]).covers (set_index, glyph_id); }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!SANITIZE (version)) return false;
    if (version.major != 1) return true;
    return SANITIZE_THIS2 (glyphClassDef, attachList) &&
	   SANITIZE_THIS2 (ligCaretList, markAttachClassDef) &&
	   (version < 0x00010002 || SANITIZE_THIS (markGlyphSetsDef[0]));
  }

  private:
  FixedVersion	version;		/* Version of the GDEF table--currently
					 * 0x00010002 */
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
  OffsetTo<MarkGlyphSets>
		markGlyphSetsDef[VAR];	/* Offset to the table of mark set
					 * definitions--from beginning of GDEF
					 * header (may be NULL).  Introduced
					 * in version 00010002. */
};
ASSERT_SIZE_VAR (GDEF, 12, OffsetTo<MarkGlyphSets>);


#endif /* HB_OT_LAYOUT_GDEF_PRIVATE_HH */
