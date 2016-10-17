#ifndef BIGINT_H
#define BIGINT_H

#include "math.h"
#include "string.h"
#include "stdbool.h"
#include "limits.h"
#include "time.h"
#include "assert.h"
#include "constants.h"

#ifdef __CYGWIN__
#include "alloca.h"
#endif

#define expoBias (0x7FFFFFFF)  //this value is subtracted from actual exponent

typedef struct
{
    u64* val;
    int size;
} BigInt;

BigInt BigIntCtor(int size);
BigInt BigIntCopy(BigInt* bi);
void BigIntDtor(BigInt* bi);
//dst must have exactly twice the width of lhs and rhs
void bimul(BigInt* restrict dst, BigInt* lhs, BigInt* rhs);     // generic (asm)

void bimulC(BigInt* restrict dst, BigInt* lhs, BigInt* rhs);  
//biadd returns true if a carry bit overflowed 
//dst, lhs and rhs must have same size for add, sub

void biadd(BigInt* restrict dst, BigInt* lhs, BigInt* rhs);  //returns 0 iff no overflow 
void bisub(BigInt* restrict dst, BigInt* lhs, BigInt* rhs);  //subtract rhs from lhs; result >= 0
void biinc(BigInt* op);
void bishlOne(BigInt* op);
void bishl(BigInt* op, int bits);
void bishr(BigInt* op, int bits);
void bishrOne(BigInt* op);
void biTwoComplement(BigInt* op);
void biPrint(BigInt* op);
void biPrintBin(BigInt* op);
bool biNthBit(BigInt* op, int n);
int lzcnt(BigInt* op);

void profiler();

#endif
