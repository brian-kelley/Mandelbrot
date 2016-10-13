#ifndef BOUNDARY_H
#define BOUNDARY_H

#include "stdbool.h"
#include "pthread.h"
#include "assert.h"
#include "string.h"
#include "constants.h"

void fastDrawBuf();     //uses boundary + fill optimization (non-smooth only)

typedef struct
{
    int x;
    int y;
} Point;

typedef struct
{
    int x;
    int y;
    int w;
    int h;
} Rect;

enum
{
    UP,
    DOWN,
    LEFT,
    RIGHT,
    INVALID
} Direction;

//Thread-local space for flood fill stack & outline point list
typedef struct
{
  Point* fillStack;
  Point* outlinePoints;
} OutlineScratch;

extern OutlineScratch* oscratch;

extern int winw;
extern int winh;
extern float* iters;

void initOutlineScratch();
//Exact boundary tracing functions
void* fillAll(void* osRaw);
//Generic get value of pixel
int getPixel(Point p);
//Get value of pixel but can return NOT_COMPUTED
int getPixelNoCompute(Point p);
//trace boundary of shape; precondition: start point is an upper-left corner
void iteratePointQueue(Point* queue, int num);   //iterate the points in parallel
void traceOutline(Point start, int val, OutlineScratch* pbuf);
bool inBounds(Point p);

void floodFill(Point p, int val, Point* stack);
void initOutlineScratch();

#endif
