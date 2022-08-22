#include "bucket.h"
#include "quick.h"
#include "oq.h"

int Hypergeometric(int NN, int Msize, int n_prime) {
  int m = 0;
  srand((unsigned)time(0));
  double rate = double(n_prime) / NN;
  for (int j = 0; j < Msize; ++j) {
    if (rand() / double(RAND_MAX) < rate) {
      m += 1;
      n_prime -= 1;
    }
    NN -= 1;
    rate = double(n_prime) / double(NN);
  }
  return m;
}

void shuffle(int *array, int n) {
  srand((unsigned)time(0));
  if (n > 1) {
    for (int i = 0; i < n - 1; ++i) {
      int j = i + rand() / (RAND_MAX / (n - i) + 1);
      int t = array[j];
      array[j] = array[i];
      array[i] = t;
    }
  }
}

int SampleTight(int inStructureId, int samplesId) {
  int N_prime = N;
  double n_prime = 1.0 * ALPHA * N;
  int alphaM2 = (int)ceil(2.0 * ALPHA * M);
  int boundary = (int)ceil(1.0 * N/M);
  int Msize, alphaM22;
  int m; // use for hypergeometric distribution
  int realNum = 0; // #pivots
  int writeBackstart = 0;
  int readStart = 0;
  int *trustedMemory = (int*)malloc(M * sizeof(int));
  
  freeAllocate(samplesId, samplesId, alphaM2 * boundary);
  
  for (int i = 0; i < boundary; i++) {
    Msize = std::min(M, N - i * M);
    alphaM22 = (int)ceil(2.0 * ALPHA * Msize);
    opOneLinearScanBlock(readStart, trustedMemory, Msize, inStructureId, 0);
    // print(trustedMemory, Msize);
    readStart += Msize;
    // step1. sample with hypergeometric distribution
    m = Hypergeometric(N_prime, Msize, n_prime);
    if (m > alphaM22 && (i != boundary - 1)) {
      return -1;
    }
    realNum += m;
    // step2. shuffle M
    shuffle(trustedMemory, Msize);
    // step3. set dummy
    memset(&trustedMemory[m], DUMMY, (Msize - m) * sizeof(int));
    // step4. write sample back to external memory
    opOneLinearScanBlock(writeBackstart, trustedMemory, alphaM22, samplesId, 1);
    writeBackstart += alphaM22;
    N_prime -= Msize;
    n_prime -= m;
  }
  
  if (realNum < M) {
    opOneLinearScanBlock(0, trustedMemory, writeBackstart, samplesId, 0);
    int realN = moveDummy(trustedMemory, writeBackstart);
    if (realN != realNum) {
      printf("Counting error after moving dummy.\n");
    }
    quickSort(trustedMemory, 0, realN - 1);
    opOneLinearScanBlock(0, trustedMemory, realN, samplesId, 1);
    // print(arrayAddr, samplesId, realN);
  } else {
    printf("RealNum >= M\n");
    return -1;
  }
  double nonDummyNum = ALPHA * N;
  printf("%d, %d\n", realNum, (int)nonDummyNum);
  free(trustedMemory);
  return realNum;
}

int SampleLoose(int inStructureId, int samplesId) {
  int N_prime = N;
  double n_prime = 1.0 * ALPHA * N;
  int boundary = (int)ceil(1.0 * N/BLOCK_DATA_SIZE);
  int Msize;
  int m; // use for hypergeometric distribution
  int k = 0;
  int realNum = 0; // #pivots
  int readStart = 0;
  int *trustedMemory = (int*)malloc(BLOCK_DATA_SIZE * sizeof(int));
  
  freeAllocate(samplesId, samplesId, n_prime);
  
  for (int i = 0; i < boundary; i++) {
    // step1. sample with hypergeometric distribution
    Msize = std::min(BLOCK_DATA_SIZE, N - i * BLOCK_DATA_SIZE);
    m = Hypergeometric(N_prime, Msize, n_prime);
    if (m > 0) {
      realNum += m;
      opOneLinearScanBlock(readStart, trustedMemory, Msize, inStructureId, 0);
      readStart += Msize;
      // step2. shuffle M
      shuffle(trustedMemory, Msize);
      // step4. write sample back to external memory
      opOneLinearScanBlock(k, trustedMemory, m, samplesId, 1);
      k += m;
    }
    N_prime -= Msize;
    n_prime -= m;
    if (n_prime <= 0) {
      break;
    }
    // TODO: ? what's m value
  }
  trustedMemory = (int*)realloc(trustedMemory, M * sizeof(int));
  if (realNum < M) {
    opOneLinearScanBlock(0, trustedMemory, realNum, samplesId, 0);
    quickSort(trustedMemory, 0, realNum - 1);
    opOneLinearScanBlock(0, trustedMemory, realNum, samplesId, 1);
  } else {
    printf("RealNum >= M\n");
    return -1;
  }
  double nonDummyNum = ALPHA * N;
  printf("%d, %d\n", realNum, (int)nonDummyNum);
  free(trustedMemory);
  return realNum;
}

int quantileCal(int sampleId, int start, int end, int p, int *trustedM1) {
  int *pivotIdx = (int*)malloc(sizeof(int) * (p+1));
  // vector<int>
  int sampleSize = end - start;
  for (int i = 1; i < p; i ++) {
    pivotIdx[i] = i * sampleSize / p;
  }
  int *trustedMemory = (int*)malloc(sizeof(int) * BLOCK_DATA_SIZE);
  int boundary = (int)ceil(1.0 * sampleSize / BLOCK_DATA_SIZE);
  int Msize;
  int totalRead = 0;
  int j = 1; // record pivotId
  for (int i = 0; i < boundary; i++) {
    Msize = std::min(BLOCK_DATA_SIZE, sampleSize - i * BLOCK_DATA_SIZE);
    opOneLinearScanBlock(start + i * BLOCK_DATA_SIZE, trustedMemory, Msize, sampleId, 0);
    totalRead += Msize;
    while (pivotIdx[j] < totalRead) {
      trustedM1[j] = trustedMemory[pivotIdx[j] % BLOCK_DATA_SIZE];
      j++;
      if (j == p) {
        break;
      }
    }
    if (j == p) {
      break;
    }
  }
  free(pivotIdx);
  free(trustedMemory);
  trustedM1[0] = INT_MIN;
  trustedM1[p] = INT_MAX;
  // TODO: ADD test pivots correct value
  bool passFlag = 1;
  for (int i = 0; i < p; ++i) {
    passFlag &= (trustedM1[i] < trustedM1[i+1]);
    if (trustedM1[i + 1] < 0) {
      passFlag = 0;
      break;
    }
  }
  return passFlag;
}

int ProcessL(int LIdIn, int LIdOut, int lsize) {
  // TODO: bucket type
  freeAllocate(LIdIn, LIdIn, lsize * 2);
  freeAllocate(LIdOut, LIdOut, lsize * 2);
  Bucket_x *L = (Bucket_x*)malloc(sizeof(Bucket_x) * BLOCK_DATA_SIZE);
  int Msize;
  int boundary = (int)ceil(1.0 * lsize / BLOCK_DATA_SIZE);
  int k = 0;
  // 1. Initialize array L and set up random Key
  for (int i = 0; i < boundary; ++i) {
    Msize = std::min(BLOCK_DATA_SIZE, lsize - i * BLOCK_DATA_SIZE);
    opOneLinearScanBlock(2 * i * BLOCK_DATA_SIZE, (int*)L, Msize, LIdIn, 0);
    for (int i = 0; i < Msize; ++i) {
      L[i].x = (int)oe_rdrand();
      // L[i].x = (int)rand();
      L[i].key = k++;
    }
    opOneLinearScanBlock(2 * i * BLOCK_DATA_SIZE, (int*)L, Msize, LIdIn, 1);
  }
  // TODO: External Memory Sort: eg merge sort
  int bucketNum = (int)ceil(1.0 * lsize / M);
  int bucketSize = lsize / bucketNum;
  int residual = lsize % bucketNum;
  int totalSize = 0;
  int *numRow = (int*)malloc(sizeof(int) * bucketNum);
  int *bucketAddr = (int*)malloc(sizeof(int) * bucketNum);
  memset(numRow, 0, sizeof(int) * bucketNum);
  for (int i = 0; i < bucketNum; ++i) {
    numRow[i] = bucketSize + (i < residual ? 1 : 0);
    if (i == 0) {
      bucketAddr[i] = 0;
    } else {
      totalSize += numRow[i - 1];
      bucketAddr[i] = totalSize;
    }
  }
  for (int i = 0; i < bucketNum; ++i) {
    bucketSort(LIdIn, i, numRow[i], bucketAddr[i]);
  }
  // print(arrayAddr, LIdIn, lsize);
  kWayMergeSort(LIdIn, LIdOut, numRow, bucketAddr, bucketNum);
  // printf("Test Process L order after kwaymergesort\n");
  // test(arrayAddr, LIdOut, lsize);
  free(numRow);
  free(bucketAddr);
  free(L);
  return 0;
}

std::pair<int, int> MultiLevelPartition(int inStructureId, int sampleId, int LIdIn, int LIdOut, int sampleSize, int p, int outStructureId1) {
  if (N <= M) {
    return std::make_pair(N, 1);
  }
  int hatN = (int)ceil(1.0 * (1 + 2 * BETA) * N);
  int M_prime = (int)ceil(1.0 * M / (1 + 2 * BETA));
  // 1. Initialize array L, extrenal memory
  int lsize = (int)ceil(1.0 * N / BLOCK_DATA_SIZE);
  // 2. set up block index array L & shuffle L
  ProcessL(LIdIn, LIdOut, lsize);
  // freeAllocate(LIdIn, LIdIn, 0);
  
  int r = (int)ceil(1.0 * log(hatN / M) / log(p));
  int p0 = (int)ceil(1.0 * hatN / (M * pow(p, r - 1)));
  
  // 3. calculate p0-quantile about sample
  int *trustedM1 = (int*)malloc(sizeof(int) * (p0 + 1));
  bool resFlag = quantileCal(sampleId, 0, sampleSize, p0, trustedM1);
  while (!resFlag) {
    printf("Quantile calculate error!\n");
    // print(arrayAddr, sampleId, sampleSize);
    resFlag = quantileCal(sampleId, 0, sampleSize, p0, trustedM1);
  }
  // print(trustedM1, p0 + 1);
  // print(arrayAddr, sampleId, sampleSize);
  // 4. allocate trusted memory
  int boundary1 = (int)ceil(2.0 * N / M_prime);
  int boundary2 = (int)ceil(1.0 * M_prime / (2 * BLOCK_DATA_SIZE));
  Bucket_x *trustedM2 = (Bucket_x*)malloc(sizeof(Bucket_x) * boundary2);
  int *trustedM3 = (int*)malloc(sizeof(int) * boundary2 * BLOCK_DATA_SIZE);
  memset(trustedM3, DUMMY, sizeof(int) * boundary2 * BLOCK_DATA_SIZE);
  
  int **bucketNum = (int**)malloc(sizeof(int*) * p0);
  for (int i = 0; i < p0; ++i) {
    bucketNum[i] = (int*)malloc(sizeof(int) * boundary1);
    memset(bucketNum[i], 0, sizeof(int) * boundary1);
  }
  int Msize1, Msize2;
  // int smallSectionSize = (int)ceil(1.0 * M / (2 * p0));
  int smallSectionSize = M / (2 * p0);
  // 5. allocate out memory using for index
  freeAllocate(outStructureId1, outStructureId1, boundary1 * smallSectionSize * p0);
  int bucketSize0 = boundary1 * smallSectionSize;
  
  int size1 = (int)ceil(1.0 * N / BLOCK_DATA_SIZE);
  int M3size = 0;  // all input number, using for check
  int blockIdx;
  // 6. level0 partition
  for (int i = 0; i < boundary1; ++i) {
    Msize1 = std::min(boundary2, size1 - i * boundary2);
    if (Msize1 <= 0) {
      // TODO: ?
      break;
    }
    opOneLinearScanBlock(2 * i * boundary2, (int*)trustedM2, Msize1, LIdOut, 0);
    
    memset(trustedM3, DUMMY, sizeof(int) * boundary2 * BLOCK_DATA_SIZE);
    for (int j = 0; j < Msize1; ++j) {
      blockIdx = trustedM2[j].key;
      // Msize2 = std::min(BLOCK_DATA_SIZE, N - j * BLOCK_DATA_SIZE);
      Msize2 = std::min(BLOCK_DATA_SIZE, N - blockIdx * BLOCK_DATA_SIZE);
      opOneLinearScanBlock(blockIdx * BLOCK_DATA_SIZE, &trustedM3[j * BLOCK_DATA_SIZE], Msize2, inStructureId, 0);
      M3size += Msize2;
    }
    for (int j = 0; j < p0; ++j) {
      int dataBoundary = boundary2 * BLOCK_DATA_SIZE;
      for (int m = 0; m < dataBoundary; ++m) {
        if ((trustedM3[m] != DUMMY) && (trustedM3[m] >= trustedM1[j]) && (trustedM3[m] < trustedM1[j + 1])) {
          opOneLinearScanBlock(j * bucketSize0 + i * smallSectionSize + bucketNum[j][i], &trustedM3[m], 1, outStructureId1, 1);
          bucketNum[j][i] += 1;
          if (bucketNum[j][i] > smallSectionSize) {
            printf("Overflow in small section M/2p0: %d\n", bucketNum[j][i]);
          }
        }
      }
    }
  }
  if (M3size != N) {
    printf("MultiLevel data process error!\n");
  }
  // 7. pad dummy
  // std::cout << "Before padding: \n";
  // print(arrayAddr, outStructureId1, boundary1 * smallSectionSize * p0);
  for (int j = 0; j < p0; ++j) {
    for (int i = 0; i < boundary1; ++i) {
      padWithDummy(outStructureId1, j * bucketSize0 + i * smallSectionSize, bucketNum[j][i], smallSectionSize);
    }
  }
  // std::cout << "After padding: \n";
  // print(arrayAddr, outStructureId1, boundary1 * smallSectionSize * p0);
  free(trustedM1);
  free(trustedM2);
  free(trustedM3);
  for (int i = 0; i < p0; ++i) {
    free(bucketNum[i]);
  }
  free(bucketNum);
  if (bucketSize0 > M) {
    printf("Each section size is greater than M, adjst parameters: %d, %d\n", bucketSize0, M);
  }
  // freeAllocate(LIdOut, LIdOut, 0);
  return std::make_pair(bucketSize0, p0);
}


int ObliviousTightSort(int inStructureId, int inSize, int sampleId, int LIdIn, int LIdOut, int outStructureId1, int outStructureId2) {
  int *trustedM = (int*)malloc(sizeof(int) * M);
  if (inSize <= M) {
    opOneLinearScanBlock(0, trustedM, inSize, inStructureId, 0);
    quickSort(trustedM, 0, inSize - 1);
    freeAllocate(outStructureId1, outStructureId1, inSize);
    opOneLinearScanBlock(0, trustedM, inSize, outStructureId1, 1);
    return outStructureId1;
  }
  int realNum = SampleTight(inStructureId, sampleId);
  int n = (int)ceil(1.0 * ALPHA * N);
  while (realNum < 0) {
    printf("Samples number error!\n");
    realNum = SampleTight(inStructureId, sampleId);
  }
  std::pair<int, int> section = MultiLevelPartition(inStructureId, sampleId, LIdIn, LIdOut, std::min(realNum, n), P, outStructureId1);
  int sectionSize = section.first;
  int sectionNum = section.second;
  int totalLevelSize = sectionNum * sectionSize;
  int j = 0;
  int k;
  
  freeAllocate(outStructureId2, outStructureId2, inSize);
  
  for (int i = 0; i < sectionNum; ++i) {
    opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outStructureId1, 0);
    // TODO: optimize to utilize bucketNum[j][i]
    k = moveDummy(trustedM, sectionSize);
    quickSort(trustedM, 0, k - 1);
    opOneLinearScanBlock(j, trustedM, k, outStructureId2, 1);
    j += k;
    if (j > inSize) {
      printf("Overflow\n");
    }
  }
  // std::cout << "After inter-sorting: \n";
  // print(arrayAddr, outStructureId2, inSize);
  return outStructureId2;
}

int ObliviousLooseSort(int inStructureId, int inSize, int sampleId, int LIdIn, int LIdOut, int outStructureId1, int outStructureId2, int *resN) {
  int *trustedM = (int*)malloc(sizeof(int) * M);
  if (inSize <= M) {
    opOneLinearScanBlock(0, trustedM, inSize, inStructureId, 0);
    quickSort(trustedM, 0, inSize - 1);
    opOneLinearScanBlock(0, trustedM, inSize, outStructureId1, 1);
    *resN = inSize;
    return outStructureId1;
  }
  int realNum = SampleLoose(inStructureId, sampleId);
  int n = (int)ceil(1.0 * ALPHA * N);
  while (realNum < 0) {
    printf("Samples number error!\n");
    realNum = SampleLoose(inStructureId, sampleId);
  }

  std::pair<int, int> section = MultiLevelPartition(inStructureId, sampleId, LIdIn, LIdOut, std::min(realNum, n), P, outStructureId1);
  int sectionSize = section.first;
  int sectionNum = section.second;
  int totalLevelSize = sectionNum * sectionSize;
  int j = 0;
  int k, Msize;
  
  freeAllocate(outStructureId2, outStructureId2, totalLevelSize);
  
  for (int i = 0; i < sectionNum; ++i) {
    opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outStructureId1, 0);
    k = moveDummy(trustedM, sectionSize);
    quickSort(trustedM, 0, k - 1);
    opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outStructureId2, 1);
  }
  *resN = totalLevelSize;
  return outStructureId2;
}