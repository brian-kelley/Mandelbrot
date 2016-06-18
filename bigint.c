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

u64 biAddWord(BigInt* dst, u64 word, int position)
{
    u128 sum = (u128) dst->val[position] + word;
    dst->val[position] = sum;
    return sum >> 64;
}

void bimulC(BigInt* dst, BigInt* lhs, BigInt* rhs)
{
    //zero out dst
    int words = lhs->size;
    for(int i = 0; i < 2 * words; i++)
        dst->val[i] = 0;
    //first, compute the low half of the full result
    u64 hi, lo;
    u128 prod;
    for(int i = words - 1; i >= 0; i--)
    {
        for(int j = words - 1; j >= 0; j--)
        {
            prod = (u128) lhs->val[i] * (u128) rhs->val[j];
            int destWord = i + j + 1;
            u128 losum = (u128) dst->val[destWord] + (u64) prod;
            u128 hisum = (u128) dst->val[destWord - 1] + (prod >> 64);
            if(losum >> 64)
                hisum++;
            dst->val[destWord] = losum;
            dst->val[destWord - 1] = hisum;
            destWord -= 2;
            while(hisum >> 64 && destWord > 0)
            {
                hisum = (u128) dst->val[destWord];
                hisum++;
                dst->val[destWord--] = hisum;
            }
        }
    }
}

void biaddC(BigInt* dst, BigInt* lhs, BigInt* rhs)
{
    //copy lhs value into dst
    bool carry;
    for(int i = lhs->size - 1; i >= 0; i--)
    {
        bool nextCarry = (lhs->val[i] & (1ULL << 63)) && (rhs->val[i] & (1ULL << 63));
        dst->val[i] = lhs->val[i] + rhs->val[i] + (carry ? 1 : 0);
        carry = nextCarry;
    }
}

void bisubC(BigInt* dst, BigInt* lhs, BigInt* rhs)
{
    //copy words of lhs into dst
    memcpy(dst->val, lhs->val, lhs->size * sizeof(u64));
    u64 carry = 1;
    u128 sum;
    for(int i = dst->size - 1; i >= 0; i--)
    {
        sum = (u128) dst->val[i] + ~rhs->val[i] + carry ? 0 : 1;
        dst->val[i] = sum;
        carry = sum >> 64;
    }
}

void bishl(BigInt* op, int bits)
{
    //note: the highest bit of each word is not used
    const int words = op->size;
    const int wordBits = 64;
    int wordShift = bits / wordBits;
    int bitShift = bits % wordBits;
    //first, apply word shift
    for(int i = 0; i < words - wordShift; i++)
        op->val[i] = op->val[i + wordShift];
    for(int i = op->size - wordShift; i < words; i++)
        op->val[i] = 0;
    if(bitShift == 0 || wordShift >= op->size)
        return;
    //shl each word by bitShift
    u64 grabMask = ((1ULL << bitShift) - 1) << (64 - bitShift);
    u64 transfer = 0;
    for(int i = words - 1; i >= 0; i--)
    {
        u64 temp = op->val[i] & grabMask;
        op->val[i] <<= bitShift;
        op->val[i] |= transfer;
        transfer = temp >> (64 - bits);
    }
}

void bishr(BigInt* op, int bits)
{
    /*
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
    */
}

void bishlOne(BigInt* op)
{
    for(int i = 0; i < op->size - 1; i++)
    {
        op->val[i] <<= 1;
        op->val[i] |= (op->val[i + 1] & (1ULL << 62)) >> 62;
    }
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

void bimulC1(BigInt* restrict dst, BigInt* lhs, BigInt* rhs)
{
    u128 prod = (u128) lhs->val[0] * rhs->val[0];
    dst->val[1] = prod;
    dst->val[0] = prod >> 64;
}

void profiler()
{
    int prec = 2;
    u64 trials = 10;
    u64 operations = 3000000;
    BigInt op1;
    op1.size = prec;
    BigInt op2 = op1;
    BigInt dst = op1;
    op1.val = (u64*) alloca(prec * sizeof(u64));
    op2.val = (u64*) alloca(prec * sizeof(u64));
    dst.val = (u64*) alloca(prec * sizeof(u64));
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
