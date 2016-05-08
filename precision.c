#include "precision.h"

static u64* scratch;

BigInt BigIntCtor(int size)
{
    BigInt rv;
    rv.val = (u64*) malloc(size * sizeof(u64));  //initialize to 0
    rv.size = size;
    memset(rv.val, 0, size * sizeof(u64));
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
            carry = !!(bi->val[i] & carryMask);
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
    int words = lhs->size;
    for(int i = 0; i < 2 * words; i++)
        dst->val[i] = 0;
    //first, compute the low half of the full result
    for(int i = words - 1; i >= 0; i--)          //i = index of word of *this
    {
        for(int j = words - 1; j >= 0; j--)      //j = index of word of rhs
        {
            //do the long multiplication
            u64 hi, lo;
            longmul(lhs->val[i], rhs->val[j], &hi, &lo);
            int destWord = i + j + 1;
            biAddWord(dst, lo, destWord);
            biAddWord(dst, hi, destWord - 1);
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

void biPrint(BigInt* op)
{
    int numQwords = ceil((op->size * 63.0) / 64.0);
    for(int qword = 0; qword < numQwords; qword++)
    {
        u64 bits = 0;
        for(int i = 0; i < 64; i++)
            bits |= (biNthBit(op, 64 * (numQwords - 1 - qword) + i) << i);
        printf("%016llx", bits);
    }
    puts("");
}

u64 biNthBit(BigInt* op, int n)
{
    if(n < 0 || n >= op->size * 63)
        return 0;
    int word = n / 63;
    int bit = n % 63;
    return op->val[op->size - 1 - word] & (1ULL << bit) ? 1 : 0;
}

void staticPrecInit(int maxPrec)
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

void FloatDtor(Float* f)
{
    BigIntDtor(&f->mantissa);
}

Float floatLoad(int prec, long double d)
{
    Float f = FloatCtor(prec);
    if(d == 0)
    {
        floatWriteZero(&f);
        return f;
    }
    f.sign = d < 0;
    long double mant = frexpl(d, &f.expo);
    f.expo -= expoBias;
    u8* mantBytes = (u8*) &mant;
    u8* highWordBytes = (u8*) &f.mantissa.val[0];
    //copy 64 bit mantissa. Byte order is LE in the long double and LE in the u64
    for(int byteCount = 0; byteCount < 8; byteCount++)
        highWordBytes[byteCount] = mantBytes[byteCount];
    //shift the mantissa down 1 bit so the high word contains 63 significant bits
    f.mantissa.val[0] &= digitMask;
    bishr(&f.mantissa, 1);
    f.mantissa.val[0] |= (1ULL << 62);  //this bit needs to be high, but may be lost by bishl
    for(int i = 1; i < prec; i++)
        f.mantissa.val[i] = 0;
    return f;
}

long double getFloatVal(Float* f)
{
    if(fzero(f))
        return 0;
    int realExpo = f->expo + expoBias;
    long double mant = 0;                   //this makes all bits 0
    u8* mantBytes = (u8*) &mant;
    u8* highWordBytes = (u8*) &scratch[0];
    //add a biased 0 exponent to mant
    //don't modify f, work from copy in scratch space
    scratch[0] = f->mantissa.val[0];
    scratch[0] <<= 1;
    //lose highest digit for legal float?
    if(f->mantissa.size > 1)
    {
        if(f->mantissa.val[1] & (1ULL << 62))
            scratch[0]++;
    }
    *((short*) &mantBytes[8]) = 16382;      //this value is weird but it works
    //copy mantissa 
    for(int byteCount = 0; byteCount < 8; byteCount++)
        mantBytes[byteCount] = highWordBytes[byteCount];
    if(f->sign)
        mant *= -1;
    return ldexpl(mant, realExpo);
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
    printf("after mul, high byte = %hhx\n", (u8) ((bigDest.val[0] & (0xFFULL << 56)) >> 56));
    if((bigDest.val[0] & (1ULL << 61)) == 0)
    {
        puts("Will shl because bit 61 is low.");
        bishl(&dst->mantissa, 2);
        newExpo--;
        dst->mantissa.val[words - 1] |= (bigDest.val[words] & (1ULL << 62));
        if(bigDest.val[1] & (1ULL << 60))
            biinc(&dst->mantissa);
    }
    else
    {
        bishlOne(&dst->mantissa);
        if(bigDest.val[words] & (1ULL << 62))
            biinc(&dst->mantissa);
    }
    dst->sign = lhs->sign ^ rhs->sign;
    dst->expo = newExpo - expoBias;
}

void fadd(Float* dst, Float* lhs, Float* rhs)
{
    //compare magnitudes (want lhs to be larger magnitude)
    int magCmp = compareFloatMagnitude(lhs, rhs);

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

int compareFloatMagnitude(Float* lhs, Float* rhs)
{
    //handle zero cases
    bool lzero = fzero(lhs);
    bool rzero = fzero(rhs);
    if(lzero && rzero)
        return 0;
    if(lzero && !rzero)
        return -1;
    if(!lzero && rzero)
        return 1;
    //if exponents different, larger exponent always the larger value
    //note that bias doesn't affect this comparison
    if(lhs->expo < rhs->expo)
        return -1;
    if(lhs->expo > rhs->expo)
        return 1;
    //now compare mantissa word by word
    for(int i = 0; i < lhs->mantissa.size; i++)
    {
        if(lhs->mantissa.val[i] < rhs->mantissa.val[i])
            return -1;
        else if(lhs->mantissa.val[i] > rhs->mantissa.val[i])
            return 1;
    }
    //both exponent and mantissas identical
    return 0;
}

void fconvert(Float* lhs, Float* rhs)
{
    if(fzero(rhs))
    {
        floatWriteZero(lhs);
        return;
    }
    //sign and expo copy directly
    lhs->sign = rhs->sign;
    lhs->expo = rhs->expo;
    BigInt* lm = &lhs->mantissa;
    BigInt* rm = &rhs->mantissa;
    //copy as much of mantissa as possible
    int copyWords = min(lm->size, rm->size);
    for(int i = 0; i < copyWords; i++)
        lm->val[i] = rm->val[i];
    //if rhs has higher precision, round lhs to nearest
    //round up iff the most significant bit not copied is 1
    if(rm->size > lm->size && rm->val[lm->size] & (1ULL << 62))
    {
        biinc(lm);
        //catch the rare event that lm overflows
        if(lm->val[0] == 0)
        {
            lm->val[0] |= (1ULL << 62);
            lhs->expo++;
        }
    }
}
