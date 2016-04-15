/*
 * Copyright © 2016  Igalia S.L.
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
 * Igalia Author(s): Frédéric Wang
 */

#include "hb-ot-shape-private.hh"
#include "hb-ot-layout-math-table.hh"
#include <algorithm>
#include <math.h>

// WARNING: The horizontal advance and vertical advances are nonnegative when
// moving rightwards and upwards. In the OpenType MATH table, the advances
// StartConnectorLength, EndConnectorLength and FullAdvance are nonnegative
// and so the layout of parts is performed from left to right for horizontal
// assemblies and from bottom to top for vertical assemblies. However, for
// consistency with hb_font_get_glyph_h_advance and hb_font_get_glyph_v_advance,
// the horizontal advances and vertical advances returned for the glyph assembly
// are also respectively nonnegative and nonpositive.

struct GlyphAssembly
{
  GlyphAssembly(hb_font_t *font, bool horizontal)
    : _font(font)
    , _horizontal(horizontal) { }

  inline bool is_horizontal() const { return _horizontal; }
  virtual unsigned int part_count() const = 0;
  virtual hb_codepoint_t glyph(unsigned int i) const = 0;
  virtual hb_position_t start_connector_length(unsigned int i) const = 0;
  virtual hb_position_t end_connector_length(unsigned int i) const = 0;
  virtual hb_position_t full_advance(unsigned int i) const = 0;
  virtual bool is_extender(unsigned int i) const = 0;

protected:
  hb_font_t *_font;
  bool _horizontal;
};

struct GlyphAssemblyFromMathTable : public GlyphAssembly
{
  GlyphAssemblyFromMathTable(hb_font_t *font, bool horizontal,
                             const OT::GlyphAssembly &glyphAssembly)
    : GlyphAssembly(font, horizontal)
    , glyphAssembly(glyphAssembly) { }

  virtual inline unsigned int part_count() const {
    return glyphAssembly.part_record_count();
  }
  virtual inline hb_codepoint_t glyph(unsigned int i) const {
    return glyphAssembly.get_part_record(i).get_glyph();
  }
  virtual inline hb_position_t start_connector_length(unsigned int i) const {
    return glyphAssembly.
      get_part_record(i).get_start_connector_length(_font, _horizontal);
  }
  virtual inline hb_position_t end_connector_length(unsigned int i) const {
    return glyphAssembly.
      get_part_record(i).get_end_connector_length(_font, _horizontal);
  }
  virtual inline hb_position_t full_advance(unsigned int i) const {
    return glyphAssembly.
      get_part_record(i).get_full_advance(_font, _horizontal);
  }
  virtual inline bool is_extender(unsigned int i) const {
    return glyphAssembly.get_part_record(i).is_extender();
  }

private:
  const OT::GlyphAssembly &glyphAssembly;
};

struct GlyphPartRecordFromUnicode
{
  GlyphPartRecordFromUnicode() : _size(0) { }

  inline unsigned int size() const { return _size; }
  inline hb_position_t glyph(unsigned int i) const {
    assert(i < _size);
    return _glyph[i];
  }
  inline hb_position_t is_extender(unsigned int i) const {
    assert(i < _size);
    return _is_extender[i];
  }
  inline void push_back(hb_position_t glyph, bool is_extender) {
    assert(_size < 5);
    _glyph[_size] = glyph;
    _is_extender[_size] = is_extender;
    _size++;
  }

private:
  hb_position_t _glyph[5];
  bool _is_extender[5];
  unsigned int _size;
};

struct GlyphAssemblyFromUnicode : public GlyphAssembly
{
  GlyphAssemblyFromUnicode(hb_font_t *font, bool horizontal,
                           const GlyphPartRecordFromUnicode &partRecord)
    : GlyphAssembly(font, horizontal)
    , partRecord(partRecord) { }

  virtual unsigned int part_count() const {
    return partRecord.size();
  }
  virtual inline hb_codepoint_t glyph(unsigned int i) const {
    return partRecord.glyph(i);
  }
  virtual inline hb_position_t start_connector_length(unsigned int i) const {
    return 0;
  }
  virtual inline hb_position_t end_connector_length(unsigned int i) const {
    return 0;
  }
  virtual inline hb_position_t full_advance(unsigned int i) const {
    return _horizontal ?
      hb_font_get_glyph_h_advance(_font, partRecord.glyph(i)) :
      -hb_font_get_glyph_v_advance(_font, partRecord.glyph(i));
  }
  virtual inline bool is_extender(unsigned int i) const {
    return partRecord.is_extender(i);
  }

private:
  const GlyphPartRecordFromUnicode &partRecord;
};

////////////////////////////////////////////////////////////////////////////////

static void
set_single_glyph (hb_font_t           *font,
                  hb_buffer_t         *buffer,
                  hb_codepoint_t      glyph)
{
  assert (hb_buffer_get_length (buffer) == 1);

  buffer->content_type = HB_BUFFER_CONTENT_TYPE_GLYPHS;
  buffer->info[0].codepoint = glyph;

  buffer->have_positions = true;

  buffer->pos[0].x_advance = hb_font_get_glyph_h_advance(font, glyph);
  buffer->pos[0].y_advance = hb_font_get_glyph_v_advance(font, glyph);

  buffer->pos[0].x_offset = 0;
  buffer->pos[0].y_offset = 0;
}

static hb_position_t
get_glyph_orthogonal_advance (hb_font_t          *font,
                              bool                horizontal,
                              hb_codepoint_t      glyph)
{
  return horizontal ?
    -hb_font_get_glyph_v_advance(font, glyph) :
    hb_font_get_glyph_h_advance(font, glyph);
}

static bool
try_base_glyph (hb_font_t           *font,
                hb_buffer_t         *buffer,
                bool                horizontal,
                hb_position_t       target_size,
                hb_codepoint_t      base_glyph)
{
  hb_codepoint_t base_advance = horizontal ?
    hb_font_get_glyph_h_advance(font, base_glyph) :
    -hb_font_get_glyph_v_advance(font, base_glyph);
  if (base_advance >= target_size) {
    set_single_glyph(font, buffer, base_glyph);
    return true;
  }
  return false;
}

struct UnicodeConstruction {
  hb_position_t  unicode;
  bool           horizontal;
  hb_position_t  extender;
  hb_position_t  start;
  hb_position_t  middle;
  hb_position_t  end;
};

static bool UnicodeConstructionCompare(const UnicodeConstruction &a,
                                       const UnicodeConstruction &b)
{
  return (a.unicode < b.unicode ||
          (a.unicode == b.unicode && !a.horizontal && b.horizontal));
};

static const UnicodeConstruction unicodeConstructions[] = {
  // uni    horiz   ext     start  middle    end
  { 0x0028, false, 0x239C, 0x239D, 0x0000, 0x239B }, // LEFT PARENTHESIS
  { 0x0029, false, 0x239F, 0x23A0, 0x0000, 0x239E }, // RIGHT PARENTHESIS
  { 0x005B, false, 0x23A2, 0x23A3, 0x0000, 0x23A1 }, // LEFT SQUARE BRACKET
  { 0x005D, false, 0x23A5, 0x23A6, 0x0000, 0x23A4 }, // RIGHT SQUARE BRACKET
  { 0x005F, true,  0x005F, 0x005F, 0x0000, 0x0000 }, // LOW LINE
  { 0x007B, false, 0x23AA, 0x23A9, 0x23A8, 0x23A7 }, // LEFT CURLY BRACKET
  { 0x007C, false, 0x007C, 0x007C, 0x0000, 0x0000 }, // VERTICAL BAR
  { 0x007D, false, 0x23AA, 0x23AD, 0x23AC, 0x23AB }, // RIGHT CURLY BRACKET
  { 0x00AF, true,  0x00AF, 0x203E, 0x0000, 0x0000 }, // MACRON
  { 0x2016, false, 0x2016, 0x2016, 0x0000, 0x0000 }, // DOUBLE VERTICAL LINE
  { 0x203E, true,  0x203E, 0x203E, 0x0000, 0x0000 }, // OVERLINE
  { 0x222B, false, 0x23AE, 0x2321, 0x0000, 0x2320 }, // INTEGRAL SIGN
  { 0x2308, false, 0x23A2, 0x0000, 0x0000, 0x23A1 }, // LEFT CEILING
  { 0x2309, false, 0x23A5, 0x0000, 0x0000, 0x23A4 }, // RIGHT CEILING
  { 0x230A, false, 0x23A2, 0x23A3, 0x0000, 0x0000 }, // LEFT FLOOR
  { 0x230B, false, 0x23A5, 0x23A6, 0x0000, 0x0000 }, // RIGHT FLOOR
  { 0x23B0, false, 0x23AA, 0x23AD, 0x0000, 0x23A7 }, // \lmoustache
  { 0x23B1, false, 0x23AA, 0x23A9, 0x0000, 0x23AB }  // \rmoustache
};
static const unsigned int unicodeConstructionsCount =
  sizeof(unicodeConstructions) / sizeof(unicodeConstructions[0]);

static bool find_unicode_construction(hb_font_t *font,
                                      hb_codepoint_t unicode,
                                      bool horizontal,
                                      GlyphPartRecordFromUnicode &parts)
{
  // Perform a binary search to find entry (unicode, horizontal).
  const UnicodeConstruction *first = unicodeConstructions;
  const UnicodeConstruction *last = first + unicodeConstructionsCount;
  UnicodeConstruction val;
  val.unicode = unicode;
  val.horizontal = horizontal;
  const UnicodeConstruction *construction =
    std::lower_bound(first, last, val, UnicodeConstructionCompare);
  if (construction == last) return false;

  // Fill the GlyphPartRecordFromUnicode with the different parts converted to
  // to glyph indices.
  hb_codepoint_t extender;
  if (!hb_font_get_glyph(font, construction->extender, 0, &extender))
    return false;

  if (construction->start) {
    hb_codepoint_t start;
    if (!hb_font_get_glyph(font, construction->start, 0, &start))
      return false;
    parts.push_back(start, false);
    parts.push_back(extender, true);
  }

  if (construction->middle) {
    hb_codepoint_t middle;
    if (!hb_font_get_glyph(font, construction->middle, 0, &middle))
      return false;
    parts.push_back(middle, false);
    parts.push_back(extender, true);
  }

  if (construction->end) {
    hb_codepoint_t end;
    if (!hb_font_get_glyph(font, construction->end, 0, &end))
      return false;
    parts.push_back(end, false);
  }

  return true;
}

static hb_position_t
get_glyph_assembly_max_orthogonal_advance (hb_font_t *font,
                                           const GlyphAssembly &glyphAssembly)
{
  hb_position_t max_orthogonal_advance = 0;

  for (unsigned int i = 0; i < glyphAssembly.part_count(); i++) {
    hb_position_t partAdvance =
      get_glyph_orthogonal_advance(font,
                                   glyphAssembly.is_horizontal(),
                                   glyphAssembly.glyph(i));
    max_orthogonal_advance = std::max(max_orthogonal_advance, partAdvance);
  }

  return max_orthogonal_advance;
}

static bool
set_glyph_assembly (hb_font_t           *font,
                    hb_buffer_t         *buffer,
                    const GlyphAssembly &glyphAssembly,
                    const hb_position_t minConnectorOverlap,
                    hb_position_t       target_size)
{
  assert (hb_buffer_get_length (buffer) == 1);
  if (glyphAssembly.part_count() == 0) return false;

  // A glyph assembly is made of a certain number of parts with start and end
  // connectors. The end connector of part i must overlap with the start
  // connector of i+1 by at least minConnectorOverlap and at most the minimal
  // length of these two connectors.
  //
  //           Part_i                                 Part_{i+1}
  //
  //                        ConnectorOverlap_{i,i+1}
  //                                ______
  //             ---               |      |
  //            /   \                                    ---
  // +++++++++++  O  ++++++++++++++++++++++             /   \
  //            \   /              +++++++++++++++++++++  O  +++++++++++++++++++
  //             ---                                    \   /
  //                                                     ---
  // |_________|     |____________________|
  // StartConnector_i     EndConnector_i
  //
  // |____________________________________|
  //             FullAdvance_i
  //                               |___________________|    |__________________|
  //                                StartConnector_{i+1}     EndConnector_{i+1}
  //                               |___________________________________________|
  //                                             FullAdvance_{i+1}
  //
  // A part can be an extender, which means that it can be repeated as many
  // times as needed to match the target size. To ensure that the width/height
  // is distributed equally and the symmetry of the shape is preserved, all
  // the extenders are repeated the same number of time (repeatCountExt) and
  // all consecutive parts use the same overlap (connectorOverlap).
  //
  // Ignoring the repetitions of extenders, we define the following constants:
  //
  // fullAdvanceSumNonExt = sum of the FullAdvance for non-extender parts.
  // fullAdvanceSumExt = sum of the FullAdvance for extender parts.
  // partCountNonExt = number of non extender parts.
  // partCountExt = number of extender parts.
  //
  // and so the number of distinct parts is partCountNonExt + partCountExt.
  //
  // If we now take into account the repetitions of extenders, then the actual
  // size of the output buffer is given by
  //
  // partCount = partCountNonExt + repeatCountExt * partCountExt
  //
  // and the actual size of the glyph assembly is given by
  //
  // fullAdvance
  // = (fullAdvanceSumNonExt + repeatCountExt * fullAdvanceSumExt)
  //   - connectorOverlap * (partCount - 1)
  // = (fullAdvanceSumNonExt - connectorOverlap * (partCountNonExt - 1))
  //  + repeatCountExt * (fullAdvanceSumExt - connectorOverlap * partCountExt)
  //
  // We want to choose connectorOverlap and repeatCountExt so that
  //
  // (*)  fullAdvance >= target_size and |fullAdvance - target_size| is minimal
  //
  // For valid input we can assume partCount >= 1 and using the constraint
  // connectorOverlap >= minConnectorOverlap, we obtain the inequality
  //
  // target_size <= fullAdvance <= A + repeatCountExt * B
  // where A, B respectively denote
  // fullAdvanceSumNonExt - minConnectorOverlap * (partCountNonExt - 1) and
  // fullAdvanceSumExt - minConnectorOverlap * partCountExt
  //
  // For valid input the value, full advances are greater than
  // minConnectorOverlap so we can assume A, B > 0. Hence the best value of
  // repeatCountExt to satisfy (*) is
  //
  // repeatCountExt = ceiling of [ (target_size - A) / B ] >= 1
  //
  // Now, we have
  // target_size <= fullAdvance = C - connectorOverlap * (partCount - 1)
  // with C = (fullAdvanceSumNonExt + repeatCountExt * fullAdvanceSumExt)
  //        >= target_size by construction of repeatCountExt
  //
  // If partCount = 1, then connectorOverlap is irrelevant.
  // Otherwise, the best value of connectorOverlap that satisfy (*) is the
  // largest value value satisfying the constraint on
  // StartConnector/EndConnectors and <= (C - target_size) / (partCount - 1).
  //
  hb_position_t fullAdvanceSumNonExt = 0;
  hb_position_t fullAdvanceSumExt = 0;
  unsigned int partCountNonExt = 0;
  unsigned int partCountExt = 0;
  for (unsigned int i = 0; i < glyphAssembly.part_count(); i++) {
    hb_position_t fullAdvance = glyphAssembly.full_advance(i);
    if (glyphAssembly.is_extender(i)) {
      fullAdvanceSumExt += fullAdvance;
      partCountExt++;
    } else {
      fullAdvanceSumNonExt += fullAdvance;
      partCountNonExt++;
    }
  }
  hb_position_t A = fullAdvanceSumNonExt - minConnectorOverlap * (partCountNonExt - 1);
  hb_position_t B = fullAdvanceSumExt - minConnectorOverlap * partCountExt;
  if (B == 0) return false; // something is wrong in part count or metrics.
  unsigned int repeatCountExt = ceil(float(target_size - A) / B);

  // Determine the actual number of glyphs necessary to draw this assembly at
  // the specified target size.
  unsigned int partCount = partCountNonExt + repeatCountExt * partCountExt;
  if (partCount == 0 ||
      partCount > HB_OT_MATH_MAXIMUM_PART_COUNT_IN_GLYPH_ASSEMBLY) return false;
  if (!hb_buffer_set_length (buffer, partCount)) return false;
  buffer->content_type = HB_BUFFER_CONTENT_TYPE_GLYPHS;
  buffer->have_positions = true;

  // Determine the connector overlap if we have at least two parts.
  hb_position_t connectorOverlap = 0;
  if (partCount >= 2) {
    // First determine the ideal overlap that would get closest to the target
    // size. The following quotient is integer operation and gives the best
    // lower approximation of the actual value with fractional pixels.
    hb_position_t C = fullAdvanceSumNonExt + repeatCountExt * fullAdvanceSumExt;
    connectorOverlap = (C - target_size) / (partCount - 1);

    // We now consider the constraints on connectors. In general, only the
    // start of the first part and then end of the last part are not connected
    // so it is the minimum of StartConnector_i for all i > 0 and of
    // EndConnector_i for all i < glyphAssembly.part_count(). However, if the
    // first or last part is an extender then it will be connected too with
    // a copy of itself.
    for (unsigned int i = 0; i < glyphAssembly.part_count(); i++) {
      bool willBeRepeated = repeatCountExt >= 2 && glyphAssembly.is_extender(i);
      if (i < glyphAssembly.part_count() || willBeRepeated)
        connectorOverlap =
          std::min(connectorOverlap, glyphAssembly.end_connector_length(i));
      else if (i > 0 || willBeRepeated)
        connectorOverlap =
          std::min(connectorOverlap, glyphAssembly.start_connector_length(i));
    }

    if (connectorOverlap < minConnectorOverlap) return false;
  }

  // Add the glyph to the buffer and position them.
  buffer->pos[0].x_offset = buffer->pos[0].y_offset = 0;
  for (unsigned int i = 0, j = 0; i < glyphAssembly.part_count(); i++) {
    unsigned int repeatCount =
      glyphAssembly.is_extender(i) ? repeatCountExt : 1;
    for (unsigned int k = 0; k < repeatCount; k++, j++) {
      buffer->info[j].codepoint = glyphAssembly.glyph(i);
      buffer->info[j].cluster = buffer->info[0].cluster;
      buffer->pos[j].x_advance = 0;
      buffer->pos[j].y_advance = 0;
      hb_position_t deltaOffset = glyphAssembly.full_advance(i) - connectorOverlap;
      if (j < partCount - 1) {
        if (glyphAssembly.is_horizontal()) {
          buffer->pos[j + 1].x_offset = buffer->pos[j].x_offset + deltaOffset;
          buffer->pos[j + 1].y_offset = 0;
        } else {
          buffer->pos[j + 1].x_offset = 0;
          buffer->pos[j + 1].y_offset = buffer->pos[j].y_offset + deltaOffset;
        }
      }
    }
  }

  // Set the advance  the glyph assembly.
  hb_position_t max_orthogonal_advance =
    get_glyph_assembly_max_orthogonal_advance (font, glyphAssembly);
  if (glyphAssembly.is_horizontal()) {
    buffer->pos[partCount - 1].x_advance =
      buffer->pos[partCount - 1].x_offset - buffer->pos[0].x_offset +
      glyphAssembly.full_advance(glyphAssembly.part_count() - 1);
    buffer->pos[partCount - 1].y_advance = -max_orthogonal_advance;
  } else {
    buffer->pos[partCount - 1].x_advance = max_orthogonal_advance;
    buffer->pos[partCount - 1].y_advance =
      -(buffer->pos[partCount - 1].y_offset - buffer->pos[0].y_offset +
        glyphAssembly.full_advance(glyphAssembly.part_count() - 1));
  }

  return true;
}

/**
 * hb_ot_shape_math_stretchy:
 * @font: an #hb_font_t a math font
 * @buffer: an #hb_buffer_t to stretch, containing only a single element
 * @horizontal: boolean indicating the stretch direction
 * @target_size: minimal size to which the glyph should be stretched
 *
 * Try and stretch the single element of @buffer to a specified target size.
 * If the content type of the buffer is HB_BUFFER_CONTENT_TYPE_UNICODE then that
 * element is a Unicode character. The stretching is performed using only
 * predefined Unicode glyph assembly such as
 *
 *                              |LEFT PARENTHESIS UPPER HOOK (U+239B)
 * LEFT PARENTHESIS (U+0028) => |LEFT PARENTHESIS EXTENSION  (U+239C)
 *                              |LEFT PARENTHESIS LOWER HOOK (U+239D)
 *
 * If the content type of the buffer is HB_BUFFER_CONTENT_TYPE_GLYPHS then that
 * element is a glyph index and we try and use the OpenType MATH table to
 * find a larger glyph from a MathGlyphVariantRecord subtable or a glyph
 * assembly described in a GlyphAssembly subtable.
 *
 * The function first tries the base element, then larger glyphs and finally a
 * glyph assembly. It stops once it finds an option that is enough to cover
 * the target size and sets the buffer to HB_BUFFER_CONTENT_TYPE_GLYPHS with
 * either a single glyph or the list of glyph assembly parts:
 *
 * - If the base element or large glyph is used, the output contains the
 * corresponding glyph positioned at the origin and with the glyph advances.
 *
 * - If a glyph assembly is used then it can not contain more than
 * HB_OT_MATH_MAXIMUM_PART_COUNT_IN_GLYPH_ASSEMBLY parts. The order of element
 * in the output buffer is from left to right glyph for horizontal assembly and
 * from the bottom to top for vertical assembly. All the glyphs have the same
 * cluster value as the element from the input buffer. All but the glyphs have
 * null advances, except the last one which holds the advance of the whole glyph
 * assembly. The first glyph in the buffer is positioned at the origin and the
 * offsets of the other glyphs are calculated relative to that glyph.
 *
 * If none of the option is enough to cover the target size, then the buffer is
 * set with the best one.
 *
 * It is up to the client to compare the output size with the desired target
 * size and to position and paint the stretched output appropriately with
 * respect to the surrounding math content.
 *
 * Return value: %FALSE if an error occured during the stretch attempt,
 *               %TRUE otherwise
 *
 * Since: ????
 **/
HB_EXTERN hb_bool_t
hb_ot_shape_math_stretchy (hb_font_t           *font,
                           hb_buffer_t         *buffer,
                           hb_bool_t           horizontal,
                           hb_position_t       target_size)
{
  // It's only possible to call this function on a buffer with a single
  // character/glyph.
  if (buffer->content_type == HB_BUFFER_CONTENT_TYPE_INVALID ||
      hb_buffer_get_length (buffer) != 1) return false;

  // If the buffer contains a single Unicode character, we try stretching it
  // using only the unicodeConstructions table.
  if (buffer->content_type == HB_BUFFER_CONTENT_TYPE_UNICODE) {
    // If the base glyph is large enough, we use it.
    hb_codepoint_t base_unicode = buffer->info[0].codepoint;
    hb_codepoint_t base_glyph;
    if (!hb_font_get_glyph (font, base_unicode, 0, &base_glyph))
      return false;

    if (try_base_glyph (font, buffer, horizontal, target_size, base_glyph))
      return true;

    GlyphPartRecordFromUnicode parts;
    if (find_unicode_construction(font, base_unicode, horizontal, parts)) {
      GlyphAssemblyFromUnicode assembly(font, horizontal, parts);
      if (set_glyph_assembly(font, buffer, assembly, 0, target_size))
        return true;
    }

    set_single_glyph(font, buffer, base_glyph);
    return true;
  }

  // If the buffer contains a single glyph, we try stretching it using the
  // OpenType MATH table.
  assert(buffer->content_type == HB_BUFFER_CONTENT_TYPE_GLYPHS);
  if (!hb_ot_layout_has_math_data (font->face)) return false;

  // If the base glyph is large enough, we use it.
  hb_codepoint_t base_glyph = buffer->info[0].codepoint;
  if (try_base_glyph (font, buffer, horizontal, target_size, base_glyph))
    return true;

  // Try and get a glyph construction or fallback to the base glyph.
  hb_position_t minConnectorOverlap;
  const OT::MathGlyphConstruction *glyph_construction;
  if (!hb_ot_layout_get_math_glyph_construction (font, base_glyph, horizontal,
                                                 minConnectorOverlap,
                                                 glyph_construction)) {
    set_single_glyph(font, buffer, base_glyph);
    return true;
  }

  // Try and get a large enough glyph variant.
  hb_position_t glyph_variant = base_glyph;
  for (unsigned int i = 0; i < glyph_construction->glyph_variant_count(); i++) {
    const OT::MathGlyphVariantRecord &record =
      glyph_construction->get_glyph_variant(i);
    glyph_variant = record.get_glyph();
    if (record.get_advance_measurement(font, horizontal) >= target_size) {
      set_single_glyph(font, buffer, glyph_variant);
      return true;
    }
  }

  // Try and get a glyph assembly or fallback to the larger glyph variant.
  if (glyph_construction->has_glyph_assembly()) {
    GlyphAssemblyFromMathTable assembly(font, horizontal,
                                        glyph_construction->
                                        get_glyph_assembly());
    if (set_glyph_assembly(font, buffer, assembly, minConnectorOverlap,
                           target_size))
      return true;
  }

  set_single_glyph(font, buffer, glyph_variant);
  return true;
}

/**
 * hb_ot_shape_math_stretchy_max_orthogonal_advance:
 * @font: an #hb_font_t a math font
 * @buffer: an #hb_buffer_t to stretch, containing only a single element
 * @horizontal: boolean indicating the stretch direction
 *
 * This function calculates the possible advances in the direction orthogonal to
 * the stretch direction when hb_ot_shape_math_stretchy with the same parameter
 * any target size is called. It then returns the one with the largest absolute
 * value.
 *
 * Return value: the advance with maximum absolute value
 *
 * Since: ????
 **/
HB_EXTERN hb_position_t
hb_ot_shape_math_stretchy_max_orthogonal_advance (hb_font_t     *font,
                                                  hb_buffer_t   *buffer,
                                                  hb_bool_t     horizontal)
{
  // Nothing to do if this is not buffer with a single character/glyph.
  if (buffer->content_type == HB_BUFFER_CONTENT_TYPE_INVALID ||
      hb_buffer_get_length (buffer) != 1)
    return 0;

  hb_position_t max_orthogonal_advance = 0;

  // If the buffer contains a single Unicode character, we try stretching it
  // using only the unicodeConstructions table.
  if (buffer->content_type == HB_BUFFER_CONTENT_TYPE_UNICODE) {
    // Consider the maximum orthogonal advance of the base glyph.
    hb_codepoint_t base_unicode = buffer->info[0].codepoint;
    hb_codepoint_t base_glyph;
    max_orthogonal_advance =
      std::max(max_orthogonal_advance,
               get_glyph_orthogonal_advance(font, horizontal, base_glyph));

    // Consider the maximum orthogonal advance of the assembly.
    GlyphPartRecordFromUnicode parts;
    if (find_unicode_construction(font, base_unicode, horizontal, parts)) {
      GlyphAssemblyFromUnicode assembly(font, horizontal, parts);
      max_orthogonal_advance =
        std::max(max_orthogonal_advance,
                 get_glyph_assembly_max_orthogonal_advance (font, assembly));
    }

    return horizontal ? max_orthogonal_advance : -max_orthogonal_advance;
  }

  // Ensure that the buffer contains a single glyph and the font has an
  // OpenType MATH table.
  assert(buffer->content_type == HB_BUFFER_CONTENT_TYPE_GLYPHS);
  if (!hb_ot_layout_has_math_data (font->face))
    return horizontal ? max_orthogonal_advance : -max_orthogonal_advance;

  // Consider the maximum orthogonal advance of the base glyph.
  hb_codepoint_t base_glyph = buffer->info[0].codepoint;

  // Try and get a glyph construction.
  hb_position_t dummy;
  const OT::MathGlyphConstruction* glyph_construction;
  if (!hb_ot_layout_get_math_glyph_construction (font, base_glyph, horizontal,
                                                 dummy,
                                                 glyph_construction))
    return horizontal ? max_orthogonal_advance : -max_orthogonal_advance;

  // Consider the maximum orthogonal advance for the MathGlyphVariantRecord's
  hb_position_t glyph_variant = base_glyph;
  for (unsigned int i = 0; i < glyph_construction->glyph_variant_count(); i++) {
    const OT::MathGlyphVariantRecord &record =
      glyph_construction->get_glyph_variant(i);
    glyph_variant = record.get_glyph();
    max_orthogonal_advance =
      std::max(max_orthogonal_advance,
               get_glyph_orthogonal_advance(font, horizontal, glyph_variant));
  }

  // Consider the maximum orthogonal advance for the GlyphAssembly.
  if (glyph_construction->has_glyph_assembly()) {
    GlyphAssemblyFromMathTable assembly(font, horizontal,
                                        glyph_construction->
                                        get_glyph_assembly());
    max_orthogonal_advance =
      std::max(max_orthogonal_advance,
               get_glyph_assembly_max_orthogonal_advance (font, assembly));
  }

  return horizontal ? max_orthogonal_advance : -max_orthogonal_advance;
}
