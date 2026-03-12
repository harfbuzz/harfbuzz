#ifndef HB_NO_FEATURES_H
#include "hb.h"
#include "hb-features.h"
#endif

#ifdef HB_HAS_CORETEXT
#ifndef HAVE_CORETEXT
#define HAVE_CORETEXT 1
#endif
#endif

#ifdef HB_HAS_DIRECTWRITE
#ifndef HAVE_DIRECTWRITE
#define HAVE_DIRECTWRITE 1
#endif
#endif

#ifdef HB_HAS_FREETYPE
#ifndef HAVE_FREETYPE
#define HAVE_FREETYPE 1
#endif
#endif

#ifdef HB_HAS_GDI
#ifndef HAVE_GDI
#define HAVE_GDI 1
#endif
#endif

#ifdef HB_HAS_GLIB
#ifndef HAVE_GLIB
#define HAVE_GLIB 1
#endif
#endif

#ifdef HB_HAS_GRAPHITE
#ifndef HAVE_GRAPHITE2
#define HAVE_GRAPHITE2 1
#endif
#endif

#ifdef HB_HAS_UNISCRIBE
#ifndef HAVE_UNISCRIBE
#define HAVE_UNISCRIBE 1
#endif
#endif

#ifdef HB_HAS_WASM
#ifndef HAVE_WASM
#define HAVE_WASM 1
#endif
#endif

#ifdef HB_HAS_CAIRO
#ifndef HAVE_CAIRO
#define HAVE_CAIRO 1
#endif
#endif

#ifdef HB_HAS_ICU
#ifndef HAVE_ICU
#define HAVE_ICU 1
#endif
#endif

#include "harfbuzz.cc"

#if defined(HB_HAS_SUBSET)
#include "graph/gsubgpos-context.cc"
#include "hb-subset-cff-common.cc"
#include "hb-subset-cff1.cc"
#include "hb-subset-cff2-to-cff1.cc"
#include "hb-subset-cff2.cc"
#include "hb-subset-input.cc"
#include "hb-subset-instancer-iup.cc"
#include "hb-subset-instancer-solver.cc"
#include "hb-subset-plan-layout.cc"
#include "hb-subset-plan-var.cc"
#include "hb-subset-plan.cc"
#include "hb-subset-serialize.cc"
#include "hb-subset-table-cff.cc"
#include "hb-subset-table-color.cc"
#include "hb-subset-table-layout.cc"
#include "hb-subset-table-other.cc"
#include "hb-subset-table-var.cc"
#include "hb-subset.cc"
#endif

#if defined(HB_HAS_RASTER)
#include "hb-raster-draw.cc"
#include "hb-raster-image.cc"
#include "hb-raster-paint.cc"
#include "hb-raster-svg-base.cc"
#include "hb-raster-svg-bbox.cc"
#include "hb-raster-svg-clip.cc"
#include "hb-raster-svg-color.cc"
#include "hb-raster-svg-defs-scan.cc"
#include "hb-raster-svg-defs.cc"
#include "hb-raster-svg-fill.cc"
#include "hb-raster-svg-gradient.cc"
#include "hb-raster-svg-parse.cc"
#include "hb-raster-svg-render.cc"
#include "hb-raster-svg-use.cc"
#include "hb-raster.cc"
#include "hb-zlib.cc"
#endif

#if defined(HB_HAS_VECTOR)
#include "hb-vector-svg-draw.cc"
#include "hb-vector-svg-paint.cc"
#include "hb-vector-svg-path.cc"
#include "hb-vector-svg-subset.cc"
#include "hb-vector-svg-utils.cc"
#include "hb-vector.cc"
#include "hb-zlib.cc"
#endif

#if defined(HB_HAS_CAIRO)
#include "hb-cairo-utils.cc"
#include "hb-cairo.cc"
#endif

#if defined(HB_HAS_ICU)
#include "hb-icu.cc"
#endif

