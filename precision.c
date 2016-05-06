#include "precision.h"

static u64* scratch;

BigInt BigIntCtor(int size)
{
    BigInt rv;
    rv.val = (u64*) calloc(size, sizeof(u64));  //initialize to 0
    rv.size = size;
    return rv;
}

BigInt BigIntCopy(BigInt* bi)
{
    BigInt copy;
    copy.size = bi->size;
    copy.val = (u64*) malloc(copy.size * sizeof(u64));
    for(int i = 0; i < bi->size; i++)
        copy.val[i] = bi->val[i];
    return copy;
}

void BigIntDtor(BigInt* bi)
{
    free(bi->val);
}

static bool biAddWord(BigInt* bi, u64 word, int position)
{
    bi->val[position] += word;
    bool carry = bi->val[position] & carryMask;
    bi->val[position] &= digitMask;
    for(int i = position - 1; i >= 0; i--)
    {
        if(carry)
        {
            bi->val[i]++;
            carry = bi->val[i] & carryMask;
            bi->val[i] &= digitMask;
        }
        else
            break;
    }
    return carry;
}

void bimul(BigInt* dst, BigInt* lhs, BigInt* rhs)
{
    //zero out dst
    for(int i = 0; i < dst->size; i++)
        dst->val[i] = 0;
    //first, compute the low half of the full result
    for(int i = lhs->size - 1; i >= 0; i--)          //i = index of word of *this
    {
        for(int j = lhs->size - 1; j >= 0; j--)      //j = index of word of rhs
        {
            //do the long multiplication
            u64 hi, lo;
            longmul(lhs->val[i], rhs->val[j], &hi, &lo);
            biAddWord(dst, lo, i + j);
            biAddWord(dst, hi, i + j + 1);
        }
    }
}

bool biadd(BigInt* dst, BigInt* lhs, BigInt* rhs)
{
    //copy lhs value into dst
    for(int i = 0; i < dst->size; i++)
        dst->val[i] = lhs->val[i];
    bool carry;
    for(int i = rhs->size - 1; i >= 0; i--)
    {
        carry = false;
        carry = biAddWord(dst, rhs->val[i], i);
    }
    //carry will have the carry bit of the last word (for i = 0)
    return carry;
}

void bisub(BigInt* dst, BigInt* lhs, BigInt* rhs)
{
    //copy words of rhs into dst
    for(int i = 0; i < dst->size; i++)
        dst->val[i] = rhs->val[i];
    BigInt addend;
    addend.size = rhs->size;
    addend.val = scratch;
    for(int i = 0; i < rhs->size; i++)
        addend.val[i] = rhs->val[i];
    biTwoComplement(&addend);
    biadd(dst, lhs, &addend);
}

void bishl(BigInt* op, int bits)
{
    if(bits >= 63 * op->size)
    {
        memset(op->val, 0, op->size * sizeof(u64));
        return;
    }
    //note: the highest bit of each word is not used
    const int wordBits = 63;
    int wordShift = bits / wordBits;
    int bitShift = bits % wordBits;
    //first, apply word shift
    for(int i = 0; i < op->size - wordShift; i++)
        op->val[i] = op->val[i + wordShift];
    for(int i = op->size - wordShift; i < op->size; i++)
        op->val[i] = 0;
    if(bitShift == 0)
        return;
    //determine the bit mask for the bits that are copied up into the next word
    int hishift = wordBits - bits;
    u64 mask = ((1ULL << bits) - 1) << hishift;
    //do the lowest word manually
    u64 moved;
    for(int i = 0; i < op->size; i++)
    {
        op->val[i] <<= bits;
        if(i < op->size - 1)
        {
            moved = op->val[i + 1] & mask;
            op->val[i] |= (moved >> hishift);
        }
    }
}

void bishr(BigInt* op, int bits)
{
    if(bits >= 63 * op->size)
    {
        memset(op->val, 0, op->size * sizeof(u64));
        return;
    }
    //note: the highest bit of each word is not used
    const int wordBits = 63;
    int wordShift = bits / wordBits;
    int bitShift = bits % wordBits;
    //first, apply word shift
    for(int i = op->size - wordShift - 1; i >= 0; i--)
        op->val[i + wordShift] = op->val[i];
    for(int i = 0; i < wordShift; i++)
        op->val[i] = 0;
    if(bitShift == 0)
        return;
    //determine the bit mask for the bits that are copied up into the next word
    int hishift = wordBits - bits;
    u64 mask = ((1ULL << bits) - 1);
    //do the lowest word manually
    u64 moved;
    for(int i = op->size - 1; i >= 0; i--)
    {
        op->val[i] >>= bits;
        if(i > 0)
        {
            moved = op->val[i - 1] & mask;
            op->val[i] |= (moved << hishift);
        }
    }
}


void bishlOne(BigInt* op)
{
    for(int i = 0; i < op->size - 1; i++)
    {
        op->val[i] <<= 1;
        op->val[i] |= (op->val[i + 1] & (1ULL << 63)) >> 63;
    }
    op->val[op->size - 1] <<= 1;
}

void biinc(BigInt* op)
{
    op->val[op->size - 1]++;
    bool carry = op->val[op->size - 1] & carryMask;
    if(carry)
    {
        op->val[op->size - 1] &= digitMask;
        for(int i = op->size - 2; i >= 0; i--)
        {
            if(carry)
                op->val[i]++;
            carry = op->val[i] & carryMask;
            if(carry)
                op->val[i] &= digitMask;
            else
                return;
        }
    }
}

void biTwoComplement(BigInt* op)
{
    for(int i = 0; i < op->size; i++)
        op->val[i] = ~op->val[i];
    biinc(op);
}

void staticFloatInit(int maxPrec)
{
    scratch = (u64*) malloc(2 * maxPrec * sizeof(u64));
}

Float FloatCtor(int prec)
{
    Float f;
    f.mantissa = BigIntCtor(prec);
    f.expo = 0;
    f.sign = false;
    return f;
}

Float floatLoadDouble(int prec, long double d)
{
    Float f = FloatCtor(prec);
    f.sign = d < 0;
    long double mant = frexpl(d, &f.expo);
    f.expo -= expoBias;
    //take the lowest 
    f.mantissa.val[0] = *((u64*) &mant) & 0x7FFFFFFFFFFFFFFF;
    for(int i = 1; i < prec; i++)
        f.mantissa.val[i] = 0;
    return f;
}

void FloatDtor(Float* f)
{
    BigIntDtor(&f->mantissa);
}

void floatWriteZero(Float* f)
{
    f->expo = 0;
    for(int i = 0; i < f->mantissa.size; i++)
        f->mantissa.val[i] = 0;
}

void fmul(Float* dst, Float* lhs, Float* rhs)
{
    if(fzero(lhs) || fzero(rhs))
    {
        floatWriteZero(dst);
        return;
    }
    //get new exponent (no bias)
    const int words = dst->mantissa.size;
    int newExpo = (lhs->expo + expoBias) + (rhs->expo + expoBias);
    BigInt bigDest;
    bigDest.val = scratch;
    bigDest.size = 2 * words;
    bimul(&bigDest, &lhs->mantissa, &rhs->mantissa);
    //copy high words into dst's mantissa
    for(int i = 0; i < words; i++)
        dst->mantissa.val[i] = bigDest.val[i];
    //2 cases here: highest bit of product is 0 or 1
    if((bigDest.val[0] & (1ULL << 62)) == 0)
    {
        bishlOne(&dst->mantissa);
        newExpo--;
        dst->mantissa.val[words - 1] |= (bigDest.val[words] & (1ULL << 62));
        if(bigDest.val[words] & (1ULL << 61))
            biinc(&dst->mantissa);
    }
    else
    {
        if(bigDest.val[words] & (1ULL << 62))
            biinc(&dst->mantissa);
    }
    dst->sign = lhs->sign ^ rhs->sign;
    dst->expo = newExpo - expoBias;
}

void fadd(Float* dst, Float* lhs, Float* rhs)
{
}

void fsub(Float* dst, Float* lhs, Float* rhs)
{
}

bool fzero(Float* f)
{
    if(f->expo != 0)
        return false;
    for(int i = 0; i < f->expo; i++)
    {
        if(f->mantissa.val[i] != 0)
            return false;
    }
    return true;
}

int fcmp(Float* lhs, Float* dst)
{
}
