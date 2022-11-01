#ifndef SHARED_H
#define SHARED_H

#include "../include/common.h"
#include "../include/definitions.h"

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
extern int64_t N, M, BLOCK_DATA_SIZE;
extern int is_tight;

struct HeapNode {
  Bucket_x *data;
  int64_t bucketIdx;
  int64_t elemIdx;
};

class Heap {
  HeapNode *harr;
  int64_t heapSize;
  int64_t batchSize;
public:
  Heap(HeapNode *a, int64_t size, int64_t bsize);
  void Heapify(int64_t i);
  int64_t left(int64_t i);
  int64_t right (int64_t i);
  void swapHeapNode(HeapNode *a, HeapNode *b);
  HeapNode *getRoot();
  int64_t getHeapSize();
  bool reduceSizeByOne();
  void replaceRoot(HeapNode x);
};

int printf(const char *fmt, ...);
int64_t greatestPowerOfTwoLessThan(double n);
int64_t smallestPowerOfKLargerThan(int64_t n, int64_t k);
void opOneLinearScanBlock(int64_t index, int64_t* block, int64_t blockSize, int structureId, int write, int64_t dummyNum);
bool cmpHelper(int64_t *a, int64_t *b);
bool cmpHelper(Bucket_x *a, Bucket_x *b);
int64_t moveDummy(int64_t *a, int64_t size);
void swapRow(int64_t *a, int64_t *b);
void swapRow(Bucket_x *a, Bucket_x *b);

#endif // !SHARED_H
