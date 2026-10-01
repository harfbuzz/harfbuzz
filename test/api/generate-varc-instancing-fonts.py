"""Regenerate the VARC instancing fixtures using fontTools."""

from copy import deepcopy
from pathlib import Path

from fontTools.ttLib import TTFont, newTable
from fontTools.ttLib.tables import otTables
from fontTools.ttLib.tables._f_v_a_r import Axis
from fontTools.ttLib.tables.TupleVariation import TupleVariation


def condition(format, **fields):
    result = otTables.ConditionTable()
    result.Format = format
    result.__dict__.update(fields)
    return result


directory = Path(__file__).parent / "fonts"
font = TTFont(directory / "varc-ac01-conditional.ttf", recalcTimestamp=False)
# Load gvar before changing the axis order.
variations = font["gvar"].variations
for tag in reversed(("DUMY", "COND")):
    axis = Axis()
    axis.axisTag = tag
    axis.minValue, axis.defaultValue, axis.maxValue = -1, 0, 1
    axis.axisNameID = font["fvar"].axes[0].axisNameID
    font["fvar"].axes.insert(0, axis)

varc = font["VARC"].table
varc.AxisIndicesList.Item = [[i + 2 for i in indices]
                            for indices in varc.AxisIndicesList.Item]
for region in varc.MultiVarStore.SparseVarRegionList.Region:
    for axis in region.SparseVarRegionAxis:
        axis.AxisIndex += 2

# COND is used only by nested conditions; DUMY is absent from VARC.
weight_condition = varc.ConditionList.ConditionTable[0]
weight_condition.AxisIndex += 2
low = condition(1, AxisIndex=1, FilterRangeMinValue=-1, FilterRangeMaxValue=0)
middle = condition(1, AxisIndex=1, FilterRangeMinValue=0, FilterRangeMaxValue=0.25)
either = condition(4, ConditionTable=[low, middle])
negated = condition(5, ConditionTable=either)
varc.ConditionList.ConditionTable[0] = condition(
    3, ConditionTable=[weight_condition, negated])

# Give the unrelated axis a visible effect in an underlying outline.
name = "glyph00003"
points = len(font["glyf"][name].getCoordinates(font["glyf"])[0])
variations[name].append(TupleVariation(
    {"DUMY": (0, 1, 1)}, [(20, 0)] * points + [(0, 0)] * 4))
# This tent is reached by a VARC override, but not by the font-level
# default of the private axis. Font-level avar2 culling must preserve it.
variations[name].append(TupleVariation(
    {"0000": (-1, -0.5, -0.25)}, [(0, 20)] * points + [(0, 0)] * 4))
font.save(directory / "varc-unrelated-axis.ttf")

font = deepcopy(font)
for axis in font["fvar"].axes:
    if axis.axisTag.startswith("000"):
        axis.flags = 1  # Hidden component axes.
avar = font["avar"] = newTable("avar")
avar.majorVersion, avar.minorVersion = 2, 0
avar.segments = {a.axisTag: {-1: -1, 0: 0, 1: 1} for a in font["fvar"].axes}
avar.table = otTables.avar()
avar.table.VarIdxMap = None
avar.table.VarStore = None
font.save(directory / "varc-unrelated-axis-avar2.ttf")
