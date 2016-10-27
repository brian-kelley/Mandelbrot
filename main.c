#include "mandelbrot.h"
#include "interactive.h"

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
  if(!interactive)
  {
    if(customPosition)
    {
      targetX = FPCtorValue(2, inputX);
      targetY = FPCtorValue(2, inputY);
    }
    else
      getInterestingLocation(deepestExpo, targetCache, useTargetCache);
  }
  prec = 1;
  zoomRate = 1;
  winw = imageWidth;
  winh = imageHeight;
  iters = malloc(winw * winh * sizeof(float));
  frameBuf = malloc(winw * winh * sizeof(unsigned));
  imgScratch = malloc(winw * winh * sizeof(float));
  pstride = FPCtorValue(prec, 4.0 / imageWidth);
  printf("Will zoom towards %.19Lf, %.19Lf\n", getValue(&targetX), getValue(&targetY));
  if(interactive)
    maxiter = 1000;
  else
    maxiter = 80000;
  zoomDepth = 0;
  if(interactive)
  {
    interactiveMain(1280, 720, winw, winh);
  }
  for(int i = 0; i < imgSkip; i++)
  {
    fpshrOne(pstride);
    recomputeMaxIter();
    upgradePrec(false);
    zoomDepth ++;
  }
  //resume file: zoomDepth, last maxiter, prec
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
    for(int i = 0; i < MONITOR_WIDTH; i++)
      putchar(' ');
    putchar('\r');
    double cyclesPerIter = (double) (numThreads * nclocks) / totalIters();
    if(supersample)
      cyclesPerIter /= 4;
    printf("Image #%i took %i second", zoomDepth, sec);
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
    fpshr(pstride, zoomRate);
    recomputeMaxIter();
    upgradePrec(false);
    zoomDepth += zoomRate;
  }
  free(imgScratch);
  free(frameBuf);
  free(iters);
}

