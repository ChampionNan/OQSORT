#ifndef SHARED_H
#define SHARED_H

#include "../include/common.h"

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
// #include <cstdlib>

#include "oqsort_t.h"

struct HeapNode {
  EncOneBlock *data;
  int64_t bucketIdx; // TODO: int64_t
  int64_t elemIdx; // TODO: int64_t
};

class Heap {
  HeapNode *harr;
  int64_t heapSize;
  int64_t batchSize;
public:
  Heap(HeapNode *a, int64_t size, int64_t bsize);
  void Heapify(int64_t i);
  int64_t left(int64_t i);
  int64_t right(int64_t i);
  void swapHeapNode(HeapNode *a, HeapNode *b);
  HeapNode *getRoot();
  int64_t getHeapSize();
  bool reduceSizeByOne();
  void replaceRoot(HeapNode x);
};

class EnclaveServer {
  public:
    EnclaveServer(int64_t N, int64_t M, int B, EncMode encmode);
    int printf(const char *fmt, ...);
    int greatestPowerOfTwoLessThan(double n);
    int64_t smallestPowerOfKLargerThan(int64_t n, int k);
    void ofb_encrypt(EncOneBlock* buffer, int blockSize);
    void ofb_decrypt(EncOneBlock* buffer, int blockSize);
    void gcm_encrypt(EncOneBlock* buffer, int blockSize);
    void gcm_decrypt(EncOneBlock* buffer, int blockSize);
    void OcallReadPage(int64_t startIdx, EncOneBlock* buffer, int pageSize, int structureId);
    void OcallWritePage(int64_t startIdx, EncOneBlock* buffer, int pageSize, int structureId);
    void opOneLinearScanBlock(int64_t index, EncOneBlock* block, int64_t elementNum, int structureId, int write, int64_t dummyNum);
    bool cmpHelper(EncOneBlock *a, EncOneBlock *b);
    int64_t moveDummy(EncOneBlock *a, int64_t size);
    void swapRow(EncOneBlock *a, int64_t i, int64_t j);

  public:
    int64_t N, M;
    int B, sigma;
    int encOneBlockSize; // sizeof(EncOneBlock)
    int encDataSize;     // sizeof(EncOneBlock) - #iv bytes
    int nonEnc; // no encryption
    EncMode encmode = OFB;
    unsigned char key[32];
    mbedtls_aes_context aes;
    mbedtls_gcm_context gcm;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    size_t iv_offset, iv_offset1;
}

#endif // !SHARED_H
