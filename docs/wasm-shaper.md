# The web assembly shaper

If the standard OpenType shaping engine doesn't give you enough flexibility, Harfbuzz allows you to write your own shaping engine in WebAssembly and embed it into your font! Any font which contains a `Wasm` table will be passed to the WebAssembly shaper.

## What you can and can't do: the WASM shaper's role in shaping

The Harfbuzz shaping engine, unlike its counterparts CoreText and DirectWrite, is only responsible for a small part of the text rendering process. Specifically, Harfbuzz is purely responsible for *shaping*; although Harfbuzz does have APIs for accessing glyph outlines, typically other libraries in the free software text rendering stack are responsible for text segmentation into runs, outline scaling and rasterizing, setting text on lines, and so on.

Harfbuzz is therefore restricted to turning a buffer of codepoints for a segmented run of the same script, language, font, and variation settings, into glyphs and positioning them. This is also all that you can do with the WASM shaper; you can influence the process of mapping a string of characters into an array of glyphs, you can determine how those glyphs are positioned and their advance widths, but you cannot manipulate outlines, variations, line breaks, or affect text layout between texts of different font, variation, language, script or OpenType feature selection.

## The WASM shaper interface

The WASM code inside a font is expected to export a function called `shape` which takes five int32 arguments and returns an int32 status value. (Zero for failure, any other value for success.) Three of the five arguments are tokens which can be passed to the API functions exported to your WASM code by the host shaping engine:

* A *shape plan* token, which can largely be ignored.
* A *font* token.
* A *buffer* token.
* A *feature* array.
* The number of features.

The general goal of WASM shaping involves receiving and manipulating a *buffer contents* structure, which is an array of *infos* and *positions* (as defined below). Initially this buffer will represent an input string in Unicode codepoints. By the end of your `shape` function, it should represent a set of glyph IDs and their positions. (User-supplied WASM code will manipulate the buffer through *buffer tokens*; the `buffer_copy_contents` and `buffer_set_contents` API functions, defined below, use these tokens to exchange buffer information with the host shaping engine.)

* The `buffer_contents_t` structure

| type | field | description|
| - | - | - |
| uint32 | length | Number of items (characters or glyphs) in the buffer
| glyph_info_t | infos | An array of `length` glyph infos |
| glyph_position_t | positions | An array of `length` glyph positions |

* The `glyph_info_t` structure

| type | field | description|
| - | - | - |
| uint32 | codepoint | (On input) A Unicode codepoint. (On output) A glyph ID. |
| uint32 | mask | Unused in WASM; can be user-defined |
| uint32 | cluster | Index of start of this graphical cluster in input string |
| uint32 | var1 | Reserved |
| uint32 | var2 | Reserved |

The `cluster` field is used to glyphs in the output glyph stream back to characters in the input Unicode sequence for hit testing, cursor positioning, etc. It must be set to a monotonically increasing value across the buffer.

* The `glyph_position_t` structure

| type | field | description|
| - | - | - |
| int32 | x_advance | X advance of the glyph |
| int32 | y_advance | Y advance of the glyph |
| int32 | x_offset | X offset of the glyph |
| int32 | y_offset | Y offset of the glyph |
| uint32 | var | Reserved |

* The `feature_t` array

To communicate user-selected OpenType features to the user-defined WASM shaper, the host shaping engine passes an array of feature structures:

| type | field | description|
| - | - | - |
| uint32 | tag | Byte-encoded feature tag |
| uint32 | value | Value: 0=off, 1=on, other values used for alternate selection |
| uint32 | start | Index into the input string representing start of the active region for this feature selection (0=start of string) |
| uint32 | end | Index into the input string representing end of the active region for this feature selection (-1=end of string) |

## API functions available

To assist the shaping code in mapping codepoints to glyphs, the WASM shaper exports the following functions. Note that these are the low level API functions; WASM authors may prefer to use higher-level abstractions around these functions, such as the `harfbuzz-wasm` Rust crate provided by Harfbuzz.

### Sub-shaping

* `shape_with`

```C
bool shape_with(
    uint32 font_token,
    uint32 buffer_token,
    feature_t* features,
    uint32 num_features,
    char* shaper
)
```

Run another shaping engine's shaping process on the given font and buffer. The only shaping engine guaranteed to be available is `ot`, the OpenType shaper, but others may also be available. This allows the WASM author to process a buffer "normally", before further manipulating it.

### Buffer access

* `buffer_copy_contents`

```C
bool buffer_copy_contents(
    uint32 buffer_token,
    buffer_contents_t* buffer_contents
)
```

Retrieves the contents of the host shaping engine's buffer into the `buffer_contents` structure. This should typically be called at the beginning of shaping.

* `buffer_set_contents`

```C
bool buffer_set_contents(
    uint32 buffer_token,
    buffer_contents_t* buffer_contents
)
```

Copy the `buffer_contents` structure back into the host shaping engine's buffer. This should typically be called at the end of shaping.

* `buffer_contents_free`

```C
bool buffer_contents_free(buffer_contents_t* buffer_contents)
```

Releases the memory taken up by the buffer contents structure.

* `buffer_contents_realloc`

```C
bool buffer_contents_realloc(
    buffer_contents_t* buffer_contents,
    uint32 size
)
```

Requests that the buffer contents structure be resized to the given size.

* `buffer_get_direction`

```C
uint32 buffer_get_direction(uint32 buffer_token)
```

Returns the buffer's direction:

* 0 = invalid
* 4 = left to right
* 5 = right to left
* 6 = top to bottom
* 7 = bottom to top

* `buffer_get_script`

```C
uint32 buffer_get_script(uint32 buffer_token)
```

Returns the byte-encoded OpenType script tag of the buffer.

* `buffer_reverse`

```C
void buffer_reverse(uint32 buffer_token)
```

Reverses the order of items in the buffer.

* `buffer_reverse_clusters`

```C
void buffer_reverse_clusters(uint32 buffer_token)
```

Reverses the order of items in the buffer while keeping items of the same cluster together.

## Font handling functions

(In the following functions, a *font* is a specific instantiation of a *face* at a particular scale factor and variation position.)

* `font_create`

```C
uint32 font_create(uint32 face_token)
```

Returns a new *font token* from the given *face token*.

* `font_get_face`

```C
uint32 font_get_face(uint32 font_token)
```

Creates a new *face token* from the given *font token*.

* `font_get_scale`

```C
void font_get_scale(
    uint32 font_token,
    int32* x_scale,
    int32* y_scale
)
```

Returns the scale of the current font.

* `font_get_glyph`

```C
uint32 font_get_glyph(
    uint32 font_token,
    uint32 codepoint,
    uint32 variation_selector
)
```

Returns the nominal glyph ID for the given codepoint, using the `cmap` table of the font to map Unicode codepoint (and variation selector) to glyph ID.

* `font_get_glyph_h_advance`/`font_get_glyph_v_advance`

```C
uint32 font_get_glyph_h_advance(uint32 font_token, uint32 glyph_id)
uint32 font_get_glyph_v_advance(uint32 font_token, uint32 glyph_id)
```

Returns the default horizontal and vertical advance respectively for the given glyph ID the current scale and variations settings.

* `font_get_glyph_extents`

```C
typedef struct
{
  uint32 x_bearing;
  uint32 y_bearing;
  uint32 width;
  uint32 height;
} glyph_extents_t;

bool font_get_glyph_extents(
    uint32 font_token,
    uint32 glyph_id,
    glyph_extents_t* extents
)
```

Returns the glyph's extents for the given glyph ID at current scale and variation settings.

* `font_glyph_to_string`

```C
void font_glyph_to_string(
    uint32 font_token,
    uint32 glyph_id,
    char* string,
    uint32 size
)
```

Copies the name of the given glyph, or, if no name is available, a string of the form `gXXXX` into the given string.

* `font_copy_glyph_outline`

```C
typedef struct
{
  float x;
  float y;
  uint32_t type;
} glyph_outline_point_t;

typedef struct
{
  uint32_t n_points;
  glyph_outline_point_t* points;
  uint32_t n_contours;
  uint32_t* contours;
} glyph_outline_t;

bool font_copy_glyph_outline(
    uint32 font_token,
    uint32 glyph_id,
    glyph_outline_t* outline
);
```

Copies the outline of the given glyph ID, at current scale and variation settings, into the outline structure provided. The outline structure returns an array of points (specifying coordinates and whether the point is oncurve or offcurve) and an array of indexes into the points array representing the end of each contour, similar to the `glyf` table structure.

* `font_copy_coords`/`font_set_coords`

```C
typedef struct
{
  uint32 length;
  int32* coords;
} coords_t;

bool font_copy_coords(uint32 font_token, &coords_t coords);
bool font_set_coords(uint32 font_token, &coords_t coords);
```

`font_copy_coords` copies the font's variation coordinates into the given structure; the resulting structure has `length` equal to the number of variation axes, with each member of the `coords` array being a F2DOT14 encoding of the normalized variation value.

`font_set_coords` sets the font's variation coordinates. Because the WASM shaper is only responsible for shaping and positioning, not outline drawing, the user should *not* expect this to affect the rendered outlines; the function is only useful in very limited circumstances, such as when instantiating a second variable font and sub-shaping a buffer using this new font.

## Face handling functions

* `face_create`

```C
typedef struct
{
  uint32_t length;
  char* data;
} blob_t;

uint32 font_get_face(blob_t* blob)
```

Creates a new *face token* from the given binary data.

* `face_copy_table`

```C
void face_copy_table(uint32 face_token, uint32 tag, blob_t* blob)
```

Copies the binary data in the OpenType table referenced by `tag` into the supplied `blob` structure.

* `face_get_upem`

```C
uint32 font_get_upem(uint32 face_token)
```

Returns the units-per-em of the font face.

### Other functions

* `blob_free`

```C
void blob_free(blob_t* blob)
```

Frees the memory allocated to a blob structure.

* `glyph_outline_free`

```C
void glyph_outline_free(glyph_outline_t* glyph_outline)
```

Frees the memory allocated to a glyph outline structure.

* `script_get_horizontal_direction`

```C
uint32 script_get_horizontal_direction(uint32 tag)
```

Returns the horizontal direction for the given ISO 15924 script tag. For return values, see `buffer_get_direction` above.

* `debugprint` / `debugprint1` ... `debugprint4`

```C
void debugprint(char* str)
void debugprint1(char* str, int32 arg1)
void debugprint2(char* str, int32 arg1, int32 arg2)
void debugprint3(char* str, int32 arg1, int32 arg2, int32 arg3)
void debugprint4(
    char* str,
    int32 arg1,
    int32 arg2,
    int32 arg3,
    int32 arg4
)
```

Produces a debugging message in the host shaper's log output; the variants `debugprint1` ... `debugprint4` suffix the message with a comma-separated list of the integer arguments.

## Enabling the WASM shaper when building Harfbuzz

First, you will need the `wasm-micro-runtime` library installed on your computer. Download `wasm-micro-runtime` from [its GitHub repository](https://github.com/bytecodealliance/wasm-micro-runtime/tree/main); then follow [the instructions for building](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.md), except run the cmake command from the repository root directory and add the `-DWAMR_BUILD_REF_TYPES=1` flag to the `cmake` line. (You may want to enable "fast JIT".) Then, install it.

So, for example:

```
$ cmake -B build -DWAMR_BUILD_REF_TYPES=1 -DWAMR_BUILD_FAST_JIT=1
$ cmake --build build --parallel
$ sudo cmake --build build --target install
```

(If you don't want to install `wasm-micro-runtime` globally, you can copy `libiwasm.*` and `libvmlib.a` into a directory that your compiler can see when building Harfbuzz.)

Once `wasm-micro-runtime` is installed, to enable the WASM shaper, you need to add the string `-Dwasm=enabled` to your meson build line. For example:

```
$ meson setup build -Dwasm=enabled
...
  Additional shapers
    Graphite2                 : NO
    WebAssembly (experimental): YES
...
$ meson compile -C build
```

## How to write a shaping engine in Rust

You may write shaping engines in any language supported by WASM, by conforming to the API described above, but Rust is particularly easy, and we have one of those high-level interface wrappers which makes the process easier. Here are the steps to create an example shaping engine in Rust: (These examples can also be found in `src/wasm/sample/rust`)

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
        item.codepoint = font.get_glyph(item.codepoint, 0);
        // Set advance width
        item.x_advance = font.get_glyph_h_advance(item.codepoint);
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
