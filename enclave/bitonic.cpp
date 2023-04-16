#include "bitonic.h"

// support for use small bitonic sort only
Bitonic::Bitonic(EnclaveServer &eServer) : eServer{eServer} , row1{nullptr}, row2{nullptr} {}

Bitonic::Bitonic(EnclaveServer &eServer, int inputId, int64_t start, int64_t initSize) : eServer{eServer}, inputId{inputId}, start{start}, initSize{initSize} {
  M = eServer.M;
  B = eServer.B;
  row1 = new EncOneBlock[B];
  row2 = new EncOneBlock[B];
}

Bitonic::Bitonic(EnclaveServer &eServer, EncOneBlock *a, int64_t start, int64_t initSize) : eServer{eServer}, a{a}, start{start}, initSize{initSize}, row1{nullptr}, row2{nullptr}{}

Bitonic::~Bitonic() {
  if (row1) delete [] row1;
  if (row1) delete [] row2;
}

void Bitonic::smallBitonicMerge(EncOneBlock *a, int64_t start, int64_t size, bool flipped, bool isDirect) {
  if (size > 1) {
    int64_t mid = eServer.greatestPowerOfTwoLessThan(size);
    for (int64_t i = 0; i < size - mid; ++i) {
      num1 = a[start + i];
      num2 = a[start + mid + i];
      if (isDirect) {
        eServer.gcm_decrypt(&num1, eServer.encOneBlockSize);
        eServer.gcm_decrypt(&num2, eServer.encOneBlockSize);
      }
      swap = eServer.cmpHelper(&num1, &num2);
      swap = swap ^ flipped;
      if (isDirect) {
        eServer.gcm_encrypt(&num1, eServer.encOneBlockSize);
        eServer.gcm_encrypt(&num2, eServer.encOneBlockSize);
      }
      eServer.oswap128((uint128_t*)&num1, (uint128_t*)&num2, swap);
      a[start + i] = num1;
      a[start + mid + i] = num2;
    }
    smallBitonicMerge(a, start, mid, flipped, isDirect);
    smallBitonicMerge(a, start + mid, size - mid, flipped, isDirect);
  }
  return;
}

//Correct, after testing
void Bitonic::smallBitonicSort(EncOneBlock *a, int64_t start, int64_t size, bool flipped, bool isDirect) {
  if (size > 1) {
    int64_t mid = eServer.greatestPowerOfTwoLessThan(size);
    smallBitonicSort(a, start, mid, 1, isDirect);
    smallBitonicSort(a, start + mid, size - mid, 0, isDirect);
    smallBitonicMerge(a, start, size, flipped, isDirect);
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
    int64_t mid = eServer.greatestPowerOfTwoLessThan(size);
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
    int64_t mid = eServer.greatestPowerOfTwoLessThan(size);
    bitonicSort(start, mid, 1);
    bitonicSort(start + mid, size - mid, 0);
    bitonicMerge(start, size, flipped);
  }
  return;
}
