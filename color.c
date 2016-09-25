#include "color.h"

Uint32 lerp(Uint32 c1, Uint32 c2, double k)
{
  Uint32 red = ((c1 & 0xFF000000) >> 24) * k + ((c2 & 0xFF000000) >> 24) * (1 - k);
  Uint32 grn = ((c1 & 0xFF0000) >> 16) * k + ((c2 & 0xFF0000) >> 16) * (1 - k);
  Uint32 blu = ((c1 & 0xFF00) >> 8) * k + ((c2 & 0xFF00) >> 8) * (1 - k);
  return (red << 24) | (grn << 16) | (blu << 8) | 0xFF;
}
