#include "image.h"

#define GET_R(px) (((px) & 0xFF000000) >> 24)
#define GET_G(px) (((px) & 0xFF0000) >> 16)
#define GET_B(px) (((px) & 0xFF00) >> 8)

static float _expo;
float* imgScratch;

Uint32 lerp(Uint32 c1, Uint32 c2, double k)
{
  double red = ((c1 & 0xFF000000) >> 24) * (1 - k) + ((c2 & 0xFF000000) >> 24) * k;
  double grn = ((c1 & 0xFF0000) >> 16) * (1 - k) + ((c2 & 0xFF0000) >> 16) * k;
  double blu = ((c1 & 0xFF00) >> 8) * (1 - k) + ((c2 & 0xFF00) >> 8) * k;
  return ((Uint32) red << 24) | ((Uint32) grn << 16) | ((Uint32) blu << 8) | 0xFF;
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
  float lhs = iters[*((int*) lhsRaw)];
  float rhs = iters[*((int*) rhsRaw)];
  if(lhs == rhs)
    return 0;
  if(lhs < 0)
    return 1;
  if(rhs < 0)
    return -1;
  if(lhs < rhs)
    return -1;
  if(lhs > rhs)
    return 1;
  return 0;
}

void handleNonColored()
{
  for(int i = 0; i < winw * winh; i++)
  {
    if(iters[i] == -1)
      frameBuf[i] = 0xFF;
    else if(iters[i] < 0)
      frameBuf[i] = 0x999999FF;
  }
}

static int floatCmp(const void* lhs, const void* rhs)
{
  float l = *((float*) lhs);
  float r = *((float*) rhs);
  if(l < r)
    return -1;
  if(l > r)
    return 1;
  return 0;
}

float getPercentileValue(float* buf, int w, int h, float proportion)
{
  assert(proportion >= 0.0);
  int count = 0;
  for(int i = 0; i < w * h; i++)
  {
    if(buf[i] > 0)
    {
      imgScratch[count++] = buf[i];
    }
  }
  assert(count > 2);
  qsort(imgScratch, count, sizeof(float), floatCmp);
  int index = count * proportion;
  if(index >= count)
    index = count - 1;
  return imgScratch[index];
}

static double expoMapFunc(double val)
{
  return pow(val, _expo) - 1;
}

static double logMapFunc(double val)
{
  return log(val + 2) / log(1.05);
}

static void applyCyclicMapping(Image* im, Mapping func)
{
  handleNonColored();
  int minVal = INT_MAX;
  for(int i = 0; i < winw * winh; i++)
  {
    if(iters[i] > 0 && iters[i] < minVal)
      minVal = iters[i];
  }
  for(int i = 0; i < winw * winh; i++)
  {
    if(iters[i] >= 0)
      imgScratch[i] = func(iters[i] * iterScale);
    else
      imgScratch[i] = -1.0;
  }
  double scale = 0.05; //(im->period * im->cycles) / (cap - minMapped);
  double perSegment = im->period / im->cycles / im->numColors;
  for(int i = 0; i < winw * winh; i++)
  {
    //subtract 1 so that effective min value maps to 0 (origin in color cycle)
    double val = imgScratch[i] * scale;
    if(val >= 0)
    {
      int segment = val / perSegment;
      double lerpK = (val - segment * perSegment) / perSegment;
      int lowColorIndex = segment % im->numColors;
      int highColorIndex = (segment + 1) % im->numColors;
      //lerp between low and high color
      frameBuf[i] = lerp(im->palette[lowColorIndex], im->palette[highColorIndex], lerpK);
    }
  }
}

void colorExpoCyclic(Image* im, double expo)
{
  _expo = expo;
  applyCyclicMapping(im, expoMapFunc);
}

void colorLogCyclic(Image* im)
{
  applyCyclicMapping(im, logMapFunc);
}

void colorHist(Image* im)
{
  //temporary equal weights for all colors, then do weighted hist coloring
  double* weights = alloca(im->numColors * sizeof(double));
  for(int i = 0; i < im->numColors; i++)
    weights[i] = 1;
  colorHistWeighted(im, weights);
}

void colorHistWeighted(Image* im, double* weights)
{
  //histogram proportion (i.e. quarter of all is 0.25) multiplied by
  int* pixelList = (int*) imgScratch;
  for(int i = 0; i < winw * winh; i++)
    pixelList[i] = i;
  //sort pixelList (iters indices) according to the iter values
  qsort(pixelList, winw * winh, sizeof(int), pixelCompare);
  if(iters[pixelList[0]] == -1)
  {
    //something wrong with computation, but fill screen with black and return
    for(int i = 0; i < winw * winh; i++)
      frameBuf[i] = 0xFF;
    return;
  }
  //get # of diverged pixels
  int diverged = winw * winh;
  while(iters[pixelList[diverged - 1]] == -1)
    diverged--;
  int* colorOffsets = alloca(im->numColors * sizeof(int));
  double* normalWeights = alloca(im->numColors * sizeof(double));
  double accum = 0;
  //divide each color weight by sum of all weights
  for(int i = 0; i < im->numColors - 1; i++)
    accum += weights[i];
  for(int i = 0; i < im->numColors - 1; i++)
    normalWeights[i] = weights[i] / accum;
  normalWeights[im->numColors] = 1;
  //determine number of pixels in each "segment"
  accum = 0;
  for(int i = 0; i < im->numColors; i++)
  {
    colorOffsets[i] = diverged * accum;
    accum += normalWeights[i];
  }
  int lowerColor = 0;
  int firstOfValue = 0; //index of first first pixel encountered with ith value
  for(int i = 0; i < diverged; i++)
  {
    if(iters[pixelList[i]] != iters[pixelList[firstOfValue]])
      firstOfValue = i;
    while(colorOffsets[lowerColor + 1] <= firstOfValue)
      lowerColor++;
    int colorLo = colorOffsets[lowerColor];
    int colorHi = colorOffsets[lowerColor + 1];
    frameBuf[pixelList[i]] = lerp(im->palette[lowerColor], im->palette[lowerColor + 1],
        (double) (firstOfValue - colorLo) / (colorHi - colorLo));
  }
  handleNonColored();
}

