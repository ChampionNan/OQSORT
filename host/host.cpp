#include <iostream>
#include <chrono>
#include <cmath>
#include <openenclave/host.h>

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


/* OCall functions */
void ocall_print_string(const char *str) {
  /* Proxy/Bridge will check the length and null-terminate
   * the input string to prevent buffer overflow.
   */
  printf("%s", str);
  fflush(stdout);
}

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

/* main function */
int main(int argc, const char* argv[]) {
  int ret = 1;
  int *resId = (int*)malloc(sizeof(int));
  oe_result_t result;
  oe_enclave_t* enclave = NULL;
  std::chrono::high_resolution_clock::time_point start, end;
  std::chrono::seconds duration;
  // 0: OSORT, 1: bucketOSort, 2: smallBSort, 3: bitonicSort, 
  int sortId = 1;

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
  start = std::chrono::high_resolution_clock::now();
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
  end = std::chrono::high_resolution_clock::now();
  if (result != OE_OK) {
    fprintf(stderr,
            "Calling into enclave_hello failed: result=%u (%s)\n",
            result,
            oe_result_str(result));
    ret = -1;
  }

  // step4: std::cout execution time
  duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  std::cout << "Time taken by sorting function: " << duration.count() << " seconds" << std::endl;

  // step5: exix part
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
