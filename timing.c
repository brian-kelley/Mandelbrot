#include "timing.h"

Time getTime()
{
  Time t;
  t.sec = time(NULL);
  t.ticks = clock();
  return t;
}

double timeDiff(Time start, Time end)
{
  //swap times if start > end
  if(start.sec > end.sec || (start.sec == end.sec && start.ticks > end.ticks))
  {
    Time temp = start;
    start = end;
    end = temp;
  }
  double secDiff = end.sec - start.sec;
  //Check if tick difference sign agrees
  double tickDiff = (double) (end.ticks - start.ticks) / CLOCKS_PER_SEC;
  if(fabs(secDiff - tickDiff) < 1)
    return tickDiff;
  //otherwise, just use seconds value (not accurate but at least correct)
  return secDiff;
}

