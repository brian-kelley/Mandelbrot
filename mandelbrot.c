#include "mandelbrot.h"

int winw;
int winh;
float* iters;
unsigned* colors;
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
int zoomRate;
int pixelsComputed;
OutlineScratch* oscratch;

//Fetch + add to get next work unit
_Atomic int workerIndex;

#define NOT_COMPUTED -2
#define BLANK -3
#define OUTSIDE_IMAGE -4

#define GIL_CANDIDATES 2

#define mrand (rand() >> 4)

void writeImage()
{
  char name[32];
  sprintf(name, "output/mandel%i.png", filecount++);
  for(int i = 0; i < winw * winh; i++)
  {
    Uint32 temp = colors[i]; 
    //convert rgba to big endian
    colors[i] = ((temp & 0xFF000000) >> 24) | ((temp & 0xFF0000) >> 8) | ((temp & 0xFF00) << 8) | ((temp & 0xFF) << 24);
  }
  lodepng_encode32_file(name, (unsigned char*) colors, winw, winh);
}

void initOutlineScratch()
{
  printf("Allocating outline scratch space for %i workers.\n", numThreads);
  oscratch = malloc(numThreads * sizeof(OutlineScratch));
  for(int i = 0; i < numThreads; i++)
  {
    OutlineScratch os;
    assert(os.fillStack = malloc(winw * winh * sizeof(Point)));
    assert(os.outlinePoints = malloc(winw * winh * sizeof(Point)));
    oscratch[i] = os;
  }
}

static Point* pushFillNeighbors(Point proc, int replaceVal, Point* stack)
{
  //fill the point in iters
  int i = proc.x + proc.y * winw;
  //push neighbors on stack that need to be processed
  if(proc.y > 0 && iters[i - winw] == replaceVal)
  {
    Point newPoint = {proc.x, proc.y - 1};
    *(stack++) = newPoint;
  }
  if(proc.x > 0 && iters[i - 1] == replaceVal)
  {
    Point newPoint = {proc.x - 1, proc.y};
    *(stack++) = newPoint;
  }
  if(proc.y < winh - 1 && iters[i + winw] == replaceVal)
  {
    Point newPoint = {proc.x, proc.y + 1};
    *(stack++) = newPoint;
  }
  if(proc.x < winw - 1 && iters[i + 1] == replaceVal)
  {
    Point newPoint = {proc.x + 1, proc.y};
    *(stack++) = newPoint;
  }
  return stack;
}

void floodFill(Point p, int val, Point* stack)
{
  //set up stack (remember base)
  int replaceVal = NOT_COMPUTED;
  if(iters[p.x + p.y * winw] != val)
    replaceVal = iters[p.x + p.y * winw];
  Point* base = stack;
  //push the first point(s) to be processed
  if(getPixel(p) == NOT_COMPUTED)
    *(stack++) = p;
  else
    stack = pushFillNeighbors(p, replaceVal, stack);
  //loop will terminate when all points have been filled
  while(stack != base)
  {
    //Pop the top point
    Point proc = *(--stack);
    //fill the point in iters
    int i = proc.x + proc.y * winw;
    //set the pixel
    iters[i] = val;
    //push neighbors on stack that need to be processed
    stack = pushFillNeighbors(proc, replaceVal, stack);
  }
}

static Uint32 rainbowColors[] =
{
  0xFF0000FF, //red
  0xFFA000FF, //orange
  0xF0F000FF, //yellow
  0x009F00FF, //green
  0x00FFFFFF, //cyan
  0x0000FFFF, //blue
  0xFF00FFFF  //violet
};

static Uint32 warmColors[] =
{
  0xFF0000FF, //red
  0xFF9000FF, //orange
  0xE0E000FF, //yellow
  0xFF9000FF  //orange
};

static Uint32 coolColors[] =
{
  0x0000FFFF, //blue
  0xE000A0FF, //violet
  0x7777FFFF, //light blue
  0x00F0F0FF, //cyan
  0x00FF80FF  //green
};

//Not recommended for cyclic coloring
static Uint32 sunsetColors[] =
{
  0x000000FF,   // Black
  0x702963FF,   // Purple
  0xC80815FF,   // Red
  0xFF7518FF,   // Orange
  0xFFD700FF    // Gold
};

static Uint32 rgbColors[] =
{
  0xFF0000FF,
  0x00FF00FF,
  0x0000FFFF
};

static Uint32 testColors[] =
{
  0xFF0000FF,   // red
  0x0000FFFF    // blue
};

static float sunsetWeights[] =
{
  1,
  3,
  2,
  2,
  1
};

static void colorMap()
{
  Image im;
  im.iters = iters;
  im.fb = colors;
  im.w = winw;
  im.h = winh;
  im.palette = rainbowColors;
  im.numColors = 7;
  im.period = 100;
  im.cycles = 1.0;
  //colorLogCyclic(&im);
  colorExpoCyclic(&im, 0.3);
}

void colorTestDrawBuf()
{
  for(int i = 0; i < winw; i++)
  {
    for(int j = 0; j < winh; j++)
    {
      iters[i + j * winw] = i;
    }
  }
  colorMap();
  for(int i = 0; i < winh; i++)
  {
    colors[i + i * winw] = 0xFF;
  }
}

bool inBounds(Point p)
{
  return p.x >= 0 && p.x < winw && p.y >= 0 && p.y < winh;
}

int getPixel(Point p)
{
  if(!inBounds(p))
    return OUTSIDE_IMAGE;
  return getPixelConvRate(p.x, p.y);
}

int getPixelNoCompute(Point p)
{
  if(!inBounds(p))
    return OUTSIDE_IMAGE;
  return iters[p.x + p.y * winw];
}

static int getPixelState(Point p, int val)
{
  int code = 0;
  Point p1 = {p.x - 1, p.y - 1};
  Point p2 = {p.x, p.y - 1};
  Point p3 = {p.x - 1, p.y};
  if(getPixel(p1) == val)
    code |= 1;
  if(getPixel(p2) == val)
    code |= 2;
  if(getPixel(p3) == val)
    code |= 4;
  if(getPixel(p) == val)
    code |= 8;
  return code;
}

void* fillAll(void* osRaw)
{
  OutlineScratch* os = (OutlineScratch*) osRaw;
  memset(os->fillStack, 0, winw * winh * sizeof(Point));
  memset(os->outlinePoints, 0, winw * winh * sizeof(Point));
  //serial-only
  for(int i = 0; i < winw * winh; i++)
  {
    if(iters[i] == NOT_COMPUTED)
    {
      Point p = {i % winw, i / winw};
      int val = getPixel(p);
      if(getPixelNoCompute((Point) {p.x - 1, p.y}) != NOT_COMPUTED &&
          getPixelNoCompute((Point) {p.x + 1, p.y}) != NOT_COMPUTED &&
          getPixelNoCompute((Point) {p.x, p.y - 1}) != NOT_COMPUTED &&
          getPixelNoCompute((Point) {p.x, p.y + 1}) != NOT_COMPUTED)
        continue;
      //if all 4-neighbors have been computed, done
      int state = getPixelState(p, val);
      if(state != 0b1111)
        traceOutline(p, val, os);
    }
  }
  return NULL;
}

static Point movePoint(Point p, int dir)
{
  Point next = p;
  switch(dir)
  {
    case UP:
      next.y--;
      break;
    case DOWN:
      next.y++;
      break;
    case LEFT:
      next.x--;
      break;
    case RIGHT:
      next.x++;
      break;
    default:
      {
        next.x = INVALID;
        next.y = INVALID;
        //puts("Fatal error: invalid direction passed to movePoint!");
        //exit(1);
      }
  }
  return next;
}

static int getExitDir(Point p, int prevDir, int val)
{
  int code = getPixelState(p, val);
  switch(code)
  {
    case 0: CRASH("Invalid pixel state in getExitDir()");
    case 1: return LEFT;
    case 2: return UP;
    case 3: return LEFT;
    case 4: return DOWN;
    case 5: return DOWN;
    case 6:
    {
      if(prevDir == RIGHT)
        return DOWN;
      else if(prevDir == LEFT)
        return UP;
      else
        CRASH("Invalid pixel state in getExitDir()");
    }
    case 7: return DOWN;
    case 8: return RIGHT;
    case 9:
    {
      if(prevDir == DOWN)
        return LEFT;
      else if(prevDir == UP)
        return RIGHT;
      else
        CRASH("Invalid pixel state in getExitDir()");
    }
    case 10: return UP;
    case 11: return LEFT;
    case 12: return RIGHT;
    case 13: return RIGHT;
    case 14: return UP;
    case 15: return INVALID; //CRASH("Invalid pixel state in getExitDir()");
    default: CRASH("Invalid pixel state code.");
  }
  return INVALID;
}

void traceOutline(Point start, int val, OutlineScratch* os)
{
  Point* outlineList = os->outlinePoints;
  Point* lastPoint = outlineList;
  //set initial direction
  int dir = UP;
  Point p = start;
  //remember bounding box of region
  int minx = winw;
  int maxx = 0;
  int miny = winh;
  int maxy = 0;
  while(true)
  {
    //get direction for moving to next point
    //if point is same as start, done with boundary
    dir = getExitDir(p, dir, val);
    if(dir == INVALID)
      return;
    if(p.x < minx)
      minx = p.x;
    if(p.x > maxx)
      maxx = p.x;
    if(p.y < miny)
      miny = p.y;
    if(p.y > maxy)
      maxy = p.y;
    //remember current point
    *(lastPoint++) = p;
    //get next point
    Point next = movePoint(p, dir);
    if(next.x == start.x && next.y == start.y)
      break;
    p = next;
  }
  //if bounding box has w or h 1, done as there is no interior to fill
  int w = maxx - minx;
  int h = maxx - minx;
  if(maxy - miny > 1 && maxx - minx > 1)
  {
    //printf("Shape bounding box: (%i, %i) to (%i, %i)\n", minx, miny, maxx, maxy);
    //get stack space for flood fill
    //Point* stack = malloc(w * h * sizeof(Point));
    //go back around shape and flood fill at all points (very low overhead if nothing to be done)
    while(true)
    {
      Point p = *(--lastPoint);
      Point down = {p.x, p.y + 1};
      Point right = {p.x + 1, p.y};
      if(getPixelNoCompute(p) == val && getPixelNoCompute(right) == val && getPixelNoCompute(down) == val)
      {
        Point fillStart = {p.x + 1, p.y + 1};
        if(getPixelNoCompute(fillStart) == NOT_COMPUTED)
          floodFill(fillStart, val, os->fillStack);
      }
      if(lastPoint == os->fillStack)
        break;
    }
  }
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
  bool reflected = false;
  if(ps > 1e-20)
  {
    //single pixel, double prec
    if(smooth)
      rv = escapeTime64Smooth(tx + (x - winw / 2) * ps, ty + (y - winh / 2) * ps);
    else
      rv = escapeTime64(tx + (x - winw / 2) * ps, ty + (y - winh / 2) * ps);
  }
  else if(ps > 1e-30)
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
    MAKE_STACK_FP(r);
    MAKE_STACK_FP(i);
    MAKE_STACK_FP(temp);
    fpcopy(&r, &pstride);
    loadValue(&temp, x - winw / 2);
    fpmul2(&r, &temp);
    fpcopy(&temp, &targetX);
    fpadd2(&r, &temp);
    fpcopy(&i, &pstride);
    loadValue(&temp, y - winh / 2);
    fpmul2(&i, &temp);
    fpcopy(&temp, &targetY);
    fpadd2(&i, &temp);
    if(smooth)
      rv = escapeTimeFPSmooth(&r, &i);
    else
      rv = escapeTimeFP(&r, &i);
    pixelsComputed++;
    iters[x + y * winw] = rv;
  }
  return rv;
};


void* simpleWorkerFunc(void* unused)
{
  while(true)
  {
    int work = atomic_fetch_add_explicit(&workerIndex, 1, memory_order_relaxed);
    if(work >= winw * winh)
      break;
    iters[work] = NOT_COMPUTED;
    getPixelConvRate(work % winw, work / winw);
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
    //get 8 pixels' worth of work and update work index simultaneously
    //memory_order_relaxed because it doesn't matter which thread gets which work
    int work = atomic_fetch_add_explicit(&workerIndex, 8, memory_order_relaxed);
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
        iters[work + i] = output[i];
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
    //get 8 pixels' worth of work and update work index simultaneously
    //memory_order_relaxed because it doesn't matter which thread gets which work
    int work = atomic_fetch_add_explicit(&workerIndex, 4, memory_order_relaxed);
    //check for all work being done (wait for join)
    if(work >= winw * winh)
      return NULL;
    for(int i = 0; i < 4; i++)
    {
      crDouble[i] = r0 + (ps * ((work + i) % winw));
      ciDouble[i] = i0 + (ps * ((work + i) / winw));
    }
  }
  return NULL;
}

void drawBufSIMD32()
{
  //reset work index (still on 1 thread, no ordering concerns)
  //TODO: Do something with thread-core affinity
  //to avoid clogging SIMD unit with hyperthreads
  atomic_store(&workerIndex, 0);
  pthread_t* threads = alloca(numThreads * sizeof(pthread_t));
  for(int i = 0; i < numThreads; i++)
    pthread_create(&threads[i], NULL, simd32Worker, NULL);
  for(int i = 0; i < numThreads; i++)
    pthread_join(threads[i], NULL);
}

void drawBufSIMD64()
{
  //reset work index (still on 1 thread, no ordering concerns)
  atomic_store(&workerIndex, 0);
  pthread_t* threads = alloca(numThreads * sizeof(pthread_t));
  for(int i = 0; i < numThreads; i++)
    pthread_create(&threads[i], NULL, simd64Worker, NULL);
  for(int i = 0; i < numThreads; i++)
    pthread_join(threads[i], NULL);
}

void simpleDrawBuf()
{
  //machine epsilon values from wikipedia
  const float eps32 = 1e-07;
  const double eps64 = 1e-15;
  float ps = getValue(&pstride);
  if(ps >= eps32)
  {
    puts("Using hardware single precision.");
    drawBufSIMD32();
    pixelsComputed = winw * winh;
  }
  else if(ps >= eps64)
  {
    puts("Using hardware double precision.");
    drawBufSIMD64();
    pixelsComputed = winw * winh;
  }
  else
  {
    printf("Using arbitrary precision (%i bits)\n", prec * 64);
    pixelsComputed = 0;
    SimpleWorkInfo* swi = malloc(numThreads * sizeof(swi));
    pthread_t* threads = malloc(numThreads * sizeof(pthread_t));
    int totalWork = winw * winh;
    for(int i = 0; i < totalWork; i++)
      iters[i] = NOT_COMPUTED;
    for(int i = 0; i < numThreads; i++)
      pthread_create(&threads[i], NULL, simpleWorkerFunc, NULL);
    for(int i = 0; i < numThreads; i++)
      pthread_join(threads[i], NULL);
    free(threads);
    free(swi);
  }
  if(colors)
    colorMap();
}

void fastDrawBuf()
{
  if(verbose)
    printf("Distance between pixels = %.2Le\n", getValue(&pstride));
  pixelsComputed = 0;
  for(int i = 0; i < winw * winh; i++)
    iters[i] = NOT_COMPUTED;
  void* os = oscratch;
  fillAll(os);
  if(colors)
    colorMap();
  return;
  pthread_t* threads = malloc(numThreads * sizeof(pthread_t));
  for(int i = 0; i < numThreads; i++)
    pthread_create(&threads[i], NULL, fillAll, &oscratch[i]);
  for(int i = 0; i < numThreads; i++)
    pthread_join(threads[i], NULL);
  if(colors)
    colorMap();
  free(threads);
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
  colors = NULL;
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
    simpleDrawBuf();
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
    if(verbose)
    {
      printf("target x: ");
      biPrint(&targetX.value);
      printf("target y: ");
      biPrint(&targetY.value);
    }
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
  const int normalIncrease = 400 * zoomRate;
  maxiter += normalIncrease;
}

bool upgradePrec()
{
  //# of bits desired after leading 1 bit in pstride
  int trailing = 12;
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
  const char* targetCache = NULL;
  bool useTargetCache = false;
  numThreads = 1;
  const int defaultWidth = 640;
  const int defaultHeight = 480;
  int imageWidth = defaultWidth;
  int imageHeight = defaultHeight;
  const char* resumeFile = NULL;
  smooth = false;
  verbose = false;
  deepestExpo = -300;
  int seed = 0;
  bool customPosition = false;
  long double inputX, inputY;
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
  colors = malloc(winw * winh * sizeof(unsigned));
  imgScratch = malloc(winw * winh * sizeof(float));
  pstride = FPCtorValue(prec, 4.0 / imageWidth);
  printf("Will zoom towards %.19Lf, %.19Lf\n", getValue(&targetX), getValue(&targetY));
  maxiter = 256;
  initOutlineScratch();
  filecount = 0;
  //resume file: filecount, last maxiter, prec
  while(getApproxExpo(&pstride) >= deepestExpo)
  {
    Time start = getTime();
    simpleDrawBuf();
    if(verbose)
    {
      double proportionComputed = (double) pixelsComputed / (winw * winh);
      printf("Iterated %i pixels (%.1f%%), %.1fx speedup\n",
        pixelsComputed, 100 * proportionComputed, 1 / proportionComputed);
    }
    writeImage();
    double dt = timeDiff(start, getTime());
    fpshrOne(pstride);
    recomputeMaxIter();
    printf("Image #%i took %f seconds (iter cap = %i).\n",
        filecount - 1, dt, maxiter);
    if(upgradePrec())
    {
      INCR_PREC(pstride);
      prec++;
      if(verbose)
        printf("*** Increasing precision to level %i (%i bits) ***\n", prec, 64 * prec);
    }
  }
  free(imgScratch);
  free(colors);
  free(iters);
}

