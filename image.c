#include "image.h"

#define GET_R(px) (((px) & 0xFF000000) >> 24)
#define GET_G(px) (((px) & 0xFF0000) >> 16)
#define GET_B(px) (((px) & 0xFF00) >> 8)

static int* _iters;
float _expo;
float* imgScratch;

Uint32 lerp(Uint32 c1, Uint32 c2, double k)
{
  double red = ((c1 & 0xFF000000) >> 24) * k + ((c2 & 0xFF000000) >> 24) * (1 - k);
  double grn = ((c1 & 0xFF0000) >> 16) * k + ((c2 & 0xFF0000) >> 16) * (1 - k);
  double blu = ((c1 & 0xFF00) >> 8) * k + ((c2 & 0xFF00) >> 8) * (1 - k);
  return ((Uint32) red << 24) | ((Uint32) grn << 16) | ((Uint32) blu << 8) | 0xFF;
}

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
      HANDLE_POS(i - 1, j - 1, cornerWeight);
      HANDLE_POS(i,     j - 1, edgeWeight);
      HANDLE_POS(i + 1, j - 1, cornerWeight);
      HANDLE_POS(i - 1, j,     edgeWeight);
      HANDLE_POS(i,     j,     centerWeight);
      HANDLE_POS(i + 1, j,     edgeWeight);
      HANDLE_POS(i - 1, j + 1, cornerWeight);
      HANDLE_POS(i,     j + 1, edgeWeight);
      HANDLE_POS(i + 1, j + 1, cornerWeight);
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
  int lhs = _iters[*((int*) lhsRaw)];
  int rhs = _iters[*((int*) rhsRaw)];
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

void handleNonColored(Image* im)
{
  for(int i = 0; i < im->w * im->h; i++)
  {
    if(im->iters[i] == -1)
      im->fb[i] = 0xFF;
    else if(im->iters[i] < 0)
      im->fb[i] = 0x999999FF;
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

static float expoMapFunc(float val)
{
  return pow(val, _expo) - 1;
}

static float logMapFunc(float val)
{
  return log(val + 20);
}

static void applyCyclicMapping(Image* im, FloatMapping func)
{
  handleNonColored(im);
  int minVal = INT_MAX;
  for(int i = 0; i < im->w * im->h; i++)
  {
    if(im->iters[i] > 0 && im->iters[i] < minVal)
      minVal = im->iters[i];
  }
  for(int i = 0; i < im->w * im->h; i++)
  {
    if(im->iters[i] >= 0)
      im->iters[i] = func(im->iters[i]);
    else
      im->iters[i] = -1.0;
  }
  float cap = getPercentileValue(im->iters, im->w, im->h, 1);
  //clamp values
  for(int i = 0; i < im->w * im->h; i++)
  {
    if(im->iters[i] > cap)
      im->iters[i] = cap;
  }
  float minMapped = func(minVal);
  //scale up to reach desired color range
  float scale = (im->period * im->cycles) / (cap - minMapped);
  float perSegment = (float) im->period / im->numColors;
  for(int i = 0; i < im->w * im->h; i++)
  {
    //subtract 1 so that effective min value maps to 0 (origin in color cycle)
    float val = im->iters[i];
    if(val >= 0)
    {
      float delta = (val - minMapped) * scale;
      val = minMapped + delta;
      int segment = val / perSegment;
      float lerpK = 1 - (val - segment * perSegment) / perSegment;
      int lowColorIndex = segment % im->numColors;
      int highColorIndex = (segment + 1) % im->numColors;
      //lerp between low and high color
      im->fb[i] = lerp(im->palette[lowColorIndex], im->palette[highColorIndex], lerpK);
    }
  }
}

void colorExpoCyclic(Image* im, float expo)
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
  /*
  //histogram proportion (i.e. quarter of all is 0.25) multiplied by
  _iters = iterbuf;
  int* pixelList = malloc(w * h * sizeof(int));
  for(int i = 0; i < w * h; i++)
    pixelList[i] = i;
  //sort pixelList (iters indices) according to the iter values
  qsort(pixelList, w * h, sizeof(int), pixelCompare);
  //get # of diverged pixels
  int diverged = w * h;
  while(iterbuf[pixelList[diverged - 1]] == -1)
    diverged--;
  int* colorOffsets = malloc((numColors + 1) * sizeof(int));
  printf("Raw weights: ");
  for(int i = 0; i < numColors; i++)
    printf("%f ", weights[i]);
  float* normalWeights = malloc(numColors * sizeof(float));
  {
    float accum = 0;
    for(int i = 0; i < numColors; i++)
      accum += weights[i];
    for(int i = 0; i < numColors; i++)
      normalWeights[i] = weights[i] / accum;
  }
  printf("Normal weights: ");
  for(int i = 0; i < numColors; i++)
    printf("%f ", normalWeights[i]);
  puts("");
  {
    float accum = 0;
    for(int i = 0; i < numColors + 1; i++)
    {
      colorOffsets[i] = diverged * accum;
      if(i < numColors)
        accum += normalWeights[i];
    }
  }
  printf("Color offsets (pixels): ");
  for(int i = 0; i < numColors + 1; i++)
    printf("%i ", colorOffsets[i]);
  puts("");
  //use color offsets to map to [0, DEFAULT_CYCLE_LEN]
  int lowerColor = 0;
  for(int i = 0; i < diverged; i++)
  {
    if(colorOffsets[lowerColor + 1] <= i)
      lowerColor++;
    int origOrd = i;
    int orig = iterbuf[pixelList[i]];
    int mapped = DEFAULT_CYCLE_LEN * (double) (i - colorOffsets[lowerColor]) /
                                     (colorOffsets[lowerColor + 1] - colorOffsets[lowerColor]);
    do
    {
      iterbuf[pixelList[i]] = mapped;
      i++;
    }
    while(iterbuf[pixelList[i]] == orig && i < diverged);
  }
  free(normalWeights);
  free(colorOffsets);
  free(pixelList);
  */
}

void colorHistWeighted(Image* im, float* weights)
{

}

void colorExpoCyclicSmooth(Image* im, float expo)
{

}

void colorLogCyclicSmooth(Image* im)
{

}

void colorHistSmooth(Image* im)
{
  
}

void colorHistWeightedSmooth(Image* im, float* weights)
{

}
