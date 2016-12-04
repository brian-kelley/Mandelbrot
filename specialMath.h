#ifndef SPECIAL_MATH
#define SPECIAL_MATH

#include "constants.h"
#include "bigint.h"
#include "fixedpoint.h"
#include "kernels.h"

/* Forward declare the non-inline kernels in specialMath.c */
float fp2(FP* restrict real, FP* restrict imag);
float fp3(FP* restrict real, FP* restrict imag);
float fp4(FP* restrict real, FP* restrict imag);
float fp5(FP* restrict real, FP* restrict imag);
float fp6(FP* restrict real, FP* restrict imag);
float fp7(FP* restrict real, FP* restrict imag);
float fp8(FP* restrict real, FP* restrict imag);
float fp9(FP* restrict real, FP* restrict imag);
float fp10(FP* restrict real, FP* restrict imag);

float fp2s(FP* restrict real, FP* restrict imag);
float fp3s(FP* restrict real, FP* restrict imag);
float fp4s(FP* restrict real, FP* restrict imag);
float fp5s(FP* restrict real, FP* restrict imag);
float fp6s(FP* restrict real, FP* restrict imag);
float fp7s(FP* restrict real, FP* restrict imag);
float fp8s(FP* restrict real, FP* restrict imag);
float fp9s(FP* restrict real, FP* restrict imag);
float fp10s(FP* restrict real, FP* restrict imag);

/* Macro-based generic specializations */
//Required functions:
//printN, loadN, storeN, getValN, setValN, zeroN, incN, negN, addN, subN, mulN

#define IMPL_TYPE(n) \
typedef struct \
{ \
  u64 w[n]; \
} FP##n;

//declare complete specialization
#define DECL_SPECIALIZATION(n) \
IMPL_TYPE(n) \
static inline void print##n(FP##n val); \
static inline FP##n load##n(FP* fp); \
static inline void store##n(FP* fp, FP##n v); \
static inline double getVal##n(FP##n v); \
static inline void setVal##n(FP##n* v, double val); \
static inline FP##n zero##n(); \
static inline FP##n shl##n(FP##n v, int bits); \
static inline FP##n inc##n(FP##n v); \
static inline FP##n neg##n(FP##n v); \
static inline FP##n add##n(FP##n a, FP##n b); \
static inline FP##n sub##n(FP##n a, FP##n b); \
static inline FP##n mul##n(FP##n a, FP##n b);

//Declare all specialization types + inline functions
DECL_SPECIALIZATION(3)
DECL_SPECIALIZATION(4)
DECL_SPECIALIZATION(5)
DECL_SPECIALIZATION(6)
DECL_SPECIALIZATION(7)
DECL_SPECIALIZATION(8)
DECL_SPECIALIZATION(9)
DECL_SPECIALIZATION(10)

#define IMPL_PRINT(n) \
inline void print##n(FP##n val) \
{ \
  for(int i = 0; i < n; i++) \
    printf("%016llx", val.w[i]); \
}

#define IMPL_LOAD(n) \
inline FP##n load##n(FP* fp) \
{ \
  FP##n v; \
  memcpy(v.w, fp->value.val, n * 8); \
  if(fp->sign) \
    v = neg##n(v); \
  return v; \
}

#define IMPL_STORE(n) \
inline void store##n(FP* fp, FP##n v) \
{ \
  bool sign = v.w[0] & (1ULL << 63); \
  if(sign) \
    v = neg##n(v); \
  memcpy(fp->value.val, v.w, n * 8); \
  fp->sign = sign; \
}

#define IMPL_GETVAL(n) \
inline double getVal##n(FP##n v) \
{ \
  MAKE_STACK_FP_PREC(fp, n); \
  store##n(&fp, v); \
  return getValue(&fp); \
}

#define IMPL_SETVAL(n) \
inline void setVal##n(FP##n* v, double val) \
{ \
  MAKE_STACK_FP_PREC(fp, n); \
  loadValue(&fp, val); \
  *v = load##n(&fp); \
}

#define IMPL_ZERO(n) \
inline FP##n zero##n() \
{ \
  FP##n v; \
  memset(v.w, 0, n * 8); \
  return v; \
}

#define IMPL_SHL(n) \
inline FP##n shl##n(FP##n v, int bits) \
{ \
  u64 transfer; \
  int transferShift = 64 - bits; \
  u64 transferMask = ((1ULL << bits) - 1) << transferShift; \
  for(int i = 0; i < n - 1; i++) \
  { \
    transfer = (v.w[i + 1] & transferMask) >> transferShift; \
    v.w[i] <<= bits; \
    v.w[i] |= transfer; \
  } \
  v.w[n - 1] <<= bits; \
  return v; \
}

#define IMPL_INC(n) \
inline FP##n inc##n(FP##n v) \
{ \
  u128 wsum = 1; \
  for(int i = n - 1; i >= 0; i--) \
  { \
    wsum += v.w[i]; \
    v.w[i] = (u64) wsum; \
    wsum >>= 64; \
  } \
  return v; \
}

#define IMPL_NEG(n) \
inline FP##n neg##n(FP##n v) \
{ \
  for(int i = 0; i < n; i++) \
    v.w[i] = ~v.w[i]; \
  return inc##n(v); \
}

#define IMPL_ADD(n) \
inline FP##n add##n(FP##n a, FP##n b) \
{ \
  u128 wsum = 0; \
  for(int i = n - 1; i >= 0; i--) \
  { \
    wsum += a.w[i]; \
    wsum += b.w[i]; \
    a.w[i] = (u64) wsum; \
    wsum >>= 64; \
  } \
  return a; \
}

#define IMPL_SUB(n) \
inline FP##n sub##n(FP##n a, FP##n b) \
{ \
  return add##n(a, neg##n(b)); \
}

#define IMPL_MUL(n) \
inline FP##n mul##n(FP##n a, FP##n b) \
{ \
  FP##n sab = (a.w[0] & (1ULL << 63)) ? neg##n(b) : zero##n(); \
  FP##n sba = (b.w[0] & (1ULL << 63)) ? neg##n(a) : zero##n(); \
  u128 partial[n]; \
  memset(partial, 0, n * sizeof(u128)); \
  for(int i = 0; i < n; i++) \
  { \
    partial[i] += sab.w[i]; \
    partial[i] += sba.w[i]; \
  } \
  for(int i = 0; i < n; i++) \
  { \
    for(int j = 0; j < n - i; j++) \
    { \
      u128 prod = (u128) a.w[i] * (u128) b.w[j]; \
      partial[i + j] += (prod >> 64); \
      if(i + j + 1 < n) \
        partial[i + j + 1] += (u64) prod; \
    } \
  } \
  FP##n result; \
  u128 sum = 0; \
  for(int i = n - 1; i >= 0; i--) \
  { \
    sum += partial[i]; \
    result.w[i] = (u64) sum; \
    sum >>= 64; \
  } \
  return shl##n(result, maxExpo); \
}

/*
#define IMPL_MUL(n) \
inline FP##n mul##n(FP##n a, FP##n b) \
{ \
  u64 f1[n * 2]; \
  u64 f2[n * 2]; \
  for(int i = 0; i < n; i++) \
  { \
    f1[i + n] = a.w[i]; \
    f2[i + n] = b.w[i]; \
  } \
  for(int i = 0; i < n; i++) \
  { \
    f1[i] = (a.w[0] & (1ULL << 63)) ? ~(0ULL) : 0; \
    f2[i] = (b.w[0] & (1ULL << 63)) ? ~(0ULL) : 0; \
  } \
  u128 partial[n * 4]; \
  memset(partial, 0, n * 4 * sizeof(u128)); \
  for(int i = 0; i < n * 2; i++) \
  { \
    for(int j = 0; j < n * 2; j++) \
    { \
      u128 prod = (u128) a.w[i] * (u128) b.w[j]; \
      partial[i + j] += (prod >> 64); \
      partial[i + j + 1] += (u64) prod; \
    } \
  } \
  FP##n result; \
  u128 sum = 0; \
  for(int i = 3 * n - 1; i >= 2 * n; i--) \
  { \
    sum += partial[i]; \
    result.w[i - 2 * n] = (u64) sum; \
    sum >>= 64; \
  } \
  return shl##n(result, maxExpo); \
}
*/

/* 128-bit specialization using u128 */
/* simpler & faster than macro */

#define LOW_8 ((u128) 0xFFFFFFFFFFFFFFFFULL)

/* each iteration does:
 * 3x mul
 * 3x add
 * 1x sub
 * 1x shl
 */

typedef u128 FP2;

/* 128-bit specializations */

//trivial wrappers for built-in add and sub
static inline FP2 add2(FP2 a, FP2 b)
{
  return a + b;
}

static inline FP2 sub2(FP2 a, FP2 b)
{
  return a - b;
}

inline void print2(FP2 val);
inline FP2 load2(FP* fp);                    //translate FP to FP2
inline void store2(FP* fp, FP2 v);           //translate FP2 to FP
inline double getVal2(FP2 fp2);              //translate FP2 to double
inline void setVal2(FP2* fp2, double val);   //translate double to FP2
inline FP2 mul2(FP2 a, FP2 b);

inline void print2(FP2 val)
{
  printf("#%016llx%016llx", (u64) (val >> 64), (u64) val);
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
  fp->value.val[1] = (u64) sv;
  sv >>= 64;
  fp->value.val[0] = (u64) sv;
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
  u128 alo = (u64) a; 
  u128 ahi = a >> 64;
  u128 blo = (u64) b;
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

//complete specialization for precision 64n bits
#define IMPL_SPECIALIZATION(n) \
IMPL_PRINT(n) \
IMPL_LOAD(n) \
IMPL_STORE(n) \
IMPL_GETVAL(n) \
IMPL_SETVAL(n) \
IMPL_ZERO(n) \
IMPL_SHL(n) \
IMPL_INC(n) \
IMPL_NEG(n) \
IMPL_ADD(n) \
IMPL_SUB(n) \
IMPL_MUL(n)

IMPL_SPECIALIZATION(3)
IMPL_SPECIALIZATION(4)
IMPL_SPECIALIZATION(5)
IMPL_SPECIALIZATION(6)
IMPL_SPECIALIZATION(7)
IMPL_SPECIALIZATION(8)
IMPL_SPECIALIZATION(9)
IMPL_SPECIALIZATION(10)

#endif

