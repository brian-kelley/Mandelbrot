#include "bitset.h"

Bitset BitsetCtor(int n)
{
  Bitset b;
  b.words = (n + 63) / 64;
  b.buf = malloc(b.words * sizeof(u64));
  b.bits = n;
  clearBitset(&b);
  return b;
}

void BitsetDtor(Bitset* b)
{
  free(b->buf);
}

int getBit(Bitset* b, int index)
{
  assert(index >= 0 && index < b->bits);
  u64 mask = 1ULL << (index % 64);
  return (b->buf[index / 64] & mask) ? 1 : 0;
}

void setBit(Bitset* b, int index, int value)
{
  assert(index >= 0 && index < b->bits);
  u64 mask = 1ULL << (index % 64);
  int word = index / 64;
  b->buf[word] &= ~mask;
  b->buf[word] |= value ? mask : 0;
}

void clearBitset(Bitset* b)
{
  memset(b->buf, 0, b->words * sizeof(u64));
}

int bitsetPopcnt(Bitset* b)
{
  //could make faster but probably not necessary
  int pop = 0;
  for(int i = 0; i < b->bits; i++)
  {
    pop += getBit(b, i);
  }
  return pop;
}
