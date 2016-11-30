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

//Generic arb-precision kernel
#define IMPL_REG_KERNEL(n) \
float fp##n(FP* restrict real, FP* restrict imag) \
{ \
  FP##n cr = load##n(real); \
  FP##n ci = load##n(imag); \
  FP##n zr = zero##n(); \
  FP##n zi = zero##n(); \
  FP##n zr2, zi2, zri; \
  u64 four; \
  { \
    MAKE_STACK_FP_PREC(temp, 1); \
    loadValue(&temp, 4); \
    four = temp.value.val[0]; \
  } \
  for(int i = 0; i < maxiter; i++) \
  { \
    zr2 = mul##n(zr, zr); \
    zri = mul##n(zr, zi); \
    zri = add##n(zri, zri); \
    zi2 = mul##n(zi, zi); \
    if(add##n(zr2, zi2).w[0] >= four) \
      return i; \
    zr = add##n(sub##n(zr2, zi2), cr); \
    zi = add##n(zri, ci); \
  } \
  return -1; \
}

#define IMPL_SMOOTH_KERNEL(n) \
float fp##n##s(FP* restrict real, FP* restrict imag) \
{ \
  FP##n cr = load##n(real); \
  FP##n ci = load##n(imag); \
  FP##n zr = zero##n(); \
  FP##n zi = zero##n(); \
  FP##n zr2, zi2, zri; \
  u64 four; \
  { \
    MAKE_STACK_FP_PREC(temp, 1); \
    loadValue(&temp, 4); \
    four = temp.value.val[0]; \
  } \
  int i; \
  for(i = 0; i < maxiter; i++) \
  { \
    zr2 = mul##n(zr, zr); \
    zri = mul##n(zr, zi); \
    zri = add##n(zri, zri); \
    zi2 = mul##n(zi, zi); \
    if(add##n(zr2, zi2).w[0] >= four) \
      break; \
    zr = add##n(sub##n(zr2, zi2), cr); \
    zi = add##n(zri, ci); \
  } \
  return smoothEscapeTime(i, getVal##n(zr), getVal##n(zi), getVal##n(cr), getVal##n(ci)); \
}

IMPL_REG_KERNEL(3)
IMPL_REG_KERNEL(4)
IMPL_REG_KERNEL(5)
IMPL_REG_KERNEL(6)
IMPL_REG_KERNEL(7)
IMPL_REG_KERNEL(8)
IMPL_REG_KERNEL(9)
IMPL_REG_KERNEL(10)
IMPL_REG_KERNEL(11)
IMPL_REG_KERNEL(12)
IMPL_REG_KERNEL(13)
IMPL_REG_KERNEL(14)
IMPL_REG_KERNEL(15)
IMPL_REG_KERNEL(16)
IMPL_REG_KERNEL(17)
IMPL_REG_KERNEL(18)
IMPL_REG_KERNEL(19)
IMPL_REG_KERNEL(20)

IMPL_SMOOTH_KERNEL(3)
IMPL_SMOOTH_KERNEL(4)
IMPL_SMOOTH_KERNEL(5)
IMPL_SMOOTH_KERNEL(6)
IMPL_SMOOTH_KERNEL(7)
IMPL_SMOOTH_KERNEL(8)
IMPL_SMOOTH_KERNEL(9)
IMPL_SMOOTH_KERNEL(10)
IMPL_SMOOTH_KERNEL(11)
IMPL_SMOOTH_KERNEL(12)
IMPL_SMOOTH_KERNEL(13)
IMPL_SMOOTH_KERNEL(14)
IMPL_SMOOTH_KERNEL(15)
IMPL_SMOOTH_KERNEL(16)
IMPL_SMOOTH_KERNEL(17)
IMPL_SMOOTH_KERNEL(18)
IMPL_SMOOTH_KERNEL(19)
IMPL_SMOOTH_KERNEL(20)

