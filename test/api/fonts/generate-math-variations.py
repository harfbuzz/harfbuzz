"""Generate MATH VariationIndex records sharing a two-axis GDEF store."""

from pathlib import Path
from io import BytesIO

from fontTools.fontBuilder import FontBuilder
from fontTools.otlLib.builder import buildMathTable, buildDevice
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont, newTable
from fontTools.ttLib.tables import otTables as ot
from fontTools.varLib.builder import (
    buildVarDevTable,
    buildVarData,
    buildVarRegionList,
    buildVarStore,
)

BASES = {
    "leading": (100, 10, 20),
    "axis": (200, -20, 30),
    "italic_a": (10, 30, -10),
    "italic_b": (20, 40, -20),
    "accent_a": (300, 50, 5),
    "accent_b": (400, 60, -5),
    "height": (50, 70, 7),
    "kern_0": (70, 80, 8),
    "kern_1": (80, 90, 9),
    "assembly": (40, 100, 10),
}


def values(font):
    math = font["MATH"].table
    result = {
        "leading": math.MathConstants.MathLeading,
        "axis": math.MathConstants.AxisHeight,
    }
    glyphs = math.MathGlyphInfo
    info = glyphs.MathItalicsCorrectionInfo
    result.update(
        ("italic_" + name, value)
        for name, value in zip(info.Coverage.glyphs, info.ItalicsCorrection)
    )
    info = glyphs.MathTopAccentAttachment
    result.update(
        ("accent_" + name, value)
        for name, value in zip(info.TopAccentCoverage.glyphs, info.TopAccentAttachment)
    )
    kern = glyphs.MathKernInfo.MathKernInfoRecords[0].TopRightMathKern
    result["height"] = kern.CorrectionHeight[0]
    result.update(("kern_" + str(i), value) for i, value in enumerate(kern.KernValue))
    result["assembly"] = math.MathVariants.VertGlyphConstruction[
        0
    ].GlyphAssembly.ItalicsCorrection
    return result


def roundtrip(font):
    stream = BytesIO()
    font.save(stream)
    stream.seek(0)
    return TTFont(stream)


def font():
    fb = FontBuilder(1000, isTTF=True)
    names = [".notdef", "a", "b"]
    fb.setupGlyphOrder(names)
    fb.setupCharacterMap({0x61: "a", 0x62: "b"})
    pen = TTGlyphPen(None)
    pen.moveTo((0, 0))
    pen.lineTo((500, 0))
    pen.lineTo((500, 500))
    pen.closePath()
    fb.setupGlyf({name: pen.glyph() for name in names})
    fb.setupHorizontalMetrics({name: (600, 0) for name in names})
    fb.setupHorizontalHeader(ascent=800, descent=-200)
    fb.setupNameTable({"familyName": "Math Variations", "styleName": "Regular"})
    fb.setupOS2(
        sTypoAscender=800, sTypoDescender=-200, usWinAscent=800, usWinDescent=200
    )
    fb.setupPost()
    fb.setupFvar([("wght", 0, 0, 1, "Weight"), ("wdth", 0, 0, 1, "Width")], [])
    font = fb.font
    buildMathTable(
        font,
        constants={
            "MathLeading": 100,
            "AxisHeight": 200,
            "AccentBaseHeight": 500,
            "FlattenedAccentBaseHeight": 600,
        },
        italicsCorrections={"a": 10, "b": 20},
        topAccentAttachments={"a": 300, "b": 400},
        mathKerns={"a": {"TopRight": ([50], [70, 80])}},
        vertGlyphAssembly={"a": ([("a", 0, 0, 0, 500)], 40)},
    )
    rows = [[999, -999], [0, 0]]
    for name, record in values(font).items():
        record.DeviceTable = buildVarDevTable(len(rows))
        rows.append(list(BASES[name][1:]))
    constants = font["MATH"].table.MathConstants
    constants.AccentBaseHeight.DeviceTable = buildVarDevTable(ot.NO_VARIATION_INDEX)
    constants.FlattenedAccentBaseHeight.DeviceTable = buildDevice({10: 1})
    font["GDEF"] = newTable("GDEF")
    gdef = font["GDEF"].table = ot.GDEF()
    gdef.Version = 0x00010003
    gdef.populateDefaults()
    gdef.VarStore = buildVarStore(
        buildVarRegionList(
            [{"wght": (0, 1, 1)}, {"wdth": (0, 1, 1)}], ["wght", "wdth"]
        ),
        [buildVarData([0, 1], rows)],
    )
    return roundtrip(font)



font = font()
font["head"].created = font["head"].modified = 2082844800
font.recalcTimestamp = False
font.save(Path(__file__).with_name("math-variations.ttf"))
