#ifndef KERNELS_H
#define KERNELS_H

#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "fixedpoint.h"
#include "image.h"
#include "x86intrin.h"

extern int maxiter;
extern int prec;

//Given integer escape time, c and z, get smooth (continuous) escape time
float smoothEscapeTime(float iters, double zr, double zi, double cr, double ci);

//Single-pixel escape time: arbitrary precision
float escapeTimeFP(FP* real, FP* imag);
float escapeTimeFPSmooth(FP* real, FP* imag);

//Single-pixel escape time: double (64) and long double (80)
float escapeTime64(double real, double imag);
float escapeTime64Smooth(double real, double imag);
float escapeTime80(long double real, long double imag);
float escapeTime80Smooth(long double real, long double imag);

// SIMD escape time: 8x32 and 4x64
// note: real, imag must be aligned to 32
void escapeTimeVec32(float* out, float* real, float* imag);
void escapeTimeVec32Smooth(float* out, float* real, float* imag);
void escapeTimeVec64(float* out, double* real, double* imag);
void escapeTimeVec64Smooth(float* out, double* real, double* imag);

#endif
