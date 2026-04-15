# harfbuzz-world.cc — site design

## Goal

A public showcase for HarfBuzz at `harfbuzz-world.cc`.  Targets
two audiences:

1. **Gamedevs** who need text in their engine and have been
   living with bitmap-cache solutions.
2. **App / library developers** who ship text-heavy products
   (document editors, readers, browsers, design tools) and want
   to understand when to use shaping, which rendering backend,
   and how to ship embedded fonts.

Tone: technical, honest, show-don't-tell.  Every claim has a
live, interactive widget backing it.

## Non-goals

- Not a tutorial or reference manual — those live in `docs/`.
- Not an advocacy site against other libraries.  Comparisons are
  fair and honest (e.g. "FreeType still wins at hinting; hb-gpu
  wins at scalable resolution-independence").
- Not a blog.  Static pages, evergreen content.

## Sitemap

```
/                             home (live GPU demo + pitch)
  /shaping                    shape text live, show clusters
  /rendering/                 the trio hub
    /rendering/raster         b&w + color raster
    /rendering/vector         SVG / PDF download, live
    /rendering/gpu            embed hb-gpu-web demo
  /color                      color font showcase (all 3 engines)
  /variable                   variable-font axis sliders (all 3)
  /subsetting                 upload font + text, download subset
  /gamedev                    hb-gpu deep dive for engine devs
  /appdev                     rendering-choice guide for app devs
  /docs                       link out to API, GitHub, tarballs
```

## Interactive demos (wasm modules)

Status: only **GPU** has a ready web build (under
`util/gpu/web/`).  The other four demos need to be developed.

| Demo        | Wasm size (est.) | Scope of work                    |
| ----------- | ---------------- | -------------------------------- |
| Shaping     | 200-300 KB       | port `hb-shape` CLI to wasm      |
| Subsetting  | 400-500 KB       | port `hb-subset` CLI to wasm     |
| Raster      | 500-700 KB       | `hb-raster` + canvas blit        |
| Vector      | 600-800 KB       | `hb-vector` + SVG/PDF I/O        |
| GPU         | ~2 MB            | done (`hb-gpu-web` + WebGPU)     |

Approach mirrors `util/gpu/web/build.sh`: single-TU
`harfbuzz-world.cc` compiled per-demo with `em++`, exporting a
narrow C API, loaded via Emscripten `Module`.

## Page patterns

- **Hero demo panel**: every page ships one live widget front
  and center.  Text / font / size inputs are the same controls
  across pages so users can carry a scenario between demos.
- **Canned scenarios**: dropdown of interesting fonts + texts
  (Arabic ligatures, Devanagari reordering, COLRv1 emoji,
  variable-font specimens) so a visitor who doesn't want to
  hunt for a font still sees the full capability.
- **"View output"** affordance: every demo exposes its raw
  output (glyph IDs, PDF bytes, atlas blob) in a collapsible
  panel, for the technical visitor.
- **Permalinks**: encode the current font + text + settings in
  the URL so scenarios are shareable.

## Concrete pages

### Home

- Single paragraph pitch.
- Live WebGL demo: animated variable font, mixed scripts + emoji.
- Three large buttons: "I'm building a game", "I'm building an
  app", "Show me everything".

### Shaping

- Input textarea; output is a table of `glyph_id cluster
  x_offset y_offset x_advance y_advance` rows.
- Visual: shaped glyphs rendered with cluster-colored
  backgrounds to show cluster groupings.
- "Download as JSON" button.
- Canned: Arabic `ش‍ے`, Devanagari `क्षि`, emoji ZWJ clusters.

### Rendering / Raster

- Same input as shaping.  Slider for size.  PNG / PPM output.
- Color toggle: b&w, grayscale, full color.
- "Download PNG" button.

### Rendering / Vector

- Same input.  Output: live `<iframe>` with SVG, "Download PDF"
  button.
- Show the file size -- vector scales to any size with one
  small file.
- Canned: a one-page PDF specimen with three scripts.

### Rendering / GPU

- Embed the existing `hb-gpu-web` demo in an iframe.
- Description of what it's actually doing (no atlas glyph
  bitmaps; every glyph shaded by its outline blob).
- FPS / ms-per-frame counter.

### Color

- Single text + font input.  Three output panes (raster, vector,
  GPU) side by side.
- Visual sameness is the whole point -- HarfBuzz produces
  pixel-compatible output across backends modulo antialiasing.
- Font list: Noto Color Emoji (COLRv1), Bungee Spice, Apple
  Color Emoji (once sbix lands).

### Variable

- Pick a variable font (Roboto Flex, Amstelvar, Bungee).
- Axis sliders (weight, width, optical size, etc.).
- Three renderers run simultaneously, showing the same
  instance baked.  GPU path keeps up at 60 fps; others show the
  re-render cost.

### Subsetting

- Upload font -> textarea for input text -> subset produced.
- Widget reports: input size, output size, percent saved,
  retained glyph count.
- Download subsetted font.
- Pitch paragraph: this is how web fonts (`unicode-range`) and
  PDF embedding are practical.

### Gamedev

- Why text in games is hard (hint: bitmap caches break at any
  scale change or rotation).
- hb-gpu explainer: slug algorithm, atlas layout diagram,
  blob-format overview (deliberately no format details because
  the blob is opaque).
- Integration snippet: minimal GLSL + buffer binding code, C
  side showing `hb_gpu_draw_encode` -> atlas upload -> draw.
- Bench numbers from `--bench` runs on a reference scene.
- Minimum requirements: GL 3.3 / D3D11 / Metal / WebGPU / Vulkan.

### Appdev

- "Which backend do I want?" decision table:

  | Your output is...       | Use...    |
  | ----------------------- | --------- |
  | Bitmap for screen       | raster    |
  | PDF embed               | vector    |
  | Scalable UI / zoom      | gpu       |
  | A single instance image | raster    |
  | Animated text           | gpu       |

- Subsetting pitch.
- Integration overview for the three major language binding
  stories (C, Rust via harfrust, JS via wasm).

## Hosting

- Static site, GitHub Pages or Cloudflare Pages.
- All content lives next to the code in `util/harfbuzz-world-site/`
  or a sibling repo; pick whichever is easier for releasing.
- CDN handles the wasm blobs.

## Implementation order

1. Scaffold the static site structure (HTML templates, shared
   CSS, shared JS for font-picker / text-input controls).
2. Port **Shaping** to wasm.  Smallest, proves the pipeline.
3. Port **Subsetting** -- pure binary I/O, showcases a different
   HarfBuzz story.
4. Port **Raster** (b&w first, then color).
5. Port **Vector** (SVG first, PDF second).
6. Integrate existing **GPU** demo.
7. Write the prose pages (gamedev, appdev, variable, color).
8. Polish, permalinks, canned scenarios.

## Open questions

- Should the site repo live in the HarfBuzz org on GitHub, or
  as its own project?  Probably same org, separate repo, so it
  can release independently.
- Do we host the downloadable subsetted fonts server-side, or
  entirely client-side via wasm?  Client-side is cleaner (no
  server, pure static) but requires shipping `libharfbuzz-subset`
  in the page.  Probably worth it.
- Font licensing for canned scenarios: which Open Font License
  fonts do we bundle, vs load-on-demand from a Google Fonts
  / Fontsource CDN?
- Analytics: none, or lightweight privacy-respecting (e.g.
  Plausible).  None is fine initially.
