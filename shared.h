#ifndef SHARED_H
#define SHARED_H

#include "common.h"

#include <mbedtls/aes.h>
#include <mbedtls/gcm.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

// #include <openenclave/enclave.h>
// #include <openenclave/corelibc/stdlib.h>
// #include <openenclave/debugmalloc.h>
// #include <openenclave/internal/rdrand.h>

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

// #include "oqsort_t.h"

class EnclaveServer {
  public:
    EnclaveServer(int64_t N, int64_t M, int B, EncMode encmode);
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
    void swapRow(EncOneBlock *a, int64_t i, int64_t j);
    void shuffle(EncOneBlock *a, int64_t size);

  public:
    int64_t N, M;
    int B, sigma;
    int encOneBlockSize; // sizeof(EncOneBlock)
    int encDataSize;     // sizeof(EncOneBlock) - #iv bytes
    int nonEnc; // no encryption
    EncMode encmode = OFB;
  private:
    unsigned char key[32];
    mbedtls_aes_context aes;
    mbedtls_gcm_context gcm;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    size_t iv_offset, iv_offset1;
    std::random_device rd;
    std::mt19937 rng{rd()};
};

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
