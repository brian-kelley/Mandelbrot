#include "floatingpoint.h"

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
    storeFloatVal(&f, d);
    return f;
}

void storeFloatVal(Float* f, long double d)
{
    if(d == 0)
    {
        floatWriteZero(f);
        return;
    }
    f->sign = d < 0;
    int expoTemp;
    long double mant = frexpl(d, &expoTemp);
    f->expo = (long long) expoTemp + expoBias;
    u8* mantBytes = (u8*) &mant;
    u8* highWordBytes = (u8*) &f->mantissa.val[0];
    //copy 64 bit mantissa. Byte order is LE in the long double and LE within the u64
    for(int byteCount = 0; byteCount < 8; byteCount++)
        highWordBytes[byteCount] = mantBytes[byteCount];
    //shift the mantissa down 1 bit so the high word contains 63 significant bits
    bishr(&f->mantissa, 1);
    f->mantissa.val[0] |= (1ULL << 62);  //this bit needs to be high to be normalized
    for(int i = 1; i < f->mantissa.size; i++)
        f->mantissa.val[i] = 0;
    if(f->mantissa.size > 1 && (mantBytes[0] & 0xA))
        f->mantissa.val[1] |= (1ULL << 62);
    FLOAT_CHECK(*f);
}

long double getFloatVal(Float* f)
{
    if(fzero(f))
        return 0;
    int realExpo = (long long) f->expo - expoBias;
    long double mant = 0;                   //this makes all bits 0
    u64 highWord;
    u8* mantBytes = (u8*) &mant;
    u8* highWordBytes = (u8*) &highWord;
    //add a biased 0 exponent to mant
    //don't modify f, work from copy in scratch space
    highWord = f->mantissa.val[0];
    highWord <<= 1;
    //lose highest digit for legal float?
    if(f->mantissa.size > 1)
    {
        if(f->mantissa.val[1] & (1ULL << 62))
            highWord++;
    }
    *((short*) &mantBytes[8]) = 16382;
    //copy mantissa 
    for(int byteCount = 0; byteCount < 8; byteCount++)
        mantBytes[byteCount] = highWordBytes[byteCount];
    if(f->sign)
        mant *= -1;
    FLOAT_CHECK(*f);
    return ldexpl(mant, realExpo);
}

void floatWriteZero(Float* f)
{
    f->expo = 0;
    for(int i = 0; i < f->mantissa.size; i++)
        f->mantissa.val[i] = 0;
    FLOAT_CHECK(*f);
}

void fmul(Float* dst, Float* lhs, Float* rhs)
{
    FLOAT_CHECK(*lhs);
    FLOAT_CHECK(*rhs);
    if(fzero(lhs) || fzero(rhs))
    {
        floatWriteZero(dst);
        return;
    }
    //get new exponent (no bias)
    const int words = dst->mantissa.size;
    int newExpo = ((long long) lhs->expo - expoBias) + ((long long) rhs->expo - expoBias);
    BigInt bigDest;
    bigDest.val = (u64*) alloca(2 * words * sizeof(u64));
    bigDest.size = 2 * words;
    bimul(&bigDest, &lhs->mantissa, &rhs->mantissa);
    for(int i = 0; i < 2 * words; i++)
        assert(!(bigDest.val[i] & carryMask));
    //2 cases here: highest bit of product is 0 or 1
    if((bigDest.val[0] & (1ULL << 62)) == 0)
    {
        bishlOne(&bigDest);
        newExpo--;
        bigDest.val[words - 1] |= (bigDest.val[words] & (1ULL << 62)) >> 62;
    }
    memcpy(dst->mantissa.val, bigDest.val, sizeof(u64) * words);
    dst->sign = lhs->sign ^ rhs->sign;
    dst->expo = (long long) newExpo + expoBias;
    FLOAT_CHECK(*dst);
}

void fadd(Float* dst, Float* lhs, Float* rhs)
{
    FLOAT_CHECK(*lhs);
    FLOAT_CHECK(*rhs);
    //compare magnitudes (want lhs to be larger magnitude)
    //want lhs to be larger magnitude
    //simply swap the pointers if rhs is bigger
    int magCmp = compareFloatMagnitude(lhs, rhs);
    if(magCmp == -1)
    {
        Float* temp = lhs;
        lhs = rhs;
        rhs = temp;
    }
    if(fzero(rhs))
    {
        fcopy(dst, lhs);
        return;
    }
    //now |lhs| > |rhs| so can add rhs to lhs directly (after shifting to match expos)
    BigInt rhsAddend;
    rhsAddend.size = rhs->mantissa.size;
    rhsAddend.val = (u64*) alloca(lhs->mantissa.size * sizeof(u64));
    //copy rhs mantissa to scratch and then shift to match exponents
    for(int i = 0; i < rhs->mantissa.size; i++)
        rhsAddend.val[i] = rhs->mantissa.val[i];
    bishr(&rhsAddend, lhs->expo - rhs->expo);
    dst->expo = lhs->expo;
    if(lhs->sign != rhs->sign)
    {
        //subtract shifted rhs from lhs using 2s complement
        bisub(&dst->mantissa, &lhs->mantissa, &rhsAddend);
        int shifts = 0;
        while(!(dst->mantissa.val[0] & (1ULL << 62)))
        {
            bishlOne(&dst->mantissa);
            dst->expo--;
            if(shifts++ > 63 * dst->mantissa.size)
            {
                floatWriteZero(dst);
                return;
            }
        }
    }
    else
    {
        //add rhs to lhs
        bool overflow = biadd(&dst->mantissa, &lhs->mantissa, &rhsAddend);
        if(overflow)
        {
            bishrOne(&dst->mantissa);
            dst->mantissa.val[0] |= (1ULL << 62);
            dst->expo++;
        }
    }
    //dst exponent is same as lhs (will be incremented if add overflowed)
    if((dst->mantissa.val[0] & (1ULL << 62)) == 0)
    {
        bishrOne(&dst->mantissa);
        dst->mantissa.val[0] |= (1ULL << 62);
        dst->expo++;
    }
    //sum will always have the same sign as lhs
    dst->sign = lhs->sign;
    FLOAT_CHECK(*dst);
}

void fsub(Float* dst, Float* lhs, Float* rhs)
{
    FLOAT_CHECK(*lhs);
    FLOAT_CHECK(*rhs);
    bool savedSign = rhs->sign;
    rhs->sign = !rhs->sign;
    fadd(dst, lhs, rhs);
    rhs->sign = savedSign;
    FLOAT_CHECK(*dst);
}

bool fzero(Float* f)
{
    if(f->expo != 0)
        return false;
    for(int i = 0; i < f->mantissa.size; i++)
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
    FLOAT_CHECK(*lhs);
    FLOAT_CHECK(*rhs);
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

void fcopy(Float* dst, Float* src)
{
    int wordsToCopy = min(dst->mantissa.size, src->mantissa.size);
    dst->sign = src->sign;
    dst->expo = src->expo;
    for(int i = 0; i < wordsToCopy; i++)
        dst->mantissa.val[i] = src->mantissa.val[i];
    FLOAT_CHECK(*dst);
    FLOAT_CHECK(*src);
}

void fuzzTest()
{
    const long double tol = 1e-15;
    srand(clock());
    int prec = 1;
    MAKE_STACK_FLOAT(op1);
    MAKE_STACK_FLOAT(op2);
    MAKE_STACK_FLOAT(sum);
    MAKE_STACK_FLOAT(diff);
    MAKE_STACK_FLOAT(prod);
    u64 tested = 0;
    while(true)
    {
        long double op[2];
        for(int i = 0; i < 2; i++)
        {
            int ex = -40 + rand() % 80;
            long double mant = (long double) 1.0 / RAND_MAX * rand();
            if(rand() & 0x10)
                mant *= -1;
            op[i] = ldexpl(mant, ex);
        }
        storeFloatVal(&op1, op[0]);
        storeFloatVal(&op2, op[1]);
        fadd(&sum, &op1, &op2);
        fsub(&diff, &op1, &op2);
        fmul(&prod, &op1, &op2);
        {
            long double actual = op[0] + op[1];
            if(fabsl((getFloatVal(&sum) - actual) / actual) > tol)
            {
                printf("Result of %.20Lf + %.20Lf = %.20Lf was wrong!\n", getFloatVal(&op1), getFloatVal(&op2), getFloatVal(&sum));
                break;
            }
        }
        {
            long double actual = op[0] - op[1];
            if(fabsl((getFloatVal(&diff) - actual) / actual) > tol)
            {
                printf("Result of %.20Lf - %.20Lf = %.20Lf was wrong!\n", getFloatVal(&op1), getFloatVal(&op2), getFloatVal(&diff));
                break;
            }
        }
        {
            long double actual = op[0] * op[1];
            if(fabsl((getFloatVal(&prod) - actual) / actual) > tol)
            {
                printf("Result of %.20Lf * %.20Lf = %.20Lf was wrong!\n", getFloatVal(&op1), getFloatVal(&op2), getFloatVal(&prod));
                break;
            }
        }
        if(tested++ % 1000000 == 999999)
            printf("%llu operand combinations tested.\n", tested);
    }
    exit(EXIT_FAILURE);
}

//Float IO: precision, sign, expo, mantissa words
Float floatRead(FILE* file)
{
    int prec;
    fread(&prec, sizeof(int), 1, file);
    Float f = FloatCtor(prec);
    fread(&f.sign, sizeof(bool), 1, file);
    fread(&f.expo, sizeof(unsigned), 1, file);
    fread(f.mantissa.val, sizeof(u64), prec, file);
    return f;
}

void floatWrite(Float* f, FILE* file)
{
    fwrite(&f->mantissa.size, sizeof(int), 1, file);
    fwrite(&f->sign, sizeof(bool), 1, file);
    fwrite(&f->expo, sizeof(unsigned), 1, file);
    fwrite(f->mantissa.val, sizeof(u64), f->mantissa.size, file);
}

void printFloat(Float* f)
{
    const int words = f->mantissa.size;
    MAKE_STACK_FLOAT_PREC(ten, words);
    storeFloatVal(&ten, 10);
    MAKE_STACK_FLOAT_PREC(mant, words);
    MAKE_STACK_FLOAT_PREC(temp, words);
    mant.expo = expoBias;
    mant.sign = false;
    for(int i = 0; i < words; i++)
    {
        mant.mantissa.val[i] = f->mantissa.val[i];
    }
    if(fzero(f))
    {
        printf("0.0\n");
        return;
    }
    if(f->sign)
    {
        putchar('-');
    }
    //will only work for integers that can fit in F80
    printf("0.");       //the start of every normalized number
    int decDigits = 63 * words * (log(2) / log(10));
    for(int i = 0; i < decDigits; i++)
    {
        bool allZero = true;
        for(int i = 0; i < words; i++)
        {
            if(mant.mantissa.val[i])
            {
                allZero = false;
                break;
            }
        }
        fmul(&temp, &mant, &ten);
        fcopy(&mant, &temp);
        //mant = mant % 10
        while(compareFloatMagnitude(&mant, &ten) == 1)
        {
            fsub(&temp, &mant, &ten);
            fcopy(&mant, &temp);
        }
        printf("%i", (int) getFloatVal(&mant));
    }
    printf(" * 2^%lli\n", ((long long) f->expo) - expoBias);
}

bool isNormal(Float* f)
{
    return fzero(f) || ((f->mantissa.val[0] & (1ULL << 62)) ? true : false);
}
