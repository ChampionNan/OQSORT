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
int64_t *X;
//structureId=3, write back array
int64_t *Y;
//structureId=1, bucket1 in bucket sort; input
Bucket_x *bucketx1;
//structureId=2, bucket 2 in bucket sort
Bucket_x *bucketx2;

int64_t *arrayAddr[NUM_STRUCTURES];
int64_t paddedSize;
double IOcost = 0;

/* OCall functions */
void ocall_print_string(const char *str) {
  /* Proxy/Bridge will check the length and null-terminate
   * the input string to prevent buffer overflow.
   */
  printf("%s", str);
  fflush(stdout);
}

void OcallReadBlock(int64_t index, int64_t* buffer, int64_t blockSize, int structureId) {
  if (blockSize == 0) {
    // printf("Unknown data size");
    return;
  }
  // memcpy(buffer, arrayAddr[structureId] + index, blockSize * structureSize[structureId]);
  memcpy(buffer, arrayAddr[structureId] + index, blockSize);
  IOcost += 1;
}

void OcallWriteBlock(int64_t index, int64_t* buffer, int64_t blockSize, int structureId) {
  if (blockSize == 0) {
    // printf("Unknown data size");
    return;
  }
  // memcpy(arrayAddr[structureId] + index, buffer, blockSize * structureSize[structureId]);
  memcpy(arrayAddr[structureId] + index, buffer, blockSize);
  IOcost += 1;
}

// TODO: Set this function as OCALL
void freeAllocate(int structureIdM, int structureIdF, int64_t size) {
  // 1. Free arrayAddr[structureId]
  if (arrayAddr[structureIdF]) {
    free(arrayAddr[structureIdF]);
  }
  // 2. malloc new asked size (allocated in outside)
  if (size <= 0) {
    return;
  }
  int64_t *addr = (int64_t*)malloc(size * sizeof(int64_t));
  memset(addr, DUMMY, size * sizeof(int64_t));
  // 3. assign malloc address to arrayAddr
  arrayAddr[structureIdM] = addr;
  return ;
}

// support int version only
void fyShuffle(int structureId, int64_t size, int64_t B) {
  int64_t total_blocks = ceil(1.0 * size / B);
  int64_t *trustedM3 = (int64_t*)malloc(sizeof(int64_t) * B);
  int k;
  std::random_device dev;
  std::mt19937 rng(dev()); 
  srand((unsigned)time(0));
  int64_t Msize1, Msize2;
  for (int64_t i = total_blocks-1; i >= 0; i--) {
    std::uniform_int_distribution<int64_t> dist(0, i);
    k = dist(rng);
    Msize1 = std::min(B, size - k * B);
    memcpy(trustedM3, arrayAddr[structureId] + k * B, Msize1 * sizeof(int64_t));
    Msize2 = std::min(B, size - i * B);
    memcpy(arrayAddr[structureId] + k * B, arrayAddr[structureId] + i * B, Msize2 * sizeof(int64_t));
    memcpy(arrayAddr[structureId] + i * B, trustedM3, Msize1 * sizeof(int64_t));
  }
  std::cout << "Finished floyd shuffle\n";
}


/* main function */
int main(int argc, const char* argv[]) {
  int ret = 1;
  int *resId = (int*)malloc(sizeof(int));
  int64_t *resN = (int64_t*)malloc(sizeof(int64_t));
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
  int64_t N = params[0], BLOCK_DATA_SIZE = params[2], M = params[1];
  int64_t FAN_OUT, BUCKET_SIZE;
  // 0: OQSORT-Tight, 1: OQSORT-Loose, 2: bucketOSort, 3: bitonicSort, 4: merge_sort
  int sortId = 1;
  int inputId = 0;

  // step1: init test numbers
  if (sortId == 3) {
    // inputId = 0;
    int64_t addi = 0;
    if (N % BLOCK_DATA_SIZE != 0) {
      addi = ((N / BLOCK_DATA_SIZE) + 1) * BLOCK_DATA_SIZE - N;
    }
    X = (int64_t*)malloc((N + addi) * sizeof(int64_t));
    paddedSize = N + addi;
    arrayAddr[inputId] = X;
    init(arrayAddr, inputId, paddedSize);
  } else if (sortId == 4) {
    inputId = 1;
    // arrayAddr[inputId] = X;
    bucketx1 = (Bucket_x*)malloc(N * sizeof(Bucket_x));
    bucketx2 = (Bucket_x*)malloc(N * sizeof(Bucket_x));
    arrayAddr[inputId] = (int64_t*)bucketx1;
    arrayAddr[inputId+1] = (int64_t*)bucketx2;
    paddedSize = N;
    init(arrayAddr, inputId, paddedSize);
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
    int64_t bucketNum = smallestPowerOfKLargerThan(ceil(2.0 * N / BUCKET_SIZE), 2);
    int64_t bucketSize = bucketNum * BUCKET_SIZE;
    std::cout << "TOTAL BUCKET SIZE: " << bucketSize << std::endl;
    std::cout << "BUCKET NUMBER: " << bucketNum << std::endl;
    std::cout << "BUCKET SIZE: " << BUCKET_SIZE << std::endl; 
    std::cout << "FAN_OUT: " << FAN_OUT << std::endl;  
    bucketx1 = (Bucket_x*)malloc(bucketSize * sizeof(Bucket_x));
    bucketx2 = (Bucket_x*)malloc(bucketSize * sizeof(Bucket_x));
    memset(bucketx1, 0xff, bucketSize*sizeof(Bucket_x));
    memset(bucketx2, 0xff, bucketSize*sizeof(Bucket_x));
    std::cout << "After bucket malloc\n";
    arrayAddr[1] = (int64_t*)bucketx1;
    arrayAddr[2] = (int64_t*)bucketx2;
    X = (int64_t*) malloc(N * sizeof(int64_t));
    arrayAddr[inputId] = X;
    paddedSize = N;
    init(arrayAddr, inputId, paddedSize);
  } else if (sortId == 0 || sortId == 1) {
    inputId = 3;
    X = (int64_t*)malloc(N * sizeof(int64_t));
    arrayAddr[inputId] = X;
    paddedSize = N;
    init(arrayAddr, inputId, paddedSize);
  }

  // step2: Create the enclave
  // result = oe_create_oqsort_enclave(argv[1], OE_ENCLAVE_TYPE_SGX, OE_ENCLAVE_FLAG_DEBUG, NULL, 0, &enclave);
  result = oe_create_oqsort_enclave(argv[1], OE_ENCLAVE_TYPE_SGX, 0, NULL, 0, &enclave);
  start = std::chrono::high_resolution_clock::now();
  if (sortId == 3) {
    std::cout << "Test bitonic sort... " << std::endl;
    callSort(enclave, sortId, inputId, paddedSize, resId, resN, params);
    // end = std::chrono::high_resolution_clock::now();
    test(arrayAddr, inputId, paddedSize);
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
    test(arrayAddr, *resId, paddedSize);
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
  printf("IO cost: %f, total: %f\n", 1.0*IOcost/N*BLOCK_DATA_SIZE, IOcost);
  print(arrayAddr, *resId, *resN);

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
