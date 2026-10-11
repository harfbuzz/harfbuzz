"""Generate constant HVAR/VVAR and GPOS regions at the default instance."""

from pathlib import Path
from copy import deepcopy
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.t2CharStringPen import T2CharStringPen
from fontTools.feaLib.builder import addOpenTypeFeaturesFromString
from fontTools.ttLib import newTable
from fontTools.ttLib.tables import otTables as ot
from fontTools.varLib.builder import buildVarRegionList, buildVarData, buildVarStore
from fontTools.varLib.builder import buildVarDevTable, buildDeltaSetIndexMap

fb = FontBuilder(1000, isTTF=False)
names = [".notdef", "a", "acutecomb"]
fb.setupGlyphOrder(names)
fb.setupCharacterMap({0x61: "a", 0x301: "acutecomb"})
fb.setupHorizontalMetrics(
    {name: (0 if name == "acutecomb" else 500, 0) for name in names}
)
fb.setupHorizontalHeader(ascent=800, descent=-200)
fb.setupVerticalMetrics({name: (1000, 0) for name in names})
fb.setupVerticalHeader(ascent=800, descent=-200)
fb.setupVerticalOrigins({name: 800 for name in names})
fb.setupNameTable({"familyName": "Default Item Variations", "styleName": "Regular"})
fb.setupOS2(sTypoAscender=800, sTypoDescender=-200, usWinAscent=800, usWinDescent=200)
fb.setupPost()
fb.setupFvar([("wght", 0, 0, 1, "Weight"), ("wdth", 0, 0, 1, "Width")], [])
pen = T2CharStringPen(None, None, CFF2=True)
fb.setupCFF2({name: pen.getCharString() for name in names})
regions = buildVarRegionList(
    [{}, {"wght": (0, 1, 1)}, {"wdth": (0, 1, 1)}], ["wght", "wdth"]
)
for tag, delta in [("HVAR", 100), ("VVAR", 200)]:
    table = newTable(tag)
    table.table = getattr(ot, tag)()
    table.table.Version = 0x00010000
    table.table.VarStore = buildVarStore(
        deepcopy(regions), [buildVarData([0, 1, 2], [[delta, 80, -40]] * len(names))]
    )
    if tag == "HVAR":
        table.table.AdvWidthMap = table.table.LsbMap = table.table.RsbMap = None
    else:
        table.table.AdvHeightMap = table.table.TsbMap = table.table.BsbMap = None
        table.table.VOrgMap = buildDeltaSetIndexMap([0] * len(names))
    fb.font[tag] = table
addOpenTypeFeaturesFromString(
    fb.font,
    "feature kern { pos a <5 7 30 0>; } kern; "
    "markClass acutecomb <anchor 0 0> @top; "
    "feature mark { pos base a <anchor 300 700> mark @top; } mark;",
)
value = fb.font["GPOS"].table.LookupList.Lookup[0].SubTable[0].Value
value.XPlaDevice = buildVarDevTable(0)
value.YPlaDevice = buildVarDevTable(1)
value.XAdvDevice = buildVarDevTable(2)
fb.font["GPOS"].table.LookupList.Lookup[0].SubTable[0].ValueFormat |= 0x70
anchor = (
    fb.font["GPOS"]
    .table.LookupList.Lookup[1]
    .SubTable[0]
    .BaseArray.BaseRecord[0]
    .BaseAnchor[0]
)
anchor.Format = 3
anchor.XDeviceTable = buildVarDevTable(0)
anchor.YDeviceTable = buildVarDevTable(1)
gdef = fb.font["GDEF"].table
gdef.Version = 0x00010003
gdef.GlyphClassDef = gdef.AttachList = gdef.LigCaretList = gdef.MarkAttachClassDef = (
    None
)
gdef.MarkGlyphSetsDef = None
gdef.VarStore = buildVarStore(
    regions, [buildVarData([0, 1, 2], [[11, 20, -10], [13, 20, -10], [40, 20, -10]])]
)
fb.font.recalcTimestamp = False
fb.font["head"].created = fb.font["head"].modified = 2082844800
fb.save(Path(__file__).parent / "default-item-variations.otf")
