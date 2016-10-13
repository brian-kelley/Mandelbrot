#ifndef MANDELBROT_H
#define MANDELBROT_H

#include "constants.h"
#include "math.h"
#include "time.h"
#include "assert.h"
#include "stdatomic.h"
#include "pthread.h"
#include "kernels.h"
#include "lodepng.h"
#include "fixedpoint.h"
#include "image.h"
#include "timing.h"
#include "complex.h"
#include "x86intrin.h"

void drawBuf();

// float
void drawBufSIMD32();
void* simd32Worker(void* unused);
// double
void drawBufSIMD64();
void* simd64Worker(void* unused);
//arbitrary precision
void* fpWorker(void* wi);

void colorTestDrawBuf();  //draws 2 periods of the color map as a spectrum

void getInterestingLocation(int minExpo, const char* cacheFile, bool useCache);
void writeImage();       //use lodePNG to write out the current conv-rate buffer
void recomputeMaxIter();    //update iteration cap between zooms
//void saveResumeState(const char* fname);
//void loadResumeState(const char* fname);

/* Low level main loop functions */
Uint32 getColor(int num);   //lookup color corresponding to the iteration count of a pixel
//iterate z = z^2 + c, where z = x + yi, return iteration count to escape or -1 if converged
float getPixelConvRate(int x, int y);
float getPixelConvRateSmooth(int x, int y);
//iterate z = z^2 + c, return iteration count
bool upgradePrec();  //given pstride, does precision need to be upgraded

float ssValue(float* outputs);

#endif

