#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <cmath>
#include <cassert>
#include <cstdint>
#include <random>
#include <exception>
#include <openenclave/host.h>

#include "../include/DataStore.h"
#include "../include/common.h"

#include "oqsort_u.h"

using namespace std;
using namespace chrono;

EncOneBlock *arrayAddr[NUM_STRUCTURES];

/* OCall functions */
void OcallSample(int inStructureId, int sampleId, int64_t N, int64_t M, int64_t n_prime, int SSD, int64_t *ret) {
  int64_t N_prime = N;
  int64_t boundary = ceil(1.0 * N_prime / M);
  int64_t realNum = 0;
  int64_t readStart = 0;
  EncOneBlock *trustedM1 = new EncOneBlock[M];
  int64_t m = 0, Msize;
  freeAllocate(sampleId, sampleId, n_prime, SSD);
  if (!SSD) {
    for (int64_t i = 0; i < boundary; ++i) {
      Msize = std::min(M, N - i * M);
      m = Hypergeometric(N_prime, Msize, n_prime);
      printf("Sampling progress: %ld / %ld, m: %ld\n", i, boundary-1, m);
      if (m > 0) {
        memcpy(trustedM1, arrayAddr[inStructureId] + readStart, Msize * sizeof(EncOneBlock));
        readStart += Msize;
        shuffle(trustedM1, Msize);
        memcpy(arrayAddr[sampleId] + realNum, trustedM1, m * sizeof(EncOneBlock));
        realNum += m;
      }
    }
  } else {
    string path = pathBase + to_string(inStructureId);
    ifstream inFile(path.c_str(), ios::in);
    path = pathBase + to_string(sampleId);
    fstream outFile(path.c_str(), ios::out | ios::in); // write
    for (int64_t i = 0; i < boundary; ++i) {
      Msize = std::min(M, N - i * M);
      m = Hypergeometric(N_prime, Msize, n_prime);
      printf("Sampling SSD progress: %ld / %ld, m: %ld\n", i, boundary-1, m);
      if (m > 0) {
        inFile.seekg(readStart * sizeof(EncOneBlock), ios::beg);
        inFile.read((char*)trustedM1, Msize * sizeof(EncOneBlock));
        readStart += Msize;
        shuffle(trustedM1, Msize);
        outFile.seekp(realNum * sizeof(EncOneBlock), ios::beg);
        outFile.write((char*)trustedM1, m * sizeof(EncOneBlock));
        realNum += m;
      }
    }
    inFile.close();
    outFile.close();
  }
  delete [] trustedM1;
  *ret = realNum;
}

void ocall_print_string(const char *str) {
  /* Proxy/Bridge will check the length and null-terminate the input string to prevent buffer overflow. */
  printf("%s", str);
  fflush(stdout);
}

// index: Block Index, blockSize: bytes
void OcallRB(size_t index, int* buffer, size_t blockSize, int structureId, int SSD) {
  if (!SSD) {
    memcpy(buffer, (int*)(&((arrayAddr[structureId])[index])), blockSize);
  } else {
    string path = pathBase + to_string(structureId);
    ifstream inFile(path.c_str(), ios::in);
    inFile.seekg(index * sizeof(EncOneBlock), ios::beg);
    inFile.read((char*)buffer, blockSize);
    inFile.close();
  }
}

// index: Block index, blockSize: bytes
void OcallWB(size_t index, int* buffer, size_t blockSize, int structureId, int SSD) {
  if (!SSD) {
    memcpy((int*)(&((arrayAddr[structureId])[index])), buffer, blockSize);
  } else {
    string path = pathBase + to_string(structureId);
    fstream outFile(path.c_str(), ios::out | ios::in);
    outFile.seekp(index * sizeof(EncOneBlock), ios::beg);
    outFile.write((char*)buffer, blockSize);
    outFile.close();
  }
}

// allocate encB for outside memory
void freeAllocate(int structureIdM, int structureIdF, size_t size, int SSD) {
  if (!SSD) {
    // 1. Free arrayAddr[structureId]
    if (arrayAddr[structureIdF]) {
      delete [] arrayAddr[structureIdF];
    }
    // 2. malloc new asked size (allocated in outside)
    if (size <= 0) {
      return;
    }
    EncOneBlock *addr = (EncOneBlock*)malloc(size * sizeof(EncOneBlock));
    memset(addr, DUMMY<int>(), size * sizeof(EncOneBlock));
    // 3. assign malloc address to arrayAddr
    arrayAddr[structureIdM] = addr;    
  } else {
    EncOneBlock *addr = (EncOneBlock*)malloc(size * sizeof(EncOneBlock));
    memset(addr, DUMMY<int>(), size * sizeof(EncOneBlock));
    string path = pathBase + to_string(structureIdM);
    ofstream outFile(path.c_str(), ios::out | ios::trunc);
    outFile.write((char*)addr, size * sizeof(EncOneBlock));
    outFile.close();
    delete [] addr;
  }
  return ;
}

// shuffle data in B block size
void fyShuffle(int structureId, size_t size, int B) {
  if (size % B != 0) {
    printf("Error! Not B's time.\n"); 
  }
  int64_t total_blocks = size / B;
  EncOneBlock *trustedM3 = new EncOneBlock[B];
  int64_t k;
  int64_t eachSec = size / 100;
  int swapSize = sizeof(EncOneBlock) * B;
  std::random_device rd;
  std::mt19937 rng{rd()};
  for (int64_t i = total_blocks-1; i >= 0; i--) {
    if (i % eachSec == 0) {
      printf("Shuffle progress %ld / %ld\n", i, total_blocks-1);
    }
    std::uniform_int_distribution<int64_t> dist(0, i);
    k = dist(rng);
    memcpy(trustedM3, arrayAddr[structureId] + k * B, swapSize);
    memcpy(arrayAddr[structureId] + k * B, arrayAddr[structureId] + i * B, swapSize);
    memcpy(arrayAddr[structureId] + i * B, trustedM3, swapSize);
  }
  std::cout << "Finished floyd shuffle\n";
}

void readParams(InputType inputtype, int &datatype, int64_t &N, int64_t &M, int &B, int &sigma, int &sortId, double &alpha, double &beta, double &gamma, int &P, int &SSD, int &argc, const char* argv[]) {
  if (inputtype == BOOST) {
    cout << "Need to be done." << endl;
    return;
  } else if (inputtype == SETINMAIN) {
    datatype = 128; // 16, 128
    M = (64 << 20) / datatype; // (MB << 20) / 1 element bytes
    N = 200 * M;
    B = (4 << 10) / datatype; // 4KB pagesize
    sigma = 40;
    // 0: OQSORT-Tight, 1: OQSORT-Loose, 2: bucketOSort, 3: bitonicSort
    sortId = 0;
    alpha = 0.02;
    beta = 0.11;
    gamma = 0.25;
    P = 274;
    SSD = 1;
  }
}

int main(int argc, const char* argv[]) {
  int ret = 1;
  int *resId = new int;
  int *resN = new int;
  int FAN_OUT, BUCKET_SIZE;
  int inputId = 1;
  oe_result_t result;
  oe_enclave_t* enclave = NULL;
  high_resolution_clock::time_point start, end;
  seconds duration;
  // step1: init test numbers
  InputType inputtype = SETINMAIN;
  int datatype, B, sigma, sortId, P, SSD;
  int64_t N, M;
  double alpha, beta, gamma;
  readParams(inputtype, datatype, N, M, B, sigma, sortId, alpha, beta, gamma, P, SSD, argc, argv);
  double params[11] = {(double)sortId, (double)inputId, (double)N, (double)M, (double)B, (double)sigma, alpha, beta, gamma, (double)P, (double)SSD};
  // step2: Create the enclave
  result = oe_create_oqsort_enclave(argv[1], OE_ENCLAVE_TYPE_SGX, 0, NULL, 0, &enclave);
  // 0: OQSORT-Tight, 1: OQSORT-Loose, 2: bucketOSort, 3: bitonicSort
  if (sortId == 3 && (N % B) != 0) {
    int64_t addi = addi = ((N / B) + 1) * B - N;
    N += addi;
  }
  DataStore data(arrayAddr, N, M, B, SSD);
  if (sortId == 2) {
    int64_t totalSize = calBucketSize(sigma, N, M, B);
    data.init(inputId, N);
    data.init(inputId + 1, totalSize);
    data.init(inputId + 2, totalSize);
  } else {
    data.init(inputId, N);
  }
  start = high_resolution_clock::now();
  callSort(enclave, resId, resN, params);
  end = high_resolution_clock::now();
  if (result != OE_OK) {
    fprintf(stderr, "Calling into enclave_hello failed: result=%u (%s)\n", result, oe_result_str(result));
    ret = -1;
  }
  // step4: std::cout execution time
  duration = duration_cast<seconds>(end - start);
  std::cout << "Time taken by sorting function: " << duration.count() << " seconds" << std::endl;
  data.print(*resId, *resN, FILEOUT, data.filepath);
  // step5: exix part
  exit:
    if (enclave) {
      oe_terminate_enclave(enclave);
    }
    delete resId;
    delete resN;
    return ret;
}