#include "bitonic.h"

void smallBitonicMerge(int64_t *a, int64_t start, int64_t size, int64_t flipped) {
  if (size == 1) {
    return;
  } else {
    int64_t swap = 0;
    int64_t mid = greatestPowerOfTwoLessThan((double)size);
    for (int64_t i = 0; i < size - mid; ++i) {
      int64_t num1 = a[start + i];
      int64_t num2 = a[start + mid + i];
      swap = num1 > num2;
      swap = swap ^ flipped;
      a[start + i] = (!swap * num1) + (swap * num2);
      a[start + i + mid] = (swap * num1) + (!swap * num2);
    }
    smallBitonicMerge(a, start, mid, flipped);
    smallBitonicMerge(a, start + mid, size - mid, flipped);
  }
  return;
}

//Correct, after testing
void smallBitonicSort(int64_t *a, int64_t start, int64_t size, int64_t flipped) {
  if (size <= 1) {
    return ;
  } else {
    int64_t mid = greatestPowerOfTwoLessThan((double)size);
    smallBitonicSort(a, start, mid, 1);
    smallBitonicSort(a, start + mid, size - mid, 0);
    smallBitonicMerge(a, start, size, flipped);
  }
  return;
}

void bitonicMerge(int structureId, int64_t start, int64_t size, int64_t flipped, int64_t* row1, int64_t* row2) {
  if (size < 1) {
    return ;
  } else if (size < MEM_IN_ENCLAVE) {
    int64_t *trustedMemory = (int64_t*)oe_malloc(size * BLOCK_DATA_SIZE * sizeof(int64_t));
    for (int64_t i = 0; i < size; ++i) {
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, &trustedMemory[i * BLOCK_DATA_SIZE], BLOCK_DATA_SIZE, structureId, 0, 0);
    }
    smallBitonicMerge(trustedMemory, 0, size * BLOCK_DATA_SIZE, flipped);
    for (int64_t i = 0; i < size; ++i) {
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, &trustedMemory[i * BLOCK_DATA_SIZE], BLOCK_DATA_SIZE, structureId, 1, 0);
    }
    oe_free(trustedMemory);
  } else {
    int64_t swap = 0;
    int64_t mid = greatestPowerOfTwoLessThan((double)size);
    for (int64_t i = 0; i < size - mid; ++i) {
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, row1, BLOCK_DATA_SIZE, structureId, 0, 0);
      opOneLinearScanBlock((start + mid + i) * BLOCK_DATA_SIZE, row2, BLOCK_DATA_SIZE, structureId, 0, 0);
      int64_t num1 = row1[0], num2 = row2[0];
      swap = num1 > num2;
      swap = swap ^ flipped;
      for (int64_t j = 0; j < BLOCK_DATA_SIZE; ++j) {
        int64_t v1 = row1[j];
        int64_t v2 = row2[j];
        row1[j] = (!swap * v1) + (swap * v2);
        row2[j] = (swap * v1) + (!swap * v2);
      }
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, row1, BLOCK_DATA_SIZE, structureId, 1, 0);
      opOneLinearScanBlock((start + mid + i) * BLOCK_DATA_SIZE, row2, BLOCK_DATA_SIZE, structureId, 1, 0);
    }
    bitonicMerge(structureId, start, mid, flipped, row1, row2);
    bitonicMerge(structureId, start + mid, size - mid, flipped, row1, row2);
  }
  return;
}

void bitonicSort(int structureId, int64_t start, int64_t size, int64_t flipped, int64_t* row1, int64_t* row2) {
  if (size < 1) {
    return;
  } else if (size < MEM_IN_ENCLAVE) {
    int64_t *trustedMemory = (int64_t*)oe_malloc(size * BLOCK_DATA_SIZE * sizeof(int64_t));
    for (int64_t i = 0; i < size; ++i) {
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, &trustedMemory[i * BLOCK_DATA_SIZE], BLOCK_DATA_SIZE, structureId, 0, 0);
    }
    smallBitonicSort(trustedMemory, 0, size * BLOCK_DATA_SIZE, flipped);
    // write back
    for (int64_t i = 0; i < size; ++i) {
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, &trustedMemory[i * BLOCK_DATA_SIZE], BLOCK_DATA_SIZE, structureId, 1, 0);
    }
    oe_free(trustedMemory);
  } else {
    int64_t mid = greatestPowerOfTwoLessThan((double)size);
    bitonicSort(structureId, start, mid, 1, row1, row2);
    bitonicSort(structureId, start + mid, size - mid, 0, row1, row2);
    bitonicMerge(structureId, start, size, flipped, row1, row2);
  }
  return;
}