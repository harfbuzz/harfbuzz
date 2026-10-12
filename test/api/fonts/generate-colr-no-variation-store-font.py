"""Generate COLRv1 sentinel-only mappings with NULL and empty stores."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.colorLib.builder import buildCOLR, buildCPAL
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.varLib.builder import (
    buildDeltaSetIndexMap,
    buildVarRegionList,
    buildVarStore,
)


def build(empty_store):
    fb = FontBuilder(1000, isTTF=True)
    names = [".notdef", "radial"]
    fb.setupGlyphOrder(names)
    fb.setupCharacterMap({0x61: "radial"})
    fb.setupGlyf({glyph: TTGlyphPen(None).glyph() for glyph in names})
    fb.setupHorizontalMetrics({glyph: (1000, 0) for glyph in names})
    fb.setupHorizontalHeader(ascent=800, descent=-200)
    fb.setupNameTable({"familyName": "COLR Sentinel Map", "styleName": "Regular"})
    fb.setupOS2(
        sTypoAscender=800, sTypoDescender=-200, usWinAscent=800, usWinDescent=200
    )
    fb.setupPost()
    fb.setupFvar([("wght", 0, 0, 1, "Weight"), ("wdth", 0, 0, 1, "Width")], [])
    paint = dict(
        Format=7,
        ColorLine=dict(
            Extend="pad",
            ColorStop=[
                dict(StopOffset=0, PaletteIndex=0, Alpha=1, VarIndexBase=6),
                dict(StopOffset=1, PaletteIndex=1, Alpha=1, VarIndexBase=8),
            ],
        ),
        x0=0, y0=0, r0=10, x1=100, y1=0, r1=20, VarIndexBase=0,
    )
    store = (
        buildVarStore(buildVarRegionList([], ["wght", "wdth"]), [])
        if empty_store else None
    )
    fb.font["COLR"] = buildCOLR(
        {"radial": paint}, version=1, glyphMap=fb.font.getReverseGlyphMap(),
        varStore=store, varIndexMap=buildDeltaSetIndexMap([0xFFFFFFFF] * 10),
        clipBoxes={"radial": (-1000, -1000, 1000, 1000)},
    )
    fb.font["CPAL"] = buildCPAL([[(0, 0, 0, 1), (1, 1, 1, 1)]])
    fb.font.recalcTimestamp = False
    fb.font["head"].created = fb.font["head"].modified = 2082844800
    suffix = "empty" if empty_store else "null"
    fb.save(Path(__file__).parent / f"colr-{suffix}-store-sentinel-map.ttf")


build(False)
build(True)
