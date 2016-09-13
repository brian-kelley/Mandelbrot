#include "math.h"
#include "stdlib.h"
#include "string.h"
#include "stdbool.h"
#include "stdio.h"
#include "limits.h"
#include "time.h"
#include "assert.h"

#ifdef __CYGWIN__
#include "alloca.h"
#endif

#ifndef BIGINT_H
#define BIGINT_H

typedef unsigned char u8;
typedef unsigned u32;
typedef unsigned long long u64;
typedef unsigned __int128 u128;

#define expoBias (0x7FFFFFFF)  //this value is subtracted from actual exponent

#define max(a, b) (a < b ? b : a)
#define min(a, b) (a < b ? a : b)

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
