# The web assembly shaper

If the standard OpenType shaping engine doesn't give you enough flexibility, Harfbuzz allows you to write your own shaping engine in WebAssembly and embed it into your font! Any font which contains a `Wasm` table will be passed to the WebAssembly shaper.

## How to write a shaping engine in Rust

Here are the steps to create an example shaping engine in Rust:

* First, install wasm-pack, which helps us to generate optimized WASM files. It writes some Javascript bridge code that we don't need, but it makes the build and deployment process much easier:

```
cargo install wasm-pack
```

* Now let's create a new library:

```
cargo new --lib my-shaper
```

* We need the target to be a dynamic library, and we're going to use `bindgen` to export our Rust function to WASM, so let's put these lines in the `Cargo.toml`:

```toml
[lib]
crate-type = ["cdylib"]
[dependencies]
wasm-bindgen = "0.2"
```

* And now we'll create our shaper code. In `src/lib.rs`:

```
use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub fn shape(_font_ref: u32, _buf_ref: u32) -> i32 {
    1
}
```

This exports a shaping function which takes two arguments, tokens representing the font and the buffer, and returns a status value. We can pass these tokens back to Harfbuzz in order to use its native functions on the font and buffer objects. More on native functions later - let's get this shaper compiled and added into a font:

* To compile the shaper, run `wasm-pack build --target nodejs`:

```
INFO]: 🎯  Checking for the Wasm target...
[INFO]: 🌀  Compiling to Wasm...
   Compiling my-shaper v0.1.0 (...)
    Finished release [optimized] target(s) in 0.20s
[WARN]: ⚠️   origin crate has no README
[INFO]: ⬇️  Installing wasm-bindgen...
[INFO]: Optimizing wasm binaries with `wasm-opt`...
[INFO]: Optional fields missing from Cargo.toml: 'description', 'repository', and 'license'. These are not necessary, but recommended
[INFO]: ✨   Done in 0.40s
```

You'll find the output WASM file in `pkg/my_shaper_bg.wasm`

* Now we need to get it into a font.

One way to do this is to use [otfsurgeon](https://github.com/simoncozens/font-engineering/blob/master/otfsurgeon), a tool for manipulating binary font tables:

```
% otfsurgeon -i test.ttf add -o test-wasm.ttf Wasm < pkg/my_shaper_bg.wasm
```

And now we can run it!

```
% hb-shape test-wasm.ttf abc --shapers=wasm
[cent=0|sterling=1|fraction=2]
```

(The `--shapers=wasm` isn't necessary, as any font with a `Wasm` table will be sent to the WASM shaper if it's enabled, but it proves the point.)

Congratulations! Our shaper did nothing, but in Rust! Now let's do something - it's time for the Hello World of WASM shaping.

* To say hello world, we're going to have to use a native function.

In debugging builds of Harfbuzz, we can print some output from the web assembly module to the host's standard output using the `debugprint` function:

```
// We don't use #[wasm_bindgen] here because that makes
// assumptions about Javascript calling conventions. We
// really do just want to import some C symbols and run
// them in unsafe-land!
extern "C" {
    pub fn debugprint(s: *const u8);
}
```

And now let's add a function on the Rust side which makes this a bit more ergonomic to use:

```
use std::ffi::CString;

fn print(s: &str) {
    let c_s = CString::new(s).unwrap();
    unsafe {
        debugprint(c_s.as_ptr() as *const u8);
    };
}

#[wasm_bindgen]
pub fn shape(font_ref: i32, buf_ref: i32) -> i32 {
    print("Hello from Rust!\n");
    1
}
```

With this compiled into a WASM module, and installed into our font again, finally our fonts can talk to us!s

```
$ hb-shape test-wasm.ttf abc
Hello from Rust!
[cent=0|sterling=1|fraction=2]
```
