#include "bucket.h"
#include "quick.h"

void mergeSplitHelper(Bucket_x *inputBuffer, int* numRow1, int* numRow2, int* inputId, int* outputId, int iter, int k, int* bucketAddr, int outputStructureId) {
  // int batchSize = BUCKET_SIZE; // 8192
  // TODO: FREE these malloc
  Bucket_x **buf = (Bucket_x**)malloc(k * sizeof(Bucket_x*));
  for (int i = 0; i < k; ++i) {
    buf[i] = (Bucket_x*)malloc(BUCKET_SIZE * sizeof(Bucket_x));
  }
  
  // int counter0 = 0, counter1 = 0;
  int randomKey;
  int *counter = (int*)malloc(k * sizeof(int));
  memset(counter, 0, k * sizeof(int));
  
  for (int i = 0; i < k * BUCKET_SIZE; ++i) {
    if ((inputBuffer[i].key != DUMMY) && (inputBuffer[i].x != DUMMY)) {
      randomKey = inputBuffer[i].key;
      for (int j = 0; j < k; ++j) {
        if (isTargetIterK(randomKey, iter, k, j)) {
          buf[j][counter[j] % BUCKET_SIZE] = inputBuffer[i];
          counter[j]++;
          // std::cout << "couter j: " << counter[j] << std::endl;
          if (counter[j] % BUCKET_SIZE == 0) {
            opOneLinearScanBlock(2 * (bucketAddr[outputId[j]] +  numRow2[outputId[j]]), (int*)buf[j], (size_t)BUCKET_SIZE, outputStructureId, 1);
            numRow2[outputId[j]] += BUCKET_SIZE;
          }
        }
      }
    }
  }
  
  for (int j = 0; j < k; ++j) {
    opOneLinearScanBlock(2 * (bucketAddr[outputId[j]] + numRow2[outputId[j]]), (int*)buf[j], (size_t)(counter[j] % BUCKET_SIZE), outputStructureId, 1);
    numRow2[outputId[j]] += counter[j] % BUCKET_SIZE;
    padWithDummy(outputStructureId, bucketAddr[outputId[j]], numRow2[outputId[j]]);
    if (numRow2[outputId[j]] > BUCKET_SIZE) {
      printf("overflow error during merge split!\n");
    }
    free(buf[j]);
  }
  free(buf);
  free(counter);
}

void mergeSplit(int inputStructureId, int outputStructureId, int *inputId, int *outputId, int k, int* bucketAddr, int* numRow1, int* numRow2, int iter) {
  // step1. Read k buckets together
  Bucket_x *inputBuffer = (Bucket_x*)malloc(k * sizeof(Bucket_x) * BUCKET_SIZE);
  for (int i = 0; i < k; ++i) {
    opOneLinearScanBlock(2 * bucketAddr[inputId[i]], (int*)(&inputBuffer[i * BUCKET_SIZE]), BUCKET_SIZE, inputStructureId, 0);
  }
  // step2. process k buckets
  mergeSplitHelper(inputBuffer, numRow1, numRow2, inputId, outputId, iter, k, bucketAddr, outputStructureId);
  free(inputBuffer);
  
}



void kWayMergeSort(int inputStructureId, int outputStructureId, int* numRow1, int* numRow2, int* bucketAddr, int bucketSize) {
  // int mergeSortBatchSize = HEAP_NODE_SIZE; // 256
  // int writeBufferSize = (int)WRITE_BUFFER_SIZE; // 8192
  int numWays = bucketSize;
  HeapNode inputHeapNodeArr[numWays];
  int totalCounter = 0;
  
  int *readBucketAddr = (int*)malloc(sizeof(int) * numWays);
  memcpy(readBucketAddr, bucketAddr, sizeof(int) * numWays);
  int writeBucketAddr = 0;
  int j = 0;
  
  for (int i = 0; i < numWays; ++i) {
    // TODO: 数据0跳过
    if (numRow1[i] == 0) {
      continue;
    }
    HeapNode node;
    node.data = (Bucket_x*)malloc(HEAP_NODE_SIZE * sizeof(Bucket_x));
    node.bucketIdx = i;
    node.elemIdx = 0;
    opOneLinearScanBlock(2 * readBucketAddr[i], (int*)node.data, (size_t)std::min(HEAP_NODE_SIZE, numRow1[i]), inputStructureId, 0);
    inputHeapNodeArr[j++] = node;
    readBucketAddr[i] += std::min(HEAP_NODE_SIZE, numRow1[i]);
  }
  
  Heap heap(inputHeapNodeArr, j, HEAP_NODE_SIZE);
  Bucket_x *writeBuffer = (Bucket_x*)malloc(WRITE_BUFFER_SIZE * sizeof(Bucket_x));
  int writeBufferCounter = 0;

  while (1) {
    HeapNode *temp = heap.getRoot();
    memcpy(writeBuffer + writeBufferCounter, temp->data + temp->elemIdx % HEAP_NODE_SIZE, sizeof(Bucket_x));
    writeBufferCounter ++;
    totalCounter ++;
    temp->elemIdx ++;
    
    if (writeBufferCounter == WRITE_BUFFER_SIZE) {
      opOneLinearScanBlock(2 * writeBucketAddr, (int*)writeBuffer, (size_t)WRITE_BUFFER_SIZE, outputStructureId, 1);
      writeBucketAddr += WRITE_BUFFER_SIZE;
      numRow2[temp->bucketIdx] += WRITE_BUFFER_SIZE;
      writeBufferCounter = 0;
      // print(arrayAddr, outputStructureId, numWays * BUCKET_SIZE);
    }
    
    if (temp->elemIdx < numRow1[temp->bucketIdx] && (temp->elemIdx % HEAP_NODE_SIZE) == 0) {
      opOneLinearScanBlock(2 * readBucketAddr[temp->bucketIdx], (int*)(temp->data), (size_t)std::min(HEAP_NODE_SIZE, numRow1[temp->bucketIdx]-temp->elemIdx), inputStructureId, 0);
      
      readBucketAddr[temp->bucketIdx] += std::min(HEAP_NODE_SIZE, numRow1[temp->bucketIdx]-temp->elemIdx);
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
  // TODO: ERROR writeBuffer
  free(writeBuffer);
  free(readBucketAddr);
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
  int k = FAN_OUT;
  int bucketNum = smallestPowerOfKLargerThan(ceil(2.0 * size / BUCKET_SIZE), k);
  if ((2 * k * BUCKET_SIZE + bucketNum * 3 + k * 2 * BUCKET_SIZE > M) || (3 * bucketNum + bucketNum * HEAP_NODE_SIZE * 2 + 2 * WRITE_BUFFER_SIZE> M)) {
    int maxM = std::max(2 * k * BUCKET_SIZE + bucketNum * 3 + k * 2 * BUCKET_SIZE, 3 * bucketNum + bucketNum * HEAP_NODE_SIZE * 2 + 2 * WRITE_BUFFER_SIZE);
    // // printf("Memory %d bytes exceeds.\n", maxM);
  }
  int ranBinAssignIters = log(bucketNum)/log(k) - 1;
  // std::cout << "Iteration times: " << log(bucketNum)/log(k) << std::endl;
  srand((unsigned)time(NULL));
  // // std::cout << "Iters:" << ranBinAssignIters << std::endl;
  int *bucketAddr = (int*)malloc(bucketNum * sizeof(int));
  int *numRow1 = (int*)malloc(bucketNum * sizeof(int));
  int *numRow2 = (int*)malloc(bucketNum * sizeof(int));
  memset(numRow1, 0, bucketNum * sizeof(int));
  memset(numRow2, 0, bucketNum * sizeof(int));
  
  for (int i = 0; i < bucketNum; ++i) {
    bucketAddr[i] = i * BUCKET_SIZE;
  }
  
  Bucket_x *trustedMemory = (Bucket_x*)malloc(BLOCK_DATA_SIZE * sizeof(Bucket_x));
  int *inputTrustMemory = (int*)malloc(BLOCK_DATA_SIZE * sizeof(int));
  int total = 0;
  int offset;

  for (int i = 0; i < size; i += BLOCK_DATA_SIZE) {
    opOneLinearScanBlock(i, inputTrustMemory, std::min(BLOCK_DATA_SIZE, size - i), structureId - 1, 0);
    int randomKey;
    for (int j = 0; j < std::min(BLOCK_DATA_SIZE, size - i); ++j) {
      // randomKey = (int)oe_rdrand();
      randomKey = rand();
      trustedMemory[j].x = inputTrustMemory[j];
      trustedMemory[j].key = randomKey;
      
      offset = bucketAddr[(i + j) % bucketNum] + numRow1[(i + j) % bucketNum];
      opOneLinearScanBlock(offset * 2, (int*)(&trustedMemory[j]), (size_t)1, structureId, 1);
      numRow1[(i + j) % bucketNum] ++;
    }
    total += std::min(BLOCK_DATA_SIZE, size - i);
  }
  free(trustedMemory);
  free(inputTrustMemory);

  for (int i = 0; i < bucketNum; ++i) {
    //// // printf("currently bucket %d has %d records/%d\n", i, numRow1[i], BUCKET_SIZE);
    padWithDummy(structureId, bucketAddr[i], numRow1[i]);
  }
  // print(arrayAddr, structureId, bucketNum * BUCKET_SIZE);
  // // std::cout << "Iters:" << ranBinAssignIters << std::endl;
  // // std::cout << "k:" << k << std::endl;
  int *inputId = (int*)malloc(k * sizeof(int));
  int *outputId = (int*)malloc(k *sizeof(int));
  int outIdx = 0;
  
  for (int i = 0; i < ranBinAssignIters; ++i) {
    if (i % 2 == 0) {
      for (int j = 0; j < bucketNum / (int)pow(k, i+1); ++j) {
        // pass (i-1) * k^i
        //// // printf("j: %d\n", j);
        for (int jj = 0; jj < (int)pow(k, i); ++jj) {
          //// // printf("jj: %d\n", jj);
          for (int m = 0; m < k; ++m) {
            //// // printf("j, jj, m: %d, %d, %d\n", j, jj, m);
            inputId[m] = j * (int)pow(k, i+1)+ jj + m * (int)pow(k, i);
            outputId[m] = (outIdx * k + m) % bucketNum;
            //// // printf("input, output: %d, %d\n", inputId[m], outputId[m]);
          }
          mergeSplit(structureId, structureId + 1, inputId, outputId, k, bucketAddr, numRow1, numRow2, i);
          outIdx ++;
        }
      }
      int count = 0;
      for (int n = 0; n < bucketNum; ++n) {
        numRow1[n] = 0;
        count += numRow2[n];
      }
      // // printf("after %dth merge split, we have %d tuples\n", i, count);
      outIdx = 0;
      //print(arrayAddr, structureId + 1, bucketNum * BUCKET_SIZE);
    } else {
      for (int j = 0; j < bucketNum / (int)pow(k, i+1); ++j) {
        //// // printf("j: %d\n", j);
        for (int jj = 0; jj < (int)pow(k, i); ++jj) {
          //// // printf("jj: %d\n", jj);
          for (int m = 0; m < k; ++m) {
            //// // printf("j, jj, m: %d, %d, %d\n", j, jj, m);
            inputId[m] = j * (int)pow(k, i+1)+ jj + m * (int)pow(k, i);
            outputId[m] = (outIdx * k + m) % bucketNum;
            //// // printf("input, output: %d, %d\n", inputId[m], outputId[m]);
          }
          mergeSplit(structureId + 1, structureId, inputId, outputId, k, bucketAddr, numRow2, numRow1, i);
          outIdx ++;
        }
      }
      int count = 0;
      for (int n = 0; n < bucketNum; ++n) {
        numRow2[n] = 0;
        count += numRow1[n];
      }
      // // printf("after %dth merge split, we have %d tuples\n", i, count);
      outIdx = 0;
      //print(arrayAddr, structureId, bucketNum * BUCKET_SIZE);
    }
    // std::cout << "----------------------------------------\n";
    // // printf("\n\n Finish random bin assignment iter%dth out of %d\n\n", i, ranBinAssignIters);
    // std::cout << "----------------------------------------\n";
  }
  
  int resultId = 0;
  if (ranBinAssignIters % 2 == 0) {
    for (int i = 0; i < bucketNum; ++i) {
      bucketSort(structureId, i, numRow1[i], bucketAddr[i]);
    }
    //// std::cout << "********************************************\n";
    //print(arrayAddr, structureId, bucketNum * BUCKET_SIZE);
    
    kWayMergeSort(structureId, structureId + 1, numRow1, numRow2, bucketAddr, bucketNum);
    
    resultId = structureId + 1;
  } else {
    for (int i = 0; i < bucketNum; ++i) {
      bucketSort(structureId + 1, i, numRow2[i], bucketAddr[i]);
    }
    //// std::cout << "********************************************\n";
    //print(arrayAddr, structureId, bucketNum * BUCKET_SIZE);
    
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
