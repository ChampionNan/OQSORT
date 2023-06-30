#include "bucket.h"
#include "quick.h"

Bucket::Bucket(EnclaveServer &eServer, int inStructureId) : eServer{eServer}, inStructureId{inStructureId} {
  N = eServer.N;
  M = eServer.M;
  B = eServer.B;
  outId1 = inStructureId + 1;
  outId2 = inStructureId + 2;
  // Init parameters
  double kappa = 1.0 * eServer.sigma * 0.693147;
  double Zd = 6 * (kappa + log(2.0 * N));
  Zd = 6 * (kappa + log(2.0 * N / Zd));
  Zd = ceil(Zd / B) * B;
  double thresh = 1.0 * M / Zd;
  FAN_OUT = eServer.greatestPowerOfTwoLessThan(thresh)/2;
  bucketNum = eServer.smallestPowerOfKLargerThan(ceil(2.0 * N / Zd), 2);
  HEAP_NODE_SIZE = fmax(floor(1.0 * M / (bucketNum + 1)), 1);
  MERGE_BATCH_SIZE = HEAP_NODE_SIZE;
  k1 = log2(FAN_OUT);
  oldK = k1;
  ranBinAssignIters = ceil(1.0*log2(bucketNum)/log2(FAN_OUT));
  Z = (int64_t)Zd; // set Z
}

bool Bucket::isTargetIterK(int randomKey, int iter, int64_t k, int64_t num) {
  randomKey = (randomKey >> iter * oldK);
  return (randomKey % k) == num;
}

void Bucket::mergeSplitHelper(EncOneBlock *inputBuffer, int64_t* numRow1, int64_t* numRow2, int64_t* inputId, int64_t* outputId, int iter, int64_t k, int64_t* bucketAddr, int outputStructureId) {
  EncOneBlock **buf = new EncOneBlock *[k];
  for (int64_t i = 0; i < k; ++i) {
    buf[i] = new EncOneBlock[Z];
  }
  // int counter0 = 0, counter1 = 0;
  int randomKey;
  int64_t *counter = new int64_t[k];
  memset(counter, 0, k * sizeof(int64_t));
  // printf("Before assign bins!\n");
  for (int64_t i = 0; i < k * Z; ++i) {
    if (inputBuffer[i].sortKey != DUMMY<int>()) {
      randomKey = inputBuffer[i].randomKey;
      for (int64_t j = 0; j < k; ++j) {
        if (isTargetIterK(randomKey, iter, k, j)) {
          buf[j][counter[j] % Z] = inputBuffer[i];
          counter[j]++;
          // std::cout << "couter j: " << counter[j] << std::endl;
          if (counter[j] % Z == 0) {
            eServer.opOneLinearScanBlock(bucketAddr[outputId[j]] +  numRow2[outputId[j]], buf[j], Z, outputStructureId, 1, 0);
            numRow2[outputId[j]] += Z;
          }
        }
      }
    }
  }
  // printf("Before final assign!\n");
  for (int64_t j = 0; j < k; ++j) {
    if (numRow2[outputId[j]] + (counter[j] % Z) > Z) {
      printf("overflow error during merge split!\n");
    }
    eServer.opOneLinearScanBlock(bucketAddr[outputId[j]] + numRow2[outputId[j]], buf[j], counter[j] % Z, outputStructureId, 1, Z-numRow2[outputId[j]]-(counter[j] % Z));
    numRow2[outputId[j]] += counter[j] % Z;
    delete [] buf[j];
  }
  delete [] counter;
  delete [] buf;
}

void Bucket::mergeSplit(int inputStructureId, int outputStructureId, int64_t *inputId, int64_t *outputId, int64_t k, int64_t* bucketAddr, int64_t* numRow1, int64_t* numRow2, int iter) {
  // step1. Read k buckets together
  // printf("In mergesplit!\n");
  EncOneBlock *inputBuffer = new EncOneBlock[k * Z];
  for (int64_t i = 0; i < k; ++i) {
    eServer.opOneLinearScanBlock(bucketAddr[inputId[i]], &inputBuffer[i * Z], Z, inputStructureId, 0, 0);
  }
  // step2. process k buckets
  mergeSplitHelper(inputBuffer, numRow1, numRow2, inputId, outputId, iter, k, bucketAddr, outputStructureId);
  delete [] inputBuffer;
  // printf("Finish mergesplit!\n");
}

void Bucket::kWayMergeSort(int inputStructureId, int outputStructureId, int64_t* numRow1, int64_t* bucketAddr, int64_t bucketNum) {
  HeapNode *inputHeapNodeArr = new HeapNode[bucketNum];
  // int totalCounter = 0;
  int64_t *readBucketAddr = new int64_t[bucketNum];
  memcpy(readBucketAddr, bucketAddr, sizeof(int64_t) * bucketNum);
  int64_t writeBucketAddr = 0;
  int64_t j = 0;
  for (int64_t i = 0; i < bucketNum; ++i) {
    if (numRow1[i] == 0) {
      continue; // only for small input
    }
    HeapNode node;
    node.data = new EncOneBlock[HEAP_NODE_SIZE];
    node.bucketIdx = i;
    node.elemIdx = 0;
    eServer.opOneLinearScanBlock(readBucketAddr[i], node.data, std::min((int64_t)HEAP_NODE_SIZE, numRow1[i]), inputStructureId, 0, 0);
    inputHeapNodeArr[j++] = node;
    readBucketAddr[i] += std::min((int64_t)HEAP_NODE_SIZE, numRow1[i]);
  }
  Heap heap(eServer, inputHeapNodeArr, j, HEAP_NODE_SIZE);
  EncOneBlock *writeBuffer = new EncOneBlock[MERGE_BATCH_SIZE];
  int64_t writeBufferCounter = 0;
  while (1) {
    HeapNode *temp = heap.getRoot();
    memcpy(writeBuffer + writeBufferCounter, temp->data + temp->elemIdx % HEAP_NODE_SIZE, eServer.encOneBlockSize);
    writeBufferCounter ++;
    // totalCounter ++;
    temp->elemIdx ++;
    if (writeBufferCounter == MERGE_BATCH_SIZE) {
      eServer.nonEnc = 1;
      eServer.opOneLinearScanBlock(writeBucketAddr, writeBuffer, MERGE_BATCH_SIZE, outputStructureId, 1, 0);
      eServer.nonEnc = 0;
      writeBucketAddr += MERGE_BATCH_SIZE;
      writeBufferCounter = 0;
    }
    
    if (temp->elemIdx < numRow1[temp->bucketIdx] && (temp->elemIdx % HEAP_NODE_SIZE) == 0) {
      eServer.opOneLinearScanBlock(readBucketAddr[temp->bucketIdx], temp->data, std::min((int64_t)HEAP_NODE_SIZE, numRow1[temp->bucketIdx]-temp->elemIdx), inputStructureId, 0, 0);
      readBucketAddr[temp->bucketIdx] += std::min((int64_t)HEAP_NODE_SIZE, numRow1[temp->bucketIdx]-temp->elemIdx);
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
  eServer.nonEnc = 1;
  eServer.opOneLinearScanBlock(writeBucketAddr, writeBuffer, writeBufferCounter, outputStructureId, 1, 0);
  eServer.nonEnc = 0;
  // numRow2[0] += writeBufferCounter;
  // TODO: ERROR writeBuffer
  delete [] writeBuffer;
  delete [] readBucketAddr;
  delete [] inputHeapNodeArr;
}

void Bucket::bucketSort(int inputStructureId, int64_t size, int64_t dataStart) {
  EncOneBlock *arr = new EncOneBlock[size];
  eServer.opOneLinearScanBlock(dataStart, arr, size, inputStructureId, 0, 0);
  // TODO: chaneg later to use oblivious version
  Quick qsort(eServer, arr);
  qsort.quickSort(0, size - 1);
  eServer.opOneLinearScanBlock(dataStart, arr, size, inputStructureId, 1, 0);
  delete [] arr;
}

// TODO: change some parameters to int64_t
int Bucket::bucketOSort() {
  int64_t *bucketAddr = new int64_t[bucketNum];
  for (int64_t i = 0; i < bucketNum; ++i) {
    bucketAddr[i] = i * Z;
  }
  int64_t *numRow1 = new int64_t[bucketNum];
  memset(numRow1, 0, bucketNum * sizeof(int64_t));
  int64_t *numRow2 = new int64_t[bucketNum];
  memset(numRow2, 0, bucketNum * sizeof(int));
  
  EncOneBlock *trustedMemory = new EncOneBlock[Z];
  EncOneBlock *inputTrustMemory = new EncOneBlock[Z];
  int64_t total = 0, readStart = 0;
  int64_t each;
  int64_t avg = N / bucketNum;
  int64_t remainder = N % bucketNum;
  printf("avg: %ld, Z: %ld\n", avg, Z);
  std::uniform_int_distribution<int64_t> dist{0, bucketNum-1};
  for (int64_t i = 0; i < bucketNum; ++i) {
    each = avg + ((i < remainder) ? 1 : 0);
    eServer.nonEnc = 1;
    eServer.opOneLinearScanBlock(readStart, inputTrustMemory, each, inStructureId, 0, 0);
    readStart += each;
    int randomKey;
    for (int64_t j = 0; j < each; ++j) {
      randomKey = dist(rng);
      trustedMemory[j] = inputTrustMemory[j];
      trustedMemory[j].randomKey = randomKey;
    }
    eServer.nonEnc = 0;
    eServer.opOneLinearScanBlock(bucketAddr[i], trustedMemory, each, outId1, 1, Z-each);
    numRow1[i] += each;
  }
  delete [] trustedMemory;
  delete [] inputTrustMemory;
  printf("finish read %lf\n", eServer.getIOcost()*B/N);
  int64_t *inputId = new int64_t[FAN_OUT];
  int64_t *outputId = new int64_t[FAN_OUT];
  int64_t outIdx = 0, expo, tempk, jboundary, jjboundary;
  int64_t tempk_i, tempk_i1;
  printf("Total bits: %f, each level solved %ld bits\n", log2(bucketNum), k1);
  for (int i = 0; i < ranBinAssignIters; ++i) {
    expo = std::min((int)k1, (int)(log2(bucketNum)-i*k1));
    tempk = pow(2, expo);
    tempk_i = pow(tempk, i);
    tempk_i1 = tempk_i * tempk;
    printf("level %d read&write %ld buckets\n", i, tempk);
    jboundary = (i != (ranBinAssignIters-1)) ? (bucketNum / tempk_i1) : 1;
    jjboundary = (i != (ranBinAssignIters-1)) ? tempk_i : (bucketNum/tempk);
    // printf("jboundary: %ld, jjboundary: %ld\n", jboundary, jjboundary);
    if (i % 2 == 0) {
      for (int64_t j = 0; j < jboundary; ++j) {
        for (int64_t jj = 0; jj < jjboundary; ++jj) {
          for (int64_t m = 0; m < tempk; ++m) {
            inputId[m] = j * tempk_i1+ jj + m * jjboundary;
            outputId[m] = (outIdx * tempk + m) % bucketNum;
          }
          mergeSplit(outId1, outId2, inputId, outputId, tempk, bucketAddr, numRow1, numRow2, i);
          outIdx ++;
        }
      }
      int64_t count = 0;
      for (int64_t n = 0; n < bucketNum; ++n) {
        numRow1[n] = 0;
        count += numRow2[n];
      }
      printf("after %dth merge split, we have %ld tuples, IO: %lf\n", i, count, eServer.getIOcost()*B/N);
      outIdx = 0;
    } else {
      for (int64_t j = 0; j < jboundary; ++j) {
        for (int64_t jj = 0; jj < jjboundary; ++jj) {
          for (int64_t m = 0; m < tempk; ++m) {
            inputId[m] = j * tempk_i1+ jj + m * jjboundary;
            outputId[m] = (outIdx * tempk + m) % bucketNum;
          }
          mergeSplit(outId2, outId1, inputId, outputId, tempk, bucketAddr, numRow2, numRow1, i);
          outIdx ++;
        }
      }
      int64_t count = 0;
      for (int n = 0; n < bucketNum; ++n) {
        numRow2[n] = 0;
        count += numRow1[n];
      }
      printf("after %dth merge split, we have %ld tuples, IO: %lf\n", i, count, eServer.getIOcost()*B/N);
      outIdx = 0;
    }
    printf("----------------------------------------\n");
    printf("\n\n Finish random bin assignment iter%dth out of %d\n\n", i, ranBinAssignIters);
    printf("----------------------------------------\n");
  }
  delete [] inputId;
  delete [] outputId;
  int resultId = 0;
  printf("Finish RBA! %lf \n", eServer.getIOcost()*B/N);
  if (ranBinAssignIters % 2 == 0) {
    for (int64_t i = 0; i < bucketNum; ++i) {
      bucketSort(outId1, numRow1[i], bucketAddr[i]);
    }
    printf("Finish bucketSort! %lf\n", eServer.getIOcost()*B/N);
    kWayMergeSort(outId1, outId2, numRow1, bucketAddr, bucketNum);
    resultId = outId2;
  } else {
    for (int64_t i = 0; i < bucketNum; ++i) {
      bucketSort(outId2, numRow2[i], bucketAddr[i]);
    }
    printf("Finish bucketSort! %lf\n", eServer.getIOcost()*B/N);
    kWayMergeSort(outId2, outId1, numRow2, bucketAddr, bucketNum);
    resultId = outId1;
  }
  printf("Finish kWayMergeSort!\n");
  delete [] numRow1;
  delete [] numRow2;
  return resultId;
}
