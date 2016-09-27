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

static hb_position_t
get_glyph_assembly_max_orthogonal_advance (hb_font_t *font,
                                           bool horizontal,
                                           const OT::GlyphAssembly &glyphAssembly)
{
  hb_position_t max_orthogonal_advance = 0;

  for (unsigned int i = 0; i < glyphAssembly.part_record_count(); i++) {
    hb_position_t partAdvance =
      get_glyph_orthogonal_advance(font,
                                   horizontal,
                                   glyphAssembly.
                                   get_part_record(i).get_glyph());
    max_orthogonal_advance = MAX (max_orthogonal_advance, partAdvance);
  }

  return max_orthogonal_advance;
}

static bool
set_glyph_assembly (hb_font_t               *font,
                    hb_buffer_t             *buffer,
                    bool                    horizontal,
                    const OT::GlyphAssembly &glyphAssembly,
                    const hb_position_t     minConnectorOverlap,
                    hb_position_t           target_size)
{
  assert (hb_buffer_get_length (buffer) == 1);
  if (glyphAssembly.part_record_count() == 0) return false;

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
  // largest value value satisfying the constraints on
  // StartConnectors/EndConnectors and <= (C - target_size) / (partCount - 1).
  //
  hb_position_t fullAdvanceSumNonExt = 0;
  hb_position_t fullAdvanceSumExt = 0;
  unsigned int partCountNonExt = 0;
  unsigned int partCountExt = 0;
  for (unsigned int i = 0; i < glyphAssembly.part_record_count(); i++) {
    hb_position_t fullAdvance =
      glyphAssembly.get_part_record(i).get_full_advance(font, horizontal);
    if (glyphAssembly.get_part_record(i).is_extender()) {
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
    // EndConnector_i for all i < glyphAssembly.part_record_count()-1. However,
    // if the first or last part is an extender then it will be connected too
    // with a copy of itself.
    for (unsigned int i = 0; i < glyphAssembly.part_record_count(); i++) {
      bool willBeRepeated = repeatCountExt >= 2 &&
        glyphAssembly.get_part_record(i).is_extender();
      if (i < glyphAssembly.part_record_count() - 1 || willBeRepeated)
        connectorOverlap =
          MIN (connectorOverlap,
               glyphAssembly.
               get_part_record(i).get_end_connector_length(font, horizontal));
      if (i > 0 || willBeRepeated)
        connectorOverlap =
          MIN (connectorOverlap,
               glyphAssembly.get_part_record(i).
               get_start_connector_length(font, horizontal));
    }

    if (connectorOverlap < minConnectorOverlap) return false;
  }

  // Add the glyphs to the buffer and position them.
  if (!hb_buffer_set_length (buffer, partCount)) return false;
  buffer->content_type = HB_BUFFER_CONTENT_TYPE_GLYPHS;
  buffer->have_positions = true;
  buffer->pos[0].x_offset = 0;
  buffer->pos[0].y_offset = 0;
  for (unsigned int i = 0, j = 0; i < glyphAssembly.part_record_count(); i++) {
    unsigned int repeatCount =
      glyphAssembly.get_part_record(i).is_extender() ? repeatCountExt : 1;
    for (unsigned int k = 0; k < repeatCount; k++, j++) {
      buffer->info[j].codepoint = glyphAssembly.get_part_record(i).get_glyph();
      buffer->info[j].cluster = buffer->info[0].cluster;
      buffer->pos[j].x_advance = 0;
      buffer->pos[j].y_advance = 0;
      if (j < partCount - 1) {
        hb_position_t deltaOffset = glyphAssembly.get_part_record(i).
          get_full_advance(font, horizontal) - connectorOverlap;
        if (horizontal) {
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
    get_glyph_assembly_max_orthogonal_advance (font, horizontal, glyphAssembly);
  if (horizontal) {
    buffer->pos[partCount - 1].x_advance =
      buffer->pos[partCount - 1].x_offset - buffer->pos[0].x_offset +
      glyphAssembly.get_part_record(glyphAssembly.part_record_count() - 1).
      get_full_advance(font, horizontal);
    buffer->pos[partCount - 1].y_advance = -max_orthogonal_advance;
  } else {
    buffer->pos[partCount - 1].x_advance = max_orthogonal_advance;
    buffer->pos[partCount - 1].y_advance =
      -(buffer->pos[partCount - 1].y_offset - buffer->pos[0].y_offset +
        glyphAssembly.get_part_record(glyphAssembly.part_record_count() - 1).
        get_full_advance(font, horizontal));
  }

  return true;
}

/**
 * hb_ot_shape_math_stretchy:
 * @font: an #hb_font_t a font with an OpenType MATH table
 * @buffer: an #hb_buffer_t of type HB_BUFFER_CONTENT_TYPE_GLYPHS, containing
 *          only a single glyph.
 * @horizontal: boolean indicating the stretch direction
 * @target_size: minimal size to which the glyph should be stretched
 *
 * Try and stretch the single glyph of @buffer to a specified target size.
 * We use the OpenType MATH table to find a larger glyph from a
 * MathGlyphVariantRecord subtable or a glyph assembly described in a
 * GlyphAssembly subtable.
 *
 * The function first tries the base glyph, then larger glyphs and finally a
 * glyph assembly. It stops once it finds an option that is enough to cover
 * the target size and sets the buffer to either a single glyph or the list of
 * glyph assembly parts:
 *
 * - If the base glyph or large glyph is used, the output contains the
 * corresponding glyph positioned at the origin and with the glyph advances.
 *
 * - If a glyph assembly is used then it can not contain more than
 * HB_OT_MATH_MAXIMUM_PART_COUNT_IN_GLYPH_ASSEMBLY parts. The order of element
 * in the output buffer is from left to right glyph for horizontal assembly and
 * from the bottom to top for vertical assembly. All the glyphs have the same
 * cluster value as the element from the input buffer. All the glyphs have
 * null advances, except the last one which holds the advance of the whole glyph
 * assembly. The first glyph in the buffer is positioned at the origin and the
 * offsets of the other glyphs are calculated relative to that glyph.
 *
 * If none of the option is enough to cover the target size, then the buffer is
 * set with the last one.
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
  // glyph and a font with a MATH table.
  if (buffer->content_type != HB_BUFFER_CONTENT_TYPE_GLYPHS ||
      hb_buffer_get_length (buffer) != 1 ||
      !hb_ot_layout_has_math_data (font->face)) return false;

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
    if (set_glyph_assembly(font,
                           buffer,
                           horizontal,
                           glyph_construction->get_glyph_assembly(),
                           minConnectorOverlap,
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
  hb_position_t max_orthogonal_advance = 0;

  // Early return for invalid parameters.
  if (buffer->content_type != HB_BUFFER_CONTENT_TYPE_GLYPHS ||
      hb_buffer_get_length (buffer) != 1 ||
      !hb_ot_layout_has_math_data (font->face)) return max_orthogonal_advance;

  // Consider the maximum orthogonal advance of the base glyph.
  hb_codepoint_t base_glyph = buffer->info[0].codepoint;
  max_orthogonal_advance =
    MAX (max_orthogonal_advance,
         get_glyph_orthogonal_advance(font, horizontal, base_glyph));

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
      MAX (max_orthogonal_advance,
           get_glyph_orthogonal_advance(font, horizontal, glyph_variant));
  }

  // Consider the maximum orthogonal advance for the GlyphAssembly.
  if (glyph_construction->has_glyph_assembly()) {
    max_orthogonal_advance =
      MAX (max_orthogonal_advance,
           get_glyph_assembly_max_orthogonal_advance (font,
                                                      horizontal,
                                                      glyph_construction->
                                                      get_glyph_assembly()));
  }

  return horizontal ? max_orthogonal_advance : -max_orthogonal_advance;
}
