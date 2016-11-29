#ifndef SPECIAL_MATH
#define SPECIAL_MATH

#include "constants.h"
#include "bigint.h"
#include "fixedpoint.h"
#include "kernels.h"

//Special value structs
//2's complement (implicit sign)

#define LOW_8 ((u128) 0xFFFFFFFFFFFFFFFFULL)

/* each iteration does:
 * 3x mul
 * 3x add
 * 1x sub
 * 1x shl
 */

typedef u128 FP2;

typedef struct
{
  u64 w1;
  u64 w2;
  u64 w3;
} FP3;

/* 128-bit specializations */

inline void print2(FP2 val);
inline FP2 load2(FP* fp);                    //translate FP to FP2
inline void store2(FP* fp, FP2 v);           //translate FP2 to FP
inline double getVal2(FP2 fp2);              //translate FP2 to double
inline void setVal2(FP2* fp2, double val);   //translate double to FP2
inline FP2 mul2(FP2 a, FP2 b);
//iterate (real, imag)
float fp2(FP* restrict real, FP* restrict imag);
float fp2s(FP* restrict real, FP* restrict imag);

/* 192-bit specializations */
inline void print3(FP3 val);
inline FP3 load3(FP* fp);                    //translate FP to FP3
inline void store3(FP* fp, FP3 v);           //translate FP3 to FP
inline double getVal3(FP3 fp3);              //translate FP3 to double
inline void setVal3(FP3* fp3, double val);   //translate double to FP3
inline FP3 zero3();
inline FP3 inc3(FP3 a);
inline FP3 neg3(FP3 a);
inline FP3 add3(FP3 a, FP3 b);
inline FP3 sub3(FP3 a, FP3 b);
inline FP3 mul3(FP3 a, FP3 b);
//iterate (real, imag)
float fp3(FP* restrict real, FP* restrict imag);
float fp3s(FP* restrict real, FP* restrict imag);

inline void print2(FP2 val)
{
  printf("#%016llx%016llx", (u64) (val >> 64), (u64) (val & LOW_8));
}

inline FP2 load2(FP* fp)
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

inline void store2(FP* fp, FP2 v)
{
  s128 sv = v;
  if(sv >= 0)
  {
    fp->sign = false;
  }
  else
  {
    fp->sign = true;
    sv = -sv;
  }
  fp->value.val[1] = sv & LOW_8;
  sv >>= 64;
  fp->value.val[0] = sv & LOW_8;
}

inline double getVal2(FP2 fp2)
{
  bool sign = false;
  s128 sv = fp2;
  if(sv < 0)
  {
    sign = true;
    sv = -sv;
  }
  double highword = sv >> 64;
  double val = highword / (1ULL << (64 - maxExpo));
  return sign ? -val : val;
}

inline void setVal2(FP2* fp2, double val)
{
  MAKE_STACK_FP_PREC(temp, 2);
  loadValue(&temp, val);
  *fp2 = load2(&temp);
}

inline FP2 mul2(FP2 a, FP2 b)
{
  u128 alo = a & LOW_8;
  u128 ahi = a >> 64;
  u128 blo = b & LOW_8;
  u128 bhi = b >> 64;
  //himask is a mask for sign bit of int128
  u128 himask = 1;
  himask <<= 127;
  u128 sab = a & himask ? -b : 0;
  u128 sba = b & himask ? -a : 0;
  return (((ahi * blo) >> 64) +
          ((bhi * alo) >> 64) +
          sab + sba +
          (ahi * bhi)) << maxExpo;
}

/* FP3 implementation */

inline void print3(FP3 val)
{
  printf("%016llx%016llx%016llx", val.w1, val.w2, val.w3);
}

inline FP3 load3(FP* fp)
{
  FP3 v = {0, 0, 0};
  v.w1 = fp->value.val[0];
  if(fp->value.size > 1)
  {
    v.w2 = fp->value.val[0];
    if(fp->value.size > 2)
    {
      v.w3 = fp->value.val[0];
    }
  }
  if(fp->sign)
  {
    v = neg3(v);
  }
  return v;
}

inline void store3(FP* fp, FP3 v)
{
  bool sign = v.w1 & (1ULL << 63);
  if(sign)
  {
    v = neg3(v);
  }
  fp->value.val[0] = v.w1;
  fp->value.val[1] = v.w2;
  fp->value.val[2] = v.w3;
  fp->sign = sign;
}

inline double getVal3(FP3 fp3)
{
  MAKE_STACK_FP_PREC(fp, 3);
  bool sign = fp3.w1 & (1ULL << 63);
  if(sign)
  {
    fp3 = neg3(fp3);
  }
  fp.value.val[0] = fp3.w1;
  fp.value.val[1] = fp3.w2;
  fp.value.val[2] = fp3.w3;
  fp.sign = sign;
  return getValue(&fp);
}

inline void setVal3(FP3* fp3, double val)
{
  MAKE_STACK_FP_PREC(fp, 3);
  loadValue(&fp, val);
  fp3->w1 = fp.value.val[0];
  fp3->w2 = fp.value.val[1];
  fp3->w3 = fp.value.val[2];
  if(fp.sign)
  {
    *fp3 = neg3(*fp3);
  }
}

inline FP3 zero3()
{
  FP3 rv;
  memset(&rv, 0, 3 * 8);
  return rv;
}

inline FP3 inc3(FP3 a)
{
  u128 wsum = 1 + a.w3;
  a.w3 = (u64) wsum;
  wsum = (wsum >> 64) + a.w2;
  a.w2 = (u64) wsum;
  wsum = (wsum >> 64) + a.w3;
  a.w3 = (u64) wsum;
  return a;
}

inline FP3 neg3(FP3 a)
{
  a.w1 = ~a.w1;
  a.w2 = ~a.w2;
  a.w3 = ~a.w3;
  a = inc3(a);
  return a;
}

inline FP3 add3(FP3 a, FP3 b)
{
  u128 wsum = a.w3 + b.w3;
  a.w3 = (u64) wsum;
  wsum = (wsum >> 64) + a.w2 + b.w2;
  a.w2 = (u64) wsum;
  wsum = (wsum >> 64) + a.w1 + b.w1;
  a.w1 = (u64) wsum;
  return a;
}

inline FP3 sub3(FP3 a, FP3 b)
{
  b = neg3(b);
  return add3(a, b);
}

/*
     sa   sa   sa   a1   a2   a3
     sb   sb   sb   b1   b2   b3
                 x------------
                         (a3*b3)
                    (a2*b3)
                    (a3*b2)
               (a1*b3)
               (a2*b2)
               (a3*b1)
          (a1*b2)
          (a2*b1)
     (a1*b1)
 
 */

inline FP3 mul3(FP3 a, FP3 b)
{
  //place -1 (high word added to w3 of result)
  u128 a1b3 = (u128) a.w1 * (u128) b.w3;
  u128 a2b2 = (u128) a.w2 * (u128) b.w2;
  u128 a3b1 = (u128) a.w3 * (u128) b.w1;
  //place 0 (high/low added to w2/w3 of result)
  u128 a1b2 = (u128) a.w1 * (u128) b.w2;
  u128 a2b1 = (u128) a.w2 * (u128) b.w1;
  //place 1 (high/low added to w1/w2 of result)
  u128 a1b1 = (u128) a.w1 * (u128) b.w1;
  //sign-word products, can be reused
  FP3 sab = (a.w1 & (1ULL << 63)) ? neg3(b) : zero3();
  FP3 sba = (b.w1 & (1ULL << 63)) ? neg3(a) : zero3();
  //add results, one word at a time
  FP3 result;
  u128 sum = (a1b3 >> 64) + (a2b2 >> 64) + (a3b1 >> 64) + (u64) a1b2 + (u64) a2b1 + sab.w3 + sba.w3;
  result.w3 = (u64) sum;
  sum >>= 64;   //sum now holds carry value to be added to w2
  sum += (a1b2 >> 64) + (a2b1 >> 64) + (u64) a1b1 + sab.w2 + sba.w2;
  result.w2 = (u64) sum;
  sum >>= 64;
  sum += (a1b1 >> 64) + sab.w1 + sba.w1;
  result.w1 = (u64) sum;
  return result;
}

#endif

