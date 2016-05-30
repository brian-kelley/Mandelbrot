#include "bigint.h"

#ifndef __APPLE__
#include "alloca.h"
#endif

#ifndef FLOATING_H 
#define FLOATING_H

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

/*
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

*/

#define FLOAT_CHECK(f) \
{ \
for(int _i = 0; _i < (f).mantissa.size; _i++) \
    assert(((f).mantissa.val[_i] & carryMask) == 0); \
    assert(isNormal(&(f))); \
}

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
void printFloat(Float* f);
bool isNormal(Float* f);

//Tests
void fuzzTest();        //test all 3 float arithmetic functions with randomized long doubles

#endif
