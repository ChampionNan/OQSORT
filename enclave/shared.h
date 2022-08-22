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
#include <algorithm>
#include <cstring>
#include <cstdarg>
#include <cstdlib>

#include "oqsort_t.h"

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
int greatestPowerOfTwoLessThan(int n);
int smallestPowerOfKLargerThan(int n, int k);
void opOneLinearScanBlock(int index, int* block, size_t blockSize, int structureId, int write);
bool cmpHelper(int *a, int *b);
bool cmpHelper(Bucket_x *a, Bucket_x *b);
void padWithDummy(int structureId, int start, int realNum, int secSize);
int moveDummy(int *a, int size);
bool isTargetIterK(int randomKey, int iter, int k, int num);
void swapRow(int *a, int *b);
void swapRow(Bucket_x *a, Bucket_x *b);

#endif // !SHARED_H
