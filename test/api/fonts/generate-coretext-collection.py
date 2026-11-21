#!/usr/bin/env python3
"""Generate a TTC whose face and instance indices differ from CT descriptors."""

from pathlib import Path

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTCollection, newTable
from fontTools.ttLib.tables.TupleVariation import TupleVariation


def make_font(family, advance, variable, postscript_names, postscript_name=True):
    builder = FontBuilder(1000, isTTF=True)
    order = [".notdef", "A"]
    builder.setupGlyphOrder(order)
    builder.setupCharacterMap({0x41: "A"})
    pen = TTGlyphPen(None)
    pen.moveTo((100, 0))
    pen.lineTo((400, 0))
    pen.lineTo((400, 600))
    pen.lineTo((100, 600))
    pen.closePath()
    builder.setupGlyf({".notdef": TTGlyphPen(None).glyph(), "A": pen.glyph()})
    builder.setupHorizontalMetrics({name: (advance, 100) for name in order})
    builder.setupHorizontalHeader(ascent=800, descent=-200)
    names = {"familyName": family, "styleName": "Regular"}
    if postscript_name:
        names["psName"] = family + "-Regular"
    builder.setupNameTable(names)
    builder.setupOS2()
    builder.setupPost()
    if variable:
        instances = []
        # The default instance is deliberately last, rather than first.
        for style, weight in [("Bold", 900), ("Light", 100), ("Regular", 400)]:
            instance = {"location": {"wght": weight}, "stylename": style}
            if postscript_names:
                instance["postscriptfontname"] = family + "-" + style
            instances.append(instance)
        builder.setupFvar([("wght", 100, 400, 900, "Weight")], instances)
        gvar = newTable("gvar")
        gvar.version = 1
        gvar.reserved = 0
        gvar.variations = {"A": [
            TupleVariation({"wght": (0, 1, 1)}, [(0, 0)] * 5 + [(100, 0)] + [(0, 0)] * 2),
            TupleVariation({"wght": (-1, -1, 0)}, [(0, 0)] * 5 + [(-50, 0)] + [(0, 0)] * 2),
        ]}
        builder.font["gvar"] = gvar
    builder.font["head"].created = builder.font["head"].modified = 2082844800
    builder.font.recalcTimestamp = False
    return builder.font


collection = TTCollection()
collection.fonts = [
    make_font("HBCoreTextFirst", 500, True, True),
    make_font("HBCoreTextSecond", 700, True, False),
    make_font("HBCoreTextThird", 900, False, False),
]
collection.fonts[0].save(Path(__file__).with_name("coretext-variable.ttf"))
collection.save(Path(__file__).with_name("coretext-collection.ttc"))

# Named instances, and no PostScript name to tell the face's descriptors by.
make_font("HBCoreTextNameless", 500, True, False, False).save(
    Path(__file__).with_name("coretext-no-postscript-name.ttf"))
