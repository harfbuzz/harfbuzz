# HarfBuzz World — site design

> *The ffmpeg of text rendering.*

Public showcase for HarfBuzz at `harfbuzz-world.cc`.  Positions
HarfBuzz as the text-rendering equivalent of ffmpeg: a universal
engine that just works, that every serious product ends up using.

## Goal

Convince game-engine developers and app developers that
HarfBuzz handles the text-rendering problem end-to-end
(shaping, subsetting, raster / vector / GPU rendering, color
fonts, variable fonts) well enough that they don't have to
roll their own.  Key outcome for a visitor: they leave knowing
where to go to integrate HarfBuzz with their project.

## Non-goals

- Not a tutorial or reference manual — those live in `docs/`.
- Not an advocacy site that names competitors.  Quality claims
  stand on their own.
- Not a blog; static, evergreen pages.
- Not a monitored product — no analytics, no tracking.

## Audience

- **Gamedevs** who've been living with bitmap-cache glyph
  atlases that break the moment anything scales or rotates.
  This is the priority audience -- hb-gpu is the freshest
  story and most under-served market.
- **App / library developers** shipping text-heavy products
  (editors, readers, browsers, design tools) who want
  concrete answers on shaping, subsetting, and embedding.

Homepage does not bifurcate audiences explicitly (no
"gamedev / appdev" header buttons).  The tile grid speaks for
itself; each tile's explainer shapes the narrative for its own
audience.

## Current limitation shaping the demos

HarfBuzz does not yet do text itemization or font fallback
(planned later this year).  Every demo is therefore
single-font, single-script, single-direction per line /
paragraph.  Multi-script hero scenarios are achieved by
rotating samples or by running several (font, text) pairs
side by side — not by mixing scripts in one line.

## Sitemap

```
/                             homepage: logo + grid of 6 tiles
  /shape                      shape text, show clusters + glyph table
  /subset                     font + text → shrunk font, downloadable
  /varfonts                   axis sliders, live re-render
  /raster                     b&w + color raster PPM/PNG
  /vector                     SVG / PDF download, live
  /gpu                        live 3D text, hb-gpu WebGL demo
  /integration                how to ship HarfBuzz in your project
  /about                      credits, links, license
```

Shape, subset, and varfonts are the "why HarfBuzz" pages.
Raster, vector, gpu are the "how you render" pages.
Integration is the conversion surface — see below.

## Visual / brand

- Wordmark: **HarfBuzz World**
- Logo: globe wrapped in multi-script letterforms, "حرف‌باز"
  cresting the top, navy on white.  (Working concept; not
  locked.)
- Site typography: PT Sans body, Anton title, Roboto Mono
  code, Prata headings.  Mirrors Behdad's personal site.
- Visual style: product-landing polish (Stripe / Linear
  cleanliness) with the warmth of behdad.org.
- **Dark mode**: yes.  Default light (logo contrast), auto-
  switch via `prefers-color-scheme: dark`.  Demos must look
  good in both since foreground / background are user-
  controlled anyway.

## Homepage

- Logo + `HarfBuzz World` wordmark.
- Tagline: *The ffmpeg of text rendering.*
- Grid of six tiles (order TBD):

  **shape · subset · varfonts · raster · vector · gpu**

- Each tile is a large icon + label + (on hover) a one-line
  description.  On load: static thumbnail rendered at build
  time (see below).  On click: navigates to the demo page.
- No preselected hero — the grid itself is the invitation.
- Mobile-responsive (WebGL, not WebGPU, for better
  small-screen compat).

## Interactive demos

Only the **GPU** web demo is ready today (`util/gpu/web/`).
Shape, subset, raster, vector, varfonts need wasm bindings +
UI.

### One combined wasm, not six separate bundles

Shipping separate wasms would duplicate the ~500 KB HarfBuzz
core six times and push ~4 MB across a full visit.  Build one
combined wasm from `harfbuzz-world.cc` (the single-TU file
that already includes every sub-library), ~1.5 MB total.

| Component          | Size (est.) |
| ------------------ | ----------- |
| Core + shaping     | ~500 KB     |
| + subsetter        | +200 KB     |
| + raster           | +100 KB     |
| + vector           | +150 KB     |
| + gpu              | +500 KB     |
| **combined**       | **~1.5 MB** |

### Loading strategy

- Homepage renders with static content + logo + grid,
  no wasm.
- Tiles show build-time thumbnails (see below).
- First demo interaction triggers wasm download; browser
  caches it for every page afterwards.
- Optional: prefetch on hover over a demo tile so typing
  into a demo never shows a spinner.

### Build-time thumbnails

Each tile's fallback image is generated headlessly at build
time.  For raster / vector / shape / subset / varfonts, use
`hb-raster` output.  For the GPU tile, start with the same
`hb-raster` output (the tile just needs to say "here's
text"); upgrade later to Mesa-llvmpipe-backed `hb-gpu -o` if
we want the GPU path's distinct visual in the thumbnail.

## Demo pages — patterns

Every demo page:

- **Hero widget**: live interactive demo, takes the page's
  top half.
- **Structured explainer** next to the widget:
  - 2 lines: what this demo calls.
  - A short C snippet (syntax-highlighted via Prism.js) —
    the minimum integration code.
  - Link to API docs in `docs/`.
- **Canned scenarios** dropdown: 3+ presets per demo
  (Arabic ligature, Devanagari reordering, emoji ZWJ
  clusters, variable-font weight sweep, etc.).
- **Permalink**: all demo state (font, text, size, backend,
  color / background, variable-font axis values) encoded in
  the URL for sharing.
- **Upload UX**: drag-drop zone, file picker, paste-URL
  input.  No hard size limit.  Consistent across demos.
- **"View output"** affordance: collapsible panel exposing
  the raw output (glyph IDs, PDF bytes, atlas blob) for
  technical visitors.

### shape

Renders via hb-raster so the user sees beautiful shaped text,
with optional overlays (cluster coloring, glyph-ID labels,
baseline / bounding-box guides) and a collapsible glyph table.
The page frames the story of "Unicode → glyphs".  Canned
scenarios showcase complex scripts.

### subset

Drag-drop a font, type an input string, the widget shows
input size / output size / percent saved / retained glyph
count.  Download button for the subsetted font.  Pitch
paragraph explains web-font `unicode-range` and PDF embedding.

### varfonts

Axis sliders on a variable font, live re-render through
hb-raster.  (Showcases that shaping and re-rendering at each
axis position is cheap.)

### raster

Input + font + size, output as PPM/PNG into a canvas.  Color
toggle.  Download.

### vector

Input + font + size, output as SVG (embedded via iframe) and
PDF (downloadable).  Show file size — "one small file renders
at any resolution."

### gpu

Embeds the existing `hb-gpu-web` demo.  Text is rendered via
outline blobs plus a slug shader — no CPU rasterization per
frame.  FPS counter.

### integration

Not a demo — the "OK I'm convinced, how do I use this?" page.
This is the site's primary conversion surface (visitor leaves
knowing where to start).

Content:

- **Package managers** matrix: one-liner install commands for
  apt, brew, pacman, vcpkg, nix, and friends.
- **Language bindings** row: C (native), Rust (harfrust),
  Python (uharfbuzz), JS (wasm via harfbuzz-world), Go, Ruby,
  etc.
- **Drop-in single-TU**: compile the one
  `harfbuzz-world.cc` file into your project — no build
  system required.  Eat our own dog food: the site itself is
  built this way.
- **CMake / Meson snippets** for the "I already have a build
  system" case.
- Deep links to `BUILD.md` for the full matrix and to
  `docs/` for API reference.

Placement (the page is not a tile, so it needs explicit
findability):

- **Header nav** — always visible: *Demos · Integration · About.*
- **Homepage after-grid CTA** — below the tile grid: a
  prominent "Ready to integrate? →" card.
- **Per-demo cross-link** — every demo's structured
  explainer already links to the API docs; add a second
  link, "→ How to integrate this in your project."

## Presets

Six canned scenarios per demo, chosen for script-family
coverage and distinctive shaping stories:

**Latin · Arabic · Devanagari · Chinese · Hangul · Emoji**

- Latin demonstrates kerning, standard ligatures, and
  auto-fractions.
- Arabic demonstrates cursive joining and ligatures.
- Devanagari demonstrates cluster reordering.
- Chinese covers CJK coverage and dense-layout rendering.
- Hangul demonstrates syllable composition from jamos.
- Emoji demonstrates COLRv1 color rendering.

Rationale: each preset exercises a unique shaping feature,
no two scripts overlap.  Japanese vertical is the dream add
but license-blocked on the only good cursive font.

Presentation: a **left-column rail** of the 5 inactive presets
as clickable icons, with the active preset's live demo
occupying the main stage.  Clicking a rail tile swaps it into
the stage and the displaced preset drops back into the rail.
Mobile adaptation: horizontal scroll strip above the stage
instead of a left column.

Each preset's data (font reference, text string, additional
settings) lives in `presets.json` (or `presets/*.json`, TBD).
Contributor-friendly: add or refine a scenario by opening a
PR that edits one file.  Fonts referenced by path (bundled)
or URL (remote).  Presets fetched on demand so they don't
bloat the initial wasm download.

## Repository layout

Sibling repo: **`harfbuzz/harfbuzz-world.cc`**.

- Independent release cadence from HarfBuzz itself.
- Static site deployable via **GitHub Pages**.
- Auto-deploy on push to `main` via GitHub Actions (wasm
  build + thumbnail build + deploy).

Directory sketch:

```
harfbuzz-world.cc/
  src/                         HTML templates, CSS, JS
  wasm/                        emscripten build output
  fonts/                       bundled OFL fonts
  presets/                     canned scenario JSON
  thumbnails/                  build-generated tile images
  scripts/                     build + thumbnail scripts
  .github/workflows/           CI
  README.md
  LICENSE
```

## Fonts

Bundle OFL fonts that cover the scripts / features we demo.
Starting list (iterate later):

- Roboto Flex (variable, Latin)
- Noto Color Emoji (COLRv1)
- Noto Sans Arabic
- Noto Sans Devanagari
- Noto Sans CJK (subset)
- Bungee Spice (color variable)
- Amstelvar (variable, design)

Font attribution via tooltip on the font-name UI in each
demo.  OFL notice in the footer plus `/about`.

## MVP (v1)

Ship when these work end to end:

- Homepage: logo + 6-tile grid + build-time thumbnails.
- GPU tile: existing hb-gpu-web demo rehoused.
- Shape / subset / raster / vector / varfonts: minimal
  working widgets.
- Structured explainer + one C snippet next to every demo.
- `presets.json` with ~3 canned scenarios per tile.
- Footer + `/about`.

Defer to v1.1: syntax-highlighting polish, tabs for
C / Rust / JS / Python snippets, per-demo social previews,
richer permalink UX, keyboard shortcuts, screen-reader audit.

## Open questions

- **Launch timing**: ship when ready.  Itemization /
  fallback lands "later this year"; we may ship before or
  after depending on appetite.  Early release lets us collect
  feedback; waiting lets us show multi-script paragraphs in
  one line.
- **Localization**: English only at launch.  i18n structure
  ready so a Chinese or other PR can land in v1.1.
- **Browsers without WebGL / wasm**: fall back to static
  thumbnails.  Rare enough that a graceful "your browser
  can't run the live demo" message is fine.
- **Font-license attribution placement**: tooltip first;
  revisit if designers push back.
- **Logo**: working concept is a globe-with-scripts mark;
  not locked yet.

## Stakeholders

Copy, curation, and implementation: Behdad, Khaled, Claude
(as coding assistant).  Community PRs welcome on presets,
copy tweaks, and translations.
