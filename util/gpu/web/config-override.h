/* Re-enable features that HB_LEAN disables but the GPU demo needs. */

#undef HB_NO_DRAW     /* GPU encoder uses draw API */
#undef HB_NO_METRICS  /* Glyph advances / extents */
#undef HB_NO_COLOR    /* COLR palette / layers for paint encoder */
#undef HB_NO_PAINT    /* Paint pipeline the paint encoder walks */
#undef HB_NO_VAR      /* hb_font_set_variations for live axis updates */
