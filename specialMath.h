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

typedef u128 FP2;

void print2(FP2 val)
{
  printf("#%016llx%016llx", (u64) (val >> 64), (u64) (val & LOW_8));
}

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
  s128 sv = (s128) v;
  if(sv >= 0)
    fp->sign = false;
  else
  {
    fp->sign = true;
    sv = -sv;
  }
  fp->value.val[1] = sv & LOW_8;
  v >>= 64;
  fp->value.val[0] = sv;
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
  u128 right, wrong;
  {
    u128 alo = a & LOW_8;
    u128 ahi = a >> 64;
    u128 blo = b & LOW_8;
    u128 bhi = b >> 64;
    u128 himask = 1;
    himask <<= 127;
    u128 sabh = a & himask ? (-bhi) << 64 : 0;
    u128 sbah = b & himask ? (-ahi) << 64 : 0;
    u128 sabl = a & himask ? (-blo) : 0;
    u128 sbal = b & himask ? (-alo) : 0;
    u128 rv = ((ahi * blo) >> 64) +
              ((bhi * alo) >> 64) +
              sabl + sbal +
              (ahi * bhi) +
              sabh + sbah;
    wrong = rv << maxExpo;
  }
  {
    s128 sa = a;
    s128 sb = b;
    bool sign = (sa < 0) ^ (sb < 0);
    if(sa < 0)
      sa = -sa;
    if(sb < 0)
      sb = -sb;
    //now have 2 positive 128-bit values to multiply
    s128 rv = (sa >> 64) * (sb >> 64) + ((sa >> 64) * (sb & LOW_8) >> 64) + ((sa & LOW_8) * (sb >> 64) >> 64);
    rv <<= maxExpo;
    if(sign)
      right = -rv;
    else
      right = rv;
  }
  printf("CORRECT: ");
  print2(right);
  putchar('\n');
  printf("WRONG  : ");
  print2(wrong);
  putchar('\n');
  putchar('\n');
  return right;
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

