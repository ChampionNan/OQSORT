#ifndef SHARED_H
#define SHARED_H

#include "../include/common.h"
#include "../include/definitions.h"

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
// #include <cstdlib>

#include "oqsort_t.h"

extern double ALPHA, BETA, P, _ALPHA, _BETA, _P;
extern int N, M, BLOCK_DATA_SIZE;
extern int is_tight;
extern int nonEnc;

struct HeapNode {
  Bucket_x *data;
  int bucketIdx;
  int elemIdx;
};

class Heap {
  HeapNode *harr;
  int heapSize;
  int batchSize;
public:
  Heap(HeapNode *a, int size, int bsize);
  void Heapify(int i);
  int left(int i);
  int right (int i);
  void swapHeapNode(HeapNode *a, HeapNode *b);
  HeapNode *getRoot();
  int getHeapSize();
  bool reduceSizeByOne();
  void replaceRoot(HeapNode x);
};

int printf(const char *fmt, ...);
int greatestPowerOfTwoLessThan(double n);
int smallestPowerOfKLargerThan(int n, int k);
void aes_init();
void cbc_encrypt(EncBlock* buffer, int blockSize);
void cbc_decrypt(EncBlock* buffer, int blockSize);
void gcm_encrypt(EncBlock* buffer, int blockSize);
void gcm_decrypt(EncBlock* buffer, int blockSize);
void OcallReadBlock(int startIdx, int offset, int* buffer, int blockSize, int structureId);
void OcallWriteBlock(int startIdx, int offset, int* buffer, int blockSize, int structureId);
void opOneLinearScanBlock(int index, int* block, int blockSize, int structureId, int write, int dummyNum=0);
bool cmpHelper(int *a, int *b);
bool cmpHelper(Bucket_x *a, Bucket_x *b);
int moveDummy(int *a, int size);
void swapRow(int *a, int *b);
void swapRow(Bucket_x *a, Bucket_x *b);

#endif // !SHARED_H
