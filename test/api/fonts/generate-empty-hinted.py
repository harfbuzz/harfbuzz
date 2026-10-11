"""Generate a zero-contour glyph with instructions and varying advance."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib.tables.ttProgram import Program
from fontTools.ttLib.tables.TupleVariation import TupleVariation

root = Path(__file__).parent
fb = FontBuilder(1000, isTTF=True)
names = [".notdef", "space", "a"]
fb.setupGlyphOrder(names)
fb.setupCharacterMap({32: "space", 97: "a"})
glyphs = {}
for name in names:
    p = TTGlyphPen(None)
    if name == "a":
        p.moveTo((0, 0))
        p.lineTo((400, 0))
        p.lineTo((400, 700))
        p.closePath()
    glyphs[name] = p.glyph()
program = Program()
program.fromAssembly(["PUSHB[ ]", "1", "POP[ ]"])
glyphs["space"].program = program
fb.setupGlyf(glyphs)
fb.setupHorizontalMetrics({g: (500, 0) for g in names})
fb.setupHorizontalHeader(ascent=800, descent=-200)
fb.setupNameTable(
    {
        "familyName": "Empty Glyph Instructions",
        "styleName": "Regular",
        "uniqueFontIdentifier": "EmptyGlyphInstructions",
        "fullName": "Empty Glyph Instructions",
        "psName": "EmptyGlyphInstructions",
    }
)
fb.setupOS2(sTypoAscender=800, sTypoDescender=-200, usWinAscent=800, usWinDescent=200)
fb.setupPost()
fb.setupFvar([("wght", 0, 0, 1, "Weight"), ("wdth", 0, 0, 1, "Width")], [])
fb.setupGvar(
    {"space": [TupleVariation({"wght": (0, 1, 1)}, [(0, 0), (50, 0), (0, 0), (0, 0)])]}
)
fb.font["maxp"].maxSizeOfInstructions = 3
fb.font["maxp"].maxStackElements = 1
fb.font.recalcTimestamp = False
fb.font["head"].created = fb.font["head"].modified = 2082844800
fb.save(root / "empty-hinted.ttf")
