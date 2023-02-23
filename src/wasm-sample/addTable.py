from fontTools.ttLib import TTFont
from fontTools.ttLib.tables.DefaultTable import DefaultTable

font = TTFont("test.ttf")

wasm_table = DefaultTable("Wasm")
wasm_table.data = open("shape.so", "rb").read()

font["Wasm"] = wasm_table

font.save("test.wasm.ttf")
