#include "math.h"
#include "stdlib.h"
#include "string.h"
#include "stdbool.h"
#include "stdio.h"
#include "time.h"

#ifndef PRECISION_H
#define PRECISION_H

typedef unsigned char u8;
typedef unsigned long long u64;

#define carryMask (1ULL << 63)
#define digitMask (~carryMask)
#define expoBias (0x7FFFFFFF)  //this value is subtracted from actual exponent

#define max(a, b) (a < b ? b : a);
#define min(a, b) (a < b ? a : b);

void staticPrecInit(int maxPrec);   //allocate math scratch space

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
void bimul(BigInt* dst, BigInt* lhs, BigInt* rhs);  
//biadd returns true if a carry bit overflowed 
//dst, lhs and rhs must have same size for add, sub
bool biadd(BigInt* dst, BigInt* lhs, BigInt* rhs);  //returns true if overflow 
void bisub(BigInt* dst, BigInt* lhs, BigInt* rhs);  //subtract rhs from lhs; result >= 0
void biinc(BigInt* op);                             //increment, no overflow check
void bishr(BigInt* op, int bits);
void bishlOne(BigInt* op);
void bishl(BigInt* op, int bits);
void biTwoComplement(BigInt* op);
void biPrint(BigInt* op);
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

Float FloatCtor(int prec);
Float floatLoad(int prec, long double d);
void FloatDtor(Float* f);
void storeFloatVal(Float* f, long double d);
long double getFloatVal(Float* f);
void floatWriteZero(Float* f);
void fmul(Float* dst, Float* lhs, Float* rhs);
void fadd(Float* dst, Float* lhs, Float* rhs);
void faddip(Float* dst, Float* rhs);
void fsub(Float* dst, Float* lhs, Float* rhs);
void fconvert(Float* dst, Float* src);              
void fcopy(Float* dst, Float* src);
bool fzero(Float* f);                                   //is the float +-0?
int compareFloatMagnitude(Float* lhs, Float* rhs);      //-1, 0, 1 resp. < = > (like strcmp)

void fuzzTest();

#endif
