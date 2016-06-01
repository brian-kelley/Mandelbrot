#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"
#include "pthread.h"
#include "lodepng.h"
#include "fixedpoint.h"
#include "image.h"

#define deepestExpo -300

/* Initialization functions */
void initPositionVars();
void initColorTable();      //compute the RGBA values and store in a static table
void getInterestingLocation(int minExpo, const char* cacheFile, bool useCache);

/* High level main loop functions */
void drawBuf();             //compute iteration count & color for every pixel in viewport
void writeImage();          //use lodePNG to write out the current conv-rate buffer
void recomputeMaxIter(int zoomExpo);    //update iteration cap between zooms
void saveResumeState(const char* fname);
void loadResumeState(const char* fname);

/* Low level main loop functions */
Uint32 getColor(int num);   //lookup color corresponding to the iteration count of a pixel
void* workerFunc(void*);         //pthread worker thread function
int getConvRate(FP* real, FP* imag);  //actually iterate z = z^2 + c, return iteration count
int getConvRateLD(long double real, long double imag); //actually iterate z = z^2 + c, return iteration count
int getPrec(int zoomExpo);  //get the precision level required (pass raw biased value)

