#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <cmath>
#include <cassert>
#include <cstdint>
#include <random>
#include <openenclave/host.h>

#include "../include/IOTools.h"
#include "../include/definitions.h"
#include "../include/common.h"

#include "oqsort_u.h"

// Globals
int *X;
//structureId=3, write back array
int *Y;
//structureId=1, bucket1 in bucket sort; input
Bucket_x *bucketx1;
//structureId=2, bucket 2 in bucket sort
Bucket_x *bucketx2;

int *arrayAddr[NUM_STRUCTURES];
int paddedSize;
double IOcost = 0;

/* OCall functions */
void ocall_print_string(const char *str) {
  /* Proxy/Bridge will check the length and null-terminate
   * the input string to prevent buffer overflow.
   */
  printf("%s", str);
  fflush(stdout);
}

// index: EncBlock, offset: int, blockSize: bytes
void OcallRB(int index, int* buffer, int blockSize, int structureId) {
  // std::cout<< "In OcallRB\n";
  memcpy(buffer, &(((EncBlock*)(arrayAddr[structureId]))[index]), blockSize); 
  IOcost += 1;
}

// index: EncBlock, offset: int, blockSize: bytes(only non-enc mode could use)
void OcallWB(int index, int offset, int* buffer, int blockSize, int structureId) {
  // std::cout<< "In OcallWB\n";
  memcpy((int*)(&(((EncBlock*)(arrayAddr[structureId]))[index]))+offset, buffer, blockSize);
  IOcost += 1;
}

// TODO: Set this function as OCALL
// allocate encB for outside memory
void freeAllocate(int structureIdM, int structureIdF, int size) {
  // 1. Free arrayAddr[structureId]
  if (arrayAddr[structureIdF]) {
    free(arrayAddr[structureIdF]);
  }
  // 2. malloc new asked size (allocated in outside)
  if (size <= 0) {
    return;
  }
  int *addr = (int*)malloc(size * sizeof(EncBlock));
  memset(addr, DUMMY, size * sizeof(EncBlock));
  // 3. assign malloc address to arrayAddr
  arrayAddr[structureIdM] = addr;
  return ;
}

// support int version only
void fyShuffle(int structureId, int size, int B) {
  if (size % B != 0) {
    printf("Error! Not 4 times.\n");
  }
  int total_blocks = size / B;
  EncBlock *trustedM3 = (EncBlock*)malloc(sizeof(EncBlock));
  int k;
  std::random_device dev;
  std::mt19937 rng(dev()); 
  // switch block i & block k
  for (int i = total_blocks-1; i >= 0; i--) {
    std::uniform_int_distribution<int> dist(0, i);
    k = dist(rng);
    memcpy(trustedM3, (EncBlock*)(arrayAddr[structureId]) + k, sizeof(EncBlock));
    memcpy((EncBlock*)(arrayAddr[structureId]) + k, (EncBlock*)(arrayAddr[structureId]) + i, sizeof(EncBlock));
    memcpy((EncBlock*)(arrayAddr[structureId]) + i, trustedM3, sizeof(EncBlock));
  }
  std::cout << "Finished floyd shuffle\n";
}


/* main function */
int main(int argc, const char* argv[]) {
  int ret = 1;
  int *resId = (int*)malloc(sizeof(int));
  int *resN = (int*)malloc(sizeof(int));
  oe_result_t result;
  oe_enclave_t* enclave = NULL;
  std::chrono::high_resolution_clock::time_point start, end;
  std::chrono::seconds duration;
  srand((unsigned)time(NULL));
  double params[9] = {-1};
  int i = 0;
  std::ifstream ifs;
  ifs.open("/home/chenbingnan/mysamples/OQSORT/params.txt",std::ios::in);
  std::string buf;
  while (getline(ifs, buf)) {
    params[i++] = std::stod(buf);
    std::cout << params[i-1] << std::endl;
  }
  if (params[3] == -1) {
    std::cout << "Parameters setting wrong!\n";
    return 0;
  }
  int N = params[0], BLOCK_DATA_SIZE = params[2], M = params[1];
  int FAN_OUT, BUCKET_SIZE;
  // 0: OQSORT-Tight, 1: OQSORT-Loose, 2: bucketOSort, 3: bitonicSort(x), 4: merge_sort(x), 5: IO test
  int sortId = 1;
  int inputId = 0;

  // step1: init test numbers
  if (sortId == 3) {
    // inputId = 0;
    int addi = 0;
    if (N % BLOCK_DATA_SIZE != 0) {
      addi = ((N / BLOCK_DATA_SIZE) + 1) * BLOCK_DATA_SIZE - N;
    }
    paddedSize = N + addi;
    freeAllocate(inputId, inputId, ceil(1.0*(N+addi)/4));
    initEnc(arrayAddr, inputId, paddedSize);
  } else if (sortId == 4) {
    inputId = 1;
    // arrayAddr[inputId] = X;
    freeAllocate(inputId, inputId, ceil(1.0*N/2));
    freeAllocate(inputId+1, inputId+1, ceil(1.0*N/2));
    paddedSize = N;
    initEnc(arrayAddr, inputId, paddedSize);
  } else if (sortId == 2) {
    // inputId = 0;
    double z1 = 6 * (KAPPA + log(2.0*N));
    double z2 = 6 * (KAPPA + log(2.0*N/z1));
    BUCKET_SIZE = BLOCK_DATA_SIZE * ceil(1.0*z2/BLOCK_DATA_SIZE);
    std::cout << "BUCKET_SIZE: " << BUCKET_SIZE << std::endl;
    double thresh = 1.0*M/BUCKET_SIZE;
    std::cout << "Threash: " << thresh << std::endl;
    FAN_OUT = greatestPowerOfTwoLessThan(thresh)/2;
    assert(FAN_OUT >= 2 && "M/Z must greater than 2");
    int bucketNum = smallestPowerOfKLargerThan(ceil(2.0 * N / BUCKET_SIZE), 2);
    int bucketSize = bucketNum * BUCKET_SIZE;
    std::cout << "TOTAL BUCKET SIZE: " << bucketSize << std::endl;
    std::cout << "BUCKET NUMBER: " << bucketNum << std::endl;
    std::cout << "BUCKET SIZE: " << BUCKET_SIZE << std::endl; 
    std::cout << "FAN_OUT: " << FAN_OUT << std::endl;  
    freeAllocate(1, 1, ceil(1.0*bucketSize/2));
    freeAllocate(2, 2, ceil(1.0*bucketSize/2));
    std::cout << "After bucket malloc\n";
    freeAllocate(inputId, inputId, ceil(1.0*N/4));
    paddedSize = N;
    // TODO: 
    initEnc(arrayAddr, inputId, paddedSize);
  } else if (sortId == 0 || sortId == 1) {
    inputId = 3;
    freeAllocate(inputId, inputId, ceil(1.0*N/4));
    paddedSize = N;
    initEnc(arrayAddr, inputId, paddedSize);
  }

  // step2: Create the enclave
  // result = oe_create_oqsort_enclave(argv[1], OE_ENCLAVE_TYPE_SGX, OE_ENCLAVE_FLAG_DEBUG, NULL, 0, &enclave);
  result = oe_create_oqsort_enclave(argv[1], OE_ENCLAVE_TYPE_SGX, 0, NULL, 0, &enclave);
  start = std::chrono::high_resolution_clock::now();
  if (sortId == 3) {
    std::cout << "Test bitonic sort... " << std::endl;
    callSort(enclave, sortId, inputId, paddedSize, resId, resN, params);
    // end = std::chrono::high_resolution_clock::now();
    testEnc(arrayAddr, inputId, paddedSize);
  } else if (sortId == 4) {
    std::cout << "Test merge_sort... " << std::endl;
    callSort(enclave, sortId, inputId, paddedSize, resId, resN, params);
    std::cout << "Result ID: " << *resId << std::endl;
    *resN = N;
    // end = std::chrono::high_resolution_clock::now();
    test(arrayAddr, *resId, paddedSize);
  } else if (sortId == 2) {
    std::cout << "Test bucket oblivious sort... " << std::endl;
    callSort(enclave, sortId, inputId + 1, paddedSize, resId, resN, params);
    std::cout << "Result ID: " << *resId << std::endl;
    *resN = N;
    // print(arrayAddr, *resId, N);
    // end = std::chrono::high_resolution_clock::now();
    testEnc(arrayAddr, *resId, paddedSize);
  } else if (sortId == 0 || sortId == 1) {
    std::cout << "Test OQSort... " << std::endl;
    callSort(enclave, sortId, inputId, paddedSize, resId, resN, params);
    std::cout << "Result ID: " << *resId << std::endl;
    // end = std::chrono::high_resolution_clock::now();
    if (*resId == -1) {
      std::cout << "TEST Failed\n";
    } else if (sortId == 0) {
      // test(arrayAddr, *resId, paddedSize);
      *resN = N;
    } else if (sortId == 1) {
      // Sample Loose has different test & print
      // testWithDummy(arrayAddr, *resId, *resN);
    } else if (sortId == 5) {
      callSort(enclave, sortId, inputId, paddedSize, resId, resN, params);
    }
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
  int multi = (sortId == 2 || sortId == 4) ? 2 : 1;
  printf("IOcost: %f, %f\n", 1.0*IOcost/N*(BLOCK_DATA_SIZE/multi), IOcost);
  printEnc(arrayAddr, *resId, *resN);
  // testEnc(arrayAddr, *resId, *resN);

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
    free(resId);
    free(resN);
    return ret;
}
