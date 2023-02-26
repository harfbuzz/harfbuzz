# The web assembly shaper

If the standard OpenType shaping engine doesn't give you enough flexibility, Harfbuzz allows you to write your own shaping engine in WebAssembly and embed it into your font! Any font which contains a `Wasm` table will be passed to the WebAssembly shaper.

## How to write a shaping engine in Rust

Here are the steps to create an example shaping engine in Rust: (These examples can also be found in `src/wasm/sample/rust`)

* First, install wasm-pack, which helps us to generate optimized WASM files. It writes some Javascript bridge code that we don't need, but it makes the build and deployment process much easier:

```
$ cargo install wasm-pack
```

* Now let's create a new library:

```
$ cargo new --lib hello-wasm
```

* We need the target to be a dynamic library, and we're going to use `bindgen` to export our Rust function to WASM, so let's put these lines in the `Cargo.toml`. The Harfbuzz sources contain a Rust crate which makes it easy to create the shaper, so we'll specify that as a dependency as well:

```toml
[lib]
crate-type = ["cdylib"]
[dependencies]
wasm-bindgen = "0.2"
harfbuzz-wasm = { path = "your-harfbuzz-source/src/wasm/rust/harfbuzz-wasm"}
```

*
* And now we'll create our shaper code. In `src/lib.rs`:

```rust
use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub fn shape(_shape_plan:u32, font_ref: u32, buf_ref: u32, _features: u32, _num_features: u32) -> i32 {
    1 // success!
}
```

This exports a shaping function which takes four arguments, tokens representing the shaping plan, the font and the buffer, and returns a status value. We can pass these tokens back to Harfbuzz in order to use its native functions on the font and buffer objects. More on native functions later - let's get this shaper compiled and added into a font:

* To compile the shaper, run `wasm-pack build --target nodejs`:

```
INFO]: 🎯  Checking for the Wasm target...
[INFO]: 🌀  Compiling to Wasm...
   Compiling hello-wasm v0.1.0 (...)
    Finished release [optimized] target(s) in 0.20s
[WARN]: ⚠️   origin crate has no README
[INFO]: ⬇️  Installing wasm-bindgen...
[INFO]: Optimizing wasm binaries with `wasm-opt`...
[INFO]: Optional fields missing from Cargo.toml: 'description', 'repository', and 'license'. These are not necessary, but recommended
[INFO]: ✨   Done in 0.40s
```

You'll find the output WASM file in `pkg/hello_wasm_bg.wasm`

* Now we need to get it into a font.

We provide a utility to do this called `addTable.py` in the `src/` directory:

```
% python3 ~/harfbuzz/src/addTable.py test.ttf test-wasm.ttf pkg/hello_wasm_bg.wasm
```

And now we can run it!

```
% hb-shape test-wasm.ttf abc --shapers=wasm
[cent=0|sterling=1|fraction=2]
```

(The `--shapers=wasm` isn't necessary, as any font with a `Wasm` table will be sent to the WASM shaper if it's enabled, but it proves the point.)

Congratulations! Our shaper did nothing, but in Rust! Now let's do something - it's time for the Hello World of WASM shaping.

* To say hello world, we're going to have to use a native function.

In debugging builds of Harfbuzz, we can print some output from the web assembly module to the host's standard output using the `debug` function. To make this easier, we've got the `harfbuzz-wasm` crate:

```rust
use harfbuzz_wasm::debug;

#[wasm_bindgen]
pub fn shape(_shape_plan:u32, _font_ref: u32, _buf_ref: u32, _features: u32, _num_features: u32) -> i32 {
    debug("Hello from Rust!\n");
    1
}
```

With this compiled into a WASM module, and installed into our font again, finally our fonts can talk to us!

```
$ hb-shape test-wasm.ttf abc
Hello from Rust!
[cent=0|sterling=1|fraction=2]
```

Now let's start to do some actual, you know, *shaping*. The first thing a shaping engine normally does is (a) map the items in the buffer from Unicode codepoints into glyphs in the font, and (b) set the advance width of the buffer items to the default advance width for those glyphs. We're going to need to interrogate the font for this information, and write back to the buffer. Harfbuzz provides us with opaque pointers to the memory for the font and buffer, but we can turn those into useful Rust structures using the `harfbuzz-wasm` crate again:

```rust
use wasm_bindgen::prelude::*;
use harfbuzz_wasm::{Font, GlyphBuffer};

#[wasm_bindgen]
pub fn shape(_shape_plan:u32, font_ref: u32, buf_ref: u32, _features: u32, _num_features: u32) -> i32 {
    let font = Font::from_ref(font_ref);
    let mut buffer = GlyphBuffer::from_ref(buf_ref);
    for mut item in buffer.glyphs.iter_mut() {
        // Map character to glyph
        item.codepoint = font.get_glyph(codepoint, 0);
        // Set advance width
        item.h_advance = font.get_glyph_h_advance(item.codepoint);
    }
    1
}
```

The `GlyphBuffer`, unlike in Harfbuzz, combines positioning and information in a single structure, to save you having to zip and unzip all the time. It also takes care of marshalling the buffer back to Harfbuzz-land; when a GlyphBuffer is dropped, it writes its contents back through the reference into Harfbuzz's address space. (If you want a different representation of buffer items, you can have one: `GlyphBuffer` is implemented as a `Buffer<Glyph>`, and if you make your own struct which implements the `BufferItem` trait, you can make a buffer out of that instead.)

One easy way to write your own shapers is to make use of OpenType shaping for the majority of your shaping work, and then make changes to the pre-shaped buffer afterwards. You can do this using the `Font.shape_with` method. Run this on a buffer reference, and then construct your `GlyphBuffer` object afterwards:

```rust
use harfbuzz_wasm::{Font, GlyphBuffer};
use tiny_rng::{Rand, Rng};
use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub fn shape(_shape_plan:u32, font_ref: u32, buf_ref: u32, _features: u32, _num_features: u32) -> i32 {
    let mut rng = Rng::from_seed(123456);

    // Use the default OpenType shaper
    let font = Font::from_ref(font_ref);
    font.shape_with(buf_ref, "ot");

    // Now we have a buffer with glyph ids, advance widths etc.
    // already filled in.
    let mut buffer = GlyphBuffer::from_ref(buf_ref);
    for mut item in buffer.glyphs.iter_mut() {
        // Randomize it!
        item.x_offset = ((rng.rand_u32() as i32) >> 24) - 120;
        item.y_offset = ((rng.rand_u32() as i32) >> 24) - 120;
    }

    1
}
```

See the documentation for the `harfbuzz-wasm` crate for all the other
