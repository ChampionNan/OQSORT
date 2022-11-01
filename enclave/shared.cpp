#include "shared.h"

Heap::Heap(HeapNode *a, int64_t size, int64_t bsize) {
  heapSize = size;
  harr = a;
  int64_t i = (heapSize - 1) / 2;
  batchSize = bsize;
  while (i >= 0) {
    Heapify(i);
    i --;
  }
}

void Heap::Heapify(int64_t i) {
  int64_t l = left(i);
  int64_t r = right(i);
  int64_t target = i;

  if (l < heapSize && cmpHelper(harr[i].data + harr[i].elemIdx % batchSize, harr[l].data + harr[l].elemIdx % batchSize)) {
    target = l;
  }
  if (r < heapSize && cmpHelper(harr[target].data + harr[target].elemIdx % batchSize, harr[r].data + harr[r].elemIdx % batchSize)) {
    target = r;
  }
  if (target != i) {
    swapHeapNode(&harr[i], &harr[target]);
    Heapify(target);
  }
}

int64_t Heap::left(int64_t i) {
  return (2 * i + 1);
}

int64_t Heap::right(int64_t i) {
  return (2 * i + 2);
}

void Heap::swapHeapNode(HeapNode *a, HeapNode *b) {
  HeapNode temp = *a;
  *a = *b;
  *b = temp;
}

HeapNode* Heap::getRoot() {
  return &harr[0];
}

int64_t Heap::getHeapSize() {
  return heapSize;
}

bool Heap::reduceSizeByOne() {
  free(harr[0].data);
  heapSize --;
  if (heapSize > 0) {
    harr[0] = harr[heapSize];
    Heapify(0);
    return true;
  } else {
    return false;
  }
}

void Heap::replaceRoot(HeapNode x) {
  harr[0] = x;
  Heapify(0);
}

/*
 * printf:
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
int printf(const char *fmt, ...) {
  char buf[BUFSIZ] = {'\0'};
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(buf, BUFSIZ, fmt, ap);
  va_end(ap);
  ocall_print_string(buf);
  return ret;
}

int64_t greatestPowerOfTwoLessThan(double n) {
  int64_t k = 1;
  while (k > 0 && k < n) {
    k = k << 1;
  }
  return k >> 1;
}

int64_t smallestPowerOfKLargerThan(int64_t n, int64_t k) {
  int64_t num = 1;
  while (num > 0 && num < n) {
    num = num * k;
  }
  return num;
}

// Functions x crossing the enclave boundary, unit: BLOCK_DATA_SIZE
void opOneLinearScanBlock(int64_t index, int64_t* block, int64_t blockSize, int structureId, int write, int64_t dummyNum=0) {
  if (blockSize + dummyNum == 0) {
    return ;
  }
  if (dummyNum < 0) {
    printf("Dummy padding error!");
    return ;
  }
  int64_t boundary = (int64_t)((blockSize + BLOCK_DATA_SIZE - 1 )/ BLOCK_DATA_SIZE);
  int64_t Msize, i;
  int multi = structureSize[structureId] / sizeof(int64_t);
  if (!write) {
    // OcallReadBlock(index, block, blockSize * structureSize[structureId], structureId);
    for (i = 0; i < boundary; ++i) {
      Msize = std::min((int64_t)BLOCK_DATA_SIZE, blockSize - i * BLOCK_DATA_SIZE);
      OcallReadBlock(index + multi * i * BLOCK_DATA_SIZE, &block[i * BLOCK_DATA_SIZE * multi], Msize * structureSize[structureId], structureId);
    }
  } else {
    // OcallWriteBlock(index, block, blockSize * structureSize[structureId], structureId);
    for (i = 0; i < boundary; ++i) {
      Msize = std::min((int64_t)BLOCK_DATA_SIZE, blockSize - i * BLOCK_DATA_SIZE);
      OcallWriteBlock(index + multi * i * BLOCK_DATA_SIZE, &block[i * BLOCK_DATA_SIZE * multi], Msize * structureSize[structureId], structureId);
    }
    if (dummyNum > 0) {
      int64_t *junk = (int64_t*)malloc(dummyNum * multi * sizeof(int64_t));
      for (int64_t j = 0; j < dummyNum * multi; ++j) {
        junk[j] = DUMMY;
      }
      int64_t startIdx = index + multi * blockSize;
      boundary = ceil(1.0 * dummyNum / BLOCK_DATA_SIZE);
      for (int64_t j = 0; j < boundary; ++j) {
        Msize = std::min((int64_t)BLOCK_DATA_SIZE, dummyNum - j * BLOCK_DATA_SIZE);
        OcallWriteBlock(startIdx + multi * j * BLOCK_DATA_SIZE, &junk[j * BLOCK_DATA_SIZE * multi], Msize * structureSize[structureId], structureId);
      }
    }
  }
  return;
}

bool cmpHelper(int64_t *a, int64_t *b) {
  return (*a > *b) ? true : false;
}

bool cmpHelper(Bucket_x *a, Bucket_x *b) {
  return (a->x > b->x) ? true : false;
}

int64_t moveDummy(int64_t *a, int64_t size) {
  // k: #elem != DUMMY
  int64_t k = 0;
  for (int64_t i = 0; i < size; ++i) {
    if (a[i] != DUMMY) {
      if (i != k) {
        swapRow(&a[i], &a[k++]);
      } else {
        k++;
      }
    }
  }
  return k;
}

void swapRow(int64_t *a, int64_t *b) {
  int64_t *temp = (int64_t*)malloc(sizeof(int64_t));
  memmove(temp, a, sizeof(int64_t));
  memmove(a, b, sizeof(int64_t));
  memmove(b, temp, sizeof(int64_t));
  free(temp);
}

void swapRow(Bucket_x *a, Bucket_x *b) {
  Bucket_x *temp = (Bucket_x*)malloc(sizeof(Bucket_x));
  memmove(temp, a, sizeof(Bucket_x));
  memmove(a, b, sizeof(Bucket_x));
  memmove(b, temp, sizeof(Bucket_x));
  free(temp);
}

