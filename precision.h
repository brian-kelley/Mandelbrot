#include "math.h"
#include "stdlib.h"
#include "string.h"
#include "stdbool.h"
#include "stdio.h"
#include "time.h"
#include "assert.h"

#ifndef __APPLE__
#include "alloca.h"
#endif //apple

#ifndef PRECISION_H
#define PRECISION_H

typedef unsigned char u8;
typedef unsigned long long u64;

#define carryMask (1ULL << 63)
#define digitMask ((1ULL << 63) - 1)
#define expoBias (0x7FFFFFFF)  //this value is subtracted from actual exponent

#define max(a, b) (a < b ? b : a)
#define min(a, b) (a < b ? a : b)

//routine implemented in asm
extern void longmul(u64 f1, u64 f2, u64* phi, u64* plo);

typedef struct
{
    u64* val;
    int size;
} BigInt;

BigInt BigIntCtor(int size);
BigInt BigIntCopy(BigInt* bi);
void BigIntDtor(BigInt* bi);
//dst must have exactly twice the width of lhs and rhs
void bimul(BigInt* restrict dst, BigInt* lhs, BigInt* rhs);  
//biadd returns true if a carry bit overflowed 
//dst, lhs and rhs must have same size for add, sub
u64 biadd(BigInt* restrict dst, BigInt* lhs, BigInt* rhs);  //returns 0 iff no overflow 
void bisub(BigInt* restrict dst, BigInt* lhs, BigInt* rhs);  //subtract rhs from lhs; result >= 0
void biinc(BigInt* op);                            //increment, no overflow check
void bishlOne(BigInt* op);
void bishl(BigInt* op, int bits);
void bishr(BigInt* op, int bits);
void bishrOne(BigInt* op);
void biTwoComplement(BigInt* op);
void biPrint(BigInt* op);
void biPrintBin(BigInt* op);
u64 biNthBit(BigInt* op, int n);

/*  Simple arbitrary precision floating point value
 *  Multiply, add, subtract
 *  Very similar to the IEEE 80-bit format, with BigInt for mantissa representation
 *  No support for math with subnormals or NaN
 *  All operands and destinations must have the same precision (fconvert can change precision)
 */

typedef struct
{
    BigInt mantissa;
    unsigned expo;
    bool sign;          //false = positive
} Float;

#define MAKE_STACK_FLOAT(name) \
    Float name; \
    name.mantissa.val = (u64*) alloca(prec * sizeof(u64)); \
    name.mantissa.size = prec;

#define MAKE_STACK_FLOAT_PREC(name, prec) \
    Float name; \
    name.mantissa.val = (u64*) alloca(prec * sizeof(u64)); \
    name.mantissa.size = prec;

#define MAKE_FLOAT(name, buf) \
    Float name; \
    name.mantissa.val = (u64*) buf; \
    name.mantissa.size = prec;

#define INCR_PREC(f) \
    f.mantissa.size++; \
    f.mantissa.val = (u64*) realloc(f.mantissa.val, (f.mantissa.size) * sizeof(u64));\
    f.mantissa.val[f.mantissa.size - 1] = 0;

#define CHANGE_PREC(f, newPrec) \
{ \
    int oldPrec = f.mantissa.size; \
    f.mantissa.size = newPrec; \
    f.mantissa.val = (u64*) realloc(f.mantissa.val, newPrec * sizeof(u64)); \
    for(int _i = oldPrec; _i < newPrec; _i++) \
        f.mantissa.val[_i] = 0; \
}

#define FLOAT_CHECK(f) \
for(int _i = 0; _i < f->mantissa.size; _i++) \
    assert((f->mantissa.val[_i] & carryMask) == 0);

Float FloatCtor(int prec);
Float floatLoad(int prec, long double d);
void FloatDtor(Float* f);
void storeFloatVal(Float* f, long double d);
long double getFloatVal(Float* f);
void floatWriteZero(Float* f);
void fmul(Float* restrict dst, Float* lhs, Float* rhs);
void fadd(Float* restrict dst, Float* lhs, Float* rhs);
void fsub(Float* restrict dst, Float* lhs, Float* rhs);
void fconvert(Float* restrict dst, Float* src);              
void fcopy(Float* restrict dst, Float* src);
bool fzero(Float* f);                                   //is the float +-0?
int compareFloatMagnitude(Float* lhs, Float* rhs);      //-1, 0, 1 resp. < = > (like strcmp)
Float floatRead(FILE* file);
void floatWrite(Float* f, FILE* file);

//Tests
void fuzzTest();        //test all 3 float arithmetic functions with randomized long doubles

#endif
