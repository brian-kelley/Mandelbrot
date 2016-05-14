#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"
#include "pthread.h"
#include "lodepng.h"
#include "precision.h"

typedef uint32_t Uint32;

#define winw 640
#define winh 400
#define deepestExpo -300
#define zoomFactor 2
#define numColors 360
#define totalIter (1 << 20)
#define iterStop (2.0 * 2.0)    //the square of the magnitude where mandelbrot iterations stop

/* Initialization functions */
void initPositionVars();    //initialize viewport (screenX, screenY, width, height, etc)
void initColorTable();      //compute the RGBA values and store in a static table
void getInterestingLocation(int minExpo, const char* cacheFile, bool useCache);

/* High level main loop functions */
void drawBuf();             //compute iteration count & color for every pixel in viewport
void writeImage();          //use lodePNG to write out the current conv-rate buffer
void increasePrecision();   //increment the precision of persistent Float values
void recomputeMaxIter();    //update iteration cap between zooms
void zoomToTarget();        //do zoom step towards predetermined target (center doesn't move)
void computeScreenPos();    //set screenX, screenY based on pstride, targetX, targetY

/* Low level main loop functions */
Uint32 getColor(int num);   //lookup color corresponding to the iteration count of a pixel
void* workerFunc(void*);         //pthread worker thread function
int getConvRate(Float* real, Float* imag);  //actually iterate z = z^2 + c, return iteration count
int getConvRateLD(long double real, long double imag); //actually iterate z = z^2 + c, return iteration count
int getPrec(int zoomExpo);                  //get the Float precision required to handle a pixel stride with expo zoomExpo