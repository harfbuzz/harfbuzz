all: test.wasm.ttf

%.so: %.c ../hb-wasm-api.h
	clang \
		--target=wasm32-unknown-wasi \
		-Wl,--no-entry \
		-fvisibility=hidden \
		-Wl,--allow-undefined \
		-nostdlib \
		-I .. \
		$< \
		-o $@

%.wasm.ttf: %.ttf shape.so addTable.py
	python addTable.py $< $@ shape.so

clean:
	$(RM) test.wasm.ttf shape.so

.PRECIOUS: shape.so
