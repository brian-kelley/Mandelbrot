#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"
#include "pthread.h"
#include "lodepng.h"
#include "fixedpoint.h"
#include "image.h"
#include "assert.h"
#include "complex.h"
//#include <glib-2.0/glib.h>

#define CRASH(msg) {printf("Error: " msg " in file " __FILE__ ", line %i\n", __LINE__); exit(1);}

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

typedef struct
{
    Point* points;
    int numPoints;
} WorkInfo;

typedef struct
{
  int start;
  int num;
} SimpleWorkInfo;

void simpleDrawBuf();   //naive but still parallel
void fastDrawBuf();     //uses boundary + fill optimization
void fillRect(Rect r);

void getInterestingLocation(int minExpo, const char* cacheFile, bool useCache);
void initColorTable();    //compute the RGBA values and store in a static table
void writeImage();       //use lodePNG to write out the current conv-rate buffer
void recomputeMaxIter();    //update iteration cap between zooms
void saveResumeState(const char* fname);
void loadResumeState(const char* fname);

/* Low level main loop functions */
Uint32 getColor(int num);   //lookup color corresponding to the iteration count of a pixel
void* workerFunc(void* wi);                //pthread worker thread function
void* simpleWorkerFunc(void* wi);                //pthread worker thread function
//iterate z = z^2 + c, where z = x + yi, return iteration count to escape or -1 if converged
int getPixelConvRate(int x, int y);
//iterate z = z^2 + c, return iteration count
int getConvRateFP(FP* real, FP* imag);
int getConvRateLD(long double real, long double imag);  //actually iterate z = z^2 + c, return iteration count
bool upgradePrec();  //given pstride, does precision need to be upgraded

void iteratePointQueue(Point* queue, int num);   //iterate the points in parallel

//Exact boundary tracing functions
void fillAll();
//Generic get value of pixel
int getPixel(Point p);
//trace boundary of shape; precondition: start point is an upper-left corner
void traceOutline(Point start, int val, Point* pbuf);
bool inBounds(Point p);

void floodFill(Point p, int val, Point* stack);
