FONTS = CharisSIL-R.wasm.ttf Scheherazade-R.wasm.ttf AwamiNastaliq-Regular.wasm.ttf
ADD_TABLE = ../../addTable.py

all: $(FONTS)

# The module must import nothing but the HarfBuzz API, and must not export
# malloc and free: HarfBuzz does not run a module that does either, unless
# built with HB_NO_WASM_BUDGET.  shape.cc defines the WASI calls that the C
# library would otherwise import.
#
# The module is interpreted, so every instruction counts.  For -flto to reach
# into Graphite, build libgraphite2.a with -O3 -flto as well.
%.wasm: %.cc ../../hb-wasm-api.h
	emcc \
		-O3 -flto \
		-I ../.. \
		-I ~/graphite/include/ \
		-fvisibility=hidden \
		-Wl,--allow-undefined \
		-Wl,--no-entry \
		-Wl,--strip-all \
		-sERROR_ON_UNDEFINED_SYMBOLS=0 \
		~/graphite/src/libgraphite2.a \
		~/wasm/wasi-sdk-19.0/share/wasi-sysroot/lib/wasm32-wasi/libc.a \
		$< \
		-o $@


%.wasm.ttf: %.ttf shape.wasm $(ADD_TABLE)
	python $(ADD_TABLE) $< $@ shape.wasm

clean:
	$(RM) shape.wasm $(FONTS)

.PRECIOUS: shape.wasm
