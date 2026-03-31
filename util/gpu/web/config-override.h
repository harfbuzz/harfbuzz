/* Re-enable features that HB_LEAN disables but the GPU demo needs. */

#undef HB_NO_DRAW     /* GPU encoder uses draw API */
#undef HB_NO_METRICS  /* Glyph advances / extents */
