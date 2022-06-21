#include <openenclave/host.h>
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <vector>
#include <math.h>

#include "../enclave/include/definitions.h"
#include "../enclave/include/common.h"

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

void OcallReadBlock(int index, int* buffer, size_t blockSize, int structureId) {
  if (blockSize == 0) {
    // printf("Unknown data size");
    return;
  }
  // memcpy(buffer, arrayAddr[structureId] + index, blockSize * structureSize[structureId]);
  memcpy(buffer, arrayAddr[structureId] + index, blockSize * structureSize[structureId]);
}

void OcallWriteBlock(int index, int* buffer, size_t blockSize, int structureId) {
  if (blockSize == 0) {
    // printf("Unknown data size");
    return;
  }
  // memcpy(arrayAddr[structureId] + index, buffer, blockSize * structureSize[structureId]);
  memcpy(arrayAddr[structureId] + index, buffer, blockSize);
  /*
  std::cout<<"=====WriteB1=====\n";
  std::cout<<blockSize * structureSize[structureId]<<std::endl;
  std::cout<<buffer[0]<<" "<<buffer[1]<<std::endl;
  std::cout<<"=====WriteB1=====\n";*/
}

int smallestPowerOfTwoLargerThan(int n) {
  int k = 1;
  while (k > 0 && k < n) {
    k = k << 1;
  }
  return k;
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

void print(int structureId) {
  int i;
  for (i = 0; i < paddedSize; i++) {
    if(structureSize[structureId] == 4) {
      // int
      int *addr = (int*)arrayAddr[structureId];
      printf("%d ", addr[i]);
    } else if (structureSize[structureId] == 8) {
      //Bucket
      Bucket_x *addr = (Bucket_x*)arrayAddr[structureId];
      printf("(%d, %d), ", addr[i].x, addr[i].key);
    }
  }
  printf("\n");
}

void print(int structureId, int size) {
  int i;
  for (i = 0; i < size; i++) {
    if(structureSize[structureId] == 4) {
      // int
      int *addr = (int*)arrayAddr[structureId];
      printf("%d ", addr[i]);
    } else if (structureSize[structureId] == 8) {
      //Bucket
      Bucket_x *addr = (Bucket_x*)arrayAddr[structureId];
      printf("(%d, %d), ", addr[i].x, addr[i].key);
    }
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
  // print();
}

void test(int structureId) {
  int pass = 1;
  int i;
  // print(structureId);
  for (i = 1; i < paddedSize; i++) {
    pass &= (((Bucket_x*)arrayAddr[structureId])[i-1].x <= ((Bucket_x*)arrayAddr[structureId])[i].x);
    if (((Bucket_x*)arrayAddr[structureId])[i].x == 0) {
      pass = 0;
      break;
    }
  }
  printf(" TEST%d %s\n", structureId, (pass) ? "PASSed" : "FAILed");
}

void test(int structureId, int size) {
  int pass = 1;
  int i;
  // print(structureId);
  for (i = 1; i < size; i++) {
    pass &= (((Bucket_x*)arrayAddr[structureId])[i-1].x <= ((Bucket_x*)arrayAddr[structureId])[i].x);
    // std::cout<<"i, pass: "<<i<<" "<<pass<<std::endl;
  }
  printf(" TEST %s\n",(pass) ? "PASSed" : "FAILed");
}

int main(int argc, const char* argv[]) {
  oe_result_t result;
  oe_result_t method_return;
  int ret = 1;
  int *resId = (int*)malloc(sizeof(int));
  oe_enclave_t* enclave = NULL;

  // 0: OSORT, 1: bucketOSort, 2: smallBSort, 3: bitonicSort, 
  int sortId = 1;

  if (sortId == 2 || sortId == 3) {
    int addi = 0;
    if (N % BLOCK_DATA_SIZE != 0) {
      addi = ((N / BLOCK_DATA_SIZE) + 1) * BLOCK_DATA_SIZE - N;
    }
    X = (int*)malloc((N + addi) * sizeof(int));
    paddedSize = N + addi;
    arrayAddr[0] = X;
    init();
  } else if (sortId == 1) {
    // srand((unsigned)time(NULL));
    int bucketNum = smallestPowerOfTwoLargerThan(ceil(2.0 * N / BUCKET_SIZE));
    int bucketSize = bucketNum * BUCKET_SIZE;
    std::cout<<"TOTAL BUCKET SIZE: " << bucketSize<<std::endl;
    bucketx1 = (Bucket_x*)malloc(bucketSize * sizeof(Bucket_x));
    bucketx2 = (Bucket_x*)malloc(bucketSize * sizeof(Bucket_x));
    memset(bucketx1, 0, bucketSize*sizeof(Bucket_x));
    memset(bucketx2, 0, bucketSize*sizeof(Bucket_x));
    arrayAddr[1] = (int*)bucketx1;
    arrayAddr[2] = (int*)bucketx2;
    X = (int *) malloc(N * sizeof(int));
    arrayAddr[0] = X;
    paddedSize = N;
    init();
  } else {
    
  }
  // Create the enclave
  
  // result = oe_create_osort_enclave(argv[1], OE_ENCLAVE_TYPE_SGX, OE_ENCLAVE_FLAG_DEBUG, NULL, 0, &enclave);
  result = oe_create_oqsort_enclave(argv[1], OE_ENCLAVE_TYPE_SGX, 0, NULL, 0, &enclave);
  if (result != OE_OK) {
    fprintf(stderr,
            "oe_create_hello_enclave(): result=%u (%s)\n",
            result,
            oe_result_str(result));
    goto exit;
  }
  // Small bitonic sort Passed
  // result = callSmallSort(enclave, X, paddedSize);
  // test();
  /*
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
  }*/
  // Bitonic sort Passed
  if (sortId == 2 || sortId == 3) {
    result = callSort(enclave, sortId, 0, paddedSize, resId);
    test();
  } else if (sortId == 1) {
    result = callSort(enclave, sortId, 1, paddedSize, resId);
    std::cout<<"Return test: "<<*resId<<std::endl;
    test(*resId);
    // test(1);
    // test(2);
    std::cout<<"OK\n";
  } else {
    // break;
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
  std::cout<<"end\n";
  exit:
    std::cout<<"before if: \n";
    if (enclave) {
      std::cout<<"enclave\n";
      oe_terminate_enclave(enclave);
    }
    // TODO: free data structures
    std::cout<<"Before free\n";
    free(X);
    free(bucketx1);
    free(bucketx2);
    std::cout<<"IN exit\n";
    return ret;
}
