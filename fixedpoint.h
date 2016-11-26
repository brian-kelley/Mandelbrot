#include "bigint.h"

#ifndef FIXEDPOINT_H
#define FIXEDPOINT_H

typedef void (*Multiply)(BigInt* res, BigInt* lhs, BigInt* rhs);
extern int fpPrec;

typedef struct
{
  BigInt value;
  bool sign;
} FP;

#define MAKE_STACK_FP(name) \
  FP name; \
  name.value.size = prec; \
  name.value.val = (u64*) alloca(prec * sizeof(u64)); \
  name.sign = false;

#define MAKE_STACK_FP_PREC(name, customPrec) \
  FP name; \
  name.value.size = customPrec; \
  name.value.val = (u64*) alloca(customPrec * sizeof(u64)); \
  name.sign = false;

#define INCR_PREC(f) \
  f.value.size++; \
  f.value.val = (u64*) realloc(f.value.val, (f.value.size) * sizeof(u64)); \
  f.value.val[f.value.size - 1] = 0;

#define DECR_PREC(f) \
  f.value.size--; \
  f.value.val = (u64*) realloc(f.value.val, (f.value.size) * sizeof(u64));

#define CHANGE_PREC(f, newPrec) \
{ \
  int oldPrec = f.value.size; \
  f.value.size = newPrec; \
  f.value.val = (u64*) realloc(f.value.val, newPrec * sizeof(u64)); \
  for(int _i = oldPrec; _i < newPrec; _i++) \
    f.value.val[_i] = 0; \
}

void globalUpdatePrec(int n);

MANDELBROT_API FP FPCtor(int prec);
MANDELBROT_API FP FPCtorValue(int prec, long double val);
MANDELBROT_API void FPDtor(FP* fp);

//Note: add/mul return true on overflow
//Use overflow signal to detect iteration stop condition

//Binary versions (like asm instructions, result stored in first, rhs not modified)
//lhs and rhs can be the same
MANDELBROT_API void fpadd2(FP* lhs, FP* rhs);
MANDELBROT_API void fpsub2(FP* lhs, FP* rhs);
MANDELBROT_API void fpmul2(FP* lhs, FP* rhs);

//Ternary versions (result stored in dst, lhs/rhs never modified)
MANDELBROT_API void fpadd3(FP* dst, FP* lhs, FP* rhs);
MANDELBROT_API void fpsub3(FP* dst, FP* lhs, FP* rhs);
MANDELBROT_API void fpmul3(FP* dst, FP* lhs, FP* rhs);

#define fpshl(fp, num) {bishl(&(fp).value, num);}
#define fpshlOne(fp) {bishlOne(&(fp).value);}
#define fpshr(fp, num) {bishr(&(fp).value, num);}
#define fpshrOne(fp) {bishrOne(&(fp).value);}

MANDELBROT_API int fpCompareMag(FP* lhs, FP* rhs);     //-1: lhs < rhs, 0: lhs == rhs, 1: lhs > rhs
MANDELBROT_API void loadValue(FP* fp, long double val);
MANDELBROT_API long double getValue(FP* fp);
MANDELBROT_API void fpcopy(FP* lhs, FP* rhs);
MANDELBROT_API int getApproxExpo(FP* lhs);

MANDELBROT_API void fpWrite(FP* value, FILE* handle);
MANDELBROT_API FP fpRead(FILE* handle);         //heap allocates space for value

MANDELBROT_API bool fpValidate(FP* val);

#endif

