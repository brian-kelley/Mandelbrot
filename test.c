#include "test.h"

static long double getRandLongDouble()
{
  int ex = -3 + (unsigned) rand() % 6;
  long double mant = 0.5 + (long double) 0.5 / RAND_MAX * rand();
  if(rand() % 2)
    mant *= -1;
  return ldexpl(mant, ex);
}

static int fpTest(int prec)
{
  srand(42);
  long double tol;
  if(prec == 1)
    tol = 1e-6;
  else
    tol = 1e-7;
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
    long double op[2] = {getRandLongDouble(), getRandLongDouble()};
    loadValue(&op1, op[0]);
    loadValue(&op2, op[1]);
    fpadd3(&sum, &op1, &op2);
    fpValidate(&sum);
    fpsub3(&diff, &op1, &op2);
    fpValidate(&diff);
    fpmul3(&prod, &op1, &op2);
    fpValidate(&prod);
    {
      long double actual = op[0] + op[1];
      if(fabsl((getValue(&sum) - actual) / actual) > tol)
      {
        printf("  <!> Failing because result of %.20Lf * %.20Lf = %.20Lf was wrong.\n", getValue(&op1), getValue(&op2), getValue(&sum));
        break;
      }
    }
    {
      long double actual = op[0] - op[1];
      if(fabsl((getValue(&diff) - actual) / actual) > tol)
      {
        printf("  <!> Failing because result of %.20Lf * %.20Lf = %.20Lf was wrong.\n", getValue(&op1), getValue(&op2), getValue(&diff));
        break;
      }
    }
    {
      long double actual = op[0] * op[1];
      if(fabsl((getValue(&prod) - actual) / actual) > tol)
      {
        printf("  <!> Failing because result of %.20Lf * %.20Lf = %.20Lf was wrong.\n", getValue(&op1), getValue(&op2), getValue(&prod));
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
    fpValidate(&sum);
    fpsub2(&diff, &op1);
    fpValidate(&diff);
    fpmul2(&prod, &op1);
    fpValidate(&prod);
    {
      if(fabsl((getValue(&sum) - actualSum) / actualSum) > tol)
      {
        printf("  <!> Failing because result of %.20Lf * %.20Lf = %.20Lf was wrong.\n", getValue(&op1), getValue(&op2), getValue(&sum));
        break;
      }
    }
    {
      if(fabsl((getValue(&diff) - actualDiff) / actualDiff) > tol)
      {
        printf("  <!> Failing because result of %.20Lf * %.20Lf = %.20Lf was wrong.\n", getValue(&op1), getValue(&op2), getValue(&diff));
        break;
      }
    }
    {
      if(fabsl((getValue(&prod) - actualProd) / actualProd) > tol)
      {
        printf("  <!> Failing because result of %.20Lf * %.20Lf = %.20Lf was wrong.\n", getValue(&op1), getValue(&op2), getValue(&prod));
        break;
      }
    }
    if(tested++ % 1000000 == 999999)
    {
      return 0;
    }
  }
  return 1;
}

//returns the number of bits that are different (least-significant)
static int differentBits(u64* buf1, u64* buf2, int n)
{
  for(int i = 64 * n - 1; i >= 0; i--)
  {
    int word = i / 64;
    u64 mask = 1ULL << (i % 64);
    if((buf1[word] & mask) == (buf2[word] & mask))
    {
      return 64 * n - i - 1;
    }
  }
  return 64 * n;
}

#define IMPL_TEST_SPECIAL(n) \
static int testSpecial##n() \
{ \
  srand(42); \
  long double tol = 1e-6; \
  int prec = n; \
  MAKE_STACK_FP(op1FP); \
  MAKE_STACK_FP(op2FP); \
  MAKE_STACK_FP(resultFP); \
  int tested = 0; \
  int success = 1; \
  int warns = 0; \
  int worst = 0; \
  while(true) \
  { \
    long double op[2] = {getRandLongDouble(), getRandLongDouble()}; \
    loadValue(&op1FP, op[0]); \
    loadValue(&op2FP, op[1]); \
    FP##n op1 = load##n(&op1FP); \
    FP##n op2 = load##n(&op2FP); \
    FP##n sum = add##n(op1, op2); \
    FP##n diff = sub##n(op1, op2); \
    FP##n prod = mul##n(op1, op2); \
    { \
      fpadd3(&resultFP, &op1FP, &op2FP); \
      long double actual = op[0] + op[1]; \
      if(fabsl((getVal##n(sum) - actual)) > tol) \
      { \
        printf("  <!> Failing because result of %.20f * %.20f = %.20f was wrong.\n", getVal##n(op1), getVal##n(op2), getVal##n(sum)); \
        break; \
      } \
      if(resultFP.sign) \
      { \
        resultFP.sign = false; \
        sum = neg##n(sum); \
      } \
      int err = differentBits(resultFP.value.val, sum.w, n); \
      if(err > 1) \
      { \
        warns++; \
        if(err > worst) \
          worst = err; \
      } \
    } \
    { \
      fpsub3(&resultFP, &op1FP, &op2FP); \
      long double actual = op[0] - op[1]; \
      if(fabsl((getVal##n(diff) - actual)) > tol) \
      { \
        printf("  <!> Failing because result of %.20f * %.20f = %.20f was wrong.\n", getVal##n(op1), getVal##n(op2), getVal##n(diff)); \
        break; \
      } \
      if(resultFP.sign) \
      { \
        resultFP.sign = false; \
        diff = neg##n(diff); \
      } \
      int err = differentBits(resultFP.value.val, sum.w, n); \
      if(err > 1) \
      { \
        warns++; \
        if(err > worst) \
          worst = err; \
      } \
    } \
    { \
      fpmul3(&resultFP, &op1FP, &op2FP); \
      long double actual = op[0] * op[1]; \
      if(fabsl((getVal##n(prod) - actual)) > tol) \
      { \
        printf("  <!> Failing because result of %.20f * %.20f = %.20f was wrong.\n", getVal##n(op1), getVal##n(op2), getVal##n(prod)); \
        break; \
      } \
      if(resultFP.sign) \
      { \
        resultFP.sign = false; \
        prod = neg##n(prod); \
      } \
      int err = differentBits(resultFP.value.val, sum.w, n); \
      if(err > 1) \
      { \
        warns++; \
        if(err > worst) \
          worst = err; \
      } \
    } \
    if(tested++ % 1000000 == 999999) \
    { \
      if(warns) \
      { \
        printf("  <+> Tests passed but %i of 3,000,000 results differed by > 1 bit.\n", warns); \
        printf("  <+> Worst error: %i bits.\n", worst); \
      } \
      success = 0; \
      break; \
    } \
  } \
  return success; \
}

IMPL_TEST_SPECIAL(3)
IMPL_TEST_SPECIAL(4)
IMPL_TEST_SPECIAL(5)
IMPL_TEST_SPECIAL(6)
IMPL_TEST_SPECIAL(7)
IMPL_TEST_SPECIAL(8)
IMPL_TEST_SPECIAL(9)
IMPL_TEST_SPECIAL(10)

static int testLoadAndGet(int prec)
{
  MAKE_STACK_FP(fp);
  for(int i = 0; i < 1000000; i++)
  {
    //test long double <--> FP
    long double v = getRandLongDouble();
    loadValue(&fp, v);
    long double vback = getValue(&fp);
    if(fabsl(v - vback) > 1e-6)
    {
      puts("  <!> Failing because FP loadValue then getValue different from original.");
      return 1;
    }
    if(prec == 2)
    {
      //test double <--> FP2
      FP2 fp2_1 = 0;
      setVal2(&fp2_1, v);
      FP2 fp2_2 = load2(&fp);
      if(abs((int) (fp2_2 - fp2_1)) > 2)
      {
        puts("  <!> Failing because FP2 loaded through double and FP are different.");
        printf("    Note: through double, FP2 is %016llx:%016llx\n", (u64) (fp2_1 >> 64), (u64) (fp2_1 & LOW_8));
        printf("              through FP, FP2 is %016llx:%016llx\n", (u64) (fp2_2 >> 64), (u64) (fp2_2 & LOW_8));
        printf("    Note:          correct FP is %016llx:%016llx\n", fp.value.val[0], fp.value.val[1]);
        return 1;
      }
      vback = getVal2(fp2_1);
      if(fabsl(v - vback) > 1e-6)
      {
        puts("  <!> Failing because FP2 loaded through double is wrong.");
        return 1;
      }
      vback = getVal2(fp2_2);
      if(fabsl(v - vback) > 1e-6)
      {
        puts("  <!> Failing because FP2 loaded through FP is wrong.");
        return 1;
      }
    }
  }
  return 0;
}

#define TEST_SPECIAL_ARITHMETIC(n) \
  err += testSpecial##n(); \
  printf("  Tested prec level %i.\n", n);

int testAll()
{
  puts("Testing generic fixed-point arithmetic.");
  int err = 0;
  for(int i = 1; i <= 4; i++)
  {
    err += fpTest(i);
    printf("  Tested prec level %i.\n", i);
  }
  puts("Testing specialized fixed-point arithmetic.");
  TEST_SPECIAL_ARITHMETIC(3)
  TEST_SPECIAL_ARITHMETIC(4)
  TEST_SPECIAL_ARITHMETIC(5)
  TEST_SPECIAL_ARITHMETIC(6)
  TEST_SPECIAL_ARITHMETIC(7)
  TEST_SPECIAL_ARITHMETIC(8)
  TEST_SPECIAL_ARITHMETIC(9)
  TEST_SPECIAL_ARITHMETIC(10)
  puts("Testing loadValue/getValue.");
  for(int i = 1; i < 3; i++)
  {
    err += testLoadAndGet(i);
    printf("  Tested prec level %i.\n", i);
  }
  puts("");
  if(err == 1)
  {
    puts("*** 1 TEST FAILED ***");
  }
  else if(err)
  {
    printf("*** %i TESTS FAILED ***\n", err);
  }
  else
  {
    puts("*** TESTS PASSED ***");
  }
  exit(err);
}
