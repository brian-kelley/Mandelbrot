#include "mandelbrot.h"
#include "kernels.c"
#include "stdatomic.h"

int winw;
int winh;
float* iters;
unsigned* frameBuf;
int lastOutline;
FP targetX;
FP targetY;
FP pstride;
int zoomDepth;
int numThreads;
int maxiter;
int prec;
int deepestExpo;
bool smooth;
bool verbose;
bool supersample;
int zoomRate;
int pixelsComputed;
const char* outputDir;
pthread_t monitor;
ColorMap colorMap = colorSunset;

// Fetch + add to get next work unit
// Set to 0 by main thread at frame start
_Atomic int workerIndex;

void* monitorFunc(void* unused)
{
  double progress = 0;
  double width = MONITOR_WIDTH - 2;
  while(progress < width)
  {
    progress = width * atomic_load_explicit(&workerIndex, memory_order_relaxed) / (winw * winh);
    putchar('\r');
    putchar('[');
    int i;
    for(i = 0; i < (int) progress; i++)
      putchar('=');
    for(; i < (int) width; i++)
      putchar(' ');
    putchar(']');
    fflush(stdout);
    usleep(100000); //wake monitor 10 times per sec
  }
  return NULL;
}

float ssValue(float* outputs)
{
  float rv = outputs[0];
  for(int i = 1; i < 4; i++)
  {
    if((rv < 0 && outputs[i] > 0) || (rv > 0 && outputs[i] > 0 && outputs[i] < rv))
    {
      rv = outputs[i];
    }
  }
  return rv;
}

void writeImage()
{
  char name[256];
  sprintf(name, "%s/mandel%i.png", outputDir, zoomDepth);
  for(int i = 0; i < winw * winh; i++)
  {
    Uint32 temp = frameBuf[i]; 
    //convert rgba to big endian
    frameBuf[i] = ((temp & 0xFF000000) >> 24) | ((temp & 0xFF0000) >> 8) | ((temp & 0xFF00) << 8) | ((temp & 0xFF) << 24);
  }
  lodepng_encode32_file(name, (unsigned char*) frameBuf, winw, winh);
}

//Macro to set up weighted histogram color mapping
//Just need Uint32 array colors, and double array weights
#define SET_UP_COLORMAP \
  Image im; \
  im.iters = iters; \
  im.fb = frameBuf; \
  im.w = winw; \
  im.h = winh; \
  im.palette = colors; \
  im.numColors = sizeof(colors) / sizeof(Uint32); \
  im.cycles = 1;

void colorSunset()
{
  Uint32 colors[] =
  {
    0x000000FF,   // Black
    0x702963FF,   // Purple
    0xC80815FF,   // Red
    0xFF7518FF,   // Orange
    0xFFD700FF    // Gold
  };
  double weights[] =
  {
    2,
    2,
    0.6,
    0.06
  };
  SET_UP_COLORMAP;
  colorHistWeighted(&im, weights);
}

void colorGalaxy()
{
  Uint32 colors[] =
  {
    0x000000FF,   // Black
    0x000045FF,   // Dark blue
    0xF0F0F0FF    // Light grey
  };
  double weights[] =
  {
    1,
    0.01
  };
  SET_UP_COLORMAP;
  colorHistWeighted(&im, weights);
}

u64 totalIters()
{
  u64 n = 0;
  for(int i = 0; i < winw * winh; i++)
  {
    if(iters[i] < 0)
      n += maxiter;
    else
      n += iters[i];
  }
  return n;
}

float getPixelConvRate(int x, int y)
{
  //printf("Getting conv rate @ %i, %i\n", x, y);
  long double tx = getValue(&targetX);
  long double ty = getValue(&targetY);
  long double ps = getValue(&pstride);
  if(iters[x + y * winw] != NOT_COMPUTED)
    return iters[x + y * winw];
  float rv = NOT_COMPUTED;
  if(ps > EPS_64)
  {
    //single pixel, double prec
    if(smooth)
      rv = escapeTime64Smooth(tx + (x - winw / 2) * ps, ty + (y - winh / 2) * ps);
    else
      rv = escapeTime64(tx + (x - winw / 2) * ps, ty + (y - winh / 2) * ps);
  }
  else if(ps > EPS_80)
  {
    //single pixel, extended double prec
    if(smooth)
      rv = escapeTime80Smooth(tx + (x - winw / 2) * ps, ty + (y - winh / 2) * ps);
    else
      rv = escapeTime80(tx + (x - winw / 2) * ps, ty + (y - winh / 2) * ps);
  }
  else
  {
    //single pixel, arb. precision
    MAKE_STACK_FP(cr);
    MAKE_STACK_FP(ci);
    MAKE_STACK_FP(temp);
    fpcopy(&cr, &pstride);
    loadValue(&temp, x - winw / 2);
    fpmul2(&cr, &temp);
    fpcopy(&temp, &targetX);
    fpadd2(&cr, &temp);
    fpcopy(&ci, &pstride);
    loadValue(&temp, y - winh / 2);
    fpmul2(&ci, &temp);
    fpcopy(&temp, &targetY);
    fpadd2(&ci, &temp);
    if(smooth)
      rv = escapeTimeFPSmooth(&cr, &ci);
    else
      rv = escapeTimeFP(&cr, &ci);
    pixelsComputed++;
  }
  iters[x + y * winw] = rv;
  return rv;
}

float getPixelConvRateSS(int x, int y)
{
  //printf("Getting conv rate @ %i, %i\n", x, y);
  long double tx = getValue(&targetX);
  long double ty = getValue(&targetY);
  long double ps = getValue(&pstride);
  if(iters[x + y * winw] != NOT_COMPUTED)
    return iters[x + y * winw];
  float rv = NOT_COMPUTED;
  float output[4];
  if(ps > EPS_64)
  {
    double r0 = tx + ps * (x - winw / 2);
    double i0 = ty + ps * (y - winh / 2);
    double cr[4] __attribute__ ((aligned(32)));
    double ci[4] __attribute__ ((aligned(32)));
    cr[0] = r0 - ps / 4;
    cr[2] = cr[0];
    cr[1] = r0 + ps / 4;
    cr[3] = cr[1];
    ci[0] = i0 - ps / 4;
    ci[1] = ci[0];
    ci[2] = i0 + ps / 4;
    ci[3] = ci[2];
    if(!smooth)
      escapeTimeVec64(output, cr, ci);
    else
      escapeTimeVec64Smooth(output, cr, ci);
  }
  else if(ps > EPS_80)
  {
    long double r0 = tx + ps * (x - winw / 2);
    long double i0 = ty + ps * (y - winh / 2);
    long double cr[8];
    long double ci[8];
    cr[0] = r0 - ps / 4;
    cr[2] = cr[0];
    cr[1] = r0 + ps / 4;
    cr[3] = cr[1];
    ci[0] = i0 - ps / 4;
    ci[1] = ci[0];
    ci[2] = i0 + ps / 4;
    ci[3] = ci[2];
    if(!smooth)
    {
      for(int i = 0; i < 4; i++)
        output[i] = escapeTime80(cr[i], ci[i]);
    }
    else
    {
      for(int i = 0; i < 4; i++)
        output[i] = escapeTime80Smooth(cr[i], ci[i]);
    }
  }
  else
  {
    //single pixel, arb. precision
    MAKE_STACK_FP(cr);
    MAKE_STACK_FP(ci);
    MAKE_STACK_FP(temp);
    fpcopy(&cr, &pstride);
    loadValue(&temp, x - winw / 2);
    fpmul2(&cr, &temp);
    fpcopy(&temp, &targetX);
    fpadd2(&cr, &temp);
    fpcopy(&ci, &pstride);
    loadValue(&temp, y - winh / 2);
    fpmul2(&ci, &temp);
    fpcopy(&temp, &targetY);
    fpadd2(&ci, &temp);
    //have ci, cr and temp space
    //set temp to ps / 4
    fpcopy(&temp, &pstride);
    fpshr(temp, 2);
    fpsub2(&cr, &temp);
    fpsub2(&ci, &temp);
    if(!smooth)
      output[0] = escapeTimeFP(&cr, &ci);
    else
      output[0] = escapeTimeFPSmooth(&cr, &ci);
    fpcopy(&temp, &pstride);
    fpshr(temp, 1);
    fpadd2(&cr, &temp);
    if(!smooth)
      output[1] = escapeTimeFP(&cr, &ci);
    else
      output[1] = escapeTimeFPSmooth(&cr, &ci);
    fpadd2(&ci, &temp);
    if(!smooth)
      output[2] = escapeTimeFP(&cr, &ci);
    else
      output[2] = escapeTimeFPSmooth(&cr, &ci);
    fpsub2(&cr, &temp);
    if(!smooth)
      output[3] = escapeTimeFP(&cr, &ci);
    else
      output[3] = escapeTimeFPSmooth(&cr, &ci);
    pixelsComputed++;
  }
  rv = ssValue(output);
  iters[x + y * winw] = rv;
  return rv;
}

void* fpWorker(void* unused)
{
  while(true)
  {
    int work = atomic_fetch_add_explicit(&workerIndex, 1, memory_order_relaxed);
    if(work >= winw * winh)
      break;
    iters[work] = NOT_COMPUTED;
    if(!supersample)
      getPixelConvRate(work % winw, work / winw);
    else
      getPixelConvRateSS(work % winw, work / winw);
  }
  return NULL;
}

void* simd32Worker(void* unused)
{
  float ps = getValue(&pstride);
  float r0 = getValue(&targetX) - (winw / 2) * ps;
  float i0 = getValue(&targetY) - (winh / 2) * ps;
  float crFloat[8] __attribute__ ((aligned(32)));
  float ciFloat[8] __attribute__ ((aligned(32)));
  float output[8];
  while(true)
  {
    int work;
    if(!supersample)
    {
      //get 8 pixels' worth of work
      //note: memory_order_relaxed because it doesn't matter which thread gets which work
      work = atomic_fetch_add_explicit(&workerIndex, 8, memory_order_relaxed);
      //check for all work being done (wait for join)
      if(work >= winw * winh)
        return NULL;
      for(int i = 0; i < 8; i++)
      {
        crFloat[i] = r0 + (ps * ((work + i) % winw));
        ciFloat[i] = i0 + (ps * ((work + i) / winw));
      }
      if(smooth)
        escapeTimeVec32Smooth(output, crFloat, ciFloat);
      else
        escapeTimeVec32(output, crFloat, ciFloat);
      for(int i = 0; i < 8; i++)
      {
        if(work + i < winw * winh)
        {
          iters[work + i] = output[i];
        }
      }
    }
    else
    {
      //get 2 pixels' worth of work, which is 8 iteration points
      work = atomic_fetch_add_explicit(&workerIndex, 2, memory_order_relaxed);
      if(work >= winw * winh)
        return NULL;
      for(int i = 0; i < 2; i++)
      {
        //get pixel center
        float realBase = r0 + (ps * ((work + i) % winw));
        float imagBase = i0 + (ps * ((work + i) / winw));
        int offset = i * 4;
        crFloat[offset + 0] = realBase - ps / 4;
        crFloat[offset + 2] = crFloat[offset + 0];
        crFloat[offset + 1] = realBase + ps / 4;
        crFloat[offset + 3] = crFloat[offset + 1];
        ciFloat[offset + 0] = imagBase - ps / 4;
        ciFloat[offset + 1] = ciFloat[offset + 0];
        ciFloat[offset + 2] = imagBase + ps / 4;
        ciFloat[offset + 3] = ciFloat[offset + 2];
      }
      if(smooth)
        escapeTimeVec32Smooth(output, crFloat, ciFloat);
      else
        escapeTimeVec32(output, crFloat, ciFloat);
      for(int i = 0; i < 2; i++)
      {
        if(work + i < winw * winh)
          iters[work + i] = ssValue(output + 4 * i);
      }
    }
  }
  return NULL;
}

void* simd64Worker(void* unused)
{
  double ps = getValue(&pstride);
  double r0 = getValue(&targetX) - (winw / 2) * ps;
  double i0 = getValue(&targetY) - (winh / 2) * ps;
  double crDouble[4] __attribute__ ((aligned(32)));
  double ciDouble[4] __attribute__ ((aligned(32)));
  float output[4];
  while(true)
  {
    int work;
    if(!supersample)
    {
      //get 8 pixels' worth of work and update work index simultaneously
      //memory_order_relaxed because it doesn't matter which thread gets which work
      work = atomic_fetch_add_explicit(&workerIndex, 4, memory_order_relaxed);
      //check for all work being done (wait for join)
      if(work >= winw * winh)
        return NULL;
      for(int i = 0; i < 4; i++)
      {
        crDouble[i] = r0 + (ps * ((work + i) % winw));
        ciDouble[i] = i0 + (ps * ((work + i) / winw));
      }
      if(!smooth)
        escapeTimeVec64(output, crDouble, ciDouble);
      else
        escapeTimeVec64Smooth(output, crDouble, ciDouble);
      for(int i = 0; i < 4; i++)
      {
        if(work + i < winw * winh)
          iters[work + i] = output[i];
      }
    }
    else
    {
      work = atomic_fetch_add_explicit(&workerIndex, 1, memory_order_relaxed);
      if(work >= winw * winh)
        return NULL;
      double realBase = r0 + (ps * (work % winw));
      double imagBase = i0 + (ps * (work / winw));
      crDouble[0] = realBase - ps / 4;
      crDouble[2] = crDouble[0];
      crDouble[1] = realBase + ps / 4;
      crDouble[3] = crDouble[1];
      ciDouble[0] = imagBase - ps / 4;
      ciDouble[1] = ciDouble[0];
      ciDouble[2] = imagBase + ps / 4;
      ciDouble[3] = ciDouble[2];
      if(!smooth)
        escapeTimeVec64(output, crDouble, ciDouble);
      else
        escapeTimeVec64Smooth(output, crDouble, ciDouble);
      iters[work] = ssValue(output);
    }
  }
  return NULL;
}

void drawBuf()
{
  workerIndex = 0;
  //machine epsilon values from wikipedia
  float ps = getValue(&pstride);
  void* (*workerFunc)(void*) = fpWorker;
  int nworkers = numThreads;
  if(ps >= EPS_32)
  {
    workerFunc = simd32Worker;
  }
  else if(ps >= EPS_64)
  {
    workerFunc = simd64Worker;
  }
  pthread_t* threads = alloca(nworkers * sizeof(pthread_t));
  for(int i = 0; i < nworkers; i++)
    pthread_create(&threads[i], NULL, workerFunc, NULL);
  for(int i = 0; i < nworkers; i++)
    pthread_join(threads[i], NULL);
  pixelsComputed = winw * winh;
  if(frameBuf)
    colorMap();
  putchar('\r');
}

void drawBufCoarse()
{
}

void getInterestingLocation(int minExpo, const char* cacheFile, bool useCache)
{
  FILE* cache = NULL;
  if(cacheFile && useCache)
  {
    cache = fopen(cacheFile, "rb");
    targetX = fpRead(cache);
    targetY = fpRead(cache);
    fclose(cache);
    return;
  }
  long double initViewport = 4;
  //make a temporary iteration count buffer
  int gpx = 32;                       //size, in pixels, of GIL iteration buffer (must be PoT)
  prec = 1;
  zoomRate = 4;
  targetX = FPCtorValue(prec, 0);
  targetY = FPCtorValue(prec, 0);
  iters = (float*) malloc(gpx * gpx * sizeof(float));
  frameBuf = NULL;
  pstride = FPCtorValue(prec, initViewport / gpx);
  winw = gpx;
  winh = gpx;
  FP fbestPX = FPCtor(1);
  FP fbestPY = FPCtor(1);
  maxiter = 5000;
  while(true)
  {
    if(getApproxExpo(&pstride) <= minExpo)
    {
      if(verbose)
        printf("Stopping because pstride <= 2^%i\n", getApproxExpo(&pstride));
      break;
    }
    if(verbose)
    {
      printf("Pixel stride = %Le. Iter cap = %i\n", getValue(&pstride), maxiter);
      printf("Pstride: ");
      biPrint(&pstride.value);
    }
    for(int i = 0; i < gpx * gpx; i++)
      iters[i] = NOT_COMPUTED;
    drawBuf();
    if(verbose)
    {
      puts("**********The buffer:**********");
      for(int i = 0; i < gpx * gpx; i++)
      {
        printf("%7i ", (int) iters[i]);
        if(i % gpx == gpx - 1)
          puts("");
      }
      puts("*******************************");
    }
    int bestPX = 0;
    int bestPY = 0;
    int bestIters = 0;
    u64 itersum = 0;
    //get point with maximum iteration count (that didn't converge)
    int cands[GIL_CANDIDATES];
    for(int i = 0; i < GIL_CANDIDATES; i++)
      cands[i] = -1;
    int cutoff = 0;
    for(int i = 0; i < gpx * gpx; i++)
    {
      itersum += iters[i];
      if(iters[i] > cutoff)
      {
        //replace minimum cand pt with (i, j) pt
        //first get index of min cand
        int minCand;
        bool allCandsFilled = true;
        for(int j = 0; j < GIL_CANDIDATES; j++)
        {
          if(cands[j] < 0)
          {
            minCand = j;
            allCandsFilled = false;
            break;
          }
          if(iters[cands[j]] < iters[cands[minCand]])
            minCand = j;
        }
        //insert new element
        cands[minCand] = i;
        //get index of min again for updating cutoff
        if(allCandsFilled)
        {
          minCand = 0;
          for(int j = 1; j < GIL_CANDIDATES; j++)
          {
            if(iters[cands[j]] < iters[cands[minCand]])
              minCand = j;
          }
          cutoff = iters[cands[minCand]];
        }
      }
    }
    //select next point randomly from 
    int zoomPixel = cands[mrand % GIL_CANDIDATES];
    bestIters = iters[zoomPixel];
    bestPX = zoomPixel % gpx;
    bestPY = zoomPixel / gpx;
    itersum /= (gpx * gpx); //compute average iter count
    recomputeMaxIter();
    if(bestIters == 0)
    {
      puts("getInterestingLocation() frame contains all converging points!");
      break;
    }
    bool allSame = true;
    int val = iters[0];
    for(int i = 0; i < gpx * gpx; i++)
    {
      if(iters[i] != val)
      {
        allSame = false;
        break;
      }
    }
    if(allSame)
    {
      puts("getInterestingLocation() frame contains all the same iteration count!");
      break;
    }
    int xmove = bestPX - gpx / 2;
    int ymove = bestPY - gpx / 2;
    if(xmove < 0)
    {
      for(int i = xmove; i < 0; i++)
        fpsub2(&targetX, &pstride);
    }
    else if(xmove > 0)
    {
      for(int i = 0; i < xmove; i++)
        fpadd2(&targetX, &pstride);
    }
    if(ymove < 0)
    {
      for(int i = ymove; i < 0; i++)
        fpsub2(&targetY, &pstride);
    }
    else if(ymove > 0)
    {
      for(int i = 0; i < ymove; i++)
        fpadd2(&targetY, &pstride);
    }
    //set screen position to the best pixel
    int prevPrec = prec;
    upgradePrec(true);
    if(verbose && prec != prevPrec)
      printf("*** Increased precision to level %i ***\n", prec);
    //zoom in according to the PoT zoom factor defined above
    printf("target x: ");
    biPrint(&targetX.value);
    printf("target y: ");
    biPrint(&targetY.value);
    fpshr(pstride, zoomRate);
  }
  free(iters); 
  //do not lose any precision when storing targetX, targetY
  puts("Saving target position.");
  if(cacheFile)
  {
    cache = fopen(cacheFile, "wb");
    //write targetX, targetY to the cache file
    fpWrite(&targetX, cache);
    fpWrite(&targetY, cache);
    fclose(cache);
  }
  FPDtor(&pstride);
  FPDtor(&fbestPX);
  FPDtor(&fbestPY);
}

void recomputeMaxIter()
{
  const int normalIncrease = 500 * zoomRate;
  maxiter += normalIncrease;
}

void upgradePrec(bool interactive)
{
  //# of bits desired after leading 1 bit in pstride
  int trailing = 12;
  // Need 2 extra bits to represent sub-pixel supersample points
  if(supersample)
    trailing += 2;
  //note: reserves extra bits according to zoom expo
  int totalBits = 64 * pstride.value.size;
  if(totalBits - lzcnt(&pstride.value) < trailing + zoomRate)
  {
    INCR_PREC(pstride);
    if(interactive)
    {
      INCR_PREC(targetX);
      INCR_PREC(targetY);
    }
    prec++;
    setFPPrec(prec);
  }
}

void downgradePrec(bool interactive)
{
  //can't downgrade if prec is 1
  if(prec == 1)
    return;
  //otherwise, can downgrade if least significant word is 0
  if(pstride.value.val[prec - 1] == 0)
  {
    DECR_PREC(pstride);
    if(interactive)
    {
      DECR_PREC(targetX);
      DECR_PREC(targetY);
    }
    prec--;
    setFPPrec(prec);
  }
}

