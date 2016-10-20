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
  for(int i = 0; i < fp->value.size; i++)
    fp->value.val[i] = 0;
  if(val < 0)
  {
    fp->sign = true;
    val *= -1;
  }
  else
    fp->sign = false;
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
  long double mantissa = frexpl(temp, &expo);
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

void fuzzTest()
{
  const long double tol = 1e-6;
  srand(clock());
  int prec = 1;
  MAKE_STACK_FP(op1);
  MAKE_STACK_FP(op2);
  MAKE_STACK_FP(sum);
  MAKE_STACK_FP(diff);
  MAKE_STACK_FP(prod);
  MAKE_STACK_FP(oldSum);
  MAKE_STACK_FP(oldDiff);
  MAKE_STACK_FP(oldProd);
  u64 tested = 0;
  while(true)
  {
    long double op[2];
    for(int i = 0; i < 2; i++)
    {
      int ex = -3 + rand() % 5;
      long double mant = (long double) 1.0 / RAND_MAX * rand();
      if(rand() & 0x10)
        mant *= -1;
      op[i] = ldexpl(mant, ex);
    }
    loadValue(&op1, op[0]);
    loadValue(&op2, op[1]);
    fpadd3(&sum, &op1, &op2);
    fpsub3(&diff, &op1, &op2);
    fpmul3(&prod, &op1, &op2);
    {
      long double actual = op[0] + op[1];
      if(fabsl((getValue(&sum) - actual) / actual) > tol)
      {
        printf("Result of %.20Lf + %.20Lf = %.20Lf was wrong!\n", getValue(&op1), getValue(&op2), getValue(&sum));
        break;
      }
    }
    {
      long double actual = op[0] - op[1];
      if(fabsl((getValue(&diff) - actual) / actual) > tol)
      {
        printf("Result of %.20Lf - %.20Lf = %.20Lf was wrong!\n", getValue(&op1), getValue(&op2), getValue(&diff));
        break;
      }
    }
    {
      long double actual = op[0] * op[1];
      if(fabsl((getValue(&prod) - actual) / actual) > tol)
      {
        printf("Result of %.20Lf * %.20Lf = %.20Lf was wrong!\n", getValue(&op1), getValue(&op2), getValue(&prod));
        break;
      }
    }
    long double actualSum = getValue(&sum) + getValue(&op1);
    long double actualDiff = getValue(&diff) - getValue(&op1);
    long double actualProd = getValue(&prod) * getValue(&op1);
    fpcopy(&oldSum, &sum);
    fpcopy(&oldDiff, &diff);
    fpcopy(&oldProd, &prod);
    fpadd2(&sum, &op1);
    fpsub2(&diff, &op1);
    fpmul2(&prod, &op1);
    {
      if(fabsl((getValue(&sum) - actualSum) / actualSum) > tol)
      {
        printf("Result of %.20Lf + %.20Lf = %.20Lf was wrong!\n", getValue(&op1), getValue(&oldSum), getValue(&sum));
        break;
      }
    }
    {
      if(fabsl((getValue(&diff) - actualDiff) / actualDiff) > tol)
      {
        printf("Result of %.20Lf - %.20Lf = %.20Lf was wrong!\n", getValue(&op1), getValue(&oldDiff), getValue(&diff));
        break;
      }
    }
    {
      if(fabsl((getValue(&prod) - actualProd) / actualProd) > tol)
      {
        printf("Result of %.20Lf * %.20Lf = %.20Lf was wrong!\n", getValue(&op1), getValue(&oldProd), getValue(&prod));
        break;
      }
    }
    if(tested++ % 1000000 == 999999)
      printf("%llu operand combinations tested.\n", tested);
  }
  printf("Failed after %llu operand combinations.\n", tested);
  exit(EXIT_FAILURE);
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

