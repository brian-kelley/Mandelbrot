#ifndef BITSET_H
#define BITSET_H

#include "constants.h"

typedef struct
{
  u64* buf;
  int words;
  int bits;
} Bitset;

MANDELBROT_API Bitset BitsetCtor(int n);
MANDELBROT_API void BitsetDtor(Bitset* b);
MANDELBROT_API int getBit(Bitset* b, int index);
MANDELBROT_API void setBit(Bitset* b, int index, int value);
MANDELBROT_API void clearBitset(Bitset* b);

#endif

