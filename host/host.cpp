#include <openenclave/host.h>
// #include <cstdio>
#include <ctime>
#include <iostream>
#include <vector>
#include <math.h>

#include "../include/IOTools.h"
#include "../include/definitions.h"
#include "../include/common.h"

#include "oqsort_u.h"

int *X;
//structureId=1, bucket1 in bucket sort; input
Bucket_x *bucketx1;
//structureId=2, bucket 2 in bucket sort
Bucket_x *bucketx2;
//structureId=3, write back array
int *Y;
int *arrayAddr[NUM_STRUCTURES];
int paddedSize;
// int structureSize[NUM_STRUCTURES] = {sizeof(int), sizeof(Bucket_x), sizeof(Bucket_x)};


void OcallReadBlock(int index, int* buffer, size_t blockSize, int structureId) {
  if (blockSize == 0) {
    // printf("Unknown data size");
    return;
  }
  // memcpy(buffer, arrayAddr[structureId] + index, blockSize * structureSize[structureId]);
  memcpy(buffer, arrayAddr[structureId] + index, blockSize);
}

void OcallWriteBlock(int index, int* buffer, size_t blockSize, int structureId) {
  if (blockSize == 0) {
    // printf("Unknown data size");
    return;
  }
  // memcpy(arrayAddr[structureId] + index, buffer, blockSize * structureSize[structureId]);
  memcpy(arrayAddr[structureId] + index, buffer, blockSize);
}


int main(int argc, const char* argv[]) {
  oe_result_t result;
  oe_result_t method_return;
  int ret = 1;
  int *resId = (int*)malloc(sizeof(int));
  oe_enclave_t* enclave = NULL;
  // 0: OSORT, 1: bucketOSort, 2: smallBSort, 3: bitonicSort, 
  int sortId = 1;
  // freopen("out2.txt", "w", stdout); 

  // step1: init test numbers
  if (sortId == 2 || sortId == 3) {
    int addi = 0;
    if (N % BLOCK_DATA_SIZE != 0) {
      addi = ((N / BLOCK_DATA_SIZE) + 1) * BLOCK_DATA_SIZE - N;
    }
    X = (int*)malloc((N + addi) * sizeof(int));
    paddedSize = N + addi;
    arrayAddr[0] = X;
  } else if (sortId == 1) {
    // srand((unsigned)time(NULL));
    int bucketNum = smallestPowerOfTwoLargerThan(ceil(2.0 * N / BUCKET_SIZE));
    int bucketSize = bucketNum * BUCKET_SIZE;
    std::cout << "TOTAL BUCKET SIZE: " << bucketSize << std::endl;
    bucketx1 = (Bucket_x*)malloc(bucketSize * sizeof(Bucket_x));
    bucketx2 = (Bucket_x*)malloc(bucketSize * sizeof(Bucket_x));
    memset(bucketx1, 0, bucketSize*sizeof(Bucket_x));
    memset(bucketx2, 0, bucketSize*sizeof(Bucket_x));
    arrayAddr[1] = (int*)bucketx1;
    arrayAddr[2] = (int*)bucketx2;
    X = (int *) malloc(N * sizeof(int));
    arrayAddr[0] = X;
    paddedSize = N;
  } else {
    // TODO: 
  }
  init(arrayAddr, 0, paddedSize);

  // step2: Create the enclave
  // result = oe_create_oqsort_enclave(argv[1], OE_ENCLAVE_TYPE_SGX, OE_ENCLAVE_FLAG_DEBUG, NULL, 0, &enclave);
  result = oe_create_oqsort_enclave(argv[1], OE_ENCLAVE_TYPE_SGX, 0, NULL, 0, &enclave);
  if (result != OE_OK) {
    fprintf(stderr,
            "oe_create_oqsort_enclave(): result=%u (%s)\n",
            result,
            oe_result_str(result));
    goto exit;
  }
  
  // step3: call sort algorithms
  if (sortId == 2 || sortId == 3) {
    std::cout << "Test bitonic sort: " << std::endl;
    result = callSort(enclave, sortId, 0, paddedSize, resId);
    test(arrayAddr, 0, paddedSize);
  } else if (sortId == 1) {
    std::cout << "Test bucket oblivious sort: " << std::endl;
    result = callSort(enclave, sortId, 1, paddedSize, resId);
    std::cout << "Result ID: " << *resId << std::endl;
    // print(arrayAddr, *resId, paddedSize);
    test(arrayAddr, *resId, paddedSize);
  } else {
    // TODO: 
  }
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
  // step4: exix part
  exit:
    if (enclave) {
      oe_terminate_enclave(enclave);
    }
    for (int i = 0; i < NUM_STRUCTURES; ++i) {
      if (arrayAddr[i]) {
        free(arrayAddr[i]);
      }
    }
    return ret;
}
