#include "boundary.h"

OutlineScratch* oscratch;

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

void fastDrawBuf()
{
  pixelsComputed = 0;
  for(int i = 0; i < winw * winh; i++)
    iters[i] = NOT_COMPUTED;
  fillAll(oscratch);
}

