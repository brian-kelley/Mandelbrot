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

inline void print2(FP2 val);
inline void store2(FP* fp, FP2 v);           //translate FP2 to FP
inline double getVal2(FP2 fp2);              //translate FP2 to double
inline void setVal2(FP2* fp2, double val);   //translate double to FP2
inline FP2 mul2(FP2 a, FP2 b);
//iterate (real, imag)
float fp2(FP* restrict real, FP* restrict imag);
float fp2s(FP* restrict real, FP* restrict imag);

inline void print2(FP2 val)
{
  printf("#%016llx%016llx", (u64) (val >> 64), (u64) (val & LOW_8));
}

inline FP2 load2(FP* fp)
{
  //load words
  if((fp->value.val[0] & (1ULL << 63)) != 0)
  {
    printf("Loaded bad value %Lf to FP2\n", getValue(fp));
    printf("Warning: bad fp. Raw high word = %016llx\n", fp->value.val[0]);
  }
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

#endif

