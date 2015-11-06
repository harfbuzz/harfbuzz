/* hb-msvc-compat.h: Compatibility Header for MSVC to build Harfbuzz Utilities */

#if (_MSC_VER < 1800)
#include <math.h>

#ifndef FLT_RADIX
#define FLT_RADIX 2
#endif

__inline long double scalbn (long double x, int exp)
{
  return x * (pow ((long double) FLT_RADIX, exp));
}

__inline float scalbnf (float x, int exp)
{
  return x * (pow ((float) FLT_RADIX, exp));
}
#endif
