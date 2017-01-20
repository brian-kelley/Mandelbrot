#include "mandelbrot.h"
#include "interactive.h"
#include "test.h"

int main(int argc, const char** argv)
{
  if(argc == 2 && strcmp(argv[1], "--test") == 0)
  {
    testAll();
  }
  //Process cli arguments
  //set all the arguments to default first
  const char* targetCache = "target.bin";
  bool useTargetCache = false;
  numThreads = 4;
  const int defaultWidth = 640;
  const int defaultHeight = 480;
  int imageWidth = defaultWidth;
  int imageHeight = defaultHeight;
  outputDir = "output";
  smooth = false;
  supersample = false;
  verbose = false;
  deepestExpo = -30;
  iterScale = 1;
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
    else if(strcmp(argv[i], "--logcolor") == 0)
    {
      colorMap = colorBasicLog;
    }
    else if(strcmp(argv[i], "--expocolor") == 0)
    {
      colorMap = colorBasicExpo;
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
  if(!interactive)
  {
    printf("Running on %i thread(s).\n", numThreads);
    if(targetCache && useTargetCache)
      printf("Will read target location from \"%s\"\n", targetCache);
    else if(targetCache)
      printf("Will write target location to \"%s\"\n", targetCache);
    printf("Will output %ix%i images.\n", imageWidth, imageHeight);
  }
  prec = 1;
  setFPPrec(prec);
  pstride = FPCtorValue(prec, 4.0 / imageWidth);
  targetX = FPCtor(prec);
  targetY = FPCtor(prec);
  if(customPosition)
  {
    CHANGE_PREC(targetX, 2);
    CHANGE_PREC(targetY, 2);
    loadValue(&targetX, inputX);
    loadValue(&targetY, inputY);
  }
  else if(interactive)
  {
    loadValue(&targetX, -0.5);
    loadValue(&targetY, 0);
  }
  else
  {
    getInterestingLocation(deepestExpo, targetCache, useTargetCache);
  }
  zoomRate = 1;
  winw = imageWidth;
  winh = imageHeight;
  iters = malloc(winw * winh * sizeof(float));
  frameBuf = malloc(winw * winh * sizeof(unsigned));
  imgScratch = malloc(winw * winh * sizeof(float));
  computed = BitsetCtor(winw * winh);
  workq = malloc(winw * winh * sizeof(int));
  if(!interactive)
    printf("Will zoom towards %.19Lf, %.19Lf\n", getValue(&targetX), getValue(&targetY));
  if(interactive)
    maxiter = 1000;
  else
    maxiter = 10000;
  zoomDepth = 0;
  for(int i = 0; i < imgSkip; i++)
  {
    fpshrOne(pstride);
    upgradeIters();
    upgradePrec(false);
    zoomDepth++;
  }
  if(interactive)
  {
    interactiveMain(winw, winh);
  }
  else
  {
    setImageIters(iters);
  }
  //resume file: zoomDepth, last maxiter, prec
  while(getApproxExpo(&pstride) >= deepestExpo)
  {
    u64 startCycles = getTime();
    time_t startTime = time(NULL);
    pthread_create(&monitor, NULL, monitorFunc, NULL);
    clearBitset(&computed);
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
      printf(" (%.1f cycles / iter, %i max iters, %s)", 
          cyclesPerIter, maxiter, getPrecString());
    }
    putchar('\n');
    writeImage();
    fpshr(pstride, zoomRate);
    upgradeIters();
    upgradePrec(false);
    zoomDepth += zoomRate;
  }
  free(imgScratch);
  free(frameBuf);
  free(iters);
}

