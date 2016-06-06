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

void drawBuf(Buffer* buf, bool doRecycle);    //compute iters/colors for pixels
void fastDrawBuf(Buffer* buf, Buffer* coarse, bool doRecycle);

void launchWorkers(Buffer* buf);
void recycle(Buffer* buf);

void getInterestingLocation(int minExpo, const char* cacheFile, bool useCache);
void initColorTable();    //compute the RGBA values and store in a static table
void writeImage(Buffer* buf);       //use lodePNG to write out the current conv-rate buffer
void recomputeMaxIter(int zoomExpo);    //update iteration cap between zooms
void saveResumeState(const char* fname);
void loadResumeState(const char* fname);

/* Low level main loop functions */
Uint32 getColor(int num);   //lookup color corresponding to the iteration count of a pixel
void applyCoarseIters(int* iters);
void* workerFunc(void* buffer);         //pthread worker thread function
int getConvRate(FP* real, FP* imag);  //actually iterate z = z^2 + c, return iteration count
int getPrec(int zoomExpo);  //get the precision level required (pass raw biased value)

