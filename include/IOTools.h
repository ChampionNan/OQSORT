#ifndef IOTOOLS_H
#define IOTOOLS_H

#include <cstdint>

int64_t greatestPowerOfTwoLessThan(double n);
int64_t smallestPowerOfKLargerThan(int64_t n, int64_t k);
void swapRow(int64_t *a, int64_t *b);
void init(int64_t **arrayAddr, int structureId, int64_t size);
void print(int64_t **arrayAddr, int structureId, int64_t size);
void test(int64_t **arrayAddr, int structureId, int64_t size);
void testWithDummy(int64_t **arrayAddr, int structureId, int64_t size);

#endif // !IOTOOLS_H
