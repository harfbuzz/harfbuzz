/* Configuration for HarfBuzz web demo builds (Emscripten).
 * HB_LEAN strips most features; config-override.h adds back
 * what the GPU demo needs. */

#define HAVE_ROUND 1
#define HB_LEAN 1
#define HB_HAS_GPU 1
#define HAVE_CONFIG_OVERRIDE_H 1
