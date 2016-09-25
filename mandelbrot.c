#include "mandelbrot.h"

int winw;
int winh;
int* iters;
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

COLOR_FUNC(rainbow, rainbowColors, 7, 1000)

static Uint32 warmColors[] =
{
  0xFF0000FF, //red
  0xFF9000FF, //orange
  0xE0E000FF, //yellow
  0xFF9000FF  //orange
};

COLOR_FUNC(warm, warmColors, 4, 1000)

static Uint32 coolColors[] =
{
  0x0000FFFF, //blue
  0xE000A0FF, //violet
  0x7777FFFF, //light blue
  0x00F0F0FF, //cyan
  0x00FFA0FF  //green
};

COLOR_FUNC(cool, coolColors, 5, 1000)

static Uint32 greys[] = 
{
  0x202020FF,
  0xE0E0E0FF
};

COLOR_FUNC(greyscale, greys, 2, 1000);

Uint32 getColor(int num)
{
  if(num == NOT_COMPUTED)
    return 0x888888FF;
  if(num == -1)
    return 0x000000FF;                  //in the mandelbrot set = opaque black
  if(num == BLANK)
    return 0xFFFFFFFF;
  return rainbow(num);
}

void colorTestDrawBuf()
{
  //(1000 colors)
  double k = 2.0 * (1000.0 / winw);
  for(int i = 0; i < winw; i++)
  {
    Uint32 color = getColor(k * i);
    for(int j = 0; j < winh; j++)
      colors[i + j * winw] = color;
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

int getPixelConvRate(int x, int y)
{
  //printf("Getting conv rate @ %i, %i\n", x, y);
  long double tx = getValue(&targetX);
  long double ty = getValue(&targetY);
  long double ps = getValue(&pstride);
  if(iters[x + y * winw] != NOT_COMPUTED)
    return iters[x + y * winw];
  int rv = NOT_COMPUTED;
  bool reflected = false;
  if(ps >= 1e-19)
  {
    //check for reflection across y = 0
    //if viewport contains the line y = 0...
    long double r = ps * (x - winw / 2) + tx;
    long double i = ps * (y - winh / 2) + ty;
    if(fabsl(ty) < ps * (winh / 2))
    {
      //get reflected pixel y for current point
      //first need number of pixels between this pixel and y = 0
      int dy = i / ps;
      int mirroredY = y - 2 * dy;
      //if mirrored position is in bounds, can copy the pixel
      if(mirroredY >= 0 && mirroredY < winh)
      {
        int mirrori = x + mirroredY * winw;
        if(iters[mirrori] != NOT_COMPUTED)
        {
          rv = iters[mirrori];
          //reflected = true;
        }
      }
    }
    if(!reflected)
    {
      rv = getConvRateLD(r, i);
      pixelsComputed++;
    }
  }
  else
  {
    //convert x, y to offsets rel. to viewport center
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
    rv = getConvRateFP(&r, &i);
    pixelsComputed++;
  }
  iters[x + y * winw] = rv;
  return rv;
}

int getConvRateFP(FP* real, FP* imag)
{
  //real, imag make up "c" in z = z^2 + c
  assert(real->value.size == prec && imag->value.size == prec);
  MAKE_STACK_FP(four);
  loadValue(&four, 4);
  MAKE_STACK_FP(zr);
  loadValue(&zr, 0);
  MAKE_STACK_FP(zi);
  loadValue(&zi, 0);
  MAKE_STACK_FP(zrsquare);
  MAKE_STACK_FP(zisquare);
  MAKE_STACK_FP(zri);
  MAKE_STACK_FP(mag);
  int iter = 0;
  for(; iter < maxiter; iter++)
  {
    fpmul3(&zrsquare, &zr, &zr);
    fpmul3(&zisquare, &zi, &zi);
    fpmul3(&zri, &zr, &zi);
    //want 2 * zr * zi
    fpshlOne(zri);
    fpsub3(&zr, &zrsquare, &zisquare);
    fpadd2(&zr, real);
    fpadd3(&zi, &zri, imag);
    fpadd3(&mag, &zrsquare, &zisquare);
    if(mag.value.val[0] >= four.value.val[0])
      break;
  }
  return iter == maxiter ? -1 : iter;
}

int getConvRateLD(long double real, long double imag)
{
  long double zr = 0;
  long double zi = 0;
  long double zrsquare, zisquare, zri, mag;
  int iter = 0;
  for(; iter < maxiter; iter++)
  {
    zrsquare = zr * zr;
    zisquare = zi * zi;
    zri = 2.0 * zr * zi;
    //want 2 * zr * zi
    zr = zrsquare - zisquare;
    zr += real;
    zi = zri + imag;
    mag = zrsquare + zisquare;
    if(mag >= 4.0)
      break;
  }
  return iter == maxiter ? -1 : iter;
}

void* simpleWorkerFunc(void* wi)
{
  SimpleWorkInfo* swi = (SimpleWorkInfo*) wi;
  for(int i = swi->start; i < swi->start + swi->n; i++)
  {
    iters[i] = NOT_COMPUTED;
    getPixelConvRate(i % winw, i / winw);
  }
  return NULL;
}

void* workerFunc(void* wiRaw)
{
  /*
     WorkInfo* wi = (WorkInfo*) wiRaw;
     MAKE_STACK_FP(pixFloat);
     MAKE_STACK_FP(x);
     MAKE_STACK_FP(yiter);
     MAKE_STACK_FP(pstrideLocal);
     fpcopy(&pstrideLocal, &pstride);
     for(int i = 0; i < wi->numPoints; i++)
     getPixelConvRate(wi->points[i].x, wi->points[i].y);
     */
  return NULL;
}

void* simd32Worker(void* unused)
{
  float ps = getValue(&pstride);
  __m256 four = _mm256_set1_ps(4);
  float r0 = getValue(&targetX) - (winw / 2) * ps;
  float i0 = getValue(&targetY) - (winh / 2) * ps;
  float crFloat[8] __attribute__ ((aligned(32)));
  float ciFloat[8] __attribute__ ((aligned(32)));
  int workIters[8] __attribute__ ((aligned(32)));
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
    __m256 cr = _mm256_load_ps(crFloat);
    __m256 ci = _mm256_load_ps(ciFloat);
    __m256 zr = _mm256_setzero_ps();
    __m256 zi = _mm256_setzero_ps();
    __m256i itercount = _mm256_setzero_si256();
    for(int i = 0; i < maxiter; i++)
    {
      __m256 zr2 = _mm256_mul_ps(zr, zr);
      __m256 zi2 = _mm256_mul_ps(zi, zi);
      __m256 mag = _mm256_add_ps(zr2, zi2);
      //compare all 8 magnitudes against 4.0 (in four)
      //predicate 0x1: a < b
      __m256 cmp = _mm256_cmp_ps(mag, four, 2);
      //prepare to add 1 to each int in itercount
      __m256i iteradd = _mm256_set1_epi32(1);
      //but, only add to lanes where four < mag was true
      iteradd = _mm256_and_si256(iteradd, cmp);
      //update itercounts
      itercount = _mm256_add_epi32(itercount, iteradd);
      //check for early break condition
      if(_mm256_testz_ps(cmp, cmp))
      {
        //all pixels have diverged: (mag < 4) false on all lanes
        _mm256_store_si256((__m256i*) workIters, itercount);
        break;
      }
      //zr = zr^2 - zi^2 + cr
      __m256 zri = _mm256_mul_ps(zr, zi);
      zr = _mm256_sub_ps(zr2, zi2);
      zr = _mm256_add_ps(zr, cr);
      zri = _mm256_add_ps(zri, zri);
      zi = _mm256_add_ps(zri, ci);
    }
    _mm256_store_si256((__m256i*) workIters, itercount);
    //take output from itersInt
    for(int i = 0; i < 8; i++)
    {
      if(workIters[i] == maxiter)
        workIters[i] = -1;
    }
    //write to iterbuf
    for(int i = 0; i < 8; i++)
    {
      if(i + work < winw * winh)
        iters[i + work] = workIters[i];
    }
  }
}

void* simd64Worker(void* unused)
{
  return NULL;
}

void drawBufSIMD32()
{
  //reset work index (still on 1 thread, no ordering concerns)
  atomic_store(&workerIndex, 0);
  pthread_t* threads = alloca(numThreads * sizeof(pthread_t));
  for(int i = 0; i < numThreads; i++)
    pthread_create(&threads[i], NULL, simd32Worker, NULL);
  for(int i = 0; i < numThreads; i++)
    pthread_join(threads[i], NULL);
}

void drawBufSIMD64()
{

}

void simpleDrawBuf()
{
  float ps = getValue(&pstride);
  if(ps >= 1e-9)
  {
    drawBufSIMD32();
  }
  else if(ps <= 1e-19)
  {
    drawBufSIMD64();
  }
  else
  {
    pixelsComputed = 0;
    SimpleWorkInfo* swi = malloc(numThreads * sizeof(swi));
    pthread_t* threads = malloc(numThreads * sizeof(pthread_t));
    int totalWork = winw * winh;
    for(int i = 0; i < totalWork; i++)
      iters[i] = NOT_COMPUTED;
    for(int i = 0; i < numThreads; i++)
    {
      swi[i].start = i * (totalWork / numThreads);
      swi[i].n = (i + 1) * (totalWork / numThreads) - swi[i].start;
    }
    for(int i = 0; i < numThreads; i++)
      pthread_create(&threads[i], NULL, simpleWorkerFunc, &swi[i]);
    for(int i = 0; i < numThreads; i++)
      pthread_join(threads[i], NULL);
    free(threads);
    free(swi);
  }
  if(colors)
  {
    //exponentialFilter(iters, winw, winh);
    for(int i = 0; i < winw * winh; i++)
      colors[i] = getColor(iters[i]);
    blockFilter(0.15, colors, winw, winh);
  }
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
  exponentialFilter(iters, winw, winh);
  if(colors)
  {
    for(int i = 0; i < winw * winh; i++)
      colors[i] = getColor(iters[i]);
    blockFilter(0.1, colors, winw, winh);
  }
  return;
  pthread_t* threads = malloc(numThreads * sizeof(pthread_t));
  for(int i = 0; i < numThreads; i++)
    pthread_create(&threads[i], NULL, fillAll, &oscratch[i]);
  for(int i = 0; i < numThreads; i++)
    pthread_join(threads[i], NULL);
  if(colors)
  {
    exponentialFilter(iters, winw, winh);
    for(int i = 0; i < winw * winh; i++)
      colors[i] = getColor(iters[i]);
  }
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
  iters = (int*) malloc(gpx * gpx * sizeof(int));
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
        printf("%7i ", iters[i]);
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
  const int normalIncrease = 750 * zoomRate;
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
  iters = malloc(imageWidth * imageHeight * sizeof(int));
  colors = malloc(imageWidth * imageHeight * sizeof(unsigned));
  winw = imageWidth;
  winh = imageHeight;
  pstride = FPCtorValue(prec, 4.0 / imageWidth);
  printf("Will zoom towards %.19Lf, %.19Lf\n", getValue(&targetX), getValue(&targetY));
  maxiter = 5000;
  initOutlineScratch();
  filecount = 0;
  //resume file: filecount, last maxiter, prec
  while(getApproxExpo(&pstride) >= deepestExpo)
  {
    time_t start = time(NULL);
    simpleDrawBuf();
    //fastDrawBuf();
    if(verbose)
    {
      double proportionComputed = (double) pixelsComputed / (winw * winh);
      printf("Iterated %i pixels (%.1f%%), %.1fx speedup\n",
        pixelsComputed, 100 * proportionComputed, 1 / proportionComputed);
    }
    writeImage();
    return 0;
    fpshrOne(pstride);
    recomputeMaxIter();
    int timeDiff = time(NULL) - start;
    printf("Image #%i took %i seconds (iter cap = %i).\n",
        filecount - 1, timeDiff, maxiter);
    if(upgradePrec())
    {
      INCR_PREC(pstride);
      prec++;
      if(verbose)
        printf("*** Increasing precision to level %i (%i bits) ***\n", prec, 64 * prec);
    }
  }
  free(iters);
  free(colors);
}

