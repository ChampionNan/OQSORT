#include "bitonic.h"

// support for use small bitonic sort only
Bitonic::Bitonic(EnclaveServer &eServer, EncOneBlock *a) : eServer{eServer}, a{a} {}

Bitonic::Bitonic(EnclaveServer &eServer, int inputId, int64_t start, int64_t initSize) : eServer{eServer}, inputId{inputId}, start{start}, initSize{initSize} {
  M = eServer.M;
  B = eServer.B;
  row1 = new EncOneBlock[B];
  row2 = new EncOneBlock[B];
}

Bitonic::~Bitonic() {
  delete [] row1;
  delete [] row2;
}

void Bitonic::smallBitonicMerge(EncOneBlock *a, int64_t start, int64_t size, int flipped) {
  if (size == 1) {
    return;
  } else {
    int swap = 0, nswap;
    int64_t mid = eServer.greatestPowerOfTwoLessThan((double)size);
    for (int64_t i = 0; i < size - mid; ++i) {
      num1 = a[start + i];
      num2 = a[start + mid + i];
      swap = eServer.cmpHelper(&num1, &num2);
      swap = swap ^ flipped;
      nswap = !swap;
      a[start + i] = (nswap * num1) + (swap * num2);
      a[start + i + mid] = (swap * num1) + (nswap * num2);
    }
    smallBitonicMerge(a, start, mid, flipped);
    smallBitonicMerge(a, start + mid, size - mid, flipped);
  }
  return;
}

void Bitonic::smallBitonicMerge(int64_t start, int64_t size, int flipped) {
  if (size == 1) {
    return;
  } else {
    // printf("In smallMerge\n");
    int swap = 0, nswap;
    int64_t mid = eServer.greatestPowerOfTwoLessThan((double)size);
    // printf("In smallMerge\n");
    for (int64_t i = 0; i < size - mid; ++i) {
      num1 = a[start + i];
      num2 = a[start + mid + i];
      swap = eServer.cmpHelper(&num1, &num2);
      swap = swap ^ flipped;
      nswap = !swap;
      a[start + i] = (nswap * num1) + (swap * num2);
      a[start + i + mid] = (swap * num1) + (nswap * num2);
    }
    // printf("Before each section\n");
    smallBitonicMerge(start, mid, flipped);
    smallBitonicMerge(start + mid, size - mid, flipped);
  }
  return;
}

//Correct, after testing
void Bitonic::smallBitonicSort(EncOneBlock *a, int64_t start, int64_t size, int flipped) {
  if (size <= 1) {
    return ;
  } else {
    int64_t mid = eServer.greatestPowerOfTwoLessThan((double)size);
    smallBitonicSort(a, start, mid, 1);
    smallBitonicSort(a, start + mid, size - mid, 0);
    smallBitonicMerge(a, start, size, flipped);
  }
  return;
}

void Bitonic::smallBitonicSort(int64_t start, int64_t size, int flipped) {
  if (size <= 1) {
    return ;
  } else {
    int64_t mid = eServer.greatestPowerOfTwoLessThan((double)size);
    smallBitonicSort(start, mid, 1);
    smallBitonicSort(start + mid, size - mid, 0);
    smallBitonicMerge(start, size, flipped);
  }
  return;
}

void Bitonic::bitonicMerge(int64_t start, int64_t size, int flipped) {
  if (size < 1) {
    return ;
  } else if (size < M/B) {
    EncOneBlock *trustedMemory = new EncOneBlock[size * B];
    for (int64_t i = 0; i < size; ++i) {
      eServer.opOneLinearScanBlock((start + i) * B, &trustedMemory[i * B], B, inputId, 0, 0);
    }
    smallBitonicMerge(trustedMemory, 0, size * B, flipped);
    for (int64_t i = 0; i < size; ++i) {
      eServer.opOneLinearScanBlock((start + i) * B, &trustedMemory[i * B], B, inputId, 1, 0);
    }
    delete [] trustedMemory;
  } else {
    int swap = 0, nswap;
    int64_t mid = eServer.greatestPowerOfTwoLessThan((double)size);
    for (int64_t i = 0; i < size - mid; ++i) {
      eServer.opOneLinearScanBlock((start + i) * B, row1, B, inputId, 0, 0);
      eServer.opOneLinearScanBlock((start + mid + i) * B, row2, B, inputId, 0, 0);
      EncOneBlock num1 = row1[0], num2 = row2[0];
      swap = eServer.cmpHelper(&num1, &num2);
      swap = swap ^ flipped;
      nswap = !swap;
      for (int j = 0; j < B; ++j) {
        EncOneBlock v1 = row1[j];
        EncOneBlock v2 = row2[j];
        row1[j] = (nswap * v1) + (swap * v2);
        row2[j] = (swap * v1) + (nswap * v2);
      }
      eServer.opOneLinearScanBlock((start + i) * B, row1, B, inputId, 1, 0);
      eServer.opOneLinearScanBlock((start + mid + i) * B, row2, B, inputId, 1, 0);
    }
    bitonicMerge(start, mid, flipped);
    bitonicMerge(start + mid, size - mid, flipped);
  }
  return;
}

void Bitonic::bitonicSort(int64_t start, int64_t size, int flipped) {
  if (size < 1) {
    return;
  } else if (size < M/B) {
    EncOneBlock *trustedMemory = new EncOneBlock[size * B];
    for (int64_t i = 0; i < size; ++i) {
      eServer.opOneLinearScanBlock((start + i) * B, &trustedMemory[i * B], B, inputId, 0, 0);
    }
    smallBitonicSort(trustedMemory, 0, size * B, flipped);
    // write back
    for (int64_t i = 0; i < size; ++i) {
      eServer.opOneLinearScanBlock((start + i) * B, &trustedMemory[i * B], B, inputId, 1, 0);
    }
    delete [] trustedMemory;
  } else {
    int64_t mid = eServer.greatestPowerOfTwoLessThan((double)size);
    bitonicSort(start, mid, 1);
    bitonicSort(start + mid, size - mid, 0);
    bitonicMerge(start, size, flipped);
  }
  return;
}
