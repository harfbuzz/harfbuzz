#!/usr/bin/env python3
# Generate a copy of AdobeVFPrototype.abc.otf whose CFF2 Private DICT has a
# StdVW blend with a huge real-number delta.  Partially instancing the font
# folds that delta through the region transform, which used to convert the
# out-of-range result straight to int.  This is a regression fixture for
# the clamp in the CFF2 Private DICT blend rewriter.
import sys
from fontTools.ttLib import TTFont

src = sys.argv[1] if len(sys.argv) > 1 else "fonts/AdobeVFPrototype.abc.otf"
out = sys.argv[2] if len(sys.argv) > 2 else "fonts/AdobeVFPrototype.abc.huge-blend.otf"

font = TTFont(src)
private = font["CFF2"].cff.topDictIndex[0].FDArray[0].Private
std_vw = list(private.rawDict["StdVW"])
std_vw[2] = 1e12
private.rawDict["StdVW"] = std_vw
private.StdVW = std_vw

font.save(out)
print("wrote", out)
