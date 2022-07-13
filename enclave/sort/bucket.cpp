#include "bucket.h"
#include "quick.h"

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

void bucketSort(int inputStructureId, int bucketId, int size, int dataStart) {
  Bucket_x *arr = (Bucket_x*)oe_malloc(BUCKET_SIZE * sizeof(Bucket_x));
  opOneLinearScanBlock(2 * dataStart, (int*)arr, (size_t)size, inputStructureId, 0);
  quickSort(arr, 0, size - 1);
  opOneLinearScanBlock(2 * dataStart, (int*)arr, (size_t)size, inputStructureId, 1);
  oe_free(arr);
}

// int inputTrustMemory[BLOCK_DATA_SIZE];
int bucketOSort(int structureId, int size) {
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
  int *inputTrustMemory = (int*)oe_malloc(BLOCK_DATA_SIZE * sizeof(int));
  int total = 0;
  int offset;

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
  oe_free(trustedMemory);
  oe_free(inputTrustMemory);

  for (int i = 0; i < bucketNum; ++i) {
    // DBGprint("currently bucket %d has %d records/%d", i, numRow1[i], BUCKET_SIZE);
    padWithDummy(structureId, bucketAddr[i], numRow1[i]);    
  }

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
  
  int resultId = 0;
  if (ranBinAssignIters % 2 == 0) {
    for (int i = 0; i < bucketNum; ++i) {
      bucketSort(structureId, i, numRow1[i], bucketAddr[i]);
    }
    kWayMergeSort(structureId, structureId + 1, numRow1, numRow2, bucketAddr, bucketNum);
    
    resultId = structureId + 1;
  } else {
    for (int i = 0; i < bucketNum; ++i) {
      bucketSort(structureId + 1, i, numRow2[i], bucketAddr[i]);
    }
    kWayMergeSort(structureId + 1, structureId, numRow2, numRow1, bucketAddr, bucketNum);
    resultId = structureId;
  }
  oe_free(bucketAddr);
  oe_free(numRow1);
  oe_free(numRow2);
  return resultId;
}
