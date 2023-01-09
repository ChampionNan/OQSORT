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

#include "oqsort_t.h"

// FIXME: Why not able to contain common files
template<typename T>
constexpr T DUMMY() {
    return std::numeric_limits<T>::max();
}

enum SortType {
  ODSTIGHT,
  ODSLOOSE,
  BUCKET,
  BITONIC
};

enum InputType {
  BOOST,
  SETINMAIN
};

enum OutputType {
  TERMINAL, 
  FILEOUT
};

enum EncMode {
  OFB,
  GCM
};

enum SecLevel {
  FULLY,
  PARTIAL
};

struct Bucket_x {
  int x;
  int key;
};

struct EncOneBlock {
  int sortKey;    // used for sorting 
  int primaryKey; // tie-breaker when soryKey equals
  int payLoad;
  int randomKey;  // bucket sort random key

  EncOneBlock() {
    sortKey = DUMMY<int>();
  }
  friend EncOneBlock operator*(const int &flag, const EncOneBlock &y) {
    EncOneBlock res;
    res.sortKey = flag * y.sortKey;
    res.primaryKey = flag * y.primaryKey;
    res.payLoad = flag * y.payLoad;
    res.randomKey = flag * y.randomKey;
    return res;
  }
  friend EncOneBlock operator+(const EncOneBlock &x, const EncOneBlock &y) {
    EncOneBlock res;
    res.sortKey = x.sortKey + y.sortKey;
    res.primaryKey = x.primaryKey + y.primaryKey;
    res.payLoad = x.payLoad + y.payLoad;
    res.randomKey = x.randomKey + y.randomKey;
    return res;
  }
  friend bool operator<(const EncOneBlock &a, const EncOneBlock &b) {
    if (a.sortKey < b.sortKey) {
      return true;
    } else if (a.sortKey > b.sortKey) {
      return false;
    } else {
      if (a.primaryKey < b.primaryKey) {
        return true;
      } else if (a.primaryKey > b.primaryKey) {
        return false;
      }
    }
    return true; // equal
  }
};

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
    void swap(std::vector<EncOneBlock> &arr, int64_t i, int64_t j);
    int64_t Sample(int inStructureId, int sampleId, int64_t N, int64_t M, int64_t n_prime, int is_tight);
    int64_t greatestPowerOfTwoLessThan(double n);
    int64_t smallestPowerOfKLargerThan(int64_t n, int k);

  public:
    int64_t N, M;
    int B, sigma;
    int64_t IOcost;
    int encOneBlockSize; // sizeof(EncOneBlock)
    int nonEnc; // no encryption
    EncMode encmode = OFB;
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
