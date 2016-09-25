#ifndef COLOR_H
#define COLOR_H

#include "image.h"
#include "assert.h"
#include "math.h"

//Linear interpolation (lerp) for colors
//k is a float/double between 0 and 1
Uint32 lerp(Uint32 c1, Uint32 c2, double k);

//Macro to create a cyclic color generating function

#define COLOR_FUNC(name, colors, num, period) \
static Uint32 name(int n) \
{ \
  n = (n + period) % period; \
  double segsize = (double) period / num; \
  int seg = (double) n / segsize; \
  double k = 1.0 - fmod(n, segsize) / segsize; \
  return lerp(colors[seg], colors[(seg + 1) % num], k); \
}


#endif
