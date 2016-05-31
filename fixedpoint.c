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
        for(int i = 0; i < words; i++)
            dst.val[i] = lhs->value.val[i];
        //2s complement rhs
        biTwoComplement(&rhs->value);
        //add
        biadd(&dst, &lhs->value, &rhs->value);
        //undo the 2s complement
        biTwoComplement(&rhs->value);
        //copy the result into the real destination FP 
        for(int i = 0; i < words; i++)
            actualDst->value.val[i] = dst.val[i];
        actualDst->sign = lhs->sign;
    }
    else
    {
        //add (can overflow)
        for(int i = words - 1; i >= 0; i--)
            biAddWord(&lhs->value, rhs->value.val[i], i);
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
    for(int i = 0; i < words; i++)
        lhs->value.val[i] = wideDst.val[i];
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
        for(int i = 0; i < lhs->value.size; i++)
            dst->value.val[i] = lhs->value.val[i];
        //2s complement rhs
        biTwoComplement(&rhs->value);
        //add
        for(int i = lhs->value.size - 1; i >= 0; i--)
            biAddWord(&dst->value, rhs->value.val[i], i);
        //undo the 2s complement
        biTwoComplement(&rhs->value);
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
    for(int i = 0; i < words; i++)
        dst->value.val[i] = wideDst.val[i];
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
    val /= (1 << (maxExpo - 1));
    fp->value.val[0] = fabsl(val) * (1ULL << 62);
    fp->sign = val < 0;
}

long double getValue(FP* fp)
{
    if(fp->value.val[0])
    {
        long double val = fp->value.val[0] / (long double) (1ULL << 62);
        val *= fp->sign ? -1 : 1;
        val *= (1 << (maxExpo - 1));
        return val;
    }
    else
    {
        int wordShift;
        for(wordShift = 0; wordShift < fp->value.size; wordShift++)
        {
            if(fp->value.val[wordShift])
                break;
        }
        if(wordShift == fp->value.size)
            return 0;
        long double val = fp->value.val[wordShift] / (long double) (1ULL << 62);
        val *= fp->sign ? -1 : 1;
        int expo;
        long double mant = frexpl(val, &expo);
        expo += (maxExpo - 1);
        expo -= 63 * wordShift;
        return ldexpl(mant, expo);
    }
}

void fpcopy(FP* lhs, FP* rhs)
{
    int words = min(lhs->value.size, rhs->value.size);
    for(int i = 0; i < words; i++)
        lhs->value.val[i] = rhs->value.val[i];
    lhs->sign = rhs->sign;
}

int getApproxExpo(FP* lhs)
{
    int expo = maxExpo;   //assume the maximum value
    int i;
    for(i = 0; i < lhs->value.size; i++)
    {
        if(lhs->value.val[i] == 0)
            expo -= 63;
        else
            break;
    }
    if(i < lhs->value.size)
    {
        u64 word = lhs->value.val[i];
        while((word & (1ULL << 62)) == 0)
        {
            expo--;
            word <<= 1;
        }
    }
    return expo;
}

void arithmeticTest()
{
    for(u64 i = 0;; i++)
    {
        if(i % 1000000 == 999999)
            printf("%llu operand combinations tested.\n", i);
    }
}

void fuzzTest()
{
    const long double tol = 1e-5;
    srand(clock());
    int prec = 1;
    MAKE_STACK_FP(op1);
    MAKE_STACK_FP(op2);
    MAKE_STACK_FP(sum);
    MAKE_STACK_FP(diff);
    MAKE_STACK_FP(prod);
    u64 tested = 0;
    while(true)
    {
        long double op[2];
        for(int i = 0; i < 2; i++)
        {
            int ex = -4 + rand() % 7;
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

