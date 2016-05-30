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
        for(int i = words - 1; i >= 0; i--)
            biAddWord(&dst, rhs->value.val[i], i);
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
    BigInt dst;
    dst.size = words * 2;
    dst.val = (u64*) alloca(words * 2 * sizeof(u64));
    for(int i = words - 1; i >= 0; i--)          //i = index of word of *this
    {
        for(int j = words - 1; j >= 0; j--)      //j = index of word of rhs
        {
            //do the long multiplication
            u64 hi, lo;
            longmul(lhs->value.val[i], rhs->value.val[j], &hi, &lo);
            int destWord = i + j + 1;
            hi <<= 1;
            hi |= ((lo & (1ULL << 63)) >> 63);
            lo &= digitMask;
            biAddWord(&dst, lo, destWord);
            biAddWord(&dst, hi, destWord - 1);
        }
    }
    //copy the result into lhs
    for(int i = 0; i < words; i++)
        lhs->value.val[i] = dst.val[i];
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
        //add (can overflow)
        for(int i = 0; i < lhs->value.size; i++)
            dst->value.val[i] = lhs->value.val[i];
        for(int i = lhs->value.size - 1; i >= 0; i--)
            biAddWord(&dst->value, rhs->value.val[i], i);
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
    memset(wideDst.val, 0, words * 2 * sizeof(u64));
    for(int i = words - 1; i >= 0; i--)          //i = index of word of *this
    {
        for(int j = words - 1; j >= 0; j--)      //j = index of word of rhs
        {
            u64 hi, lo;
            longmul(lhs->value.val[i], rhs->value.val[j], &hi, &lo);
            int destWord = i + j + 1;
            hi <<= 1;
            hi |= ((lo & (1ULL << 63)) >> 63);
            lo &= digitMask;
            biAddWord(&wideDst, lo, destWord);
            biAddWord(&wideDst, hi, destWord - 1);
        }
    }
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
    long double val = fp->value.val[0] / (long double) (1ULL << 62);
    val *= fp->sign ? -1 : 1;
    val *= (1 << (maxExpo - 1));
    return val;
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
