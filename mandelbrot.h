#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"
#include "pthread.h"
#include "lodepng.h"
#include "fixedpoint.h"
#include "image.h"

typedef struct
{
    int* iters;
    unsigned* colors;
    FP pstride;
    int w;
    int h;
} Buffer;

/* Initialization functions */
void initColorTable();      //compute the RGBA values and store in a static table
void getInterestingLocation(int minExpo, const char* cacheFile, bool useCache);

/* High level main loop functions */
void drawBuf(Buffer* buf, bool recycle);      //compute iteration count & color for every pixel in viewport
void writeImage(Buffer* buf);          //use lodePNG to write out the current conv-rate buffer
void recomputeMaxIter(int zoomExpo);    //update iteration cap between zooms
void saveResumeState(const char* fname);
void loadResumeState(const char* fname);

/* Low level main loop functions */
Uint32 getColor(int num);   //lookup color corresponding to the iteration count of a pixel
void applyCoarseIters(int* iters);
void* workerFunc(void* buffer);         //pthread worker thread function
int getConvRate(FP* real, FP* imag);  //actually iterate z = z^2 + c, return iteration count
int getPrec(int zoomExpo);  //get the precision level required (pass raw biased value)

