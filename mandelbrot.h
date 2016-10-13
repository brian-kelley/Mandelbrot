#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <stdatomic.h>
#include "pthread.h"
#include "kernels.h"
#include "lodepng.h"
#include "fixedpoint.h"
#include "image.h"
#include "timing.h"
#include "complex.h"
#include "x86intrin.h"

#define CRASH(msg) {printf("Error: " msg " in file " __FILE__ ", line %i\n", __LINE__); exit(1);}

extern int maxiter;
extern int prec;

enum
{
    UP,
    DOWN,
    LEFT,
    RIGHT,
    INVALID
} Direction;

typedef struct
{
    int x;
    int y;
    int w;
    int h;
} Rect;

typedef struct
{
    int x;
    int y;
} Point;

//Thread-local space for flood fill stack & outline point list
typedef struct
{
  Point* fillStack;
  Point* outlinePoints;
} OutlineScratch;

//Just give 1D in iters
typedef struct
{
  int start;
  int n;
} SimpleWorkInfo;

void simpleDrawBuf();
void fastDrawBuf();     //uses boundary + fill optimization (non-smooth only)

// float
void drawBufSIMD32();
void* simd32Worker(void* unused);
// double
void drawBufSIMD64();
void* simd64Worker(void* unused);
//arbitrary precision
void* simpleWorkerFunc(void* wi);

void colorTestDrawBuf();  //draws 2 periods of the color map as a spectrum

void getInterestingLocation(int minExpo, const char* cacheFile, bool useCache);
void initColorTable();    //compute the RGBA values and store in a static table
void writeImage();       //use lodePNG to write out the current conv-rate buffer
void recomputeMaxIter();    //update iteration cap between zooms
void saveResumeState(const char* fname);
void loadResumeState(const char* fname);

/* Low level main loop functions */
Uint32 getColor(int num);   //lookup color corresponding to the iteration count of a pixel
//iterate z = z^2 + c, where z = x + yi, return iteration count to escape or -1 if converged
float getPixelConvRate(int x, int y);
float getPixelConvRateSmooth(int x, int y);
//iterate z = z^2 + c, return iteration count
bool upgradePrec();  //given pstride, does precision need to be upgraded

void iteratePointQueue(Point* queue, int num);   //iterate the points in parallel

//Exact boundary tracing functions
void* fillAll(void* osRaw);
//Generic get value of pixel
int getPixel(Point p);
//Get value of pixel but can return NOT_COMPUTED
int getPixelNoCompute(Point p);
//trace boundary of shape; precondition: start point is an upper-left corner
void traceOutline(Point start, int val, OutlineScratch* pbuf);
bool inBounds(Point p);
float ssValue(float* outputs);

void floodFill(Point p, int val, Point* stack);
void initOutlineScratch();

