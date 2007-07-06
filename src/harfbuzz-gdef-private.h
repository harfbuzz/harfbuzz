#ifndef HARFBUZZ_GDEF_PRIVATE_H
#define HARFBUZZ_GDEF_PRIVATE_H

#include "harfbuzz-open-private.h"

struct GDEFHeader {
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

struct GlyphClassDef : ClassDef {
  static const uint16_t BaseGlyph		= 0x0001u;
  static const uint16_t LigatureGlyph		= 0x0002u;
  static const uint16_t MarkGlyph		= 0x0003u;
  static const uint16_t ComponentGlyph		= 0x0004u;
};


struct AttachList {
  /* XXX */

  Offset	coverage;		/* Offset to Coverage table -- from
					 * beginning of AttachList table */
  USHORT	glyphCount;		/* Number of glyphs with attachment
					 * points */
  Offset	attachPoint[];		/* Array of offsets to AttachPoint
					 * tables--from beginning of AttachList
					 * table--in Coverage Index order */
};

struct AttachPoint {
  /* XXX */

  USHORT	pointCount;		/* Number of attachment points on
					 * this glyph */
  USHORT	pointIndex[];		/* Array of contour point indices--in
					 * increasing numerical order */
};

/*
 * Ligature Caret Table
 */

struct CaretValue;

struct LigCaretList {
  /* XXX */

  Offset	coverage;		/* Offset to Coverage table--from
					 * beginning of LigCaretList table */
  USHORT	ligGlyphCount;		/* Number of ligature glyphs */
  Offset	ligGlyph[];		/* Array of offsets to LigGlyph
					 * tables--from beginning of
					 * LigCaretList table--in Coverage
					 * Index order */
};

struct LigGlyph {
  /* Caret value tables, in increasing coordinate order */
  DEFINE_OFFSET_ARRAY_TYPE (CaretValue, caretValue, caretCount);
  /* XXX */

  USHORT	caretCount;		/* Number of CaretValues for this
					 * ligature (components - 1) */
  Offset	caretValue[];		/* Array of offsets to CaretValue
					 * tables--from beginning of LigGlyph
					 * table--in increasing coordinate
					 * order */
};

struct CaretValueFormat1 {
  USHORT	caretValueFormat;	/* Format identifier--format = 1 */
  SHORT		coordinate;		/* X or Y value, in design units */

  inline int get_caret_value (int ppem) const {
    return /* XXX garbage */ coordinate / ppem;
  }
};

struct CaretValueFormat2 {
  USHORT	caretValueFormat;	/* Format identifier--format = 2 */
  USHORT	caretValuePoint;	/* Contour point index on glyph */

  inline int get_caret_value (int ppem) const {
    return /* XXX garbage */ 0 / ppem;
  }
};

struct CaretValueFormat3 {
  inline const Device* get_device (void) const {
    return (const Device*)((const char*)this + deviceTable);
  }

  USHORT	caretValueFormat;	/* Format identifier--format = 3 */
  SHORT		coordinate;		/* X or Y value, in design units */
  Offset	deviceTable;		/* Offset to Device table for X or Y
					 * value--from beginning of CaretValue
					 * table */

  inline int get_caret_value (int ppem) const {
    return /* XXX garbage */ (coordinate + get_device()->get_delta (ppem)) / ppem;
  }
};

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

  union {
  USHORT	caretValueFormat;	/* Format identifier */
  CaretValueFormat1	format1;
  CaretValueFormat2	format2;
  CaretValueFormat3	format3;
  /* XXX old HarfBuzz code has a format 4 here! */
  } u;
};

#endif /* HARFBUZZ_GDEF_PRIVATE_H */
