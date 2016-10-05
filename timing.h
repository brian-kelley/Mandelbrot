#ifndef TIMING_H
#define TIMING_H

#include "time.h"
#include "math.h"

typedef struct
{
  time_t sec;
  clock_t ticks;
} Time;

//Get current time.
Time getTime();

//Absolute value difference between times
double timeDiff(Time start, Time end);

#endif
