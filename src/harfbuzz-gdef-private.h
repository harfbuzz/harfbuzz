#ifndef HARFBUZZ_GDEF_PRIVATE_H
#define HARFBUZZ_GDEF_PRIVATE_H

#include "harfbuzz-open-private.h"

struct GlyphClassDef : ClassDef {
  static const uint16_t BaseGlyph		= 0x0001u;
  static const uint16_t LigatureGlyph		= 0x0002u;
  static const uint16_t MarkGlyph		= 0x0003u;
  static const uint16_t ComponentGlyph		= 0x0004u;
};

struct AttachPoint;

struct AttachList {
  /* AttachPoint tables, in Coverage Index order */
  /* TODO get attach lists */
/*  DEFINE_INDIRECT_OFFSET_ARRAY_TYPE (AttachPoint, attachPoint, glyphCount, get_coverage);

//  get_coverage

  inline Coverage* get_default_language_system (void) {
    if (!defaultLangSys)
      return NULL;
    return (LangSys *)((char*)this + defaultLangSys);
  }

*/

  private:
  Offset	coverage;		/* Offset to Coverage table -- from
					 * beginning of AttachList table */
  USHORT	glyphCount;		/* Number of glyphs with attachment
					 * points */
  Offset	attachPoint[];		/* Array of offsets to AttachPoint
					 * tables--from beginning of AttachList
					 * table--in Coverage Index order */
};
ASSERT_SIZE (AttachList, 4);

struct AttachPoint {
  /* TODO */

  private:
  USHORT	pointCount;		/* Number of attachment points on
					 * this glyph */
  USHORT	pointIndex[];		/* Array of contour point indices--in
					 * increasing numerical order */
};
ASSERT_SIZE (AttachPoint, 2);

/*
 * Ligature Caret Table
 */

struct CaretValue;

struct LigCaretList {
  /* TODO */

  private:
  Offset	coverage;		/* Offset to Coverage table--from
					 * beginning of LigCaretList table */
  USHORT	ligGlyphCount;		/* Number of ligature glyphs */
  Offset	ligGlyph[];		/* Array of offsets to LigGlyph
					 * tables--from beginning of
					 * LigCaretList table--in Coverage
					 * Index order */
};
ASSERT_SIZE (LigCaretList, 4);

struct LigGlyph {
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
ASSERT_SIZE (LigGlyph, 2);

struct CaretValueFormat1 {

  inline int get_caret_value (int ppem) const {
    return /* TODO garbage */ coordinate / ppem;
  }

  private:
  USHORT	caretValueFormat;	/* Format identifier--format = 1 */
  SHORT		coordinate;		/* X or Y value, in design units */
};
ASSERT_SIZE (CaretValueFormat1, 4);

struct CaretValueFormat2 {

  inline int get_caret_value (int ppem) const {
    return /* TODO garbage */ 0 / ppem;
  }

  private:
  USHORT	caretValueFormat;	/* Format identifier--format = 2 */
  USHORT	caretValuePoint;	/* Contour point index on glyph */
};
ASSERT_SIZE (CaretValueFormat2, 4);

struct CaretValueFormat3 {

  inline const Device* get_device (void) const {
    return (const Device*)((const char*)this + deviceTable);
  }

  inline int get_caret_value (int ppem) const {
    return /* TODO garbage */ (coordinate + get_device()->get_delta (ppem)) / ppem;
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

  inline unsigned int get_size (void) const {
    switch (u.caretValueFormat) {
    case 1: return sizeof (u.format1);
    case 2: return sizeof (u.format2);
    case 3: return sizeof (u.format3);
    default:return sizeof (u.caretValueFormat);
    }
  }

  /* XXX  we need access to a load-contour-point vfunc here */
  inline int get_caret_value (int ppem) const {
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


#define DEFINE_ACCESSOR0(const, Type, name, Name) \
  inline const Type* name (void) const { \
    if (!Name) return NULL; \
    return (const Type *)((const char*)this + Name); \
  }
#define DEFINE_ACCESSOR(Type, name, Name) \
	DEFINE_ACCESSOR0(const, Type, name, Name) \
	DEFINE_ACCESSOR0(     , Type, name, Name)

struct GDEFHeader {

  DEFINE_ACCESSOR (ClassDef, get_glyph_class_def, glyphClassDef);
  DEFINE_ACCESSOR (AttachList, get_attach_list, attachList);
  DEFINE_ACCESSOR (LigCaretList, get_lig_caret_list, ligCaretList);
  DEFINE_ACCESSOR (ClassDef, get_mark_attach_class_def, markAttachClassDef);

  /* Returns 0 if not found. */
  inline int get_glyph_class (uint16_t glyph_id) const {
    const ClassDef *class_def = get_glyph_class_def ();
    if (!class_def) return 0;
    return class_def->get_class (glyph_id);
  }

  /* Returns 0 if not found. */
  inline int get_mark_attachment_type (uint16_t glyph_id) const {
    const ClassDef *class_def = get_mark_attach_class_def ();
    if (!class_def) return 0;
    return class_def->get_class (glyph_id);
  }

  /* TODO get_attach and get_lig_caret */

  private:
  Fixed		version;		/* Version of the GDEF table--initially
					 * 0x00010000 */
  Offset	glyphClassDef;		/* Offset to class definition table
					 * for glyph type--from beginning of
					 * GDEF header (may be NULL) */
  Offset	attachList;		/* Offset to list of glyphs with
					 * attachment points--from beginning
					 * of GDEF header (may be NULL) */
  Offset	ligCaretList;		/* Offset to list of positioning points
					 * for ligature carets--from beginning
					 * of GDEF header (may be NULL) */
  Offset	markAttachClassDef;	/* Offset to class definition table for
					 * mark attachment type--from beginning
					 * of GDEF header (may be NULL) */
};
ASSERT_SIZE (GDEFHeader, 12);

#endif /* HARFBUZZ_GDEF_PRIVATE_H */
