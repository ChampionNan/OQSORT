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

// Return sorted samples
int SampleTight(int inStructureId, int samplesId, int *trustedM2) {
  // sampleFlag = 1;
  int N_prime = N;
  int n_prime = ceil(1.0 * ALPHA * N);
  int M2 = (int)ceil(1.0 * M / 2);
  int boundary = (int)ceil(1.0 * N / M2);
  int Msize, alphaM22;
  int m; // use for hypergeometric distribution
  int realNum = 0; // #pivots
  int readStart = 0;
  int *trustedM1 = (int*)malloc(M2 * sizeof(int));
  
  for (int i = 0; i < boundary; i++) {
    Msize = std::min(M2, N - i * M2);
    alphaM22 = (int)ceil(2.0 * ALPHA * Msize);
    opOneLinearScanBlock(readStart, trustedM1, Msize, inStructureId, 0);
    // print(trustedMemory, Msize);
    readStart += Msize;
    // step1. sample with hypergeometric distribution
    m = Hypergeometric(N_prime, Msize, n_prime);
    if (m > alphaM22 && (i != boundary - 1)) {
      return -1;
    }
    // step2. shuffle M
    shuffle(trustedM1, Msize);
    // step3. set dummy (REMOVE)
    // step4. write sample back to external memory
    memcpy(&trustedM2[realNum], trustedM1, m * sizeof(int));
    realNum += m;
    N_prime -= Msize;
    n_prime -= m;
    if (n_prime <= 0) {
      break;
    }
  }
  
  quickSort(trustedM2, 0, realNum - 1);
  double nonDummyNum = ALPHA * N;
  printf("%d, %f\n", realNum, nonDummyNum);
  free(trustedM1);
  // sampleFlag = 0;
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
  printf("%d, %f\n", realNum, nonDummyNum);
  free(trustedMemory);
  return realNum;
}

int quantileCalT(int *samples, int start, int end, int p, int *trustedM1) {
  int *pivotIdx = (int*)malloc(sizeof(int) * (p+1));
  // vector<int>
  int sampleSize = end - start;
  for (int i = 1; i < p; i ++) {
    pivotIdx[i] = i * sampleSize / p;
  }
  for (int i = 1; i < p; i++) {
    trustedM1[i] = samples[pivotIdx[i]];
  }
  free(pivotIdx);
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
  free(samples);
  return passFlag;
}

int quantileCalL(int sampleId, int start, int end, int p, int *trustedM1) {
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

std::pair<int, int> MultiLevelPartitionT(int inStructureId, int *samples, int sampleSize, int p, int outStructureId1) {
  // partitionFlag = 1;
  if (N <= M) {
    return std::make_pair(N, 1);
  }
  int hatN = (int)ceil(1.0 * (1 + 2 * BETA) * N);
  int M_prime = (int)ceil(1.0 * M / (1 + 2 * BETA));
  
  // 2. set up block index array L & shuffle (REMOVE)
  int r = (int)ceil(1.0 * log(hatN / M) / log(p));
  int p0 = (int)ceil(1.0 * hatN / (M * pow(p, r - 1)));
  // 3. calculate p0-quantile about sample
  int *trustedM1 = (int*)malloc(sizeof(int) * (p0 + 1));
  bool resFlag = quantileCalT(samples, 0, sampleSize, p0, trustedM1);
  while (!resFlag) {
    printf("Quantile calculate error!\n");
    resFlag = quantileCalT(samples, 0, sampleSize, p0, trustedM1);
  }
  // 4. allocate trusted memory
  int boundary1 = (int)ceil(1.0 * N / M_prime);
  int boundary2 = (int)ceil(1.0 * M_prime / BLOCK_DATA_SIZE);
  int dataBoundary = boundary2 * BLOCK_DATA_SIZE;
  int smallSectionSize = M / p0;
  int bucketSize0 = boundary1 * smallSectionSize;
  // input data
  int *trustedM3 = (int*)malloc(sizeof(int) * boundary2 * BLOCK_DATA_SIZE);
  memset(trustedM3, DUMMY, sizeof(int) * boundary2 * BLOCK_DATA_SIZE);
  int **bucketNum = (int**)malloc(sizeof(int*) * p0);
  for (int i = 0; i < p0; ++i) {
    bucketNum[i] = (int*)malloc(sizeof(int) * boundary1);
    memset(bucketNum[i], 0, sizeof(int) * boundary1);
  }
  freeAllocate(outStructureId1, outStructureId1, boundary1 * smallSectionSize * p0);
  
  // int Rsize = 0;  // all input number, using for check
  int k, Msize1, Msize2;
  int blocks_done = 0;
  int total_blocks = (int)ceil(1.0 * N / BLOCK_DATA_SIZE);
  int *shuffleB = (int*)malloc(sizeof(int) * BLOCK_DATA_SIZE);
  // 6. level0 partition
  for (int i = 0; i < boundary1; ++i) {
    for (int j = 0; j < boundary2; ++j) {
      if (total_blocks - 1 - blocks_done == 0) {
        k = 0;
      } else {
        k = rand() % (total_blocks - 1 - blocks_done);
      }
      Msize1 = std::min(BLOCK_DATA_SIZE, N - k * BLOCK_DATA_SIZE);
      opOneLinearScanBlock(k * BLOCK_DATA_SIZE, &trustedM3[j * BLOCK_DATA_SIZE], Msize1, inStructureId, 0);
      memset(shuffleB, DUMMY, sizeof(int) * BLOCK_DATA_SIZE);
      Msize2 = std::min(BLOCK_DATA_SIZE, N - (total_blocks-1-blocks_done) * BLOCK_DATA_SIZE);
      opOneLinearScanBlock((total_blocks-1-blocks_done) * BLOCK_DATA_SIZE, shuffleB, Msize2, inStructureId, 0);
      opOneLinearScanBlock(k * BLOCK_DATA_SIZE, shuffleB, BLOCK_DATA_SIZE, inStructureId, 1);
      blocks_done += 1;
      if (blocks_done == total_blocks) {
        break;
      }
    }
    for (int j = 0; j < p0; ++j) {
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
    memset(trustedM3, DUMMY, sizeof(int) * boundary2 * BLOCK_DATA_SIZE);
  }
  
  // 7. pad dummy
  for (int j = 0; j < p0; ++j) {
    for (int i = 0; i < boundary1; ++i) {
      padWithDummy(outStructureId1, j * bucketSize0 + i * smallSectionSize, bucketNum[j][i], smallSectionSize);
    }
  }
  free(trustedM1);
  free(trustedM3);
  for (int i = 0; i < p0; ++i) {
    free(bucketNum[i]);
  }
  free(bucketNum);
  if (bucketSize0 > M) {
    printf("Each section size is greater than M, adjst parameters.\n"); 
  }
  // freeAllocate(LIdOut, LIdOut, 0);
  // partitionFlag = 0;
  return std::make_pair(bucketSize0, p0);
}

std::pair<int, int> MultiLevelPartitionL(int inStructureId, int sampleId, int sampleSize, int p, int outStructureId1) {
  // partitionFlag = 1;
  if (N <= M) {
    return std::make_pair(N, 1);
  }
  int hatN = (int)ceil(1.0 * (1 + 2 * BETA) * N);
  int M_prime = (int)ceil(1.0 * M / (1 + 2 * BETA));
  
  // 2. set up block index array L & shuffle (REMOVE)
  int r = (int)ceil(1.0 * log(hatN / M) / log(p));
  int p0 = (int)ceil(1.0 * hatN / (M * pow(p, r - 1)));
  // 3. calculate p0-quantile about sample
  int *trustedM1 = (int*)malloc(sizeof(int) * (p0 + 1));
  bool resFlag = quantileCalL(sampleId, 0, sampleSize, p0, trustedM1);
  while (!resFlag) {
    printf("Quantile calculate error!\n");
    resFlag = quantileCalL(sampleId, 0, sampleSize, p0, trustedM1);
  }
  // 4. allocate trusted memory
  int boundary1 = (int)ceil(1.0 * N / M_prime);
  int boundary2 = (int)ceil(1.0 * M_prime / BLOCK_DATA_SIZE);
  int dataBoundary = boundary2 * BLOCK_DATA_SIZE;
  int smallSectionSize = M / p0;
  int bucketSize0 = boundary1 * smallSectionSize;
  // input data
  int *trustedM3 = (int*)malloc(sizeof(int) * boundary2 * BLOCK_DATA_SIZE);
  memset(trustedM3, DUMMY, sizeof(int) * boundary2 * BLOCK_DATA_SIZE);
  int **bucketNum = (int**)malloc(sizeof(int*) * p0);
  for (int i = 0; i < p0; ++i) {
    bucketNum[i] = (int*)malloc(sizeof(int) * boundary1);
    memset(bucketNum[i], 0, sizeof(int) * boundary1);
  }
  freeAllocate(outStructureId1, outStructureId1, boundary1 * smallSectionSize * p0);
  
  // int Rsize = 0;  // all input number, using for check
  int k, Msize1, Msize2;
  int blocks_done = 0;
  int total_blocks = (int)ceil(1.0 * N / BLOCK_DATA_SIZE);
  int *shuffleB = (int*)malloc(sizeof(int) * BLOCK_DATA_SIZE);
  // 6. level0 partition
  for (int i = 0; i < boundary1; ++i) {
    for (int j = 0; j < boundary2; ++j) {
      if (total_blocks - 1 - blocks_done == 0) {
        k = 0;
      } else {
        k = rand() % (total_blocks - 1 - blocks_done);
      }
      Msize1 = std::min(BLOCK_DATA_SIZE, N - k * BLOCK_DATA_SIZE);
      opOneLinearScanBlock(k * BLOCK_DATA_SIZE, &trustedM3[j * BLOCK_DATA_SIZE], Msize1, inStructureId, 0);
      memset(shuffleB, DUMMY, sizeof(int) * BLOCK_DATA_SIZE);
      Msize2 = std::min(BLOCK_DATA_SIZE, N - (total_blocks-1-blocks_done) * BLOCK_DATA_SIZE);
      opOneLinearScanBlock((total_blocks-1-blocks_done) * BLOCK_DATA_SIZE, shuffleB, Msize2, inStructureId, 0);
      opOneLinearScanBlock(k * BLOCK_DATA_SIZE, shuffleB, BLOCK_DATA_SIZE, inStructureId, 1);
      blocks_done += 1;
      if (blocks_done == total_blocks) {
        break;
      }
    }
    for (int j = 0; j < p0; ++j) {
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
    memset(trustedM3, DUMMY, sizeof(int) * boundary2 * BLOCK_DATA_SIZE);
  }
  // 7. pad dummy
  for (int j = 0; j < p0; ++j) {
    for (int i = 0; i < boundary1; ++i) {
      padWithDummy(outStructureId1, j * bucketSize0 + i * smallSectionSize, bucketNum[j][i], smallSectionSize);
    }
  }
  free(trustedM1);
  free(trustedM3);
  for (int i = 0; i < p0; ++i) {
    free(bucketNum[i]);
  }
  free(bucketNum);
  if (bucketSize0 > M) {
    printf("Each section size is greater than M, adjst parameters:\n");
  }
  // freeAllocate(LIdOut, LIdOut, 0);
  // partitionFlag = 0;
  return std::make_pair(bucketSize0, p0);
}

int ObliviousTightSort(int inStructureId, int inSize, int sampleId, int outStructureId1, int outStructureId2) {
  int *trustedM;
  if (inSize <= M) {
    trustedM = (int*)malloc(sizeof(int) * M);
    opOneLinearScanBlock(0, trustedM, inSize, inStructureId, 0);
    quickSort(trustedM, 0, inSize - 1);
    freeAllocate(outStructureId1, outStructureId1, inSize);
    opOneLinearScanBlock(0, trustedM, inSize, outStructureId1, 1);
    free(trustedM);
    return outStructureId1;
  }
  int M2 = (int)ceil(1.0 * M / 2);
  int *trustedM2 = (int*)malloc(M2 * sizeof(int));
  int realNum = SampleTight(inStructureId, sampleId, trustedM2);
  int n = (int)ceil(1.0 * ALPHA * N);
  while (realNum < 0) {
    printf("Samples number error!\n");
    realNum = SampleTight(inStructureId, sampleId, trustedM2);
  }
  
  std::pair<int, int> section = MultiLevelPartitionT(inStructureId, trustedM2, std::min(realNum, n), P, outStructureId1);
  int sectionSize = section.first;
  int sectionNum = section.second;
  int j = 0;
  int k;
  
  freeAllocate(outStructureId2, outStructureId2, inSize);
  // finalFlag = 1;
  trustedM = (int*)malloc(sizeof(int) * M);
  for (int i = 0; i < sectionNum; ++i) {
    opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outStructureId1, 0);
    // TODO: optimize to utilize bucketNum[j][i]
    k = moveDummy(trustedM, sectionSize);
    quickSort(trustedM, 0, k - 1);
    // print(trustedM, k);
    opOneLinearScanBlock(j, trustedM, k, outStructureId2, 1);
    j += k;
    if (j > inSize) {
      printf("Overflow\n");
    }
  }
  free(trustedM);
  // print(arrayAddr, outStructureId2, inSize);
  // finalFlag = 0;
  return outStructureId2;
}

std::pair<int, int> ObliviousLooseSort(int inStructureId, int inSize, int sampleId, int outStructureId1, int outStructureId2) {
  int *trustedM = (int*)malloc(sizeof(int) * M);
  if (inSize <= M) {
    opOneLinearScanBlock(0, trustedM, inSize, inStructureId, 0);
    quickSort(trustedM, 0, inSize - 1);
    opOneLinearScanBlock(0, trustedM, inSize, outStructureId1, 1);
    return {outStructureId1, inSize};
  }
  int realNum = SampleLoose(inStructureId, sampleId);
  int n = (int)ceil(1.0 * ALPHA * N);
  while (realNum < 0) {
    printf("Samples number error!\n");
    realNum = SampleLoose(inStructureId, sampleId);
  }

  std::pair<int, int> section = MultiLevelPartitionL(inStructureId, sampleId, std::min(realNum, n), P, outStructureId1);
  int sectionSize = section.first;
  int sectionNum = section.second;
  int totalLevelSize = sectionNum * sectionSize;
  int k;
  
  freeAllocate(outStructureId2, outStructureId2, totalLevelSize);
  // finalFlag = 1;
  for (int i = 0; i < sectionNum; ++i) {
    opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outStructureId1, 0);
    k = moveDummy(trustedM, sectionSize);
    quickSort(trustedM, 0, k - 1);
    opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outStructureId2, 1);
  }
  // finalFlag = 0;
  return {outStructureId2, totalLevelSize};
}
