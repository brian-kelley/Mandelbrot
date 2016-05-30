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

bool fpadd2(FP* lhs, FP* rhs)
{
    const int words = lhs->value.size;
    bool overflow = false;
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
        //2s complement rhs
        biTwoComplement(&rhs->value);
        //add
        for(int i = words - 1; i >= 0; i--)
            biAddWord(&lhs->value, rhs->value.val[i], i);
        //undo the 2s complement
        biTwoComplement(&rhs->value);
    }
    else
    {
        //add (can overflow)
        for(int i = words - 1; i >= 0; i--)
            overflow |= biAddWord(&lhs->value, rhs->value.val[i], i);
    }
    return overflow;
}

bool fpsub2(FP* lhs, FP* rhs)
{
    bool savedSign = rhs->sign;
    rhs->sign = !rhs->sign;
    bool overflow = fpadd2(lhs, rhs);
    rhs->sign = savedSign;
    return overflow;
}

bool fpmul2(FP* lhs, FP* rhs)
{
    bool overflow = false;
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
            overflow |= biAddWord(&dst, lo, destWord);
            overflow |= biAddWord(&dst, hi, destWord - 1);
        }
    }
    //copy the result into lhs
    for(int i = 0; i < words; i++)
        lhs->value.val[i] = dst.val[i];
    lhs->sign = lhs->sign != rhs->sign;
    return overflow;
}

bool fpadd3(FP* restrict dst, FP* lhs, FP* rhs)
{
    bool overflow = false;
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
        //copy rhs into dst
        for(int i = 0; i < lhs->value.size; i++)
            dst->value.val[i] = lhs->value.val[i];
        for(int i = lhs->value.size - 1; i >= 0; i--)
            overflow |= biAddWord(&dst->value, rhs->value.val[i], i);
    }
    return overflow;
}

bool fpsub3(FP* restrict dst, FP* lhs, FP* rhs)
{
    bool savedSign = rhs->sign;
    rhs->sign = !rhs->sign;
    bool overflow = fpadd3(dst, lhs, rhs);
    rhs->sign = savedSign;
    return overflow;
}

bool fpmul3(FP* restrict dst, FP* lhs, FP* rhs)
{
    bool overflow = false;
    const int words = lhs->value.size;
    //bimul requires a destination twice as wide as operands
    BigInt wideDst;
    wideDst.size = words * 2;
    wideDst.val = (u64*) alloca(words * 2 * sizeof(u64));
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
            overflow |= biAddWord(&wideDst, lo, destWord);
            overflow |= biAddWord(&wideDst, hi, destWord - 1);
        }
    }
    //copy the result into dst
    for(int i = 0; i < words; i++)
        dst->value.val[i] = wideDst.val[i];
    lhs->sign = lhs->sign != rhs->sign;
    return overflow;
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
    fp->value.val[0] = val * (1ULL << 63);
    fp->sign = val < 0;
}

long double getValue(FP* fp)
{
    return ((long double) fp->value.val[0] / (1ULL << 63)) * (fp->sign ? -1 : 1);
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
    int expo = 2;   //assume the maximum value
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
