#include "mandelbrot.h"

unsigned* colortable;
int winw;
int winh;
int* iters;
unsigned* colors;
bool* outlineHits;
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
int savings;

#define NUM_COLORS 360
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

void initColorTable()
{
  for(int i = 0; i < NUM_COLORS; i++)
  {
    colortable[i] = mrand | 0xFF;
  }
  /*
    colortable = (unsigned*) malloc(NUM_COLORS * sizeof(unsigned));
    int phase = (mrand % 6) * 60;
    for(int i = 0; i < NUM_COLORS; i++)
    {
        int t = (phase + i * 2 + 240) % NUM_COLORS;
        unsigned r = 0;
        unsigned g = 0;
        unsigned b = 0;
        float slope = 255.0 / 120;
        if(t <= 120)
            r = 255 - slope * t;
        if(t >= 240)
            r = 255 - slope * (360 - t);
        if(t <= 240)
            g = 255 - slope * abs((t - 120) % 360);
        if(t >= 120)
            b = 255 - slope * abs((t - 240) % 360);
        colortable[i] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
    }
    */
}

void floodFill(Point p, int val, Point* stack)
{
  //set up stack (remember base)
  Point* base = stack;
  //push the first point
  *(stack++) = p;
  //loop will terminate when all points have been filled
  while(stack != base)
  {
    //Pop the top point
    Point proc = *(--stack);
    //fill the point in iters
    int i = proc.x + proc.y * winw;
    iters[i] = val;
    outlineHits[i] = true;
    //push neighbors on stack that need to be processed
    if(proc.y > 0 && iters[i - winw] == NOT_COMPUTED)
    {
      Point newPoint = {proc.x, proc.y - 1};
      *(stack++) = newPoint;
    }
    if(proc.x > 0 && iters[i - 1] == NOT_COMPUTED)
    {
      Point newPoint = {proc.x - 1, proc.y};
      *(stack++) = newPoint;
    }
    if(proc.y < winh - 1 && iters[i + winw] == NOT_COMPUTED)
    {
      Point newPoint = {proc.x, proc.y + 1};
      *(stack++) = newPoint;
    }
    if(proc.x < winw - 1 && iters[i + 1] == NOT_COMPUTED)
    {
      Point newPoint = {proc.x + 1, proc.y};
      *(stack++) = newPoint;
    }
  }
}

Uint32 getColor(int num)
{
    if(num == NOT_COMPUTED)
        return 0x888888FF;
    if(num == -1)
        return 0x000000FF;                  //in the mandelbrot set = opaque black
    if(num == BLANK)
        return 0xFFFFFFFF;
    //make a steeper gradient for the first few iterations (better image 0)
    return colortable[(num * 3) % NUM_COLORS];
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

static int getPixelState(Point p, int val)
{
  int code = 0;
  Point p1 = {p.x - 1, p.y - 1};
  Point p2 = {p.x, p.y - 1};
  Point p3 = {p.x - 1, p.y};
  if(getPixel(p1) == val)
  {
    outlineHits[p1.x + p1.y * winw] = true;
    code |= 1;
  }
  if(getPixel(p2) == val)
  {
    outlineHits[p2.x + p2.y * winw] = true;
    code |= 2;
  }
  if(getPixel(p3) == val)
  {
    outlineHits[p3.x + p3.y * winw] = true;
    code |= 4;
  }
  if(getPixel(p) == val)
  {
    outlineHits[p.x + p.y * winw] = true;
    code |= 8;
  }
  return code;
}

void fillAll()
{
  bool doGold = false;
  if(doGold)
  {
    for(int i = 0; i < winw; i++)
      for(int j = 0; j < winh; j++)
        getPixel((Point) {i, j});
    return;
  }
  memset(outlineHits, 0, winw * winh);
  Point* pathbuf = malloc(winw * winh * sizeof(Point));
  while(true)
  {
    int i;
    for(i = 0; i < winw * winh; i++)
    {
      if(!outlineHits[i])
      {
        Point p = {i % winw, i / winw};
        int val = getPixel(p);
        printf("Filling shape starting at (%i, %i), val = %i\n", p.x, p.y, val);
        traceOutline(p, val, pathbuf);
        break;
      }
    }
    if(i == winw * winh)
      break;
  }
  return;

  /*
  Point origin = {0, 0};
  traceOutline(origin, getPixel(origin));
  //sweep sequentially through iters
  //find the first point that has different value than point left of it, which hasn't been hit by outline
  for(int y = 0; y < winh; y++)
  {
    for(int x = 0; x < winw - 1; x++)
    {
      int i = x + y * winw;
      if(outlineHits[i] && iters[i + 1] != NOT_COMPUTED && !outlineHits[i + 1])
      {
        Point p = {x + 1, y};
        printf("Filling second shape at (%i, %i)\n", x + 1, y);
        traceOutline(p, iters[i + 1]);
        return;
      }
    }
  }

  //first need to compute all pixels on screen boundary 
  for(int i = 0; i < winw; i++)
    getPixelConvRate(i, 0);
  for(int i = 0; i < winw; i++)
    getPixelConvRate(i, winh - 1);
  for(int i = 1; i < winh - 1; i++)
    getPixelConvRate(0, i);
  for(int i = 1; i < winh - 1; i++)
    getPixelConvRate(winw - 1, i);
  */
  //fill all boundaries in x+ then y+ directions
  //finish frame by filling solid fields (very fast, no math)
  //fillOutlines();
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
      if(prevDir == RIGHT)
        return DOWN;
      else if(prevDir == LEFT)
        return UP;
      else
        CRASH("Invalid pixel state in getExitDir()");
    case 7: return DOWN;
    case 8: return RIGHT;
    case 9:
      if(prevDir == DOWN)
        return LEFT;
      else if(prevDir == UP)
        return RIGHT;
      else
        CRASH("Invalid pixel state in getExitDir()");
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

void traceOutline(Point start, int val, Point* pbuf)
{
  Point* lastPoint = pbuf;
  printf("(%i, %i)\n", start.x, start.y);
  //set initial direction
  int dir = UP;
  Point p = start;
  //remember bounding box of region
  int minx = winw;
  int maxx = 0;
  int miny = winh;
  int maxy = 0;
  puts("Tracing outline.");
  while(true)
  {
    //printf("  at (%i,%i)\n", p.x, p.y);
    //get direction for moving to next point
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
    //if point is same as start, done with boundary
    if(next.x == start.x && next.y == start.y)
      break;
    p = next;
  }
  //if bounding box has w or h 1, done as there is no interior to fill
  int w = maxx - minx;
  int h = maxx - minx;
  if(maxy - miny <= 1 || maxx - minx <= 1)
    return;
  printf("Shape bounding box: (%i, %i) to (%i, %i)\n", minx, miny, maxx, maxy);
  //get stack space for flood fill
  //Point* stack = malloc(w * h * sizeof(Point));
  Point* stack = malloc(winw * winh * sizeof(Point));
  //go back around shape and flood fill at all points (very low overhead if nothing to be done)
  while(true)
  {
    Point p = *(--lastPoint);
    if(iters[p.x + p.y * winw] == val)
      floodFill(p, val, stack);
    if(lastPoint == pbuf)
      break;
  }
  free(stack);
}

int getPixelConvRate(int x, int y)
{
    if(iters[x + y * winw] != NOT_COMPUTED)
        return iters[x + y * winw];
    //printf("Getting conv rate @ %i, %i\n", x, y);
    int rv;
    if(prec < 3)
    {
        long double r = getValue(&pstride) * (x - winw / 2) + getValue(&targetX);
        long double i = getValue(&pstride) * (y - winh / 2) + getValue(&targetY);
        rv = getConvRateLD(r, i);
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
    }
    iters[x + y * winw] = rv;
    //printf("getPixel value: %i\n", rv);
    return rv;
}

int getConvRateFP(FP* real, FP* imag)
{
    //printf("Iterating (%Lf, %Lf)\n", getValue(real), getValue(imag));
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

void fillRect(Rect r)
{
    //printf("Filling rect: (%i, %i), %ix%i\n", r.x, r.y, r.w, r.h);
    //get the exact # of pixels to be computed
    int numPx = 0;
    //get indices of corners
    int upperLeft = r.x + r.y * winw;
    int upperRight = upperLeft + r.w - 1;
    int lowerLeft = upperLeft + (r.h - 1) * winw;
    Point* queue = malloc(2 * (r.w + r.h) * sizeof(Point));
    int count = 0;
    for(int i = 0; i < r.w; i++)
    {
        if(iters[upperLeft + i] == NOT_COMPUTED)
        {
            queue[count].x = r.x + i;
            queue[count].y = r.y;
            count++;
        }
        if(iters[lowerLeft + i] == NOT_COMPUTED)
        {
            queue[count].x = r.x + i;
            queue[count].y = r.y + r.h - 1;
            count++;
        }
    }
    for(int i = 0; i < r.h; i++)
    {
        if(iters[upperLeft + i * winw] == NOT_COMPUTED)
        {
            queue[count].x = r.x;
            queue[count].y = r.y + i;
            count++;
        }
        if(iters[upperRight + i * winw] == NOT_COMPUTED)
        {
            queue[count].x = r.x + r.w - 1;
            queue[count].y = r.y + i;
            count++;
        }
    }
    iteratePointQueue(queue, count);
    free(queue);
    //done if rectangle can't be subdivided & is fully computed
    //done if all boundary points have same iter count
    int iterCount = iters[upperLeft];
    bool allSame = true;
    for(int i = 0; i < r.w; i++)
    {
        if(iters[i + upperLeft] != iterCount ||
           iters[i + lowerLeft] != iterCount)
        {
            allSame = false;
            break;
        }
    }
    for(int i = 1; i < r.h - 1; i++)
    {
        if(iters[i * winw + upperLeft] != iterCount ||
           iters[i * winw + upperRight] != iterCount)
        {
            allSame = false;
            break;
        }
    }
    if(allSame)
    {
        //flood fill the rectangle with iterCount
        for(int i = 1; i < r.w - 1; i++)
        {
            for(int j = 1; j < r.h - 1; j++)
            {
                iters[(i + r.x) + (j + r.y) * winw] = iterCount;
                savings++;
            }
        }
        return;
    }
    //partition the rect into two smaller ones
    //make sure they share all possible edges, but otherwise don't overlap
    //cut along shorter dimension
    //cut at the first different pixel along boundary
    if(r.w > r.h)
    {
        Rect first = {r.x, r.y, r.w / 2, r.h};
        fillRect(first);
        Rect second = {r.x + r.w / 2, r.y, r.w - r.w / 2, r.h};
        fillRect(second);
        return;
    }
    else
    {
        Rect first = {r.x, r.y, r.w, r.h / 2};
        fillRect(first);
        Rect second = {r.x, r.y + r.h / 2, r.w, r.h - r.h / 2};
        fillRect(second);
        return;
    }
}

void iteratePointQueue(Point* queue, int num)
{
    if(num == 0)
        return;
    //have count points to recompute
    //partition evenly across threads
    //don't make more threads than there are points
    int numWorkers = min(numThreads, num);
    pthread_t* threads = alloca(numWorkers * sizeof(pthread_t));
    WorkInfo* wi = alloca(numWorkers * sizeof(WorkInfo));
    for(int i = 0; i < numWorkers; i++)
    {
        //get work range in points for this thread
        int start = i * (double) num / numWorkers;
        int end = (i + 1) * (double) num / numWorkers;
        //make sure all work gets finished
        if(i == numWorkers - 1)
            end = num;
        wi[i].points = &queue[start];
        wi[i].numPoints = end - start;
        pthread_create(&threads[i], NULL, workerFunc, &wi[i]);
    }
    for(int i = 0; i < numWorkers; i++)
        pthread_join(threads[i], NULL);
}

void* workerFunc(void* wiRaw)
{
    WorkInfo* wi = (WorkInfo*) wiRaw;
    MAKE_STACK_FP(pixFloat);
    MAKE_STACK_FP(x);
    MAKE_STACK_FP(yiter);
    MAKE_STACK_FP(pstrideLocal);
    fpcopy(&pstrideLocal, &pstride);
    for(int i = 0; i < wi->numPoints; i++)
        getPixelConvRate(wi->points[i].x, wi->points[i].y);
    return NULL;
}

void* simpleWorkerFunc(void* wiRaw)
{
    SimpleWorkInfo* wi = (SimpleWorkInfo*) wiRaw;
    MAKE_STACK_FP(pixFloat);
    MAKE_STACK_FP(x);
    MAKE_STACK_FP(yiter);
    MAKE_STACK_FP(pstrideLocal);
    fpcopy(&pstrideLocal, &pstride);
    for(int i = wi->start; i < wi->start + wi->num; i++)
        getPixelConvRate(i % winw, i / winw);
    return NULL;
}

void simpleDrawBuf()
{
    for(int i = 0; i < winw * winh; i++)
      iters[i] = NOT_COMPUTED;
    int num = winw * winh;
    pthread_t* threads = alloca(numThreads * sizeof(pthread_t));
    SimpleWorkInfo* wi = alloca(numThreads * sizeof(SimpleWorkInfo));
    for(int i = 0; i < numThreads; i++)
    {
        //get work range in points for this thread
        int start = i * (double) num / numThreads;
        int end = (i + 1) * (double) num / numThreads;
        //make sure all work gets finished
        if(i == numThreads - 1)
            end = num;
        wi[i].start = start;
        wi[i].num = end - start;
        pthread_create(&threads[i], NULL, simpleWorkerFunc, &wi[i]);
    }
    for(int i = 0; i < numThreads; i++)
        pthread_join(threads[i], NULL);
    reduceIters(iters, 2000 / winw, winw, winh);
    if(colors)
    {
        for(int i = 0; i < winw * winh; i++)
            colors[i] = getColor(iters[i]);
    }
}

void fastDrawBuf()
{
    if(verbose)
        printf("Drawing buf with pstride = %Le\n", getValue(&pstride));
    savings = 0;
    for(int i = 0; i < winw * winh; i++)
      iters[i] = NOT_COMPUTED;
    fillAll();
    if(colors)
    {
        for(int i = 0; i < winw * winh; i++)
            colors[i] = getColor(iters[i]);
    }
    if(verbose)
        printf("Saved %i of %i pixels (%f%%)\n", savings, winw * winh, 100.0 * savings / (winw * winh));
}

void recomputeMaxIter()
{
    const int normalIncrease = 2500 * zoomRate;
    maxiter += normalIncrease;
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
    int gpx = 16;                       //size, in pixels, of GIL iteration buffer (must be PoT)
    prec = 1;
    globalUpdatePrec(prec);
    zoomRate = 2;
    targetX = FPCtorValue(prec, 0);
    targetY = FPCtorValue(prec, 0);
    iters = (int*) malloc(gpx * gpx * sizeof(int));
    colors = NULL;
    pstride = FPCtorValue(prec, initViewport / gpx);
    winw = gpx;
    winh = gpx;
    FP fbestPX = FPCtor(1);
    FP fbestPY = FPCtor(1);
    maxiter = 300;
    while(true)
    {
        if(getApproxExpo(&pstride) <= minExpo)
        {
            if(verbose)
              printf("Stopping because pstride <= 2^%i\n", getApproxExpo(&pstride));
            break;
        }
        printf("Pixel stride = %Le, iter cap is %i\n", getValue(&pstride), maxiter);
        printf("Pstride as int: ");
        biPrint(&pstride.value);
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
        printf("Zooming toward pixel (%i, %i)\n", bestPX, bestPY);
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
            globalUpdatePrec(prec);
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

bool upgradePrec()
{
  //min # of trailing zeroes in reserve
  int trailing = 12;
  u64 mask = 1;
  bool rv = false;
  u64 lastWord = pstride.value.val[prec - 1];
  for(int i = 0; i < zoomRate + trailing; i++)
  {
    if(lastWord & mask)
    {
      rv = true;
      break;
    }
    mask <<= 1;
  }
  return rv;
}

int main(int argc, const char** argv)
{
    //Process cli arguments
    //set all the arguments to default first
    const char* targetCache = NULL;
    bool useTargetCache = false;
    numThreads = 4;
    const int defaultWidth = 640;
    const int defaultHeight = 480;
    int imageWidth = defaultWidth;
    int imageHeight = defaultHeight;
    const char* resumeFile = NULL;
    verbose = false;
    deepestExpo = -300;
    int seed = 0;
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
    getInterestingLocation(deepestExpo, targetCache, useTargetCache);
    prec = 1;
    globalUpdatePrec(prec);
    zoomRate = 1;
    iters = malloc(imageWidth * imageHeight * sizeof(int));
    colors = malloc(imageWidth * imageHeight * sizeof(unsigned));
    outlineHits = malloc(imageWidth * imageHeight * sizeof(bool));
    winw = imageWidth;
    winh = imageHeight;
    pstride = FPCtorValue(prec, 4.0 / imageWidth);
    printf("Will zoom towards %.19Lf, %.19Lf\n", getValue(&targetX), getValue(&targetY));
    maxiter = 10;
    //maxiter = 10000;
    colortable = (Uint32*) malloc(sizeof(Uint32) * 360);
    initColorTable();
    filecount = 0;
    //resume file: filecount, last maxiter, prec
    while(getApproxExpo(&pstride) >= deepestExpo)
    {
        time_t start = time(NULL);
        fastDrawBuf();
        //simpleDrawBuf();
        writeImage();
        return 0;
        fpshrOne(pstride);
        recomputeMaxIter();
        int timeDiff = time(NULL) - start;
        printf("Image #%i took %i seconds. ", filecount - 1, timeDiff);
        if(verbose)
        {
            printf("iter cap: %i, ", maxiter);
            printf("precision: %i, ", prec);
            printf("px/sec/thread: %i\n", winw * winh / max(1, timeDiff) / numThreads);
        }
        else
            puts(".");
        if(upgradePrec())
        {
            INCR_PREC(pstride);
            prec++;
            globalUpdatePrec(prec);
            if(verbose)
                printf("*** Increasing precision to level %i (%i bits) ***\n",
                    prec, 64 * prec);
        }
    }
    free(iters);
    free(colors);
    free(colortable);
}

