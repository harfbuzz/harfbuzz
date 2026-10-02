"""Generate precision fixtures with FontTools supporting LookupVariationRecord."""

from pathlib import Path

from fontTools.feaLib.builder import addOpenTypeFeaturesFromString
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import newTable
from fontTools.ttLib.tables import otTables as ot
from fontTools.varLib import builder, featureVars


def value(default, index=ot.NO_VARIATION_INDEX):
    condition = ot.ConditionTable()
    condition.Format, condition.DefaultValue, condition.VarIdx = 2, default, index
    return condition


def compound(format, children):
    condition = ot.ConditionTable()
    condition.Format, condition.ConditionTable = format, children
    if format != 5:
        condition.ConditionCount = len(children)
    return condition


for tag, default, peak in (
    ("GSUB", 0, 1),
    ("GPOS", 0, 1),
    ("GSUB", 32767, 1),
    ("GPOS", 32767, 1),
    ("GSUB", 0, 0.75),
    ("GPOS", 0, 0.75),
):
    for legacy in (True, False):
        fb = FontBuilder(1000)
        order = [".notdef"] + [
            name for char in "ABCDEF" for name in (char, char + ".alt")
        ]
        fb.setupGlyphOrder(order)
        fb.setupCharacterMap({ord(char): char for char in "ABCDEF"})
        fb.setupGlyf({name: TTGlyphPen(None).glyph() for name in order})
        fb.setupHorizontalMetrics({name: (600, 0) for name in order})
        fb.setupHorizontalHeader(ascent=800, descent=-200)
        fb.setupOS2()
        fb.setupNameTable(
            {"familyName": "Layout condition precision", "styleName": "Regular"}
        )
        fb.setupPost()
        fb.setupFvar([(axis, -1, 0, 1, axis) for axis in ("TEST", "DUMY")], [])
        font = fb.font
        addOpenTypeFeaturesFromString(
            font,
            """
            languagesystem DFLT dflt;
            lookup DefaultSub { sub A by A.alt; } DefaultSub;
            lookup ConditionalSub { sub B by B.alt; } ConditionalSub;
            feature liga { lookup DefaultSub; } liga;
            feature zzzz { lookup ConditionalSub; } zzzz;
            lookup DefaultPos { pos [A A.alt] 10; } DefaultPos;
            lookup ConditionalPos { pos B 20; } ConditionalPos;
            feature kern { lookup DefaultPos; } kern;
            feature zzzz { lookup ConditionalPos; } zzzz;
        """,
        )
        gdef = font["GDEF"] = newTable("GDEF")
        gdef.table = ot.GDEF()
        gdef.table.Version = 0x10003
        for field in (
            "GlyphClassDef",
            "AttachList",
            "LigCaretList",
            "MarkAttachClassDef",
            "MarkGlyphSetsDef",
        ):
            setattr(gdef.table, field, None)
        gdef.table.VarStore = builder.buildVarStore(
            builder.buildVarRegionList(
                [
                    {"TEST": (0, peak, 1)},
                    {"DUMY": (0, 1, 1)},
                    {"TEST": (0, peak, 1), "DUMY": (0, 1, 1)},
                ],
                ["TEST", "DUMY"],
            ),
            [
                builder.buildVarData(
                    [0, 1, 2], [[1, -32768 if default else -1, 0], [1, 0, -1]]
                )
            ],
        )
        position = font["GPOS"].table.LookupList.Lookup[0].SubTable[0]
        position.Value.XPlaDevice = builder.buildVarDevTable(0)
        position.ValueFormat |= 0x10
        condition = compound(
            3,
            [
                compound(4, [value(default, 0), value(0)]),
                compound(5, compound(5, value(0, 1))),
            ],
        )
        table = font[tag].table
        feature = next(
            i
            for i, rec in enumerate(table.FeatureList.FeatureRecord)
            if rec.FeatureTag == ("liga" if tag == "GSUB" else "kern")
        )
        variations = table.FeatureVariations = ot.FeatureVariations()
        table.Version = 0x10001
        variations.Version = 0x10000 if legacy else 0x10001
        variations.FeatureVariationRecord = []
        variations.FeatureVariationCount = 0
        if legacy:
            variations.FeatureVariationRecord = [
                featureVars.buildFeatureVariationRecord(
                    [condition],
                    [featureVars.buildFeatureTableSubstitutionRecord(feature, [1])],
                )
            ]
            variations.FeatureVariationCount = 1
        else:
            record = ot.LookupVariationRecord()
            record.FeatureIndex = feature
            lookups = record.FeatureLookupsTable = ot.FeatureLookupsTable()
            lookups.Version, lookups.Flags = 0x10000, 1
            conditional = ot.LookupConditionRecord()
            conditional.ConditionTable = condition
            conditional.LookupIndexList = ot.LookupIndexList()
            conditional.LookupIndexList.LookupIndex = [1]
            conditional.LookupIndexList.LookupIndexCount = 1
            lookups.LookupConditionRecord = [conditional]
            lookups.LookupConditionCount = 1
            variations.LookupVariationRecord = [record]
            variations.LookupVariationCount = 1
        font["head"].created = font["head"].modified = 3800000000
        font.recalcTimestamp = False
        suffix = "-large" if default else "-third" if peak != 1 else ""
        font.save(
            Path(__file__).parent
            / f"feature-variation-precision-{tag}-{'legacy' if legacy else 'lookup'}{suffix}.ttf"
        )
