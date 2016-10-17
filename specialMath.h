#ifndef SPECIAL_MATH
#define SPECIAL_MATH

#include "constants.h"
#include "bigint.h"
#include "fixedpoint.h"

//Special value structs
//2's complement (implicit sign)

#define LOW_8 ((u128) 0xFFFFFFFFFFFFFFFFULL)

typedef u128 FP2;

inline FP2 load2(FP* fp)
{
  //load words
  s128 v;
  v = fp->value.val[1];
  v <<= 64;
  v |= fp->value.val[0];
  //2s complement if fp negative
  if(fp->sign)
  {
    v *= -1;
  }
  return (FP2) v;
}

inline void store2(FP* fp, FP2 fp2)
{
  s128 sv = (s128) fp2;
  if(sv < 0)
    fp->sign = false;
  else
  {
    fp->sign = true;
    sv *= -1;
  }
  fp->value.val[1] = sv & LOW_8;
  sv >>= 64;
  fp->value.val[0] = sv;
}

inline double getVal2(FP2 fp2)
{
  MAKE_STACK_FP_PREC(temp, 2);
  store2(&temp, fp2);
  return getValue(&temp);
}

inline void setVal2(FP2* fp2, double val)
{
  MAKE_STACK_FP_PREC(temp, 2);
  loadValue(&temp, val);
  *fp2 = load2(&temp);
}

inline FP2 mul2(FP2 a, FP2 b)
{
  FP2 aa = (a >> 64) * (b >> 64);
  FP2 ab = (a >> 64) * (b & LOW_8);
  FP2 ba = (a & LOW_8) * (b >> 64);
  return aa + (ab >> 64) + (ba >> 64);
}

float fp2(FP* real, FP* imag)
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
      return i;
    zr = zr2 - zi2 + cr;
    zi = zri + ci;
  }
  return -1;
}

float fp2s(FP* real, FP* imag)
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
      MAKE_STACK_FP_PREC(zrFinal, 2);
      MAKE_STACK_FP_PREC(ziFinal, 2);
      store2(&zrFinal, zr);
      store2(&ziFinal, zi);
    }
    zr = zr2 - zi2 + cr;
    zi = zri + ci;
  }
  return -1;
}

#endif

