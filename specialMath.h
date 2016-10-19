#ifndef SPECIAL_MATH
#define SPECIAL_MATH

#include "constants.h"
#include "bigint.h"
#include "fixedpoint.h"

//Special value structs
//2's complement (implicit sign)

#define LOW_8 ((u128) 0xFFFFFFFFFFFFFFFFULL)

//3x mul
//3x add
//1x sub
//1x shl

typedef s128 FP2;

FP2 load2(FP* fp)
{
  //load words
  s128 v = fp->value.val[0];
  v <<= 64;
  v |= fp->value.val[1];
  //2s complement if fp negative
  if(fp->sign)
  {
    v = -v;
  }
  return v;
}

void store2(FP* fp, FP2 v)
{
  if(v >= 0)
    fp->sign = false;
  else
  {
    fp->sign = true;
    v = -v;
  }
  fp->value.val[1] = v & LOW_8;
  v >>= 64;
  fp->value.val[0] = v;
}

double getVal2(FP2 fp2)
{
  MAKE_STACK_FP_PREC(temp, 2);
  store2(&temp, fp2);
  return getValue(&temp);
}

void setVal2(FP2* fp2, double val)
{
  MAKE_STACK_FP_PREC(temp, 2);
  loadValue(&temp, val);
  *fp2 = load2(&temp);
}

FP2 mul2(FP2 a, FP2 b)
{
  bool sign = (a < 0) ^ (b < 0);
  if(a < 0)
    a = -a;
  if(b < 0)
    b = -b;
  //now have 2 positive 128-bit values to multiply
  s128 rv = (a >> 64) * (b >> 64) + ((a >> 64) * (b & LOW_8) >> 64) + ((a & LOW_8) * (b >> 64) >> 64);
  rv <<= maxExpo;
  if(sign)
    return -rv;
  else
    return rv;
}

float fp2(FP* restrict real, FP* restrict imag)
{
  FP2 cr = load2(real);
  FP2 ci = load2(imag);
  FP2 zr = 0;
  FP2 zi = 0;
  FP2 zr2, zi2, zri;
  u64 four;
  {
    MAKE_STACK_FP_PREC(temp, 1);
    loadValue(&temp, 4);
    four = temp.value.val[0];
  }
  for(int i = 0; i < maxiter; i++)
  {
    zr2 = mul2(zr, zr);
    zri = 2 * mul2(zr, zi);
    zi2 = mul2(zi, zi);
    if(((zr2 + zi2) >> 64) >= four)
      return i;
    zr = zr2 - zi2 + cr;
    zi = zri + ci;
  }
  return -1;
}

float fp2s(FP* restrict real, FP* restrict imag)
{
  FP2 cr = load2(real);
  FP2 ci = load2(imag);
  FP2 zr = 0;
  FP2 zi = 0;
  FP2 zr2, zi2, zri;
  FP2 four;
  {
    MAKE_STACK_FP_PREC(temp, 1);
    loadValue(&temp, 4);
    four = temp.value.val[0];
    four <<= 64;
  }
  for(int i = 0; i < maxiter; i++)
  {
    zr2 = mul2(zr, zr);
    zri = 2 * mul2(zr, zi);
    zi2 = mul2(zi, zi);
    if(zr2 + zi2 >= four)
    {
      return smoothEscapeTime(i, getVal2(zr), getVal2(zi), getVal2(cr), getVal2(ci));
    }
    zr = zr2 - zi2 + cr;
    zi = zri + ci;
  }
  return -1;
}

#endif

