#include "mandelbrot.h"
#include "kernels.c"
#include "stdatomic.h"

int winw;
int winh;
float* iters;             //escape time count for whole image
unsigned* frameBuf;       //RGBA 8888 colors for whole image
Bitset computed;          //Bit i is whether pixel i is known
int* workq;               //simple "queue" of pixels to compute for a refinement step
_Atomic int workCounter;  //index of next work unit
_Atomic int pixelsDone;   //total number of pixels finalized, including savings
int workSize;             //number of pixels in workq
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
int refinement;
bool runWorkers;
_Atomic int savings;
float iterScale;
int gridSize;

void* monitorFunc(void* unused)
{
  double progress = 0;
  double width = MONITOR_WIDTH - 2;
  while(progress < (width - 1e-6))
  {
    progress = width * atomic_load_explicit(&pixelsDone, memory_order_relaxed) / (winw * winh);
    //move to start of line
    putchar('\r');
    putchar('[');
    int i;
    for(i = 0; i < (int) progress; i++)
      putchar('=');
    for(; i < (int) width; i++)
      putchar(' ');
    putchar(']');
    fflush(stdout);
    usleep(100000); //wake monitor to update 10 times per sec
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

const char* getPrecString()
{
  static char precStrBuf[64];
  long double psval = getValue(&pstride);
  if(psval > EPS_32)
    return "32-bit (float)";
  else if(psval > EPS_64)
    return "64-bit (double)";
  else if(psval > EPS_80)
    return "80-bit (long double)";
  else
  {
    sprintf(precStrBuf, "%i-bit (software fixed-point)", 64 * prec);
    return precStrBuf;
  }
}

void writeImage()
{
  colorMap();
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
  im.palette = colors; \
  im.numColors = sizeof(colors) / sizeof(Uint32); \
  im.cycles = 1; \
  im.period = 1;

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
    0.16
  };
  SET_UP_COLORMAP;
  colorHistWeighted(&im, weights);
}

void colorBasicExpo()
{
  Uint32 colors[] =
  {
    0xFF0000FF, //red
    0xFF8800FF, //orange
    0xFFFF00FF, //yellow
    0x00BB00FF, //green
    0x0000FFFF, //blue
    0x550055FF  //dark purple
  };
  SET_UP_COLORMAP;
  colorExpoCyclic(&im, 0.6);
}

void colorBasicLog()
{
  Uint32 colors[] =
  {
    0xFF0000FF, //red
    0xFF8800FF, //orange
    0xFFFF00FF, //yellow
    0x00BB00FF, //green
    0x0000FFFF, //blue
    0x550055FF  //dark purple
  };
  SET_UP_COLORMAP;
  colorLogCyclic(&im);
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
  if(supersample)
    n *= 4;
  return n;
}

float getPixelConvRate(int x, int y)
{
  //printf("Getting conv rate @ %i, %i\n", x, y);
  long double tx = getValue(&targetX);
  long double ty = getValue(&targetY);
  long double ps = getValue(&pstride);
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
  setBit(&computed, x + y * winw, 1);
  return rv;
}

float getPixelConvRateSS(int x, int y)
{
  //printf("Getting conv rate @ %i, %i\n", x, y);
  long double tx = getValue(&targetX);
  long double ty = getValue(&targetY);
  long double ps = getValue(&pstride);
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
  setBit(&computed, x + y * winw, 1);
  return rv;
}

//Get 1 pixel to work on, (return false if no work left)
static bool fetchWorkPixel(int* px)
{
  int work = atomic_fetch_add_explicit(&workCounter, 1, memory_order_relaxed);
  if(workSize == 0 && work >= winw * winh)
    return false;
  else if(workSize != 0 && work >= workSize)
    return false;
  if(workSize == 0)
  {
    //work is already the pixel index
    *px = work;
    setBit(&computed, *px, 1);
  }
  else
  {
    //work is an index in workq, get the pixel index
    *px = workq[work];
    setBit(&computed, *px, 1);
  }
  return true;
}

void abortWorkers()
{
  runWorkers = false;
}

void* fpWorker(void* unused)
{
  while(true)
  {
    if(!runWorkers)
      return NULL;
    int work;
    if(!fetchWorkPixel(&work))
      return NULL;
    if(!supersample)
      getPixelConvRate(work % winw, work / winw);
    else
      getPixelConvRateSS(work % winw, work / winw);
    //mark pixel as computed
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
    if(!runWorkers)
      return NULL;
    if(!supersample)
    {
      int work[8];
      //get 8 pixels' worth of work
      for(int i = 0; i < 8; i++)
      {
        if(!fetchWorkPixel(work + i))
        {
          if(i == 0)
          {
            //no work left to do
            return NULL;
          }
          else
            work[i] = -1;
        }
        if(work[i] >= 0)
        {
          crFloat[i] = r0 + (ps * (work[i] % winw));
          ciFloat[i] = i0 + (ps * (work[i] / winw));
        }
        else
        {
          //dummy work that diverges immediately
          crFloat[i] = 4;
          ciFloat[i] = 4;
        }
      }
      if(smooth)
        escapeTimeVec32Smooth(output, crFloat, ciFloat);
      else
        escapeTimeVec32(output, crFloat, ciFloat);
      for(int i = 0; i < 8; i++)
      {
        if(work[i] >= 0 && work[i] + i < winw * winh)
        {
          iters[work[i]] = output[i];
        }
      }
    }
    else
    {
      //get 2 pixels' worth of work, which is 8 iteration points
      int work[2];
      for(int i = 0; i < 2; i++)
      {
        if(!fetchWorkPixel(work + i))
        {
          if(i == 0)
            return NULL;
          else
            work[i] = -1;
        }
        //get pixel center
        float realBase = r0 + (ps * (work[i] % winw));
        float imagBase = i0 + (ps * (work[i] / winw));
        int offset = i * 4;
        if(work[i] >= 0)
        {
          crFloat[offset + 0] = realBase - ps / 4;
          crFloat[offset + 2] = crFloat[offset + 0];
          crFloat[offset + 1] = realBase + ps / 4;
          crFloat[offset + 3] = crFloat[offset + 1];
          ciFloat[offset + 0] = imagBase - ps / 4;
          ciFloat[offset + 1] = ciFloat[offset + 0];
          ciFloat[offset + 2] = imagBase + ps / 4;
          ciFloat[offset + 3] = ciFloat[offset + 2];
        }
        else
        {
          crFloat[offset + 0] = 4;
          crFloat[offset + 2] = 4;
          crFloat[offset + 1] = 4;
          crFloat[offset + 3] = 4;
          ciFloat[offset + 0] = 4;
          ciFloat[offset + 1] = 4;
          ciFloat[offset + 2] = 4;
          ciFloat[offset + 3] = 4;
        }
      }
      if(smooth)
        escapeTimeVec32Smooth(output, crFloat, ciFloat);
      else
        escapeTimeVec32(output, crFloat, ciFloat);
      for(int i = 0; i < 2; i++)
      {
        if(work[i] >= 0 && work[i] < winw * winh)
        {
          iters[work[i]] = ssValue(output + 4 * i);
        }
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
    if(!runWorkers)
    {
      return NULL;
    }
    if(!supersample)
    {
      int work[4];
      for(int i = 0; i < 4; i++)
      {
        if(!fetchWorkPixel(work + i))
        {
          if(i == 0)
            return NULL;
          else
            work[i] = -1;
        }
        if(i >= 0)
        {
          crDouble[i] = r0 + (ps * (work[i] % winw));
          ciDouble[i] = i0 + (ps * (work[i] / winw));
        }
        else
        {
          crDouble[i] = 4;
          ciDouble[i] = 4;
        }
      }
      if(!smooth)
        escapeTimeVec64(output, crDouble, ciDouble);
      else
        escapeTimeVec64Smooth(output, crDouble, ciDouble);
      for(int i = 0; i < 4; i++)
      {
        if(work[i] >= 0 && work[i] < winw * winh)
        {
          iters[work[i]] = output[i];
        }
      }
    }
    else
    {
      int work;
      if(!fetchWorkPixel(&work))
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

static void launchWorkers()
{
  workCounter = 0;
  void* (*workerFunc)(void*) = fpWorker;
  int nworkers = numThreads;
  //select kernel based on required precision
  long double ps = getValue(&pstride);
  if(ps >= EPS_32)
    workerFunc = simd32Worker;
  else if(ps >= EPS_64)
    workerFunc = simd64Worker;
  //note: all threads should finish at almost exactly the same time
  pthread_t* threads = alloca(nworkers * sizeof(pthread_t));
  for(int i = 0; i < nworkers; i++)
    pthread_create(&threads[i], NULL, workerFunc, NULL);
  for(int i = 0; i < nworkers; i++)
    pthread_join(threads[i], NULL);
}

void drawBuf()
{
  savings = 0;
  pixelsDone = 0;
  for(int i = 0; i < winw * winh; i++)
  {
    iters[i] = -2;
  }
  runWorkers = true;
  refinement = 0;
  while(runWorkers && atomic_load(&pixelsDone) < winw * winh)
  {
    refinementStep();
  }
  //runWorkers can be set to false during the process to interrupt workers
  //must check if all pixels are actually computed
  if(atomic_load(&pixelsDone) == winw * winh)
  {
    gridSize = 1;
    refinement = -1;
  }
  pixelsComputed = winw * winh - savings;
}

void drawBufQuick()
{
  savings = 0;
  for(int i = 0; i < winw * winh; i++)
  {
    iters[i] = -2;
  }
  runWorkers = true;
  refinement = 0;
  while(atomic_load(&pixelsDone) < winw * winh)
    refinementStepQuick();
  if(atomic_load(&pixelsDone) == winw * winh)
  {
    gridSize = 1;
    refinement = -1;
  }
  pixelsComputed = winw * winh;
}

void refinementStep()
{
  workSize = 0;
  //iterate over blocks for the current refinement level
  //note: important not to include duplicate pixels in workq
  //positions = (winw / 2^refinement * i) = (winw * i) >> refinement
  int prevlox = -1;
  int prevloy = -1;
  //iterate over blocks
  for(int bi = 0; bi < (1 << refinement); bi++)
  {
    int lox = (winw * bi) >> refinement;
    int hix = (winw * (bi + 1)) >> refinement;
    if(lox == prevlox)
      continue;
    for(int bj = 0; bj < (1 << refinement); bj++)
    {
      int loy = (winh * bj) >> refinement;
      int hiy = (winh * (bj + 1)) >> refinement;
      if(loy == prevloy)
        continue;
      //go along upper and left boundary of block, add each non-computed pixel to workq
      for(int x = lox; x < hix; x++)
      {
        if(!getBit(&computed, x + loy * winw) && x < winw)
        {
          workq[workSize++] = x + loy * winw;
          //mark pixel as computed because it will actually be computed before it is read again
          setBit(&computed, x + loy * winw, 1);
        }
      }
      for(int y = loy + 1; y < hiy - 1; y++)
      {
        if(!getBit(&computed, lox + y * winw) && y < winh)
        {
          workq[workSize++] = lox + y * winw;
        }
      }
      prevloy = loy;
    }
    prevlox = lox;
  }
  //add right and bottom edges of image (once per drawBuf only)
  if(refinement == 0)
  {
    for(int x = 0; x < winw; x++)
    {
      int index = (winh - 1) * winw + x;
      if(!getBit(&computed, index))
      {
        workq[workSize++] = index;
      }
    }
    for(int y = 0; y < winh; y++)
    {
      int index = (y + 1) * winw - 1;
      if(!getBit(&computed, index))
      {
        workq[workSize++] = index;
      }
    }
  }
  //do the work
  launchWorkers();
  atomic_fetch_add_explicit(&pixelsDone, workSize, memory_order_relaxed);
  //iterate over blocks again & check if boundary is all one value
  //if so, flood fill block with the value & mark interior as computed
  for(int bi = 0; bi < (1 << refinement); bi++)
  {
    int lox = (winw * bi) >> refinement;
    int hix = (winw * (bi + 1)) >> refinement;
    if(hix - lox <= 2)
      continue;
    if(hix >= winw)
      hix = winw - 1;
    for(int bj = 0; bj < (1 << refinement); bj++)
    {
      int loy = (winh * bj) >> refinement;
      int hiy = (winh * (bj + 1)) >> refinement;
      if(hiy - loy <= 2)
        continue;
      if(hiy >= winh)
        hiy = winh - 1;
      //go along boundary of block
      float val = iters[lox + loy * winw];
      bool allSame = true;
      for(int x = lox; x < hix; x++)
      {
        if(iters[x + loy * winw] != val || iters[x + hiy * winw] != val)
        {
          allSame = false;
          break;
        }
      }
      if(allSame)
      {
        for(int y = loy + 1; y < hiy - 1; y++)
        {
          if(iters[lox + y * winw] != val || iters[hix + y * winw] != val)
          {
            allSame = false;
            break;
          }
        }
      }
      if(allSame)
      {
        //fill block
        for(int x = lox; x < hix; x++)
        {
          for(int y = loy; y < hiy; y++)
          {
            iters[x + y * winw] = val;
            if(!getBit(&computed, x + y * winw))
            {
              atomic_fetch_add_explicit(&savings, 1, memory_order_relaxed);
              atomic_fetch_add_explicit(&pixelsDone, 1, memory_order_relaxed);
              setBit(&computed, x + y * winw, 1);
            }
          }
        }
      }
      else
      {
        //still fill non-computed pixels within block with the block's value
        //they are not actually correct so don't mark them as computed
        for(int x = lox; x < hix; x++)
        {
          for(int y = loy; y < hiy; y++)
          {
            if(!getBit(&computed, x + y * winw))
              iters[x + y * winw] = val;
          }
        }
      }
    }
  }
  if((1 << refinement) >= winw && (1 << refinement) >= winh)
  {
    //done
    refinement = -1;
    gridSize = 1;
    return;
  }
  refinement++;
  gridSize = min(gridSize, max(winw, winh) >> refinement);
}

void refinementStepQuick()
{
  workSize = 0;
  int prevx = -1;
  int prevy = -1;
  //iterate over blocks
  for(int bi = 0; bi < (1 << refinement); bi++)
  {
    int x = (winw * bi) >> refinement;
    if(x == prevx)
      continue;
    for(int bj = 0; bj < (1 << refinement); bj++)
    {
      int y = (winh * bj) >> refinement;
      if(y == prevy)
        continue;
      //go along upper and left boundary of block, add each non-computed pixel to workq
      if(!getBit(&computed, x + y * winw))
        workq[workSize++] = x + y * winw;
      prevy = y;
    }
    prevx = x;
  }
  launchWorkers();
  if(runWorkers == false)
  {
    //just abort immediately, nothing else can be updated
    //work is not wasted either, the pixels that were completed will be saved
    return;
  }
  //go back & fill blocks with color (if blocks are not yet single pixels)
  for(int bi = 0; bi < (1 << refinement); bi++)
  {
    int lox = (winw * bi) >> refinement;
    int hix = (winw * (bi + 1)) >> refinement;
    for(int bj = 0; bj < (1 << refinement); bj++)
    {
      int loy = (winh * bj) >> refinement;
      int hiy = (winh * (bj + 1)) >> refinement;
      //go along upper and left boundary of block, add each non-computed pixel to workq
      for(int x = lox; x < hix; x++)
      {
        for(int y = loy; y < hiy; y++)
        {
          if(x == lox && y == loy)
          {
            continue;
          }
          if(gridSize >= (winw >> refinement))
          {
            iters[x + y * winw] = iters[lox + loy * winw];
            setBit(&computed, x + y * winw, 0);
          }
        }
      }
    }
  }
  if((1 << refinement) >= winw && (1 << refinement) >= winh)
  {
    //done
    refinement = -1;
    gridSize = 1;
    return;
  }
  refinement++;
  gridSize = min(gridSize, max(winw, winh) >> refinement);
}

int _sampleCompare(const void* p1, const void* p2)
{
  const float f1 = *((const float*) p1);
  const float f2 = *((const float*) p2);
  if(f1 == f2)
    return 0;
  if(f1 == -1.0f)
    return 1;
  if(f2 == -1.0f)
    return -1;
  if(f1 < f2)
    return -1;
  else
    return 1;
}

void refineDeepPixels()
{
  //proportion of pixels to fully refine
  //final result not exactly this but will be close and doesn't need to be
  const double proportion = 0.08;
  //number of random samples to take (for getting approx. iter count of pixel at the cutoff)
  const int n = 4096; //not too big, must fit on stack
  float samples[n];
  for(int i = 0; i < n; i++)
  {
    samples[i] = iters[rand() % (winw * winh)];
  }
  //see image.c: weightedHist() for similar pixel statistics
  qsort(samples, n, sizeof(float), _sampleCompare);
  int nonConverged = n;
  while(samples[nonConverged - 1] < 0.0f && nonConverged > 0)
    nonConverged--;
  if(nonConverged == 0)
    return; //nothing left to do, image either done or needs more refinement steps
  float cutoff = samples[(int) ((1.0 - proportion) * nonConverged)];
  printf("Fully refining all pixels with iters >= %f (median is %f)\n", cutoff, samples[nonConverged / 2]);
  //now fully iterate all refinement blocks with value >= cutoff
  workSize = 0;
  for(int i = 0; i < winw * winh; i++)
  {
    if(!getBit(&computed, i) && iters[i] >= cutoff)
    {
      setBit(&computed, i, 1);
      workq[workSize++] = i;
    }
  }
  printf("Computing %i pixels.\n", workSize);
  launchWorkers();
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
    upgradeIters();
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
    printf("Target x: ");
    biPrint(&targetX.value);
    printf("Target y: ");
    biPrint(&targetY.value);
    fpshr(pstride, zoomRate);
  }
  free(iters); 
  //do not lose any precision when storing targetX, targetY
  puts("Saving target position.");
  if(cacheFile)
  {
    saveTargetCache(cacheFile);
  }
  FPDtor(&pstride);
  FPDtor(&fbestPX);
  FPDtor(&fbestPY);
}

void saveTargetCache(const char* cacheFile)
{
  FILE* cache = fopen(cacheFile, "wb");
  //write targetX, targetY to the cache file
  fpWrite(&targetX, cache);
  fpWrite(&targetY, cache);
  fclose(cache);
}

void upgradePrec(bool interactive)
{
  //# of bits desired after leading 1 bit in pstride
  int trailing = 20;
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

void upgradeIters()
{
  maxiter += 400 * zoomRate;
}

void downgradeIters()
{
  maxiter -= 400 * zoomRate;
}

