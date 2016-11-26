#include "fixedpoint.h"

FP FPCtor(int prec)
{
  FP rv;
  rv.value.size = prec;
  rv.value.val = (u64*) malloc(prec * sizeof(u64));
  rv.sign = false;
  return rv;
}

FP FPCtorValue(int prec, long double val)
{
  FP rv = FPCtor(prec);
  loadValue(&rv, val);
  return rv;
}

void FPDtor(FP* fp)
{
  free(fp->value.val);
}

void fpadd2(FP* lhs, FP* rhs)
{
  const int words = lhs->value.size;
  if(lhs->sign != rhs->sign)
  {
    FP* actualDst = lhs;
    BigInt dst;
    dst.size = words;
    dst.val = (u64*) alloca(words * sizeof(u64));
    //subtract (can't overflow, result magnitude can only be less than either operand)
    //swap lhs/rhs pointers if |lhs| < |rhs|
    if(fpCompareMag(lhs, rhs) == -1)
    {
      FP* temp = lhs;
      lhs = rhs;
      rhs = temp;
    }
    // |lhs| >= |rhs|
    bisub(&dst, &lhs->value, &rhs->value);
    for(int i = 0; i < words; i++)
      actualDst->value.val[i] = dst.val[i];
    actualDst->sign = lhs->sign;
  }
  else
  {
    //add (no overflow check)
    BigInt tempLHS = {(u64*) alloca(words * sizeof(u64)), words};
    memcpy(tempLHS.val, lhs->value.val, sizeof(u64) * words);
    biadd(&lhs->value, &tempLHS, &rhs->value);
  }
}

void fpsub2(FP* lhs, FP* rhs)
{
  bool savedSign = rhs->sign;
  rhs->sign = !rhs->sign;
  fpadd2(lhs, rhs);
  rhs->sign = savedSign;
}

void fpmul2(FP* lhs, FP* rhs)
{
  const int words = lhs->value.size;
  //bimul requires a destination twice as wide as operands
  BigInt wideDst;
  wideDst.size = words * 2;
  wideDst.val = (u64*) alloca(words * 2 * sizeof(u64));
  bimul(&wideDst, &lhs->value, &rhs->value);
  memcpy(lhs->value.val, wideDst.val, words * sizeof(u64));
  lhs->sign = lhs->sign != rhs->sign;
  fpshl(*lhs, maxExpo);
}

void fpadd3(FP* restrict dst, FP* lhs, FP* rhs)
{
  if(lhs->sign != rhs->sign)
  {
    //subtract (can't overflow, result magnitude can only be less than either operand)
    //swap lhs/rhs pointers if |lhs| < |rhs|
    if(fpCompareMag(lhs, rhs) == -1)
    {
      FP* temp = lhs;
      lhs = rhs;
      rhs = temp;
    }
    //copy lhs words into dst
    bisub(&dst->value, &lhs->value, &rhs->value);
    dst->sign = lhs->sign;
  }
  else
  {
    biadd(&dst->value, &lhs->value, &rhs->value);
    dst->sign = lhs->sign;
  }
}

void fpsub3(FP* restrict dst, FP* lhs, FP* rhs)
{
  bool savedSign = rhs->sign;
  rhs->sign = !rhs->sign;
  fpadd3(dst, lhs, rhs);
  rhs->sign = savedSign;
}

void fpmul3(FP* restrict dst, FP* lhs, FP* rhs)
{
  const int words = lhs->value.size;
  //bimul requires a destination twice as wide as operands
  BigInt wideDst;
  wideDst.size = words * 2;
  wideDst.val = (u64*) alloca(words * 2 * sizeof(u64));
  bimul(&wideDst, &lhs->value, &rhs->value);
  //copy the result into dst
  memcpy(dst->value.val, wideDst.val, words * sizeof(u64));
  dst->sign = lhs->sign != rhs->sign;
  fpshl(*dst, maxExpo);
}

int fpCompareMag(FP* lhs, FP* rhs)     //-1: lhs < rhs, 0: lhs == rhs, 1: lhs > rhs
{
  for(int i = 0; i < lhs->value.size; i++)
  {
    if(lhs->value.val[i] < rhs->value.val[i])
      return -1;
    else if(lhs->value.val[i] > rhs->value.val[i])
      return 1;
  }
  return 0;
}

void loadValue(FP* fp, long double val)
{
  assert(fabsl(val) < (1 << (maxExpo - 1)));
  for(int i = 0; i < fp->value.size; i++)
    fp->value.val[i] = 0;
  if(val < 0)
  {
    fp->sign = true;
    val = -val;
  }
  else
    fp->sign = false;
  if(val == 0)
    return;
  int expo;
  long double mant = frexpl(val, &expo);
  mant *= (1ULL << 32);
  mant *= (1ULL << 32);
  fp->value.val[0] = (u64) mant;
  fpshr(*fp, maxExpo - expo);
}

long double getValue(FP* fp)
{
  int lz = lzcnt(&fp->value);
  const int words = fp->value.size;
  //if no 1 bits in mantissa, can return 0
  if(lz == 64 * words)
    return 0.0;
  u64* buf = alloca(words * sizeof(u64));
  for(int i = 0; i < words; i++)
    buf[i] = fp->value.val[i];
  BigInt tempCopy;
  tempCopy.val = buf;
  tempCopy.size = words;
  bishl(&tempCopy, lz);
  //now can read off the high word of temp copy as the value
  long double temp = tempCopy.val[0];
  int expo;
  frexpl(temp, &expo);
  expo -= (40 + 64 + maxExpo + lz);
  long double value = ldexpl(temp, expo);
  if(fp->sign)
    value *= -1;
  return value;
}

void fpcopy(FP* lhs, FP* rhs)
{
  int words = min(lhs->value.size, rhs->value.size);
  for(int i = 0; i < words; i++)
    lhs->value.val[i] = rhs->value.val[i];
  lhs->sign = rhs->sign;
}

int getApproxExpo(FP* fp)
{
  long double val = getValue(fp);
  int expo;
  frexpl(val, &expo);
  return expo;
}

bool fpValidate(FP* val)
{
  //mantissa as signed integer must be positive
  return !(val->value.val[0] & (1ULL << 63));
}

void fpWrite(FP* fp, FILE* handle)
{
  //size (u32), then sign (u8), then value (series of u64)
  fwrite(&fp->value.size, sizeof(unsigned), 1, handle);
  fwrite(&fp->sign, sizeof(bool), 1, handle);
  fwrite(fp->value.val, sizeof(u64), fp->value.size, handle);
}

FP fpRead(FILE* handle)
{
  unsigned size;
  fread(&size, sizeof(unsigned), 1, handle);
  FP fp = FPCtor(size);
  fread(&fp.sign, sizeof(bool), 1, handle);
  fread(fp.value.val, sizeof(u64), size, handle);
  return fp;
}

