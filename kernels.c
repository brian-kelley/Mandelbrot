#include "kernels.h"

float smoothEscapeTime(float intIters, double zr, double zi, double cr, double ci)
{
  if(intIters == -1 || intIters == maxiter)
    return -1;
  const int n = 5;
  double zr2, zi2, zri;
  for(int j = 0; j < n; j++)
  {
    zr2 = zr * zr;
    zi2 = zi * zi;
    zri = 2 * zr * zi;
    zr = zr2 - zi2 + cr;
    zi = zri + ci;
  }
  double finalMag = sqrt(zr * zr + zi * zi);
  float smoothed = intIters + n + 1 - logl(logl(finalMag)) / M_LN2;
  if(smoothed < 1)
    return 1;
  return smoothed;
}

float escapeTimeFP(FP* real, FP* imag)
{
  //real, imag make up "c" in z = z^2 + c
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
      return iter;
  }
  return -1;
}

float escapeTimeFPSmooth(FP* real, FP* imag)
{
  //real, imag make up "c" in z = z^2 + c
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
  int iter;
  for(iter = 0; iter < maxiter; iter++)
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
    {
      break;
    }
  }
  //did not diverge
  return smoothEscapeTime(iter, getValue(&zr), getValue(&zi), getValue(real), getValue(imag));
}

float escapeTime64(double cr, double ci)
{
  printf("maxiter = %i\n", maxiter);
  int iter = 0;
  double zr = 0;
  double zi = 0;
  double zri, zr2, zi2, mag;
  for(; iter < maxiter; iter++)
  {
    zr2 = zr * zr;
    zi2 = zi * zi;
    zri = 2 * zr * zi;
    mag = zr2 + zi2;
    if(mag >= 4)
    {
      break;
    }
    zr = zr2 - zi2 + cr;
    zi = zri + ci;
  }
  return iter == maxiter ? -1 : iter;
}

float escapeTime80(long double cr, long double ci)
{
  int iter = 0;
  long double zr = 0;
  long double zi = 0;
  long double zri, zr2, zi2, mag;
  for(; iter < maxiter; iter++)
  {
    zr2 = zr * zr;
    zi2 = zi * zi;
    zri = 2 * zr * zi;
    mag = zr2 + zi2;
    if(mag >= 4)
    {
      break;
    }
    zr = zr2 - zi2 + cr;
    zi = zri + ci;
  }
  return iter == maxiter ? -1 : iter;
}

float escapeTime64Smooth(double cr, double ci)
{
  int iter = 0;
  double zr = 0;
  double zi = 0;
  double zri, zr2, zi2, mag;
  for(; iter < maxiter; iter++)
  {
    zr2 = zr * zr;
    zi2 = zi * zi;
    zri = 2 * zr * zi;
    mag = zr2 + zi2;
    if(mag >= 4)
    {
      break;
    }
    zr = zr2 - zi2 + cr;
    zi = zri + ci;
  }
  return smoothEscapeTime(iter, zr, zi, cr, ci);
}

float escapeTime80Smooth(long double cr, long double ci)
{
  int iter = 0;
  long double zr = 0;
  long double zi = 0;
  long double zri, zr2, zi2, mag;
  for(; iter < maxiter; iter++)
  {
    zr2 = zr * zr;
    zi2 = zi * zi;
    zri = 2 * zr * zi;
    mag = zr2 + zi2;
    if(mag >= 4)
    {
      break;
    }
    zr = zr2 - zi2 + cr;
    zi = zri + ci;
  }
  return smoothEscapeTime(iter, zr, zi, cr, ci);
}

void escapeTimeVec32(float* out, float* real, float* imag)
{
  __m256 four = _mm256_set1_ps(4);
  __m256 cr = _mm256_load_ps(real);
  __m256 ci = _mm256_load_ps(imag);
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
      break;
    }
    //zr = zr^2 - zi^2 + cr
    __m256 zri = _mm256_mul_ps(zr, zi);
    zr = _mm256_sub_ps(zr2, zi2);
    zr = _mm256_add_ps(zr, cr);
    zri = _mm256_add_ps(zri, zri);
    zi = _mm256_add_ps(zri, ci);
  }
  int tempIters[8] __attribute__ ((aligned(32)));
  _mm256_store_si256((__m256i*) tempIters, itercount);
  //take output from itersInt
  for(int i = 0; i < 8; i++)
  {
    if(tempIters[i] == maxiter)
      out[i] = -1;
    else
      out[i] = tempIters[i];
  }
}

void escapeTimeVec32Smooth(float* out, float* real, float* imag)
{
  __m256 four = _mm256_set1_ps(4);
  __m256 cr = _mm256_load_ps(real);
  __m256 ci = _mm256_load_ps(imag);
  __m256 zr = _mm256_setzero_ps();
  __m256 zi = _mm256_setzero_ps();
  __m256i itercount = _mm256_setzero_si256();
  __m256 prevCmp = _mm256_setzero_ps();
  float finalReal[8] __attribute__ ((aligned(32)));
  float finalImag[8] __attribute__ ((aligned(32)));
  for(int i = 0; i < maxiter; i++)
  {
    __m256 zr2 = _mm256_mul_ps(zr, zr);
    __m256 zi2 = _mm256_mul_ps(zi, zi);
    __m256 mag = _mm256_add_ps(zr2, zi2);
    //compare all 8 magnitudes against 4.0 (in four)
    //predicate 0x1: a < b
    __m256 cmp = _mm256_cmp_ps(mag, four, 2);
    __m256 newDiv = _mm256_xor_ps(cmp, prevCmp);
    prevCmp = cmp;
    _mm256_maskstore_ps(finalReal, newDiv, zr);
    _mm256_maskstore_ps(finalImag, newDiv, zi);
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
      break;
    }
    //zr = zr^2 - zi^2 + cr
    __m256 zri = _mm256_mul_ps(zr, zi);
    zr = _mm256_sub_ps(zr2, zi2);
    zr = _mm256_add_ps(zr, cr);
    zri = _mm256_add_ps(zri, zri);
    zi = _mm256_add_ps(zri, ci);
  }
  int tempIters[8] __attribute__ ((aligned(32)));
  _mm256_store_si256((__m256i*) tempIters, itercount);
  //take output from itersInt
  for(int i = 0; i < 8; i++)
  {
    if(tempIters[i] == maxiter)
      out[i] = -1;
    else
      out[i] = smoothEscapeTime(tempIters[i], finalReal[i], finalImag[i], real[i], imag[i]);
  }
}

void escapeTimeVec64(float* out, double* real, double* imag)
{
  __m256d four = _mm256_set1_pd(4);
  __m256d cr = _mm256_load_pd(real);
  __m256d ci = _mm256_load_pd(imag);
  __m256d zr = _mm256_setzero_pd();
  __m256d zi = _mm256_setzero_pd();
  __m256i itercount = _mm256_setzero_si256();
  for(int i = 0; i < maxiter; i++)
  {
    __m256d zr2 = _mm256_mul_pd(zr, zr);
    __m256d zi2 = _mm256_mul_pd(zi, zi);
    __m256d mag = _mm256_add_pd(zr2, zi2);
    //compare all 8 magnitudes against 4.0 (in four)
    //predicate 0x1: a < b
    __m256d cmp = _mm256_cmp_pd(mag, four, 2);
    //prepare to add 1 to each int in itercount
    __m256i iteradd = _mm256_set1_epi64x(1);
    //but, only add to lanes where four < mag was true
    iteradd = _mm256_and_si256(iteradd, cmp);
    //update itercounts
    itercount = _mm256_add_epi32(itercount, iteradd);
    //check for early break condition
    if(_mm256_testz_pd(cmp, cmp))
    {
      //all pixels have diverged: (mag < 4) false on all lanes
      break;
    }
    //zr = zr^2 - zi^2 + cr
    __m256d zri = _mm256_mul_pd(zr, zi);
    zr = _mm256_sub_pd(zr2, zi2);
    zr = _mm256_add_pd(zr, cr);
    zri = _mm256_add_pd(zri, zri);
    zi = _mm256_add_pd(zri, ci);
  }
  long long tempIters[4] __attribute__ ((aligned(32)));
  _mm256_store_si256((__m256i*) tempIters, itercount);
  //take output from itersInt
  for(int i = 0; i < 4; i++)
  {
    if(tempIters[i] == maxiter)
      out[i] = -1;
    else
      out[i] = tempIters[i];
  }
}

void escapeTimeVec64Smooth(float* out, double* real, double* imag)
{
  __m256d four = _mm256_set1_pd(4);
  __m256d cr = _mm256_load_pd(real);
  __m256d ci = _mm256_load_pd(imag);
  __m256d zr = _mm256_setzero_pd();
  __m256d zi = _mm256_setzero_pd();
  __m256i itercount = _mm256_setzero_si256();
  __m256d prevCmp = _mm256_setzero_pd();
  double finalReal[4] __attribute__ ((aligned(32)));
  double finalImag[4] __attribute__ ((aligned(32)));
  for(int i = 0; i < maxiter; i++)
  {
    __m256d zr2 = _mm256_mul_pd(zr, zr);
    __m256d zi2 = _mm256_mul_pd(zi, zi);
    __m256d mag = _mm256_add_pd(zr2, zi2);
    //compare all 8 magnitudes against 4.0 (in four)
    //predicate 0x1: a < b
    __m256d cmp = _mm256_cmp_pd(mag, four, 2);
    __m256 newDiv = _mm256_xor_pd(cmp, prevCmp);
    prevCmp = cmp;
    _mm256_maskstore_pd(finalReal, newDiv, zr);
    _mm256_maskstore_pd(finalImag, newDiv, zi);
    //prepare to add 1 to each int in itercount
    __m256i iteradd = _mm256_set1_epi64x(1);
    //but, only add to lanes where four < mag was true
    iteradd = _mm256_and_si256(iteradd, cmp);
    //update itercounts
    itercount = _mm256_add_epi32(itercount, iteradd);
    //check for early break condition
    if(_mm256_testz_pd(cmp, cmp))
    {
      //all pixels have diverged: (mag < 4) false on all lanes
      break;
    }
    //zr = zr^2 - zi^2 + cr
    __m256d zri = _mm256_mul_pd(zr, zi);
    zr = _mm256_sub_pd(zr2, zi2);
    zr = _mm256_add_pd(zr, cr);
    zri = _mm256_add_pd(zri, zri);
    zi = _mm256_add_pd(zri, ci);
  }
  long long tempIters[4] __attribute__ ((aligned(32)));
  _mm256_store_si256((__m256i*) tempIters, itercount);
  //write to iterbuf
  for(int i = 0; i < 4; i++)
  {
    out[i] = smoothEscapeTime(tempIters[i], finalReal[i], finalImag[i], real[i], imag[i]);
  }
}

