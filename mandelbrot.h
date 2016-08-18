#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"
#include "pthread.h"
#include "lodepng.h"
#include "fixedpoint.h"
#include "image.h"
#include "assert.h"

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

void drawBuf();    //compute iters/colors for pixels
void fillRect(Rect r);

void getInterestingLocation(int minExpo, const char* cacheFile, bool useCache);
void initColorTable();    //compute the RGBA values and store in a static table
void writeImage();       //use lodePNG to write out the current conv-rate buffer
void recomputeMaxIter(int zoomExpo);    //update iteration cap between zooms
void saveResumeState(const char* fname);
void loadResumeState(const char* fname);

/* Low level main loop functions */
Uint32 getColor(int num);   //lookup color corresponding to the iteration count of a pixel
void* workerFunc(void* points);                //pthread worker thread function
void getPixelConvRate(int x, int y);
int getConvRateFP(FP* real, FP* imag);  //actually iterate z = z^2 + c, return iteration count
int getConvRateLD(long double real, long double imag);  //actually iterate z = z^2 + c, return iteration count
bool upgradePrec(FP* pstride);  //given pstride, does precision need to be upgraded

void iteratePointQueue(Point* queue, int num);   //iterate the points in parallel
