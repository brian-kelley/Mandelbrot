#include "mandelbrot.h"
#include "kernels.c"

int winw;
int winh;
float* iters;
unsigned* frameBuf;
int lastOutline;
FP targetX;
FP targetY;
FP pstride;
int filecount;
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

const int monitorWidth = 79;

#define EPS_32 (1e-07)
#define EPS_64 (1e-15)
#define EPS_80 (1e-19)

// Fetch + add to get next work unit
// Set to 0 by main thread at frame start
_Atomic int workerIndex;

void* monitorFunc(void* unused)
{
  double progress = 0;
  double width = monitorWidth - 2;
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
  sprintf(name, "%s/mandel%i.png", outputDir, filecount);
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
  maxiter = 10;
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
    if(upgradePrec())
    {
      INCR_PREC(pstride);
      INCR_PREC(targetX);
      INCR_PREC(targetY);
      prec++;
      if(verbose)
        printf("*** Increased precision to level %i ***\n", prec);
    }
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

bool upgradePrec()
{
  //# of bits desired after leading 1 bit in pstride
  int trailing = 12;
  // Need 2 extra bits to represent sub-pixel supersample points
  if(supersample)
    trailing += 2;
  //note: reserves extra bits according to zoom expo
  int totalBits = 64 * pstride.value.size;
  if(totalBits - lzcnt(&pstride.value) < trailing + zoomRate)
    return true;
  return false;
}

int main(int argc, const char** argv)
{
  //Process cli arguments
  //set all the arguments to default first
  const char* targetCache = "target.bin";
  bool useTargetCache = false;
  numThreads = 1;
  const int defaultWidth = 640;
  const int defaultHeight = 480;
  int imageWidth = defaultWidth;
  int imageHeight = defaultHeight;
  const char* resumeFile = NULL;
  outputDir = "output";
  smooth = false;
  supersample = false;
  verbose = false;
  deepestExpo = -30;
  int seed = 0;
  bool customPosition = false;
  long double inputX, inputY;
  int imgSkip = 0;
#ifdef INTERACTIVE
  //if interactive support enabled, default to interactive mode
  bool interactive = true;
#else
  bool interactive = false;
#endif
  for(int i = 1; i < argc; i++)
  {
    if(strcmp(argv[i], "-n") == 0)
    {
      //# of threads is next
      sscanf(argv[++i], "%i", &numThreads);
      //sanity check it
      if(numThreads < 1)
      {
        puts("Can't have < 1 threads.");
        numThreads = 1;
      }
    }
    else if(strcmp(argv[i], "--targetcache") == 0)
      targetCache = argv[++i];
    else if(strcmp(argv[i], "--usetargetcache") == 0)
      useTargetCache = true;
    else if(strcmp(argv[i], "--size") == 0)
    {
      if(2 != sscanf(argv[++i], "%ix%i", &imageWidth, &imageHeight))
      {
        puts("Size must be specified in the format <width>x<height>");
        imageHeight = defaultWidth;
        imageHeight = defaultHeight;
      }
    }
    else if(strcmp(argv[i], "--resume") == 0)
      resumeFile = argv[++i];
    else if(strcmp(argv[i], "--verbose") == 0)
      verbose = true;
    else if(strcmp(argv[i], "--depth") == 0)
      sscanf(argv[++i], "%i", &deepestExpo);
    else if(strcmp(argv[i], "--seed") == 0)
      sscanf(argv[++i], "%i", &seed);
    else if(strcmp(argv[i], "--smooth") == 0)
      smooth = true;
    else if(strcmp(argv[i], "--output") == 0)
      outputDir = argv[++i];
    else if(strcmp(argv[i], "--supersample") == 0)
      supersample = true;
    else if(strcmp(argv[i], "--start") == 0)
      sscanf(argv[++i], "%i", &imgSkip);
    else if(strcmp(argv[i], "--cli") == 0)
      interactive = false;
    else if(strcmp(argv[i], "--color") == 0)
    {
      i++;
      if(strcmp(argv[i], "sunset") == 0)
        colorMap = colorSunset;
      else if(strcmp(argv[i], "galaxy") == 0)
        colorMap = colorGalaxy;
      else
      {
        printf("Invalid color scheme: \"%s\"\n", argv[i]);
        puts("Valid color schemes:");
        puts("sunset");
        puts("galaxy");
        exit(EXIT_FAILURE);
      }
    }
    else if(strcmp(argv[i], "--position") == 0)
    {
      customPosition = true;
      int valid = 0;
      valid += sscanf(argv[++i], "%Lf", &inputX);
      valid += sscanf(argv[++i], "%Lf", &inputY);
      if(valid != 2)
      {
        puts("Invalid x/y formatting! Will generate location instead.");
        customPosition = false;
      }
    }
  }
  if(seed == 0)
    seed = clock();
  srand(seed);
  printf("Running on %i thread(s).\n", numThreads);
  if(targetCache && useTargetCache)
    printf("Will read target location from \"%s\"\n", targetCache);
  else if(targetCache)
    printf("Will write target location to \"%s\"\n", targetCache);
  printf("Will output %ix%i images.\n", imageWidth, imageHeight);
#ifdef INTERACTIVE 
  if(interactive)
  {
    interactiveMain(1280, 720, 1024, 640);
  }
#endif
  if(customPosition)
  {
    targetX = FPCtorValue(2, inputX);
    targetY = FPCtorValue(2, inputY);
  }
  else
    getInterestingLocation(deepestExpo, targetCache, useTargetCache);
  prec = 1;
  zoomRate = 1;
  winw = imageWidth;
  winh = imageHeight;
  iters = malloc(winw * winh * sizeof(float));
  frameBuf = malloc(winw * winh * sizeof(unsigned));
  imgScratch = malloc(winw * winh * sizeof(float));
  pstride = FPCtorValue(prec, 4.0 / imageWidth);
  printf("Will zoom towards %.19Lf, %.19Lf\n", getValue(&targetX), getValue(&targetY));
  maxiter = 80000;
  filecount = 0;
  for(int i = 0; i < imgSkip; i++)
  {
    fpshrOne(pstride);
    recomputeMaxIter();
    if(upgradePrec())
    {
      INCR_PREC(pstride);
      prec++;
      setFPPrec(prec);
    }
    filecount++;
  }
  //resume file: filecount, last maxiter, prec
  while(getApproxExpo(&pstride) >= deepestExpo)
  {
    u64 startCycles = getTime();
    time_t startTime = time(NULL);
    pthread_create(&monitor, NULL, monitorFunc, NULL);
    drawBuf();
    u64 nclocks = getTime() - startCycles;
    int sec = time(NULL) - startTime;
    pthread_join(monitor, NULL);
    //clear monitor bar and carriage return
    putchar('\r');
    for(int i = 0; i < monitorWidth; i++)
      putchar(' ');
    putchar('\r');
    double cyclesPerIter = (double) (numThreads * nclocks) / totalIters();
    if(supersample)
      cyclesPerIter /= 4;
    printf("Image #%i took %i second", filecount, sec);
    if(sec != 1)
      putchar('s');
    if(verbose)
    {
      int precBits;
      long double psval = getValue(&pstride);
      if(psval > EPS_32)
        precBits = 32;
      else if(psval > EPS_64)
        precBits = 64;
      else if(psval > EPS_80)
        precBits = 80;
      else
        precBits = 64 * prec;
      printf(" (%.1f cycles / iter, %i max iters, %i bit precision)", 
          cyclesPerIter, maxiter, precBits);
    }
    putchar('\n');
    writeImage();
    fpshrOne(pstride);
    recomputeMaxIter();
    if(upgradePrec())
    {
      INCR_PREC(pstride);
      prec++;
      setFPPrec(prec);
    }
    filecount++;
  }
  free(imgScratch);
  free(frameBuf);
  free(iters);
}

