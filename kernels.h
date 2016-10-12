#ifndef KERNELS_H
#define KERNELS_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <stdatomic.h>
#include "pthread.h"
#include "lodepng.h"
#include "fixedpoint.h"
#include "image.h"
#include "timing.h"
#include "complex.h"
#include "x86intrin.h"

extern int maxiter;
extern int prec;

float smoothEscapeTime(float iters, double zr, double zi, double cr, double ci);

float escapeTimeFP(FP* real, FP* imag);
float escapeTimeFPSmooth(FP* real, FP* imag);
float escapeTime64(double real, double imag);
float escapeTime64Smooth(double real, double imag);
float escapeTime80(long double real, long double imag);
float escapeTime80Smooth(long double real, long double imag);

// SIMD escape functions.
// real, imag must be aligned to 32
void escapeTimeVec32(float* out, float* real, float* imag);
void escapeTimeVec32Smooth(float* out, float* real, float* imag);
void escapeTimeVec64(float* out, double* real, double* imag);
void escapeTimeVec64Smooth(float* out, double* real, double* imag);

#endif
