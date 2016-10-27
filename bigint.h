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

MANDELBROT_API BigInt BigIntCtor(int size);
MANDELBROT_API BigInt BigIntCopy(BigInt* bi);
MANDELBROT_API void BigIntDtor(BigInt* bi);
//dst must have exactly twice the width of lhs and rhs
MANDELBROT_API void bimul(BigInt* dst, BigInt* lhs, BigInt* rhs);     // generic (asm)

MANDELBROT_API void bimulC(BigInt* dst, BigInt* lhs, BigInt* rhs);  
//biadd returns true if a carry bit overflowed 
//dst, lhs and rhs must have same size for add, sub

MANDELBROT_API void biadd(BigInt* dst, BigInt* lhs, BigInt* rhs);  //returns 0 iff no overflow 
MANDELBROT_API void bisub(BigInt* dst, BigInt* lhs, BigInt* rhs);  //subtract rhs from lhs; result >= 0
MANDELBROT_API void biinc(BigInt* op);
MANDELBROT_API void bishlOne(BigInt* op);
MANDELBROT_API void bishl(BigInt* op, int bits);
MANDELBROT_API void bishr(BigInt* op, int bits);
MANDELBROT_API void bishrOne(BigInt* op);
MANDELBROT_API void biTwoComplement(BigInt* op);
MANDELBROT_API void biPrint(BigInt* op);
MANDELBROT_API void biPrintBin(BigInt* op);
MANDELBROT_API bool biNthBit(BigInt* op, int n);
MANDELBROT_API int lzcnt(BigInt* op);

void profiler();

#endif
