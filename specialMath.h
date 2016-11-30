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
float fp11(FP* restrict real, FP* restrict imag);
float fp12(FP* restrict real, FP* restrict imag);
float fp13(FP* restrict real, FP* restrict imag);
float fp14(FP* restrict real, FP* restrict imag);
float fp15(FP* restrict real, FP* restrict imag);
float fp16(FP* restrict real, FP* restrict imag);
float fp17(FP* restrict real, FP* restrict imag);
float fp18(FP* restrict real, FP* restrict imag);
float fp19(FP* restrict real, FP* restrict imag);
float fp20(FP* restrict real, FP* restrict imag);

float fp2s(FP* restrict real, FP* restrict imag);
float fp3s(FP* restrict real, FP* restrict imag);
float fp4s(FP* restrict real, FP* restrict imag);
float fp5s(FP* restrict real, FP* restrict imag);
float fp6s(FP* restrict real, FP* restrict imag);
float fp7s(FP* restrict real, FP* restrict imag);
float fp8s(FP* restrict real, FP* restrict imag);
float fp9s(FP* restrict real, FP* restrict imag);
float fp10s(FP* restrict real, FP* restrict imag);
float fp11s(FP* restrict real, FP* restrict imag);
float fp12s(FP* restrict real, FP* restrict imag);
float fp13s(FP* restrict real, FP* restrict imag);
float fp14s(FP* restrict real, FP* restrict imag);
float fp15s(FP* restrict real, FP* restrict imag);
float fp16s(FP* restrict real, FP* restrict imag);
float fp17s(FP* restrict real, FP* restrict imag);
float fp18s(FP* restrict real, FP* restrict imag);
float fp19s(FP* restrict real, FP* restrict imag);
float fp20s(FP* restrict real, FP* restrict imag);

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
DECL_SPECIALIZATION(11)
DECL_SPECIALIZATION(12)
DECL_SPECIALIZATION(13)
DECL_SPECIALIZATION(14)
DECL_SPECIALIZATION(15)
DECL_SPECIALIZATION(16)
DECL_SPECIALIZATION(17)
DECL_SPECIALIZATION(18)
DECL_SPECIALIZATION(19)
DECL_SPECIALIZATION(20)

#define IMPL_PRINT(n) \
inline void print##n(FP##n val) \
{ \
  for(int i = 0; i < n; i++) \
    printf("%016llx", val.w[i]); \
}

#define IMPL_LOAD(n) \
inline FP##n load##n(FP* fp) \
{ \
  FP##n v = zero##n(); \
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
  u64 mask = (1ULL << bits) - 1; \
  int transferShift = 64 - bits; \
  for(int i = 0; i < n - 1; i++) \
  { \
    transfer = v.w[i + 1] & mask; \
    v.w[i] <<= bits; \
    v.w[i] |= (transfer >> transferShift); \
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
  memset(partial, 0, n * 16); \
  for(int i = 0; i < n; i++) \
  { \
    partial[i] += sab.w[i]; \
    partial[i] += sba.w[i]; \
  } \
  for(int i = 0; i < n; i++) \
  { \
    for(int j = 0; j < n - i - 1; j++) \
    { \
      u128 prod = (u128) a.w[i] * (u128) b.w[i]; \
      partial[i + j] += (prod >> 64); \
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

/* FP3 implementation */
/* note: declared above with DECL_SPECIALIZATION(3) */

inline void print3(FP3 val)
{
  printf("%016llx%016llx%016llx", val.w[0], val.w[1], val.w[2]);
}

inline FP3 load3(FP* fp)
{
  FP3 v = {{0, 0, 0}};
  v.w[0] = fp->value.val[0];
  if(fp->value.size > 1)
  {
    v.w[1] = fp->value.val[0];
    if(fp->value.size > 2)
    {
      v.w[2] = fp->value.val[0];
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
  bool sign = v.w[0] & (1ULL << 63);
  if(sign)
  {
    v = neg3(v);
  }
  fp->value.val[0] = v.w[0];
  fp->value.val[1] = v.w[1];
  fp->value.val[2] = v.w[2];
  fp->sign = sign;
}

inline double getVal3(FP3 fp3)
{
  MAKE_STACK_FP_PREC(fp, 3);
  bool sign = fp3.w[0] & (1ULL << 63);
  if(sign)
  {
    fp3 = neg3(fp3);
  }
  fp.value.val[0] = fp3.w[0];
  fp.value.val[1] = fp3.w[1];
  fp.value.val[2] = fp3.w[2];
  fp.sign = sign;
  return getValue(&fp);
}

inline void setVal3(FP3* fp3, double val)
{
  MAKE_STACK_FP_PREC(fp, 3);
  loadValue(&fp, val);
  fp3->w[0] = fp.value.val[0];
  fp3->w[1] = fp.value.val[1];
  fp3->w[2] = fp.value.val[2];
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
  u128 wsum = 1 + a.w[2];
  a.w[2] = (u64) wsum;
  wsum = (wsum >> 64) + a.w[1];
  a.w[1] = (u64) wsum;
  wsum = (wsum >> 64) + a.w[2];
  a.w[2] = (u64) wsum;
  return a;
}

inline FP3 neg3(FP3 a)
{
  a.w[0] = ~a.w[0];
  a.w[1] = ~a.w[1];
  a.w[2] = ~a.w[2];
  a = inc3(a);
  return a;
}

inline FP3 add3(FP3 a, FP3 b)
{
  u128 wsum = a.w[2] + b.w[2];
  a.w[2] = (u64) wsum;
  wsum = (wsum >> 64) + a.w[1] + b.w[1];
  a.w[1] = (u64) wsum;
  wsum = (wsum >> 64) + a.w[0] + b.w[0];
  a.w[0] = (u64) wsum;
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

IMPL_SHL(3)

inline FP3 mul3(FP3 a, FP3 b)
{
  //place -1 (high word added to w[2] of result)
  u128 a1b3 = (u128) a.w[0] * (u128) b.w[2];
  u128 a2b2 = (u128) a.w[1] * (u128) b.w[1];
  u128 a3b1 = (u128) a.w[2] * (u128) b.w[0];
  //place 0 (high/low added to w[1]/w[2] of result)
  u128 a1b2 = (u128) a.w[0] * (u128) b.w[1];
  u128 a2b1 = (u128) a.w[1] * (u128) b.w[0];
  //place 1 (high/low added to w[0]/w[1] of result)
  u128 a1b1 = (u128) a.w[0] * (u128) b.w[0];
  //sign-word products, can be reused
  FP3 sab = (a.w[0] & (1ULL << 63)) ? neg3(b) : zero3();
  FP3 sba = (b.w[0] & (1ULL << 63)) ? neg3(a) : zero3();
  //add results, one word at a time
  FP3 result;
  u128 sum = (a1b3 >> 64) + (a2b2 >> 64) + (a3b1 >> 64) + (u64) a1b2 + (u64) a2b1 + sab.w[2] + sba.w[2];
  result.w[2] = (u64) sum;
  sum >>= 64;   //sum now holds carry value to be added to w[1]
  sum += (a1b2 >> 64) + (a2b1 >> 64) + (u64) a1b1 + sab.w[1] + sba.w[1];
  result.w[1] = (u64) sum;
  sum >>= 64;
  sum += (a1b1 >> 64) + sab.w[0] + sba.w[0];
  result.w[0] = (u64) sum;
  return shl3(result, maxExpo);
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

IMPL_SPECIALIZATION(4)
IMPL_SPECIALIZATION(5)
IMPL_SPECIALIZATION(6)
IMPL_SPECIALIZATION(7)
IMPL_SPECIALIZATION(8)
IMPL_SPECIALIZATION(9)
IMPL_SPECIALIZATION(10)
IMPL_SPECIALIZATION(11)
IMPL_SPECIALIZATION(12)
IMPL_SPECIALIZATION(13)
IMPL_SPECIALIZATION(14)
IMPL_SPECIALIZATION(15)
IMPL_SPECIALIZATION(16)
IMPL_SPECIALIZATION(17)
IMPL_SPECIALIZATION(18)
IMPL_SPECIALIZATION(19)
IMPL_SPECIALIZATION(20)

#endif

