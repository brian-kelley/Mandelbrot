#include "math.h"
#include "stdlib.h"
#include "string.h"
#include "stdbool.h"
#include "stdio.h"

#ifndef PRECISION_H
#define PRECISION_H

typedef unsigned long long u64;

#define carryMask (1ULL << 63)
#define digitMask (~carryMask)
#define expoBias (0x7FFFFFFF)  //this value is subtracted from actual exponent

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
//dst must have twice the width of lhs and rhs
void bimul(BigInt* dst, BigInt* lhs, BigInt* rhs);  
//biadd returns true if a carry bit overflowed 
//dst, lhs and rhs must have same size for add, sub
bool biadd(BigInt* dst, BigInt* lhs, BigInt* rhs); 
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
 *  Very similar to the IEEE 80-bit format, with extended mantissa
 *  No support for math with subnormals
 */
typedef struct
{
    BigInt mantissa;
    int expo;
    bool sign;          //false = positive
} Float;

Float FloatCtor(int prec);
Float floatLoad(int prec, long double d);
void FloatDtor(Float* f);
void floatWriteZero(Float* f);
void fmul(Float* dst, Float* lhs, Float* rhs);
void fadd(Float* dst, Float* lhs, Float* rhs);
void fsub(Float* dst, Float* lhs, Float* rhs);
bool fzero(Float* f);                               //is the float +-0?
int fcmp(Float* lhs, Float* rhs);   //-1, 0, 1 resp. < = > (like strcmp)

#endif
