#include "hb-fontations.h"

extern "C" void _hb_fontations_font_set_funcs (hb_font_t *font);

void
hb_fontations_font_set_funcs (hb_font_t *font)
{
  _hb_fontations_font_set_funcs (font);
}
