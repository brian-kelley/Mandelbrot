#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "stdio.h"
#include "stdlib.h"

typedef unsigned char u8;
typedef unsigned u32;
typedef unsigned long long u64;
typedef unsigned __int128 u128;
typedef __int128 s128;

#define max(a, b) (a < b ? b : a)
#define min(a, b) (a < b ? a : b)

//Special iter values
#define NOT_COMPUTED -2
#define BLANK -3
#define OUTSIDE_IMAGE -4

// How many points with high iter value to choose in getInterestingLocation
// frame, which are then selected among randomly to get next frame's center
#define GIL_CANDIDATES 2

//gives pretty good values even for bad rand
#define mrand (rand() >> 4)

#define CRASH(msg) {printf("Error: " msg " in file " __FILE__ ", line %i\n", __LINE__); exit(1);}

extern float* iters;
extern int winw;
extern int winh;
extern int maxiter;
extern int prec;
extern int numThreads;
extern int pixelsComputed;

extern float getPixelConvRate(int x, int y);

#endif
