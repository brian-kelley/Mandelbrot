#ifndef IMAGE_H
#define IMAGE_H

#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include "stdbool.h"

typedef uint32_t Uint32;

#ifndef max
#define max(a, b) (a < b ? b : a)
#define min(a, b) (a < b ? a : b)
#endif

//constant = how much color is borrowed from each neighbor (i.e. 0.1 means multiply neighbors by 0.1, and old color by 0.6)
void blockFilter(float constant, Uint32* buf, int w, int h);
void reduceIters(int* iterbuf, int diffCap, int w, int h);
void histogramFilter(int* iterbuf, int w, int h);
void logarithmFilter(int* iterbuf, int w, int h);

#endif
