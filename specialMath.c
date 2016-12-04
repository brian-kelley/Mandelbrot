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
  FP##n zr2, zi2, zri, mag; \
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
    mag = add##n(zr2, zi2); \
    if(mag.w[0] >= four) \
    { \
      return i; \
    } \
    zr = add##n(sub##n(zr2, zi2), cr); \
    zi = add##n(zri, ci); \
  } \
  return -1; \
}

/*
#define IMPL_REG_KERNEL(n) \
float fp##n(FP* restrict real, FP* restrict imag) \
{ \
  FP##n cr = load##n(real); \
  FP##n ci = load##n(imag); \
  FP##n zr = zero##n(); \
  FP##n zi = zero##n(); \
  FP##n zr2, zi2, zri, mag; \
  u64 four; \
  { \
    MAKE_STACK_FP_PREC(temp, 1); \
    loadValue(&temp, 4); \
    four = temp.value.val[0]; \
    printf("four high word: %016llx\n", four); \
  } \
  for(int i = 0; i < 1000; i++) \
  { \
    printf("iter %5i: z = %.3f + %0.3fi\n", i, getVal##n(zr), getVal##n(zi)); \
    zr2 = mul##n(zr, zr); \
    zri = mul##n(zr, zi); \
    zri = add##n(zri, zri); \
    zi2 = mul##n(zi, zi); \
    mag = add##n(zr2, zi2); \
    if(mag.w[0] >= four) \
    { \
      printf(">>>> mag = %f so breaking.\n", getVal##n(mag)); \
      printf("mag words: "); \
      print##n(mag); \
      puts(""); \
      return i; \
    } \
    zr = add##n(sub##n(zr2, zi2), cr); \
    zi = add##n(zri, ci); \
  } \
  return -1; \
}
*/

#define IMPL_SMOOTH_KERNEL(n) \
float fp##n##s(FP* restrict real, FP* restrict imag) \
{ \
  FP##n cr = load##n(real); \
  FP##n ci = load##n(imag); \
  FP##n zr = zero##n(); \
  FP##n zi = zero##n(); \
  FP##n zr2, zi2, zri, mag; \
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
    mag = add##n(zr2, zi2); \
    if(mag.w[0] >= four) \
    { \
      printf(">>>> mag = %f so breaking.\n", getVal##n(mag)); \
      printf("mag words: "); \
      print##n(mag); \
      puts(""); \
      break; \
    } \
    zr = add##n(sub##n(zr2, zi2), cr); \
    zi = add##n(zri, ci); \
  } \
  if(i == maxiter) \
    i = -1; \
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

IMPL_SMOOTH_KERNEL(3)
IMPL_SMOOTH_KERNEL(4)
IMPL_SMOOTH_KERNEL(5)
IMPL_SMOOTH_KERNEL(6)
IMPL_SMOOTH_KERNEL(7)
IMPL_SMOOTH_KERNEL(8)
IMPL_SMOOTH_KERNEL(9)
IMPL_SMOOTH_KERNEL(10)

