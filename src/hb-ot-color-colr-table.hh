/*
 * Copyright © 2018  Ebrahim Byagowi
 * Copyright © 2020  Google, Inc.
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
 * Google Author(s): Calder Kitagawa
 */

#ifndef HB_OT_COLOR_COLR_TABLE_HH
#define HB_OT_COLOR_COLR_TABLE_HH

#include "hb-open-type.hh"
#include "hb-ot-layout-common.hh"

/*
 * COLR -- Color
 * https://docs.microsoft.com/en-us/typography/opentype/spec/colr
 */
#define HB_OT_TAG_COLR HB_TAG('C','O','L','R')


namespace OT {


struct LayerRecord
{
  operator hb_ot_color_layer_t () const { return {glyphId, colorIdx}; }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  public:
  HBGlyphID	glyphId;	/* Glyph ID of layer glyph */
  Index		colorIdx;	/* Index value to use with a
				 * selected color palette.
				 * An index value of 0xFFFF
				 * is a special case indicating
				 * that the text foreground
				 * color (defined by a
				 * higher-level client) should
				 * be used and shall not be
				 * treated as actual index
				 * into CPAL ColorRecord array. */
  public:
  DEFINE_SIZE_STATIC (4);
};

struct BaseGlyphRecord
{
  int cmp (hb_codepoint_t g) const
  { return g < glyphId ? -1 : g > glyphId ? 1 : 0; }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)));
  }

  public:
  HBGlyphID	glyphId;	/* Glyph ID of reference glyph */
  HBUINT16	firstLayerIdx;	/* Index (from beginning of
				 * the Layer Records) to the
				 * layer record. There will be
				 * numLayers consecutive entries
				 * for this base glyph. */
  HBUINT16	numLayers;	/* Number of color layers
				 * associated with this glyph */
  public:
  DEFINE_SIZE_STATIC (6);
};

typedef HBUINT32 VarIdx;

template <typename T>
struct Variable
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  T      value;
  VarIdx varIdx; /* Use 0xFFFFFFFF to indicate no variation. */
  public:
  DEFINE_SIZE_STATIC (4 + sizeof (T));
};

// Variation structures
typedef Variable<FWORD>    VarFWORD;
typedef Variable<UFWORD>   VarUFWORD;
typedef Variable<HBFixed>  VarFixed;
typedef Variable<F2DOT14>  VarF2DOT14;

// Color structures

// The ColorIndex alpha is multiplied into the alpha of the CPAL entry
// (converted to float -- divide by 255) looked up using paletteIndex to
// produce a final alpha.
struct ColorIndex
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT16    paletteIndex;
  F2DOT14     alpha; /* Default 1.0. Values outside [0.,1.] reserved. */
  public:
  DEFINE_SIZE_STATIC (4);
};

struct VarColorIndex
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT16      paletteIndex;
  VarF2DOT14    alpha; /* Default 1.0. Values outside [0.,1.] reserved. */
  public:
  DEFINE_SIZE_STATIC (8);
};

struct ColorStop
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  F2DOT14       stopOffset;
  ColorIndex    color;
  public:
  DEFINE_SIZE_STATIC (6);
};

struct VarColorStop
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  VarF2DOT14 stopOffset;
  VarColorIndex color;
  public:
  DEFINE_SIZE_STATIC (14);
};

struct Extend : HBUINT8
{
  enum {
    EXTEND_PAD     = 0,
    EXTEND_REPEAT  = 1,
    EXTEND_REFLECT = 2,
  };
  public:
  DEFINE_SIZE_STATIC (1);
};

struct ColorLine
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  stops.sanitize (c));
  }

  Extend                extend;
  ArrayOf<ColorStop>    stops;
  public:
  DEFINE_SIZE_ARRAY_SIZED (3, stops);
};

struct VarColorLine
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  stops.sanitize (c));
  }

  Extend                   extend;
  ArrayOf<VarColorStop>    stops;
  public:
  DEFINE_SIZE_ARRAY_SIZED (3, stops);
};

// Composition modes

// Compositing modes are taken from https://www.w3.org/TR/compositing-1/
// NOTE: a brief audit of major implementations suggests most support most
// or all of the specified modes.
struct CompositeMode : HBUINT8
{
  enum {
    // Porter-Duff modes
    // https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators
    COMPOSITE_CLEAR          =  0,  // https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_clear
    COMPOSITE_SRC            =  1,  // https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_src
    COMPOSITE_DEST           =  2,  // https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_dst
    COMPOSITE_SRC_OVER       =  3,  // https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_srcover
    COMPOSITE_DEST_OVER      =  4,  // https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_dstover
    COMPOSITE_SRC_IN         =  5,  // https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_srcin
    COMPOSITE_DEST_IN        =  6,  // https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_dstin
    COMPOSITE_SRC_OUT        =  7,  // https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_srcout
    COMPOSITE_DEST_OUT       =  8,  // https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_dstout
    COMPOSITE_SRC_ATOP       =  9,  // https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_srcatop
    COMPOSITE_DEST_ATOP      = 10,  // https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_dstatop
    COMPOSITE_XOR            = 11,  // https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_xor
    COMPOSITE_PLUS           = 12,  // https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_plus
  
    // Blend modes
    // https://www.w3.org/TR/compositing-1/#blending
    COMPOSITE_SCREEN         = 13,  // https://www.w3.org/TR/compositing-1/#blendingscreen
    COMPOSITE_OVERLAY        = 14,  // https://www.w3.org/TR/compositing-1/#blendingoverlay
    COMPOSITE_DARKEN         = 15,  // https://www.w3.org/TR/compositing-1/#blendingdarken
    COMPOSITE_LIGHTEN        = 16,  // https://www.w3.org/TR/compositing-1/#blendinglighten
    COMPOSITE_COLOR_DODGE    = 17,  // https://www.w3.org/TR/compositing-1/#blendingcolordodge
    COMPOSITE_COLOR_BURN     = 18,  // https://www.w3.org/TR/compositing-1/#blendingcolorburn
    COMPOSITE_HARD_LIGHT     = 19,  // https://www.w3.org/TR/compositing-1/#blendinghardlight
    COMPOSITE_SOFT_LIGHT     = 20,  // https://www.w3.org/TR/compositing-1/#blendingsoftlight
    COMPOSITE_DIFFERENCE     = 21,  // https://www.w3.org/TR/compositing-1/#blendingdifference
    COMPOSITE_EXCLUSION      = 22,  // https://www.w3.org/TR/compositing-1/#blendingexclusion
    COMPOSITE_MULTIPLY       = 23,  // https://www.w3.org/TR/compositing-1/#blendingmultiply
  
    // Modes that, uniquely, do not operate on components
    // https://www.w3.org/TR/compositing-1/#blendingnonseparable
    COMPOSITE_HSL_HUE        = 24,  // https://www.w3.org/TR/compositing-1/#blendinghue
    COMPOSITE_HSL_SATURATION = 25,  // https://www.w3.org/TR/compositing-1/#blendingsaturation
    COMPOSITE_HSL_COLOR      = 26,  // https://www.w3.org/TR/compositing-1/#blendingcolor
    COMPOSITE_HSL_LUMINOSITY = 27,  // https://www.w3.org/TR/compositing-1/#blendingluminosity
  };
  public:
  DEFINE_SIZE_STATIC (1);
};

// Affine 2D transformations

// This is a standard 2x3 matrix for 2D affine transformation.
struct Affine2x3
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBFixed xx;
  HBFixed yx;
  HBFixed xy;
  HBFixed yy;
  HBFixed dx;
  HBFixed dy;
  public:
  DEFINE_SIZE_STATIC (24);
};

struct VarAffine2x3
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  VarFixed xx;
  VarFixed yx;
  VarFixed xy;
  VarFixed yy;
  VarFixed dx;
  VarFixed dy;
  public:
  DEFINE_SIZE_STATIC (48);
};

struct Paint;
struct PaintColrLayers
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8               format; /* format = 1 */
  HBUINT8               numLayers;
  HBUINT32              firstLayerIndex;  /* index into COLRv1::layersV1 */
  public:
  DEFINE_SIZE_STATIC (6);
};

struct PaintSolid
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8       format; /* format = 2 */
  ColorIndex    color;
  public:
  DEFINE_SIZE_STATIC (5);
};

struct PaintVarSolid
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8          format; /* format = 3 */
  VarColorIndex    color;
  public:
  DEFINE_SIZE_STATIC (9);
};

struct PaintLinearGradient
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                            format; /* format = 4 */
  NNOffsetTo<ColorLine, HBUINT24>    colorLine;
  FWORD                              x0;
  FWORD                              y0;
  FWORD                              x1;
  FWORD                              y1;
  FWORD                              x2; /* Normal; Equal to (x1,y1) in simple cases. */
  FWORD                              y2;
  public:
  DEFINE_SIZE_STATIC (16);
};

struct PaintVarLinearGradient
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                               format; /* format = 5 */
  NNOffsetTo<VarColorLine, HBUINT24>    colorLine;
  VarFWORD                              x0;
  VarFWORD                              y0;
  VarFWORD                              x1;
  VarFWORD                              y1;
  VarFWORD                              x2; // Normal; Equal to (x1,y1) in simple cases.
  VarFWORD                              y2;
  public:
  DEFINE_SIZE_STATIC (40);
};

struct PaintRadialGradient
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                            format; /* format = 6 */
  NNOffsetTo<ColorLine, HBUINT24>    colorLine;
  FWORD                              x0;
  FWORD                              y0;
  UFWORD                             radius0;
  FWORD                              x1;
  FWORD                              y1;
  UFWORD                             radius1;
  public:
  DEFINE_SIZE_STATIC (16);
};

struct PaintVarRadialGradient
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                               format; /* format = 7 */
  NNOffsetTo<VarColorLine, HBUINT24>    colorLine;
  VarFWORD                              x0;
  VarFWORD                              y0;
  VarUFWORD                             radius0;
  VarFWORD                              x1;
  VarFWORD                              y1;
  VarUFWORD                             radius1;
  public:
  DEFINE_SIZE_STATIC (40);
};

struct PaintSweepGradient
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                            format; /* format = 8 */
  NNOffsetTo<ColorLine, HBUINT24>    colorLine;
  FWORD                              centerX;
  FWORD                              centerY;
  HBFixed                            startAngle;
  HBFixed                            endAngle;
  public:
  DEFINE_SIZE_STATIC (16);
};

struct PaintVarSweepGradient
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                               format; /* format = 9 */
  NNOffsetTo<VarColorLine, HBUINT24>    colorLine;
  VarFWORD                              centerX;
  VarFWORD                              centerY;
  VarFixed                              startAngle;
  VarFixed                              endAngle;
  public:
  DEFINE_SIZE_STATIC (32);
};

// Paint a non-COLR glyph, filled as indicated by paint.
struct PaintGlyph
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                        format; /* format = 10 */
  NNOffsetTo<Paint, HBUINT24>    paint;
  HBUINT16                       gid;    /* not a COLR-only gid */
                                         /* shall be less than maxp.numGlyphs */
  public:
  DEFINE_SIZE_STATIC (6);
};

struct PaintColrGlyph
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                  format; /* format = 11 */
  HBUINT16                 gid;    /* shall be a COLR gid */
  public:
  DEFINE_SIZE_STATIC (3);
};

struct PaintTransform
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                        format; /* format = 12 */
  NNOffsetTo<Paint, HBUINT24>    src;
  Affine2x3                      transform;
  public:
  DEFINE_SIZE_STATIC (28);
};

struct PaintVarTransform
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                        format; /* format = 13 */
  NNOffsetTo<Paint, HBUINT24>    src;
  VarAffine2x3                   transform;
  public:
  DEFINE_SIZE_STATIC (52);
};

struct PaintTranslate
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                        format; /* format = 14 */
  NNOffsetTo<Paint, HBUINT24>    src;
  HBFixed                        dx;
  HBFixed                        dy;
  public:
  DEFINE_SIZE_STATIC (12);
};

struct PaintVarTranslate
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                        format; /* format = 15 */
  NNOffsetTo<Paint, HBUINT24>    src;
  VarFixed                       dx;
  VarFixed                       dy;
  public:
  DEFINE_SIZE_STATIC (20);
};

struct PaintRotate
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                        format; /* format = 16 */
  NNOffsetTo<Paint, HBUINT24>    src;
  HBFixed                        angle;
  HBFixed                        centerX;
  HBFixed                        centerY;
  public:
  DEFINE_SIZE_STATIC (16);
};

struct PaintVarRotate
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                        format; /* format = 17 */
  NNOffsetTo<Paint, HBUINT24>    src;
  VarFixed                       angle;
  VarFixed                       centerX;
  VarFixed                       centerY;
  public:
  DEFINE_SIZE_STATIC (28);
};

struct PaintSkew
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                        format; /* format = 18 */
  NNOffsetTo<Paint, HBUINT24>    src;
  HBFixed                        xSkewAngle;
  HBFixed                        ySkewAngle;
  HBFixed                        centerX;
  HBFixed                        centerY;
  public:
  DEFINE_SIZE_STATIC (20);
};

struct PaintVarSkew
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                        format; /* format = 19 */
  NNOffsetTo<Paint, HBUINT24>    src;
  VarFixed                       xSkewAngle;
  VarFixed                       ySkewAngle;
  VarFixed                       centerX;
  VarFixed                       centerY;
  public:
  DEFINE_SIZE_STATIC (36);
};

struct PaintComposite
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8                        format; /* format = 20 */
  NNOffsetTo<Paint, HBUINT24>    src;
  CompositeMode                  mode;   // If mode is unrecognized use COMPOSITE_CLEAR
  NNOffsetTo<Paint, HBUINT24>    backdrop;
  public:
  DEFINE_SIZE_STATIC (8);
};

struct Paint
{
  protected:
  union {
  HBUINT8                   format;
  PaintColrLayers           paintformat1;
  PaintSolid                paintformat2;
  PaintVarSolid             paintformat3;
  PaintLinearGradient       paintformat4;
  PaintVarLinearGradient    paintformat5;
  PaintRadialGradient       paintformat6;
  PaintVarRadialGradient    paintformat7;
  PaintSweepGradient        paintformat8;
  PaintVarSweepGradient     paintformat9;
  PaintGlyph                paintformat10;
  PaintColrGlyph            paintformat11;
  PaintTransform            paintformat12;
  PaintVarTransform         paintformat13;
  PaintTranslate            paintformat14;
  PaintVarTranslate         paintformat15;
  PaintRotate               paintformat16;
  PaintVarRotate            paintformat17;
  PaintSkew                 paintformat18;
  PaintVarSkew              paintformat19;
  PaintComposite            paintformat20;
  } u;
};

struct BaseGlyphV1Record
{
  int cmp (hb_codepoint_t g) const
  { return g < glyphId ? -1 : g > glyphId ? 1 : 0; }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)));
  }

  public:
  HBGlyphID            glyphId;    /* Glyph ID of reference glyph */
  LNNOffsetTo<Paint>   paint;      /* Typically PaintColrLayers */
  public:
  DEFINE_SIZE_STATIC (6);
};

struct COLR
{
  static constexpr hb_tag_t tableTag = HB_OT_TAG_COLR;

  bool has_data () const { return numBaseGlyphs; }

  unsigned int get_glyph_layers (hb_codepoint_t       glyph,
				 unsigned int         start_offset,
				 unsigned int        *count, /* IN/OUT.  May be NULL. */
				 hb_ot_color_layer_t *layers /* OUT.     May be NULL. */) const
  {
    const BaseGlyphRecord &record = (this+baseGlyphsZ).bsearch (numBaseGlyphs, glyph);

    hb_array_t<const LayerRecord> all_layers = (this+layersZ).as_array (numLayers);
    hb_array_t<const LayerRecord> glyph_layers = all_layers.sub_array (record.firstLayerIdx,
								       record.numLayers);
    if (count)
    {
      + glyph_layers.sub_array (start_offset, count)
      | hb_sink (hb_array (layers, *count))
      ;
    }
    return glyph_layers.length;
  }

  struct accelerator_t
  {
    accelerator_t () {}
    ~accelerator_t () { fini (); }

    void init (hb_face_t *face)
    { colr = hb_sanitize_context_t ().reference_table<COLR> (face); }

    void fini () { this->colr.destroy (); }

    bool is_valid () { return colr.get_blob ()->length; }

    void closure_glyphs (hb_codepoint_t glyph,
			 hb_set_t *related_ids /* OUT */) const
    { colr->closure_glyphs (glyph, related_ids); }

    private:
    hb_blob_ptr_t<COLR> colr;
  };

  void closure_glyphs (hb_codepoint_t glyph,
		       hb_set_t *related_ids /* OUT */) const
  {
    const BaseGlyphRecord *record = get_base_glyph_record (glyph);
    if (!record) return;

    auto glyph_layers = (this+layersZ).as_array (numLayers).sub_array (record->firstLayerIdx,
								       record->numLayers);
    if (!glyph_layers.length) return;
    related_ids->add_array (&glyph_layers[0].glyphId, glyph_layers.length, LayerRecord::min_size);
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  (this+baseGlyphsZ).sanitize (c, numBaseGlyphs) &&
			  (this+layersZ).sanitize (c, numLayers)));
  }

  template<typename BaseIterator, typename LayerIterator,
	   hb_requires (hb_is_iterator (BaseIterator)),
	   hb_requires (hb_is_iterator (LayerIterator))>
  bool serialize (hb_serialize_context_t *c,
		  unsigned version,
		  BaseIterator base_it,
		  LayerIterator layer_it)
  {
    TRACE_SERIALIZE (this);
    if (unlikely (base_it.len () != layer_it.len ()))
      return_trace (false);

    if (unlikely (!c->extend_min (this))) return_trace (false);
    this->version = version;
    numLayers = 0;
    numBaseGlyphs = base_it.len ();
    baseGlyphsZ = COLR::min_size;
    layersZ = COLR::min_size + numBaseGlyphs * BaseGlyphRecord::min_size;

    for (const hb_item_type<BaseIterator> _ : + base_it.iter ())
    {
      auto* record = c->embed (_);
      if (unlikely (!record)) return_trace (false);
      record->firstLayerIdx = numLayers;
      numLayers += record->numLayers;
    }

    for (const hb_item_type<LayerIterator>& _ : + layer_it.iter ())
      _.as_array ().copy (c);

    return_trace (true);
  }

  const BaseGlyphRecord* get_base_glyph_record (hb_codepoint_t gid) const
  {
    if ((unsigned int) gid == 0) // Ignore notdef.
      return nullptr;
    const BaseGlyphRecord* record = &(this+baseGlyphsZ).bsearch (numBaseGlyphs, (unsigned int) gid);
    if ((record && (hb_codepoint_t) record->glyphId != gid))
      record = nullptr;
    return record;
  }

  bool subset (hb_subset_context_t *c) const
  {
    TRACE_SUBSET (this);

    const hb_map_t &reverse_glyph_map = *c->plan->reverse_glyph_map;

    auto base_it =
    + hb_range (c->plan->num_output_glyphs ())
    | hb_map_retains_sorting ([&](hb_codepoint_t new_gid)
			      {
				hb_codepoint_t old_gid = reverse_glyph_map.get (new_gid);

				const BaseGlyphRecord* old_record = get_base_glyph_record (old_gid);
				if (unlikely (!old_record))
				  return hb_pair_t<bool, BaseGlyphRecord> (false, Null (BaseGlyphRecord));

				BaseGlyphRecord new_record = {};
				new_record.glyphId = new_gid;
				new_record.numLayers = old_record->numLayers;
				return hb_pair_t<bool, BaseGlyphRecord> (true, new_record);
			      })
    | hb_filter (hb_first)
    | hb_map_retains_sorting (hb_second)
    ;

    auto layer_it =
    + hb_range (c->plan->num_output_glyphs ())
    | hb_map (reverse_glyph_map)
    | hb_map_retains_sorting ([&](hb_codepoint_t old_gid)
			      {
				const BaseGlyphRecord* old_record = get_base_glyph_record (old_gid);
				hb_vector_t<LayerRecord> out_layers;

				if (unlikely (!old_record ||
					      old_record->firstLayerIdx >= numLayers ||
					      old_record->firstLayerIdx + old_record->numLayers > numLayers))
				  return hb_pair_t<bool, hb_vector_t<LayerRecord>> (false, out_layers);

				auto layers = (this+layersZ).as_array (numLayers).sub_array (old_record->firstLayerIdx,
											     old_record->numLayers);
				out_layers.resize (layers.length);
				for (unsigned int i = 0; i < layers.length; i++) {
				  out_layers[i] = layers[i];
				  hb_codepoint_t new_gid = 0;
				  if (unlikely (!c->plan->new_gid_for_old_gid (out_layers[i].glyphId, &new_gid)))
				    return hb_pair_t<bool, hb_vector_t<LayerRecord>> (false, out_layers);
				  out_layers[i].glyphId = new_gid;
				}

				return hb_pair_t<bool, hb_vector_t<LayerRecord>> (true, out_layers);
			      })
    | hb_filter (hb_first)
    | hb_map_retains_sorting (hb_second)
    ;

    if (unlikely (!base_it || !layer_it || base_it.len () != layer_it.len ()))
      return_trace (false);

    COLR *colr_prime = c->serializer->start_embed<COLR> ();
    return_trace (colr_prime->serialize (c->serializer, version, base_it, layer_it));
  }

  protected:
  HBUINT16	version;	/* Table version number (starts at 0). */
  HBUINT16	numBaseGlyphs;	/* Number of Base Glyph Records. */
  LNNOffsetTo<SortedUnsizedArrayOf<BaseGlyphRecord>>
		baseGlyphsZ;	/* Offset to Base Glyph records. */
  LNNOffsetTo<UnsizedArrayOf<LayerRecord>>
		layersZ;	/* Offset to Layer Records. */
  HBUINT16	numLayers;	/* Number of Layer Records. */
  // Version-1 additions
  LNNOffsetTo<LArrayOf<BaseGlyphV1Record>>	baseGlyphsV1;
  LOffsetTo<LOffsetLArrayOf<Paint>>		layersV1;
  LOffsetTo<VariationStore>			varStore;
  public:
  DEFINE_SIZE_MIN (14);
};

} /* namespace OT */


#endif /* HB_OT_COLOR_COLR_TABLE_HH */
