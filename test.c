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

static int testSpecial2()
{
  srand(42);
  long double tol = 1e-6;
  int prec = 2;
  MAKE_STACK_FP(op1FP);
  MAKE_STACK_FP(op2FP);
  int tested = 0;
  int success = 1;
  while(true)
  {
    long double op[2] = {getRandLongDouble(), getRandLongDouble()};
    loadValue(&op1FP, op[0]);
    loadValue(&op2FP, op[1]);
    FP2 op1 = load2(&op1FP);
    FP2 op2 = load2(&op2FP);
    FP2 sum = op1 + op2;
    FP2 diff = op1 - op2;
    FP2 prod = mul2(op1, op2);
    {
      long double actual = op[0] + op[1];
      if(fabsl((getVal2(sum) - actual)) > tol)
      {
        printf("  <!> Failing because result of %.20f * %.20f = %.20f was wrong.\n", getVal2(op1), getVal2(op2), getVal2(sum));
        break;
      }
    }
    {
      long double actual = op[0] - op[1];
      if(fabsl((getVal2(diff) - actual)) > tol)
      {
        printf("  <!> Failing because result of %.20f * %.20f = %.20f was wrong.\n", getVal2(op1), getVal2(op2), getVal2(diff));
        break;
      }
    }
    {
      long double actual = op[0] * op[1];
      if(fabsl((getVal2(prod) - actual)) > tol)
      {
        printf("  <!> Failing because result of %.20f * %.20f = %.20f was wrong.\n", getVal2(op1), getVal2(op2), getVal2(prod));
        break;
      }
    }
    if(tested++ % 100000 == 99999)
    {
      success = 0;
      break;
    }
  }
  return success;
}

static int testLoadAndGet(int prec)
{
  MAKE_STACK_FP(fp);
  for(int i = 0; i < 10000; i++)
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

int testAll()
{
  puts("Testing generic fixed-point arithmetic.");
  int err = 0;
  for(int i = 1; i <= 4; i++)
  {
    err += fpTest(i);
    printf("  Tested prec level %i.\n", i);
  }
  puts("Testing 128-bit fixed-point arithmetic.");
  err += testSpecial2();
  puts("Testing loadValue/getValue.");
  for(int i = 1; i < 3; i++)
  {
    err += testLoadAndGet(i);
    printf("  Tested prec level %i.\n", i);
  }
  puts("");
  if(err)
  {
    puts("*** TEST FAILED ***");
  }
  else
  {
    puts("*** TEST PASSED ***");
  }
  exit(err);
}
