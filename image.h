#ifndef IMAGE_H
#define IMAGE_H

#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include "stdbool.h"
#include "assert.h"
#include "limits.h"

typedef uint32_t Uint32;

#ifndef max
#define max(a, b) (a < b ? b : a)
#define min(a, b) (a < b ? a : b)
#endif

extern float* imgScratch;

//if not otherwise specified, iter value range [0, 1000) is one full cycle of a color spectrum
//is used by histogram colorings where range is not specified by caller
#define DEFAULT_CYCLE_LEN 1000

typedef double (*Mapping) (double orig);

typedef struct
{
  float* iters;
  Uint32* fb;
  int w;
  int h;
  Uint32* palette;
  int numColors;
  double cycles;     //number of repeats of the colormap in one image
  int period;
} Image;

//constant = how much color is borrowed from each neighbor (i.e. 0.1 means multiply neighbors by 0.1, and old color by 0.6)
void blockFilter(double constant, Uint32* buf, int w, int h);
void reduceIters(int* iterbuf, int diffCap, int w, int h);

//Linear interpolation (lerp) for colors
//k is a float/double between 0 and 1
Uint32 lerp(Uint32 c1, Uint32 c2, double k);

void handleNonColored(Image* im);

float getPercentileValue(float* buf, int w, int h, float proportion);

// *** Color options, many combinations ***//
// Exponential, logarithmic, histogram
// Banded or smooth
// Non-weighted or weighted

void colorExpoCyclic(Image* im, double expo);
void colorLogCyclic(Image* im);
void colorHist(Image* im);
void colorHistWeighted(Image* im, double* weights);

#endif

