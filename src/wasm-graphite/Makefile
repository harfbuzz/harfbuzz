all: test.wasm.ttf

%.wasm: %.cc ../hb-wasm-api.h
	emcc \
		-I .. \
		-I ~/graphite/include/ \
		-fvisibility=hidden \
		-Wl,--allow-undefined \
		-Wl,--no-entry \
		-sERROR_ON_UNDEFINED_SYMBOLS=0 \
		~/graphite/src/libgraphite2.a \
		~/wasm/wasi-sdk-19.0/share/wasi-sysroot/lib/wasm32-wasi/libc.a \
		$< \
		-o $@

%.wasm.ttf: %.ttf shape.wasm addTable.py
	python addTable.py $< $@ shape.wasm

clean:
	$(RM) test.wasm.ttf shape.wasm

.PRECIOUS: shape.wasm
