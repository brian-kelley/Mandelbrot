#include "bigint.h"

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

bool biAddWord(BigInt* bi, u64 word, int position)
{
    word &= digitMask;
    bi->val[position] += word;
    u64 carry = bi->val[position] & carryMask;
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

void bimulC(BigInt* dst, BigInt* lhs, BigInt* rhs)
{
    //zero out dst
    int words = lhs->size;
    for(int i = 0; i < 2 * words; i++)
        dst->val[i] = 0;
    //first, compute the low half of the full result
    u64 hi, lo;
    unsigned __int128 prod;
    for(int i = words - 1; i >= 0; i--)
    {
        for(int j = words - 1; j >= 0; j--)
        {
            prod = (unsigned __int128) lhs->val[i] * (unsigned __int128) rhs->val[j];
            int destWord = i + j + 1;
            lo = prod & 0x7FFFFFFFFFFFFFFF;
            hi = prod >> 63;
            biAddWord(dst, lo, destWord);
            biAddWord(dst, hi, destWord - 1);
        }
    }
}

u64 biadd(BigInt* dst, BigInt* lhs, BigInt* rhs)
{
    //copy lhs value into dst
    for(int i = 0; i < dst->size; i++)
        dst->val[i] = lhs->val[i];
    u64 carry;
    for(int i = rhs->size - 1; i >= 0; i--)
        carry = biAddWord(dst, rhs->val[i], i);
    //carry will have the carry bit of the last word (for i = 0)
    return carry;
}

void bisub(BigInt* dst, BigInt* lhs, BigInt* rhs)
{
    //copy words of lhs into dst
    for(int i = 0; i < dst->size; i++)
        dst->val[i] = lhs->val[i];
    for(int i = dst->size - 1; i >= 0; i--)
        biAddWord(dst, ~rhs->val[i] & digitMask, i);
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
        op->val[i] &= digitMask;
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
        op->val[i] &= digitMask;
        op->val[i] |= (op->val[i + 1] & (1ULL << 62)) >> 62;
    }
    op->val[op->size - 1] <<= 1;
    op->val[op->size - 1] &= digitMask;
}

void bishrOne(BigInt* op)
{
    for(int i = op->size - 1; i > 0; i--)
    {
        op->val[i] >>= 1;
        op->val[i] |= (op->val[i - 1] & 1ULL) << 62;
    }
    op->val[0] >>= 1;
}

void biTwoComplement(BigInt* op)
{
    for(int i = 0; i < op->size; i++)
    {
        op->val[i] = ~op->val[i];
        op->val[i] &= digitMask;
    }
    biinc(op);
}

void biPrint(BigInt* op)
{
    for(int i = 0; i < op->size; i++)
    {
        printf("%016llx", op->val[i]);
    }
    puts("");
}

void biPrintBin(BigInt* op)
{
    for(int i = 0; i < op->size; i++)
    {
        u64 mask = (1ULL << 62);
        for(int j = 0; j <= 62; j++)
        {
            int bit = op->val[i] & mask ? 1 : 0;
            printf("%i", bit);
            mask >>= 1;
        }
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

void profiler()
{
    int prec = 5;
    u64 trials = 10;
    u64 operations = 3000000;
    BigInt op1 = BigIntCtor(prec);
    BigInt op2 = BigIntCtor(prec);
    BigInt dst = BigIntCtor(prec);
#define profile(func) \
    { \
        clock_t start = clock(); \
        for(u64 i = 0; i < trials; i++) \
        { \
            for(int j = 0; j < 2; j++) \
            { \
                op1.val[j] = 0; \
                op2.val[j] = 0; \
            } \
            for(int j = 2; j < prec; j++) \
            { \
                op1.val[j] = ((u64) rand()) << 32 ^ (u64) rand(); \
                op2.val[j] = ((u64) rand()) << 32 ^ (u64) rand(); \
            } \
            for(u64 j = 0; j < operations; j++) \
            { \
                func(&dst, &op1, &op2); \
            } \
        } \
        double perSec = (double) trials * (double) operations / ((double) clock() - start) * CLOCKS_PER_SEC; \
        printf("Function %s ran %e times per sec.\n", #func, perSec); \
    }
#define profileUnary(func) \
    { \
        clock_t start = clock(); \
        for(u64 i = 0; i < trials; i++) \
        { \
            for(int j = 0; j < prec; j++) \
            { \
                op1.val[j] = ((u64) rand()) << 32 ^ (u64) rand(); \
            } \
            for(u64 j = 0; j < operations; j++) \
            { \
                func(&dst); \
            } \
        } \
        double perSec = (double) trials * (double) operations / ((double) clock() - start) * CLOCKS_PER_SEC; \
        printf("Function %s ran %e times per sec.\n", #func, perSec); \
    }
    profile(bimul);
    profile(bimulC);
    profile(biadd);
    profile(bisub);
    profileUnary(biinc);
}
