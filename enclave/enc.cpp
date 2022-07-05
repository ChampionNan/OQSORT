#include "enc.h"
#include <openenclave/enclave.h>
#include "./include/definitions.h"
#include "../enclave/include/common.h"
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
  Heap(HeapNode *a, int size, int bsize) {
    heapSize = size;
    harr = a;
    int i = (heapSize - 1) / 2;
    batchSize = bsize;
    while (i >= 0) {
      Heapify(i);
      i --;
    }
  }
  
  void Heapify(int i) {
    int l = left(i);
    int r = right(i);
    int target = i;
    
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
  
  int left(int i) {
    return (2 * i + 1);
  }
  
  int right (int i) {
    return (2 * i + 2);
  }
  
  void swapHeapNode(HeapNode *a, HeapNode *b) {
    HeapNode temp = *a;
    *a = *b;
    *b = temp;
  }
  
  HeapNode *getRoot() {
    return &harr[0];
  }
  
  int getHeapSize() {
    return heapSize;
  }
  
  bool reduceSizeByOne() {
    oe_free(harr[0].data);
    heapSize --;
    if (heapSize > 0) {
      harr[0] = harr[heapSize];
      Heapify(0);
      return true;
    } else {
      return false;
    }
  }
  
  void replaceRoot(HeapNode x) {
    harr[0] = x;
    Heapify(0);
  }
};

int paddedSize;
int sizeFlag; // represent which kind of data type
// int structureSize[NUM_STRUCTURES] = {4, 8, 8};

// Functions x crossing the enclave boundary
void opOneLinearScanBlock(int index, int* block, size_t blockSize, int structureId, int write) {
  if (!write) {
    // std::cout << "OcallReadBlock: " <<blockSize << " " << blockSize * structureSize[structureId] << std::endl; 
    OcallReadBlock(index, block, blockSize * structureSize[structureId], structureId);
    // OcallReadBlock(index, block, blockSize, structureId); 
  } else {
    // std::cout << "OcallWriteBlock: " <<blockSize << " " << blockSize * structureSize[structureId] << std::endl; 
    OcallWriteBlock(index, block, blockSize * structureSize[structureId], structureId);
    // OcallWriteBlock(index, block, blockSize, structureId);
  }
  return;
}

int greatestPowerOfTwoLessThan(int n) {
	int k = 1;
	while (k > 0 && k < n) {
		k = k << 1;
	}
	return k >> 1;
}

int smallestPowerOfTwoLargerThan(int n) {
  int k = 1;
  while (k > 0 && k < n) {
    k = k << 1;
  }
  return k;
}

void smallBitonicMerge(int *a, int start, int size, int flipped) {
  if (size == 1) {
    return;
  } else {
    int swap = 0;
    int mid = greatestPowerOfTwoLessThan(size);
    for (int i = 0; i < size - mid; ++i) {
      int num1 = a[start + i];
      int num2 = a[start + mid + i];
      swap = num1 > num2;
      swap = swap ^ flipped;
      a[start + i] = (!swap * num1) + (swap * num2);
      a[start + i + mid] = (swap * num1) + (!swap * num2);
    }
    smallBitonicMerge(a, start, mid, flipped);
    smallBitonicMerge(a, start + mid, size - mid, flipped);
  }
  return;
}

//Correct, after testing
void smallBitonicSort(int *a, int start, int size, int flipped) {
  if (size <= 1) {
    return ;
  } else {
    int mid = greatestPowerOfTwoLessThan(size);
    smallBitonicSort(a, start, mid, 1);
    smallBitonicSort(a, start + mid, size - mid, 0);
    smallBitonicMerge(a, start, size, flipped);
  }
  return;
}


void bitonicMerge(int structureId, int start, int size, int flipped, int* row1, int* row2) {
  if (size < 1) {
    return ;
  } else if (size < MEM_IN_ENCLAVE) {
    int *trustedMemory = (int*)oe_malloc(size * BLOCK_DATA_SIZE * sizeof(int));
    for (int i = 0; i < size; ++i) {
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, &trustedMemory[i * BLOCK_DATA_SIZE], BLOCK_DATA_SIZE, structureId, 0);
    }
    smallBitonicMerge(trustedMemory, 0, size * BLOCK_DATA_SIZE, flipped);
    for (int i = 0; i < size; ++i) {
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, &trustedMemory[i * BLOCK_DATA_SIZE], BLOCK_DATA_SIZE, structureId, 1);
    }
    oe_free(trustedMemory);
  } else {
    int swap = 0;
    int mid = greatestPowerOfTwoLessThan(size);
    for (int i = 0; i < size - mid; ++i) {
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, row1, BLOCK_DATA_SIZE, structureId, 0);
      opOneLinearScanBlock((start + mid + i) * BLOCK_DATA_SIZE, row2, BLOCK_DATA_SIZE, structureId, 0);
      int num1 = row1[0], num2 = row2[0];
      swap = num1 > num2;
      swap = swap ^ flipped;
      for (int j = 0; j < BLOCK_DATA_SIZE; ++j) {
        int v1 = row1[j];
        int v2 = row2[j];
        row1[j] = (!swap * v1) + (swap * v2);
        row2[j] = (swap * v1) + (!swap * v2);
      }
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, row1, BLOCK_DATA_SIZE, structureId, 1);
      opOneLinearScanBlock((start + mid + i) * BLOCK_DATA_SIZE, row2, BLOCK_DATA_SIZE, structureId, 1);
    }
    bitonicMerge(structureId, start, mid, flipped, row1, row2);
    bitonicMerge(structureId, start + mid, size - mid, flipped, row1, row2);
  }
  return;
}

void bitonicSort(int structureId, int start, int size, int flipped, int* row1, int* row2) {
  if (size < 1) {
    return;
  } else if (size < MEM_IN_ENCLAVE) {
    int *trustedMemory = (int*)oe_malloc(size * BLOCK_DATA_SIZE * sizeof(int));
    for (int i = 0; i < size; ++i) {
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, &trustedMemory[i * BLOCK_DATA_SIZE], BLOCK_DATA_SIZE, structureId, 0);
    }
    smallBitonicSort(trustedMemory, 0, size * BLOCK_DATA_SIZE, flipped);
    // write back
    for (int i = 0; i < size; ++i) {
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, &trustedMemory[i * BLOCK_DATA_SIZE], BLOCK_DATA_SIZE, structureId, 1);
    }
    oe_free(trustedMemory);
  } else {
    int mid = greatestPowerOfTwoLessThan(size);
    bitonicSort(structureId, start, mid, 1, row1, row2);
    bitonicSort(structureId, start + mid, size - mid, 0, row1, row2);
    bitonicMerge(structureId, start, size, flipped, row1, row2);
  }
  return;
}

void padWithDummy(int structureId, int start, int realNum) {
  int len = BUCKET_SIZE - realNum;
  if (len <= 0) {
    return ;
  }
  Bucket_x *junk = (Bucket_x*)oe_malloc(len * sizeof(Bucket_x));

  for (int i = 0; i < len; ++i) {
    junk[i].x = -1;
    junk[i].key = -1;
  }
  
  opOneLinearScanBlock(2 * (start + realNum), (int*)junk, len, structureId, 1);
  oe_free(junk);
}

bool isTargetBitOne(int randomKey, int iter) {
  assert(iter >= 1);
  return (randomKey & (0x01 << (iter - 1))) == 0 ? false : true;
}

void mergeSplitHelper(Bucket_x *inputBuffer, int inputBufferLen, int* numRow2, int outputId0, int outputId1, int iter, int* bucketAddr, int outputStructureId) {
  int batchSize = 256; // 8192
  Bucket_x *buf0 = (Bucket_x*)oe_malloc(batchSize * sizeof(Bucket_x));
  Bucket_x *buf1 = (Bucket_x*)oe_malloc(batchSize * sizeof(Bucket_x));
  int counter0 = 0, counter1 = 0;
  int randomKey = 0;
  
  for (int i = 0; i < inputBufferLen; ++i) {
    if ((inputBuffer[i].key != DUMMY) && (inputBuffer[i].x != DUMMY)) {
      randomKey = inputBuffer[i].key;
      
      if (isTargetBitOne(randomKey, iter + 1)) {
        buf1[counter1 % batchSize] = inputBuffer[i];
        counter1 ++;
        if (counter1 % batchSize == 0) {
          opOneLinearScanBlock(2 * (bucketAddr[outputId1] +  numRow2[outputId1]), (int*)buf1, (size_t)batchSize, outputStructureId, 1);
          numRow2[outputId1] += batchSize;
          memset(buf1, NULLCHAR, batchSize * sizeof(Bucket_x));
        }
      } else {
        buf0[counter0 % batchSize] = inputBuffer[i];
        counter0 ++;
        if (counter0 % batchSize == 0) {
          opOneLinearScanBlock(2 * (bucketAddr[outputId0] + numRow2[outputId0]), (int*)buf0, (size_t)batchSize, outputStructureId, 1);
          numRow2[outputId0] += batchSize;
          memset(buf0, NULLCHAR, batchSize * sizeof(Bucket_x));
        }
      }
    }
  }
  
  opOneLinearScanBlock(2 * (bucketAddr[outputId1] + numRow2[outputId1]), (int*)buf1, (size_t)(counter1 % batchSize), outputStructureId, 1);
  numRow2[outputId1] += counter1 % batchSize;
  opOneLinearScanBlock(2 * (bucketAddr[outputId0] + numRow2[outputId0]), (int*)buf0, (size_t)(counter0 % batchSize), outputStructureId, 1);
  numRow2[outputId0] += counter0 % batchSize;
  
  oe_free(buf0);
  oe_free(buf1);
}

void mergeSplit(int inputStructureId, int outputStructureId, int inputId0, int inputId1, int outputId0, int outputId1, int* bucketAddr, int* numRow1, int* numRow2, int iter) {
  Bucket_x *inputBuffer = (Bucket_x*)oe_malloc(sizeof(Bucket_x) * BUCKET_SIZE);
  // BLOCK#0
  opOneLinearScanBlock(2 * bucketAddr[inputId0], (int*)inputBuffer, BUCKET_SIZE, inputStructureId, 0);
  mergeSplitHelper(inputBuffer, numRow1[inputId0], numRow2, outputId0, outputId1, iter, bucketAddr, outputStructureId);
  if (numRow2[outputId0] > BUCKET_SIZE || numRow2[outputId1] > BUCKET_SIZE) {
    // DBGprint("overflow error during merge split!\n");
  }
  
  // BLOCK#1
  opOneLinearScanBlock(2 * bucketAddr[inputId1], (int*)inputBuffer, BUCKET_SIZE, inputStructureId, 0);
  mergeSplitHelper(inputBuffer, numRow1[inputId1], numRow2, outputId0, outputId1, iter, bucketAddr, outputStructureId);
  
  if (numRow2[outputId0] > BUCKET_SIZE || numRow2[outputId1] > BUCKET_SIZE) {
    // DBGprint("overflow error during merge split!\n");
  }
  
  padWithDummy(outputStructureId, bucketAddr[outputId1], numRow2[outputId1]);
  padWithDummy(outputStructureId, bucketAddr[outputId0], numRow2[outputId0]);
  
  oe_free(inputBuffer);
}

void kWayMergeSort(int inputStructureId, int outputStructureId, int* numRow1, int* numRow2, int* bucketAddr, int bucketSize) {
  int mergeSortBatchSize = (int)MERGE_SORT_BATCH_SIZE; // 256
  int writeBufferSize = (int)WRITE_BUFFER_SIZE; // 8192
  int numWays = bucketSize;
  HeapNode inputHeapNodeArr[numWays];
  int totalCounter = 0;
  
  int *readBucketAddr = (int*)oe_malloc(sizeof(int) * numWays);
  memcpy(readBucketAddr, bucketAddr, sizeof(int) * numWays);
  int writeBucketAddr = 0;
  
  for (int i = 0; i < numWays; ++i) {
    HeapNode node;
    node.data = (Bucket_x*)oe_malloc(mergeSortBatchSize * sizeof(Bucket_x));
    node.bucketIdx = i;
    node.elemIdx = 0;
    opOneLinearScanBlock(2 * readBucketAddr[i], (int*)node.data, (size_t)std::min(mergeSortBatchSize, numRow1[i]), inputStructureId, 0);
    inputHeapNodeArr[i] = node;
    readBucketAddr[i] += std::min(mergeSortBatchSize, numRow1[i]);
  }
  
  Heap heap(inputHeapNodeArr, numWays, mergeSortBatchSize);
  Bucket_x *writeBuffer = (Bucket_x*)oe_malloc(writeBufferSize * sizeof(Bucket_x));
  int writeBufferCounter = 0;

  while (1) {
    HeapNode *temp = heap.getRoot();
    memcpy(writeBuffer + writeBufferCounter, temp->data + temp->elemIdx % mergeSortBatchSize, sizeof(Bucket_x));
    writeBufferCounter ++;
    totalCounter ++;
    temp->elemIdx ++;
    
    if (writeBufferCounter == writeBufferSize) {
      opOneLinearScanBlock(2 * writeBucketAddr, (int*)writeBuffer, (size_t)writeBufferSize, outputStructureId, 1);
      writeBucketAddr += writeBufferSize;
      numRow2[temp->bucketIdx] += writeBufferSize;
      writeBufferCounter = 0;
    }
    
    if (temp->elemIdx < numRow1[temp->bucketIdx] && (temp->elemIdx % mergeSortBatchSize) == 0) {
      opOneLinearScanBlock(2 * readBucketAddr[temp->bucketIdx], (int*)(temp->data), (size_t)std::min(mergeSortBatchSize, numRow1[temp->bucketIdx]-temp->elemIdx), inputStructureId, 0);
      
      readBucketAddr[temp->bucketIdx] += std::min(mergeSortBatchSize, numRow1[temp->bucketIdx]-temp->elemIdx);
      heap.Heapify(0);
      
    } else if (temp->elemIdx >= numRow1[temp->bucketIdx]) {
      bool res = heap.reduceSizeByOne();
      if (!res) {
        break;
      }
    } else {
      heap.Heapify(0);
    }
  }
  opOneLinearScanBlock(2 * writeBucketAddr, (int*)writeBuffer, (size_t)writeBufferCounter, outputStructureId, 1);
  numRow2[0] += writeBufferCounter;
  oe_free(writeBuffer);
  oe_free(readBucketAddr);
}

void swapRow(Bucket_x *a, Bucket_x *b) {
  Bucket_x *temp = (Bucket_x*)oe_malloc(sizeof(Bucket_x));
  memmove(temp, a, sizeof(Bucket_x));
  memmove(a, b, sizeof(Bucket_x));
  memmove(b, temp, sizeof(Bucket_x));
  oe_free(temp);
}

bool cmpHelper(Bucket_x *a, Bucket_x *b) {
  return (a->x > b->x) ? true : false;
}

int partition(Bucket_x *arr, int low, int high) {
  Bucket_x *pivot = arr + high;
  int i = low - 1;
  for (int j = low; j <= high - 1; ++j) {
    if (cmpHelper(pivot, arr + j)) {
      i++;
      if (i != j) {
        swapRow(arr + i, arr + j);
      }
    }
  }
  if (i + 1 != high) {
    swapRow(arr + i + 1, arr + high);
  }
  return (i + 1);
}

void quickSort(Bucket_x *arr, int low, int high) {
  if (high > low) {
    int mid = partition(arr, low, high);
    quickSort(arr, low, mid - 1);
    quickSort(arr, mid + 1, high);
  }
}

void bucketSort(int inputStructureId, int bucketId, int size, int dataStart) {
  Bucket_x *arr = (Bucket_x*)oe_malloc(BUCKET_SIZE * sizeof(Bucket_x));
  opOneLinearScanBlock(2 * dataStart, (int*)arr, (size_t)size, inputStructureId, 0);
  quickSort(arr, 0, size - 1);
  opOneLinearScanBlock(2 * dataStart, (int*)arr, (size_t)size, inputStructureId, 1);
  oe_free(arr);
}

int inputTrustMemory[BLOCK_DATA_SIZE];

int bucketOSort(int structureId, int size) {
  // DBGprint("1\n");
  int bucketNum = smallestPowerOfTwoLargerThan(ceil(2.0 * size / BUCKET_SIZE));
  int ranBinAssignIters = log2(bucketNum) - 1;

  int *bucketAddr = (int*)oe_malloc(bucketNum * sizeof(int));
  int *numRow1 = (int*)oe_malloc(bucketNum * sizeof(int));
  int *numRow2 = (int*)oe_malloc(bucketNum * sizeof(int));
  memset(numRow1, 0, bucketNum * sizeof(int));
  memset(numRow2, 0, bucketNum * sizeof(int)); 
  
  for (int i = 0; i < bucketNum; ++i) {
    bucketAddr[i] = i * BUCKET_SIZE;
  }
  
  Bucket_x *trustedMemory = (Bucket_x*)oe_malloc(BLOCK_DATA_SIZE * sizeof(Bucket_x));
  // int *inputTrustMemory = (int*)oe_malloc(BLOCK_DATA_SIZE * sizeof(int));
  int total = 0;
  int offset;
  // DBGprint("2\n");
  for (int i = 0; i < size; i += BLOCK_DATA_SIZE) {
    opOneLinearScanBlock(i, inputTrustMemory, std::min(BLOCK_DATA_SIZE, size - i), structureId - 1, 0);
    int randomKey;
    for (int j = 0; j < std::min(BLOCK_DATA_SIZE, size - i); ++j) {
      // oe_random(&randomKey, 4);
      randomKey = (int)oe_rdrand();
      trustedMemory[j].x = inputTrustMemory[j];
      trustedMemory[j].key = randomKey;
      
      offset = bucketAddr[(i + j) % bucketNum] + numRow1[(i + j) % bucketNum];
      opOneLinearScanBlock(offset * 2, (int*)(&trustedMemory[j]), (size_t)1, structureId, 1);
      numRow1[(i + j) % bucketNum] ++;
    }
    total += std::min(BLOCK_DATA_SIZE, size - i);
  }
  // DBGprint("3\n");
  
  // oe_free(inputTrustMemory);
  for (int i = 0; i < bucketNum; ++i) {
    // DBGprint("currently bucket %d has %d records/%d", i, numRow1[i], BUCKET_SIZE);
    padWithDummy(structureId, bucketAddr[i], numRow1[i]);    
  }
  // DBGprint("4\n");
  for (int i = 0; i < ranBinAssignIters; ++i) {
    if (i % 2 == 0) {
      for (int j = 0; j < bucketNum / 2; ++j) {
        int jj = (j / (int)pow(2, i)) * (int)pow(2, i);
        mergeSplit(structureId, structureId + 1, j + jj, j + jj + (int)pow(2, i), 2 * j, 2 * j + 1, bucketAddr, numRow1, numRow2, i);
      }
      int count = 0;
      for (int k = 0; k < bucketNum; ++k) {
        numRow1[k] = 0;
        count += numRow2[k];
      }
      // DBGprint("after %dth merge split, we have %d tuples\n", i, count);
    } else {
      for (int j = 0; j < bucketNum / 2; ++j) {
        int jj = (j / (int)pow(2, i)) * (int)pow(2, i);
        mergeSplit(structureId + 1, structureId, j + jj, j + jj + (int)pow(2, i), 2 * j, 2 * j + 1, bucketAddr, numRow2, numRow1, i);
      }
      int count = 0;
      for (int k = 0; k < bucketNum; ++k) {
        numRow2[k] = 0;
        count += numRow1[k];
      }
      // DBGprint("after %dth merge split, we have %d tuples\n", i, count);
    }
    // DBGprint("\n\n Finish random bin assignment iter%dth out of %d\n\n", i, ranBinAssignIters);
  }
  // DBGprint("5\n");
  int resultId = 0;
  if (ranBinAssignIters % 2 == 0) {
    for (int i = 0; i < bucketNum; ++i) {
      bucketSort(structureId, i, numRow1[i], bucketAddr[i]);
    }
    kWayMergeSort(structureId, structureId + 1, numRow1, numRow2, bucketAddr, bucketNum);
    
    // std::cout<<("hello");

    // fprintf(stdout, "%s: Line %d:\t", __FILE__, __LINE__);
    resultId = structureId + 1;
  } else {
    for (int i = 0; i < bucketNum; ++i) {
      bucketSort(structureId + 1, i, numRow2[i], bucketAddr[i]);
    }
    kWayMergeSort(structureId + 1, structureId, numRow2, numRow1, bucketAddr, bucketNum);
    resultId = structureId;
  }
  // paddedSize = N;
  // test(resultId, paddedSize);
  // std::cout<<"5\n";
  oe_free(trustedMemory);
  oe_free(bucketAddr);
  oe_free(numRow1);
  oe_free(numRow2);
  // std::cout << "6\n";
  return resultId;
}

// trusted function
void callSort(int sortId, int structureId, int paddedSize, int *resId) {
  // bitonic sort
  // printf("size: %d\n", paddedSize);
  if (sortId == 1) {
    // DBGprint("Before call");
    sizeFlag = 2;
     *resId = bucketOSort(structureId, paddedSize);
  }
  if (sortId == 3) {
    sizeFlag = 1;
    int size = paddedSize / BLOCK_DATA_SIZE;
    int *row1 = (int*)oe_malloc(BLOCK_DATA_SIZE * sizeof(int));
    int *row2 = (int*)oe_malloc(BLOCK_DATA_SIZE * sizeof(int));
    bitonicSort(structureId, 0, size, 0, row1, row2);
    oe_free(row1);
    oe_free(row2);
    // return -1;
  }
  // return -1;
}

// trusted function
void callSmallSort(int *a, size_t dataSize) {
  int size = (int)dataSize;
  smallBitonicSort(a, 0, size, 0);
  return;
}
