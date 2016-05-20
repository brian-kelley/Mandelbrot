//#define DEBUG
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"
#include "pthread.h"
#include "lodepng.h"
#include "precision.h"

typedef uint32_t Uint32;

#define deepestExpo -300

/* Initialization functions */
void initPositionVars();    //initialize viewport (screenX, screenY, width, height, etc)
void initColorTable();      //compute the RGBA values and store in a static table
void getInterestingLocation(int minExpo, const char* cacheFile, bool useCache);

/* High level main loop functions */
void drawBuf();             //compute iteration count & color for every pixel in viewport
void writeImage();          //use lodePNG to write out the current conv-rate buffer
void increasePrecision();   //increment the precision of persistent Float values
void recomputeMaxIter(int zoomExpo);    //update iteration cap between zooms
void zoomToTarget();        //do zoom step towards predetermined target (center doesn't move)
void computeScreenPos();    //set screenX, screenY based on pstride, targetX, targetY
void saveResumeState(const char* fname);
void loadResumeState(const char* fname);

/* Low level main loop functions */
Uint32 getColor(int num);   //lookup color corresponding to the iteration count of a pixel
void* workerFunc(void*);         //pthread worker thread function
int getConvRate(Float* real, Float* imag);  //actually iterate z = z^2 + c, return iteration count
int getConvRateLD(long double real, long double imag); //actually iterate z = z^2 + c, return iteration count
int getPrec(int zoomExpo);  //get the precision level required (pass raw biased value)
