#include "bucket.h"
#include "quick.h"

void mergeSplitHelper(Bucket_x *inputBuffer, int inputBufferLen, int* numRow2, int* outputId, int iter, int k, int* bucketAddr, int outputStructureId) {
  // step1. malloc k bucket size memory
  Bucket_x **buf = (Bucket_x**)oe_malloc(k * sizeof(Bucket_x*));
  for (int i = 0; i < k; ++i) {
    buf[i] = (Bucket_x*)oe_malloc(MERGE_SORT_BATCH_SIZE * sizeof(Bucket_x));
  }
  // step2. set up counters for each bucket
  int randomKey = 0;
  int *counter = (int*)oe_malloc(k * sizeof(int));
  memset(counter, 0, k * sizeof(int));
  // step3. assign to certain bucket
  for (int i = 0; i < inputBufferLen; ++i) {
    if ((inputBuffer[i].key != DUMMY) && (inputBuffer[i].x != DUMMY)) {
      randomKey = inputBuffer[i].key;
      for (int j = 0; j < k; ++j) {
        if (isTargetIterK(randomKey, iter, k, j)) {
          buf[j][counter[j] % MERGE_SORT_BATCH_SIZE] = inputBuffer[i];
          counter[j]++;
          if (counter[j] % MERGE_SORT_BATCH_SIZE == 0) {
            opOneLinearScanBlock(2 * (bucketAddr[outputId[j]] +  numRow2[outputId[j]]), (int*)buf[j], (size_t)MERGE_SORT_BATCH_SIZE, outputStructureId, 1);
            numRow2[outputId[j]] += MERGE_SORT_BATCH_SIZE;
          }
        }
      }
    }
  }
  // step4. write back the rest
  for (int j = 0; j < k; ++j) {
    opOneLinearScanBlock(2 * (bucketAddr[outputId[j]] + numRow2[outputId[j]]), (int*)buf[j], (size_t)(counter[j] % MERGE_SORT_BATCH_SIZE), outputStructureId, 1);
    numRow2[outputId[j]] += counter[j] % MERGE_SORT_BATCH_SIZE;
    oe_free(buf[j]);
  }
  oe_free(counter);
}

void mergeSplit(int inputStructureId, int outputStructureId, int *inputId, int *outputId, int k, int* bucketAddr, int* numRow1, int* numRow2, int iter) {
  // step1. Read k buckets together
  // TODO: change memory bounded by MEM_IN_ENCLAVE
  Bucket_x *inputBuffer = (Bucket_x*)oe_malloc(k * sizeof(Bucket_x) * BUCKET_SIZE);
  for (int i = 0; i < k; ++i) {
    opOneLinearScanBlock(2 * bucketAddr[inputId[i]], (int*)(&inputBuffer[i * BUCKET_SIZE]), BUCKET_SIZE, inputStructureId, 0);
  }
  // step2. process k buckets
  for (int i = 0; i < k; ++i) {
    mergeSplitHelper(&inputBuffer[i * BUCKET_SIZE], numRow1[inputId[i]], numRow2, outputId, iter, k, bucketAddr, outputStructureId);
    for (int j = 0; j < k; ++j) {
      if (numRow2[outputId[j]] > BUCKET_SIZE) {
        // printf("overflow error during merge split!\n");
      }
    }
  }
  
  for (int j = 0; j < k; ++j) {
    padWithDummy(outputStructureId, bucketAddr[outputId[j]], numRow2[outputId[j]]);
  }
  
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
  int k = M / BUCKET_SIZE;
  int bucketNum = smallestPowerOfKLargerThan(ceil(2.0 * size / BUCKET_SIZE), k);
  int ranBinAssignIters = log(bucketNum)/log(k) - 1;
  // std::cout << "Iteration times: " << ranBinAssignIters << std::endl;
  int *bucketAddr = (int*)malloc(bucketNum * sizeof(int));
  int *numRow1 = (int*)malloc(bucketNum * sizeof(int));
  int *numRow2 = (int*)malloc(bucketNum * sizeof(int));
  memset(numRow1, 0, bucketNum * sizeof(int));
  memset(numRow2, 0, bucketNum * sizeof(int));
  
  for (int i = 0; i < bucketNum; ++i) {
    bucketAddr[i] = i * BUCKET_SIZE;
  }
  
  Bucket_x *trustedMemory = (Bucket_x*)malloc(M * sizeof(Bucket_x));
  int *inputTrustMemory = (int*)malloc(M * sizeof(int));
  int total = 0;
  int offset;

  for (int i = 0; i < size; i += M) {
    opOneLinearScanBlock(i, inputTrustMemory, std::min(M, size - i), structureId - 1, 0);
    int randomKey;
    for (int j = 0; j < std::min(M, size - i); ++j) {
      // oe_random(&randomKey, 4);
      randomKey = (int)rand();
      trustedMemory[j].x = inputTrustMemory[j];
      trustedMemory[j].key = randomKey;
      
      offset = bucketAddr[(i + j) % bucketNum] + numRow1[(i + j) % bucketNum];
      opOneLinearScanBlock(offset * 2, (int*)(&trustedMemory[j]), (size_t)1, structureId, 1);
      numRow1[(i + j) % bucketNum] ++;
    }
    total += std::min(M, size - i);
  }
  free(trustedMemory);
  free(inputTrustMemory);

  for (int i = 0; i < bucketNum; ++i) {
    // DBGprint("currently bucket %d has %d records/%d", i, numRow1[i], BUCKET_SIZE);
    padWithDummy(structureId, bucketAddr[i], numRow1[i]);
  }
  int *inputId = (int*)malloc(k * sizeof(int));
  int *outputId = (int*)malloc(k *sizeof(int));
  
  for (int i = 0; i < ranBinAssignIters; ++i) {
    if (i % 2 == 0) {
      for (int j = 0; j < bucketNum / k; ++j) {
        int jj = (j / (int)pow(k, i)) * (int)pow(k, i);
        for (int m = 0; m < k; ++m) {
          inputId[m] = j + jj + m * (int)pow(k, i);
          outputId[m] = k * j + m;
        }
        
        mergeSplit(structureId, structureId + 1, inputId, outputId, k, bucketAddr, numRow1, numRow2, i);
      }
      int count = 0;
      for (int k = 0; k < bucketNum; ++k) {
        numRow1[k] = 0;
        count += numRow2[k];
      }
      // printf("after %dth merge split, we have %d tuples\n", i, count);
    } else {
      for (int j = 0; j < bucketNum / k; ++j) {
        int jj = (j / (int)pow(k, i)) * (int)pow(k, i);
        for (int m = 0; m < k; ++m) {
          inputId[m] = j + jj + m * (int)pow(k, i);
          outputId[m] = k * j + m;
        }
        mergeSplit(structureId + 1, structureId, inputId, outputId, k, bucketAddr, numRow2, numRow1, i);
      }
      int count = 0;
      for (int k = 0; k < bucketNum; ++k) {
        numRow2[k] = 0;
        count += numRow1[k];
      }
      // printf("after %dth merge split, we have %d tuples\n", i, count);
    }
    // printf("\n\n Finish random bin assignment iter%dth out of %d\n\n", i, ranBinAssignIters);
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
  // test(arrayAddr, resultId, N);
  // print(arrayAddr, resultId, N);
  free(bucketAddr);
  free(numRow1);
  free(numRow2);
  return resultId;
}
