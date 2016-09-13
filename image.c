#include "image.h"

#define GET_R(px) (((px) & 0xFF000000) >> 24)
#define GET_G(px) (((px) & 0xFF0000) >> 16)
#define GET_B(px) (((px) & 0xFF00) >> 8)

int* iters;

void blockFilter(float constant, Uint32* buf, int w, int h)
{
  Uint32* blurred = (Uint32*) malloc(w * h * sizeof(Uint32));
  const Uint32 BLACK = 0xFF;        //opaque black
  //write the blurred pixels into the new buffer
  bool fadeIntoBlack = false;
  for(int i = 0; i < w; i++)
  {
    for(int j = 0; j < h; j++)
    {
      float rsum = 0;
      float gsum = 0;
      float bsum = 0;
      float totalWeight = 0;  //want total weight to be 1 after adding all neighbors and self
      if(i > 0)
      {
        if(!fadeIntoBlack || buf[i - 1 + w * j] != BLACK)
        {
          rsum += constant * GET_R(buf[i - 1 + w * j]);
          gsum += constant * GET_G(buf[i - 1 + w * j]);
          bsum += constant * GET_B(buf[i - 1 + w * j]);
          totalWeight += constant;
        }
      }
      if(i < w - 1)
      {
        if(!fadeIntoBlack || buf[i - 1 + w * j] != BLACK)
        {
          rsum += constant * GET_R(buf[i + 1 + w * j]);
          gsum += constant * GET_G(buf[i + 1 + w * j]);
          bsum += constant * GET_B(buf[i + 1 + w * j]);
          totalWeight += constant;
        }
      }
      if(j > 0)
      {
        if(!fadeIntoBlack || buf[i - 1 + w * j] != BLACK)
        {
          rsum += constant * GET_R(buf[i + w * (j - 1)]);
          gsum += constant * GET_G(buf[i + w * (j - 1)]);
          bsum += constant * GET_B(buf[i + w * (j - 1)]);
          totalWeight += constant;
        }
      }
      if(j < h - 1)
      {
        if(!fadeIntoBlack || buf[i - 1 + w * j] != BLACK)
        {
          rsum += constant * GET_R(buf[i + w * (j + 1)]);
          gsum += constant * GET_G(buf[i + w * (j + 1)]);
          bsum += constant * GET_B(buf[i + w * (j + 1)]);
          totalWeight += constant;
        }
      }
      rsum += (1.0f - totalWeight) * GET_R(buf[i + w * j]);
      gsum += (1.0f - totalWeight) * GET_G(buf[i + w * j]);
      bsum += (1.0f - totalWeight) * GET_B(buf[i + w * j]);
      //now [rgb]sum have the final color
      //clamp to [0, 0x100)
      rsum = min(rsum, 0xFF);
      gsum = min(gsum, 0xFF);
      bsum = min(bsum, 0xFF);
      rsum = max(rsum, 0);
      gsum = max(gsum, 0);
      bsum = max(bsum, 0);
      blurred[i + j * w] = ((unsigned) rsum << 24) | ((unsigned) gsum << 16) | ((unsigned) bsum << 8) | 0xFF;
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
  if(rhs == 1)
    return -1;
  if(lhs < rhs)
    return -1;
  if(lhs > rhs)
    return 1;
  return 0;
}

void histogramFilter(int* iterbuf, int w, int h)
{
  //BROKEN - WIP
  //histogram proportion (i.e. quarter of all is 0.25) multiplied by
  //DELTA_FACTOR to get change in color map index (index is truncated to int)
  const double DELTA_FACTOR = 50;
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
  int lowest = iters[pixelList[0]];
  /*
  for(int y = 0; y < h; y++)
  {
    for(int x = 0; x < w; x++)
    {
      int index = x + y * w;
      printf("%i ", iters[pixelList[index]]);
    }
    puts("");
  }
  puts("");
  puts("");
  */
  puts("Color mappings:");
  int printed = 0;
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
    printf("(%5i -> %5i) ", val, mapping);
    if((printed++) % 10 == 9)
      puts("");
    for(int j = start; j < start + num; j++)
      iters[pixelList[j]] = mapping;
  }
  puts("");
  free(pixelList);
}

void logarithmFilter(int* iterbuf, int w, int h)
{
  for(int i = 0; i < w * h; i++)
  {
    if(iterbuf[i] != -1)
    {
      iterbuf[i] = 40 * log(iterbuf[i] + 100);
    }
  }
}

