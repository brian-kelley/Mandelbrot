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
            hi <<= 1;
            hi |= ((lo & (1ULL << 63)) >> 63);
            lo &= digitMask;
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
    //copy words of rhs into dst
    for(int i = 0; i < dst->size; i++)
        dst->val[i] = rhs->val[i];
    BigInt addend;
    addend.size = rhs->size;
    addend.val = (u64*) alloca(addend.size * sizeof(u64));
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
    {
        op->val[i] = ~op->val[i];
        op->val[i] &= digitMask;
    }
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

