all: test-fallback.wasm.ttf test-ot.wasm.ttf

%.wasm: %.cc ../hb-wasm-api.h
	clang \
		--target=wasm32-unknown-wasi \
		-Wl,--no-entry \
		-fvisibility=hidden \
		-Wl,--allow-undefined \
		-nostdlib \
		-I .. \
		$< \
		-o $@

test-fallback.wasm.ttf: test.ttf shape-fallback.wasm addTable.py
	python addTable.py $< $@ shape-fallback.wasm

test-ot.wasm.ttf: test.ttf shape-ot.wasm addTable.py
	python addTable.py $< $@ shape-ot.wasm

clean:
	$(RM) test-fallback.wasm.ttf test-ot.wasm.ttf shape-fallback.wasm shape-ot.wasm

.PRECIOUS: *.wasm
