#ifndef SHARED_H
#define SHARED_H

#include <mbedtls/aes.h>
#include <mbedtls/gcm.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

#include <openenclave/enclave.h>
#include <openenclave/corelibc/stdlib.h>
#include <openenclave/debugmalloc.h>
#include <openenclave/internal/rdrand.h>

#include <random>
#include <cassert>
#include <cmath>
#include <cstdint> 
#include <algorithm>
#include <cstring>
#include <cstdarg>
#include <unordered_set>
#include <time.h>
#include <vector>
#include <iostream>
#include <exception>

#include "../include/common.h"

#include "oqsort_t.h"

typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;

class EnclaveServer {
  public:
    EnclaveServer(int64_t N, int64_t M, int B, EncMode encmode);
    double getIOcost();
    double getIOtime();
    int printf(const char *fmt, ...);
    void ofb_encrypt(EncOneBlock* buffer, int blockSize);
    void ofb_decrypt(EncOneBlock* buffer, int blockSize);
    void gcm_encrypt(EncOneBlock* buffer, int blockSize);
    void gcm_decrypt(EncOneBlock* buffer, int blockSize);
    void OcallReadPage(int64_t startIdx, EncOneBlock* buffer, int pageSize, int structureId);
    void OcallWritePage(int64_t startIdx, EncOneBlock* buffer, int pageSize, int structureId);
    void opOneLinearScanBlock(int64_t index, EncOneBlock* block, int64_t elementNum, int structureId, int write, int64_t dummyNum);
    bool cmpHelper(EncOneBlock *a, EncOneBlock *b);
    int64_t moveDummy(EncOneBlock *a, int64_t size);
    void setValue(EncOneBlock *a, int64_t size, int value);
    void setDummy(EncOneBlock *a, int64_t size);
    void swapRow(EncOneBlock *a, int64_t i, int64_t j);
    void swap(std::vector<EncOneBlock> &arr, int64_t i, int64_t j);
    void oswap(EncOneBlock *a, EncOneBlock *b, bool cond);
    void oswap128(uint128_t *a, uint128_t *b, bool cond);
    void regswap(EncOneBlock *a, EncOneBlock *b);
    int64_t Sample(int inStructureId, int sampleId, int64_t N, int64_t M, int64_t n_prime);
    int64_t greatestPowerOfTwoLessThan(double n);
    int64_t greatestPowerOfTwoLessThan(int64_t n);
    int64_t smallestPowerOfKLargerThan(int64_t n, int k);

  public:
    int64_t N, M;
    int B, sigma;
    double IOcost;
    double IOtime;
    clock_t IOstart, IOend;
    int encOneBlockSize; // sizeof(EncOneBlock)
    int nonEnc; // no encryption
    EncMode encmode = OFB;
    int tie_breaker = 0;
  private:
    unsigned char key[32];
    mbedtls_aes_context aes;
    mbedtls_gcm_context gcm;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    size_t iv_offset, iv_offset1;
    unsigned char iv[16];
};

struct HeapNode {
  EncOneBlock *data;
  int64_t bucketIdx;
  int64_t elemIdx;
};

class Heap {
  HeapNode *harr;
  int64_t heapSize;
  int64_t batchSize;
public:
  Heap(EnclaveServer &eServer, HeapNode *a, int64_t size, int64_t bsize);
  void Heapify(int64_t i);
  int64_t left(int64_t i);
  int64_t right(int64_t i);
  void swapHeapNode(HeapNode *a, HeapNode *b);
  HeapNode *getRoot();
  int64_t getHeapSize();
  bool reduceSizeByOne();
  void replaceRoot(HeapNode x);
private:
  EnclaveServer eServer;
};

#endif // !SHARED_H
