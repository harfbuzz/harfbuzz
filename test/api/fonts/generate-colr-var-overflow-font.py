"""Generate COLRv1 paints and clip boxes with overflowing variation deltas."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.colorLib.builder import buildCOLR, buildCPAL
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.varLib.builder import buildVarRegionList, buildVarData, buildVarStore

formats = (5, 7, 9, 13, 15, 19, 23, 27, 31)
# Exercise float rounding at INT_MAX, sums beyond int32, and field-width overflow.
deltas = (2147483647, -2147483648, 1073741824, -1073741824, 32768, -32769)
fb = FontBuilder(1000, isTTF=True)
num_coordinates = len(formats) * len(deltas)
names = [".notdef"] + [f"paint{i}" for i in range(num_coordinates + 3 * len(deltas))]
fb.setupGlyphOrder(names)
fb.setupCharacterMap({0x61 + i: name for i, name in enumerate(names[1:])})
fb.setupGlyf({name: TTGlyphPen(None).glyph() for name in names})
fb.setupHorizontalMetrics({name: (1000, 0) for name in names})
fb.setupHorizontalHeader(ascent=800, descent=-200)
fb.setupNameTable({"familyName": "COLR Variation Overflow", "styleName": "Regular"})
fb.setupOS2(sTypoAscender=800, sTypoDescender=-200, usWinAscent=800, usWinDescent=200)
fb.setupPost()
fb.setupFvar([("wght", 0, 0, 1, "Weight"), ("wdth", 0, 0, 1, "Width")], [])
rows, paints, clips = [], {}, {}


def variations(values):
    index = len(rows)
    rows.extend([[value] * 3 + [0] for value in values])
    return index


for i, name in enumerate(names[1:num_coordinates + 1]):
    format = formats[i % len(formats)]
    delta = deltas[i // len(formats)]
    base = 300 if delta > 0 else -300
    solid = dict(Format=2, PaletteIndex=0, Alpha=1)
    paint = dict(Format=format)
    if format in (5, 7, 9):
        paint["ColorLine"] = dict(Extend=0, ColorStop=[
            dict(StopOffset=offset, PaletteIndex=0, Alpha=1, VarIndexBase=0xFFFFFFFF)
            for offset in (0, 1)
        ])
    else:
        paint["Paint"] = solid
    if format == 13:
        paint["Transform"] = dict(
            **{field: base / 65536 for field in ("xx", "yx", "xy", "yy", "dx", "dy")},
            VarIndexBase=variations([delta] * 6),
        )
    else:
        if format in (5, 7):
            fields = ("x0", "y0", "x1", "y1", "x2", "y2") if format == 5 else (
                "x0", "y0", "r0", "x1", "y1", "r1"
            )
            paint.update({field: 300 if field.startswith("r") else base for field in fields})
            # Keep radii positive so full instances retain the radial paint.
            values = [abs(delta) if field.startswith("r") else delta for field in fields]
            values = [min(value, 2147483647) for value in values]
        elif format == 9:
            paint.update(centerX=base, centerY=base, startAngle=0, endAngle=180)
            values = [delta, delta, 0, 0]
        elif format == 15:
            paint.update(dx=base, dy=base)
            values = [delta, delta]
        else:
            paint.update(centerX=base, centerY=base)
            if format == 19:
                paint.update(scaleX=1, scaleY=1)
                values = [0, 0, delta, delta]
            elif format == 23:
                paint.update(scale=1)
                values = [0, delta, delta]
            elif format == 27:
                paint.update(angle=0)
                values = [0, delta, delta]
            else:
                paint.update(xSkewAngle=0, ySkewAngle=0)
                values = [0, 0, delta, delta]
        paint["VarIndexBase"] = variations(values)
    paints[name] = paint
    if i % len(formats) == 0:
        clips[name] = (base, base, base, base, variations([delta] * 4))

for i, name in enumerate(names[num_coordinates + 1:]):
    delta = deltas[i // 3]
    format = (3, 9, 17)[i % 3]
    paint = dict(Format=format)
    if format == 3:
        paint.update(PaletteIndex=0, Alpha=1, VarIndexBase=variations([delta]))
    elif format == 9:
        paint.update(centerX=0, centerY=0, startAngle=0, endAngle=180,
                     VarIndexBase=variations([0, 0, delta, delta]))
        paint["ColorLine"] = dict(Extend=0, ColorStop=[
            dict(StopOffset=offset, PaletteIndex=0, Alpha=1,
                 VarIndexBase=variations([delta, delta]))
            for offset in (0, 1)
        ])
    else:
        paint.update(scaleX=1, scaleY=1, VarIndexBase=variations([delta, delta]),
                     Paint=dict(Format=2, PaletteIndex=0, Alpha=1))
    paints[name] = paint

# The unpinned wdth region keeps radial paints variable in partial instances.
regions = buildVarRegionList(
    [{"wght": (0, 1, 1)}, {"wdth": (0, 1, 1)}], ["wght", "wdth"]
)
fb.font["COLR"] = buildCOLR(
    paints,
    version=1,
    glyphMap=fb.font.getReverseGlyphMap(),
    varStore=buildVarStore(regions, [buildVarData([0, 0, 0, 1], rows, optimize=False)]),
    clipBoxes=clips,
)
fb.font["CPAL"] = buildCPAL([[(1, 0, 0, 1)]])
fb.font.recalcTimestamp = False
fb.font["head"].created = fb.font["head"].modified = 2082844800
fb.save(Path(__file__).parent / "colr-var-overflow.ttf")
