#include "specialMath.h"

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
  u64 four;
  {
    MAKE_STACK_FP_PREC(temp, 1);
    loadValue(&temp, 4);
    four = temp.value.val[0];
  }
  int i;
  for(i = 0; i < maxiter; i++)
  {
    zr2 = mul2(zr, zr);
    zri = 2 * mul2(zr, zi);
    zi2 = mul2(zi, zi);
    if(((zr2 + zi2) >> 64) >= four)
      break;
    zr = zr2 - zi2 + cr;
    zi = zri + ci;
  }
  return smoothEscapeTime(i, getVal2(zr), getVal2(zi), getVal2(cr), getVal2(ci));
}

float fp3(FP* restrict real, FP* restrict imag)
{
  FP3 cr = load3(real);
  FP3 ci = load3(imag);
  FP3 zr = zero3();
  FP3 zi = zero3();
  FP3 zr2, zi2, zri;
  u64 four;
  {
    MAKE_STACK_FP_PREC(temp, 1);
    loadValue(&temp, 4);
    four = temp.value.val[0];
  }
  for(int i = 0; i < maxiter; i++)
  {
    zr2 = mul3(zr, zr);
    zri = mul3(zr, zi);
    zri = add3(zri, zri);
    zi2 = mul3(zi, zi);
    if(add3(zr2, zi2).w1 >= four)
      return i;
    zr = add3(sub3(zr2, zi2), cr);
    zi = add3(zri, ci);
  }
  return -1;
}

float fp3s(FP* restrict real, FP* restrict imag)
{
  FP3 cr = load3(real);
  FP3 ci = load3(imag);
  FP3 zr = zero3();
  FP3 zi = zero3();
  FP3 zr2, zi2, zri;
  u64 four;
  {
    MAKE_STACK_FP_PREC(temp, 1);
    loadValue(&temp, 4);
    four = temp.value.val[0];
  }
  int i;
  for(i = 0; i < maxiter; i++)
  {
    zr2 = mul3(zr, zr);
    zri = mul3(zr, zi);
    zri = add3(zri, zri);
    zi2 = mul3(zi, zi);
    if(add3(zr2, zi2).w1 >= four)
      break;
    zr = add3(sub3(zr2, zi2), cr);
    zi = add3(zri, ci);
  }
  return smoothEscapeTime(i, getVal3(zr), getVal3(zi), getVal3(cr), getVal3(ci));
}

