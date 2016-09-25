#include "image.h"

#define GET_R(px) (((px) & 0xFF000000) >> 24)
#define GET_G(px) (((px) & 0xFF0000) >> 16)
#define GET_B(px) (((px) & 0xFF00) >> 8)

int* iters;

void blockFilter(double constant, Uint32* buf, int w, int h)
{
  Uint32* blurred = (Uint32*) malloc(w * h * sizeof(Uint32));
  const Uint32 BLACK = 0xFF;        //opaque black
  //write the blurred pixels into the new buffer
  bool fadeIntoBlack = false;
#define HANDLE_POS(xx, yy, weight) \
  { \
    if((xx) >= 0 && (xx) < (w) && (yy) >= 0 && (yy) < (h)) \
    { \
      r += (weight) * GET_R(buf[(xx) + (yy) * (w)]); \
      g += (weight) * GET_G(buf[(xx) + (yy) * (w)]); \
      b += (weight) * GET_B(buf[(xx) + (yy) * (w)]); \
      totalWeight += (weight); \
    } \
  }
  double cornerWeight = constant * constant;
  double edgeWeight = (1.0 - 2.0 * constant) * constant;
  double centerWeight = (1.0 - 2.0 * constant) * (1.0 - 2.0 * constant);
  for(int i = 0; i < w; i++)
  {
    for(int j = 0; j < h; j++)
    {
      double r = 0.0;
      double g = 0.0;
      double b = 0.0;
      double totalWeight = 0;
      /*
      HANDLE_POS(i - 1, j - 1, cornerWeight);
      HANDLE_POS(i,     j - 1, edgeWeight);
      HANDLE_POS(i + 1, j - 1, cornerWeight);
      HANDLE_POS(i - 1, j,     edgeWeight);
      */
      HANDLE_POS(i,     j,     centerWeight);
      /*
      HANDLE_POS(i + 1, j,     edgeWeight);
      HANDLE_POS(i - 1, j + 1, cornerWeight);
      HANDLE_POS(i,     j + 1, edgeWeight);
      HANDLE_POS(i + 1, j + 1, cornerWeight);
      */
      double invWeight = 1.0 / totalWeight;
      int rr = r * invWeight;
      int gg = g * invWeight;
      int bb = b * invWeight;
      rr = min(rr, 255);
      gg = min(gg, 255);
      bb = min(bb, 255);
      rr = max(rr, 0);
      gg = max(gg, 0);
      bb = max(bb, 0);
      blurred[i + j * w] = ((unsigned) rr << 24) |
                           ((unsigned) gg << 16) |
                           ((unsigned) bb << 8) | 0xFF;
    }
  }
  //copy blurred pixels back to original buffer
  memcpy(buf, blurred, w * h * sizeof(Uint32));
  free(blurred);
}

void reduceIters(int* iterbuf, int diffCap, int w, int h)
{
  bool changed;
  do
  {
    changed = false;
    for(int i = 1; i < w - 1; i++)
    {
      for(int j = 1; j < h - 1; j++)
      {
        int thisIter = iterbuf[i + j * w];
        if(thisIter == -1)
          continue;
        int minNeighbor = thisIter;
        for(int ii = -1; ii <= 1; ii++)
        {
          for(int jj = -1; jj <= 1; jj++)
          {
            int neighbor = iterbuf[(i + ii) + (j + jj) * w];
            if(neighbor != -1 && neighbor < thisIter)
              minNeighbor = neighbor;
          }
        }
        if(thisIter - minNeighbor > diffCap)
        {
          iterbuf[i + j * w] = minNeighbor + diffCap;
          //changed = true;
        }
      }
    }
  }
  while(changed);
}

//comparator returns -1 if lhs < rhs, 1 if lhs > rhs
static int pixelCompare(const void* lhsRaw, const void* rhsRaw)
{
  int lhs = iters[*((int*) lhsRaw)];
  int rhs = iters[*((int*) rhsRaw)];
  if(lhs == rhs)
    return 0;
  if(lhs == -1)
    return 1;
  if(rhs == -1)
    return -1;
  if(lhs < rhs)
    return -1;
  if(lhs > rhs)
    return 1;
  return 0;
}

void histogramFilter(int* iterbuf, int w, int h)
{
  //histogram proportion (i.e. quarter of all is 0.25) multiplied by
  //DELTA_FACTOR to get change in color map index (index is truncated to int)
  const double DELTA_FACTOR = 1500;
  iters = iterbuf;
  int* pixelList = malloc(w * h * sizeof(int));
  for(int i = 0; i < w * h; i++)
    pixelList[i] = i;
  //sort pixelList (iters indices) according to the iter values
  qsort(pixelList, w * h, sizeof(int), pixelCompare);
  //get # of diverged pixels
  int diverged = w * h;
  while(iters[pixelList[diverged - 1]] == -1)
    diverged--;
  printf("%i of %i (%f%%) diverged (colored)\n", diverged, w * h, 100.0 * diverged / (w * h));
  //int lowest = iters[pixelList[0]];
  int lowest = 0;
  for(int i = 0; i < w * h; i++)
  {
    int val = iters[pixelList[i]];
    if(val >= 0 && val < iters[pixelList[lowest]])
      lowest = i;
  }
  for(int i = 0; i < diverged; i++)
  {
    if(iters[pixelList[i]] == -1)
    {
      //reached the end of colored pixels, remapping done
      break;
    }
    int start = i;
    int val = iters[pixelList[start]];
    while(iters[pixelList[i]] == val && i < w * h)
      i++;
    int num = i - start;
    int mapping = lowest + (DELTA_FACTOR * start / diverged);
    for(int j = start; j < start + num; j++)
      iters[pixelList[j]] = mapping;
  }
  free(pixelList);
}

static int logMap(int orig)
{
  return 50.0 * log(orig + 20);
}

void logarithmFilter(int* iterbuf, int w, int h)
{
  const int desiredRange = 2000;   //2 full spectrum cycles?
  //scan for minimum and maximum colored values
  int lo = INT_MAX;
  int hi = 0;
  for(int i = 0; i < w * h; i++)
  {
    int val = iterbuf[i];
    if(val > 0)
    {
      if(val < lo)
        lo = val;
      if(val > hi)
        hi = val;
    }
  }
  int mapLo = logMap(lo);
  int mapHi = logMap(hi);
  double rangeScale = (double) desiredRange / (mapHi - mapLo);
  for(int i = 0; i < w * h; i++)
  {
    if(iterbuf[i] != -1)
      iterbuf[i] = mapLo + (logMap(iterbuf[i]) - mapLo) * rangeScale;
  }
}

//#define EXPO_SCALE 80.0
//#define MAP_EXPO 0.45

#define EXPO_SCALE 300.0
#define MAP_EXPO 0.2

static double expoMap(double orig)
{
  return EXPO_SCALE * pow(orig, MAP_EXPO);
}

static double invExpoMap(double orig)
{
  // y = 80x^0.45
  // y/80 = x^0.45
  // (y/80)^(1/0.45) = (x^0.45)^(1/0.45)
  // (y/80)^(1/0.45) = x
  return pow(orig / EXPO_SCALE, 1.0 / MAP_EXPO);
}

void exponentialFilter(int* iterbuf, int w, int h)
{
  const int desiredRange = 2000;   //2 full spectrum cycles?
  //get lowest colored value in buf
  int lowest = INT_MAX;
  for(int i = 0; i < w * h; i++)
  {
    if(iterbuf[i] >= 0 && iterbuf[i] < lowest)
      lowest = iterbuf[i];
  }
  int lowmap = expoMap(lowest);
  int cap = invExpoMap(lowmap + desiredRange);
  for(int i = 0; i < w * h; i++)
  {
    if(iterbuf[i] > cap)
      iterbuf[i] = cap;
  }
  for(int i = 0; i < w * h; i++)
  {
    if(iterbuf[i] != -1)
      iterbuf[i] = expoMap(iterbuf[i]);
  }
}
