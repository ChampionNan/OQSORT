#ifndef BITONIC_H
#define BITONIC_H

#include "../shared.h"

void smallBitonicMerge(int64_t *a, int64_t start, int64_t size, int64_t flipped);
void smallBitonicSort(int64_t *a, int64_t start, int64_t size, int64_t flipped);
void bitonicMerge(int structureId, int64_t start, int64_t size, int64_t flipped, int64_t* row1, int64_t* row2);
void bitonicSort(int structureId, int64_t start, int64_t size, int64_t flipped, int64_t* row1, int64_t* row2);
int64_t greatestPowerOfTwoLessThan(int64_t n);

#endif // !BITONIC_H
