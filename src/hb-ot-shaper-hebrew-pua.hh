#ifndef HB_OT_SHAPER_HEBREW_PUA_HH
#define HB_OT_SHAPER_HEBREW_PUA_HH

#include "hb.hh"


static inline uint16_t
_hb_hebrew_pua_map (unsigned u)
{
  if ((u >= 0x0020u && u <= 0x0040u) ||
      (u >= 0x005Bu && u <= 0x005Fu) ||
      (u >= 0x007Bu && u <= 0x007Eu))
    return 0xF000u + u;

  if ((u >= 0x05B0u && u <= 0x05C3u) ||
      (u >= 0x05D0u && u <= 0x05EAu))
    return 0xEB10u + u;

  if (u >= 0x2074u && u <= 0x2079u)
    return 0xD010u + u;

  switch (u)
  {
    case 0x00A3u: return 0xF0A3u;
    case 0x00A7u: return 0xF0A7u;
    case 0x00B0u: return 0xF0B0u;
    case 0x00B2u: return 0xF082u;
    case 0x00B3u: return 0xF083u;
    case 0x00B7u: return 0xF0B7u;
    case 0x00B9u: return 0xF081u;
    case 0x00BCu: return 0xF0BCu;
    case 0x00BDu: return 0xF0BDu;
    case 0x00BEu: return 0xF0BEu;
    case 0x00D7u: return 0xF0AAu;
    case 0x00F7u: return 0xF0BAu;
    case 0x200Eu: return 0xF0FDu;
    case 0x200Fu: return 0xF0FEu;
    case 0x2070u: return 0xF080u;
    case 0x20AAu: return 0xF0A4u;
    default:      return 0;
  }
}


#endif /* HB_OT_SHAPER_HEBREW_PUA_HH */
