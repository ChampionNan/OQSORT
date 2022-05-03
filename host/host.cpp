#include <openenclave/host.h>
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <vector>
#include "../enclave/definitions.h"

#include "osort_u.h"

int *X;
int *Y;
int *arrayAddr[NUM_STRUCTURES];
int paddedSize;

void OcallReadBlock(int index, int* buffer, size_t blockSize, int structureId) {
  if (blockSize == 0) {
    printf("Unknown data size");
    return;
  }
  memcpy(buffer, &(arrayAddr[structureId][index]), blockSize * sizeof(int));
}

void OcallWriteBlock(int index, int* buffer, size_t blockSize, int structureId) {
  if (blockSize == 0) {
    printf("Unknown data size");
    return;
  }
  memcpy(&(arrayAddr[structureId][index]), buffer, blockSize * sizeof(int));
}


void init() {
  int i;
  for (i = 0; i < paddedSize; i++) {
    X[i] = (paddedSize - i);
  }
}

void print() {
  int i;
  for (i = 0; i < paddedSize; i++) {
    printf("%d ", X[i]);
  }
  printf("\n");
}

void test() {
  int pass = 1;
  int i;
  for (i = 1; i < paddedSize; i++) {
    pass &= (X[i-1] <= X[i]);
  }
  printf(" TEST %s\n",(pass) ? "PASSed" : "FAILed");
  print();
}


int main(int argc, const char* argv[]) {
  oe_result_t result;
  oe_result_t method_return;
  int ret = 1;
  oe_enclave_t* enclave = NULL;

  int addi = 0;
  if (N % BLOCK_DATA_SIZE != 0) {
    addi = ((N / BLOCK_DATA_SIZE) + 1) * BLOCK_DATA_SIZE - N;
  }
  X = (int*)malloc((N + addi) * sizeof(int));
  paddedSize = N + addi;
  arrayAddr[0] = X;

  // Create the enclave
  result = oe_create_osort_enclave(
    argv[1], OE_ENCLAVE_TYPE_SGX, 0, NULL, 0, &enclave);
  if (result != OE_OK) {
    fprintf(stderr,
            "oe_create_hello_enclave(): result=%u (%s)\n",
            result,
            oe_result_str(result));
    goto exit;
  }
  // Small bitonic sort Passed
  init();
  result = callSmallSort(enclave, X, paddedSize);
  test();
  if (result != OE_OK) {
    fprintf(stderr,
            "Calling into enclave_hello failed: result=%u (%s)\n",
            result,
            oe_result_str(result));
    ret = -1;
  } else {
    if (method_return != OE_OK) {
      ret = -1;
    }
  }

  // Bitonic sort Passed
  init();
  print();
  result = callSort(enclave, 3, 0, paddedSize);
  test();
  if (result != OE_OK) {
    fprintf(stderr,
            "Calling into enclave_hello failed: result=%u (%s)\n",
            result,
            oe_result_str(result));
    ret = -1;
  } else {
    if (method_return != OE_OK) {
      ret = -1;
    }
  } 

  exit:
    if (enclave) {
      oe_terminate_enclave(enclave);
    }
    return ret;
}
