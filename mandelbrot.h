#ifndef MANDELBROT_H
#define MANDELBROT_H

#include "math.h"
#include "time.h"
#include "assert.h"
#include "unistd.h"
#include "pthread.h"
#include "lodepng.h"

#include "constants.h"
#include "kernels.h"
#include "fixedpoint.h"
#include "image.h"
#include "timing.h"
#include "x86intrin.h"

typedef void (*ColorMap)(void);

extern int winw;
extern int winh;
extern float* iters;
extern unsigned* frameBuf;
extern int lastOutline;
extern FP targetX;
extern FP targetY;
extern FP pstride;
extern int zoomDepth;
extern int numThreads;
extern int maxiter;
extern int prec;
extern int deepestExpo;
extern bool smooth;
extern bool verbose;
extern bool supersample;
extern int zoomRate;
extern int pixelsComputed;
extern const char* outputDir;
extern pthread_t monitor;
extern ColorMap colorMap;

//Compute framebuffer (and colors if applicable)
MANDELBROT_API void drawBuf();

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
//Precision updates: update pstride, and update targetX and targetY if interactive.
MANDELBROT_API void upgradePrec(bool interactive);    //upgrade precision if needed
MANDELBROT_API void downgradePrec(bool interactive);  //downgrade precision if possible

MANDELBROT_API const char* getPrecString();

float ssValue(float* outputs);

u64 totalIters();

//total number of chars to use for CLI progress monitor
#define MONITOR_WIDTH 79
void* monitorFunc(void* unused);

//color schemes
//color function to use (cli-configurable)
void colorSunset();
void colorGalaxy();

#endif

