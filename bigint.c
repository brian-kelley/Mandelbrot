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

void bimul(BigInt* dst, BigInt* lhs, BigInt* rhs)
{
    //zero out dst
    int words = lhs->size;
    for(int i = 0; i < 2 * words; i++)
        dst->val[i] = 0;
    //first, compute the low half of the full result
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

void biadd(BigInt* dst, BigInt* lhs, BigInt* rhs)
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

void bisub(BigInt* dst, BigInt* lhs, BigInt* rhs)
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
    //note: the highest bit of each word is not used
    const int words = op->size;
    const int wordBits = 64;
    int wordShift = bits / wordBits;
    int bitShift = bits % wordBits;
    //first, apply word shift
    for(int i = 0; i < words - wordShift; i++)
        op->val[i + wordShift] = op->val[i];
    for(int i = 0; i < wordShift; i++)
        op->val[i] = 0;
    if(bitShift == 0 || wordShift >= op->size)
        return;
    //shr each word by bitShift
    u64 grabMask = ((1ULL << bitShift) - 1);
    u64 transfer = 0;
    for(int i = 0; i < words; i++)
    {
        u64 temp = op->val[i] & grabMask;
        op->val[i] >>= bitShift;
        op->val[i] |= transfer;
        transfer = temp << (64 - bits);
    }
}

void bishlOne(BigInt* op)
{
    u64 transfer = 0;
    for(int i = op->size - 1; i >= 0; i--)
    {
        u64 newTransfer = (op->val[i] & (1ULL << 63)) >> 63;
        op->val[i] <<= 1;
        op->val[i] |= transfer;
        transfer = newTransfer;
    }
}

void bishrOne(BigInt* op)
{
    u64 transfer = 0;
    for(int i = 0; i < op->size; i++)
    {
        u64 newTransfer = (op->val[i] & 1) << 63;
        op->val[i] >>= 1;
        op->val[i] |= transfer;
        transfer = newTransfer;
    }
}

void biinc(BigInt* op)
{
  //todo
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
        u64 mask = (1ULL << 63);
        for(int j = 0; j < 64; j++)
        {
            int bit = op->val[i] & mask ? 1 : 0;
            printf("%i", bit);
            mask >>= 1;
        }
    }
    puts("");
}

bool biNthBit(BigInt* op, int n)
{
    int words = n / 64;
    int bits = n % 64;
    if(words < 0 || words >= op->size)
        return false;
    return (op->val[words] & (1ULL << (63 - bits))) ? true : false;
}

int lzcnt(BigInt* op)
{
  int i;
  for(i = 0; i < op->size; i++)
  {
    if(op->val[i])
      break;
  }
  if(i == op->size)
    return i * 64;
  int rv = i * 64;
  u64 mask = (1ULL << 63);
  while(mask)
  {
    if((op->val[i] & mask) == 0)
      rv++;
    else
      break;
    mask >>= 1;
  }
  return rv;
}

void profiler()
{
    int prec = 10;
    u64 trials = 2;
    u64 operations = 300000;
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
        printf("%10s ran %e times per sec.\n", #func, perSec); \
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
        printf("%10s ran %e times per sec.\n", #func, perSec); \
    }
    profile(bimul);
    profile(biadd);
    profile(bisub);
    profileUnary(biinc);
}
