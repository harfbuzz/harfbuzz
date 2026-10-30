"""Generate a COLRv1 glyph that paints thousands of layers under one clip glyph."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.colorLib.builder import buildCOLR, buildCPAL
from fontTools.pens.ttGlyphPen import TTGlyphPen


def build(num_points, num_layers, filename):
    fb = FontBuilder(1000, isTTF=True)
    names = [".notdef", "A", "clip"]
    fb.setupGlyphOrder(names)
    fb.setupCharacterMap({0x41: "A"})
    # A sawtooth contour: every edge crosses every scanline of the glyph.
    pen = TTGlyphPen(None)
    pen.moveTo((0, 0))
    for i in range(1, num_points):
        pen.lineTo((i * 1000 // num_points, 1000 if i % 2 else 0))
    pen.closePath()
    fb.setupGlyf({".notdef": TTGlyphPen(None).glyph(), "A": TTGlyphPen(None).glyph(), "clip": pen.glyph()})
    fb.setupHorizontalMetrics({name: (1000, 0) for name in names})
    fb.setupHorizontalHeader(ascent=800, descent=-200)
    fb.setupNameTable({"familyName": "COLR Clip Paint Budget", "styleName": "Regular"})
    fb.setupOS2(
        sTypoAscender=800, sTypoDescender=-200, usWinAscent=800, usWinDescent=200
    )
    fb.setupPost()
    # PaintColrLayers nests as 255-wide lists; identical lists share one
    # LayerList range, so the glyph stays small however many layers it paints.
    solid = dict(Format=2, PaletteIndex=0, Alpha=1)
    layers = [dict(Format=1, Layers=[solid] * 255)] * (num_layers // 255)
    paints = {
        "A": dict(Format=10, Glyph="clip", Paint=dict(Format=1, Layers=layers)),
    }
    fb.font["COLR"] = buildCOLR(
        paints,
        version=1,
        glyphMap=fb.font.getReverseGlyphMap(),
    )
    fb.font["CPAL"] = buildCPAL([[(1.0, 0.0, 0.0, 1.0)]])
    fb.save(filename)


build(300, 63 * 255, Path(__file__).with_name("colr-clip-paint-budget.ttf"))
