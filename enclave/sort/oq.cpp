#include "bucket.h"
#include "quick.h"
#include "bitonic.h"
#include "oq.h"

ODS::ODS(EnclaveServer &eServer, double alpha, double beta, double gamma, int P, int is_tight, SecLevel seclevel, int sampleId) : eServer{eServer}, alpha{alpha}, beta{beta}, gamma{gamma}, P{P}, is_tight{is_tight}, seclevel{seclevel}, sampleId{sampleId} {
  N = eServer.N;
  M = eServer.M;
  B = eServer.B;
  smallM = log2(M);
  sortedSampleId = sampleId + 1;
}

void ODS::calParams(int64_t inSize, int p, int64_t &hatN, int64_t &M_prime, int64_t &r, int64_t &p0) {
  hatN = ceil(1.0 * (1 + beta + gamma) * inSize);
  M_prime = ceil(1.0 * M / (1 + beta + gamma));
  r = ceil(1.0 * log(hatN / M) / log(p));
  p0 = ceil(1.0 * hatN / (M * pow(p, r - 1)));
}

void ODS::floydSampler(int64_t n, int64_t k, std::vector<int64_t> &x) {
  std::unordered_set<int64_t> H;
  for (int64_t i = n - k; i < n; ++i) {
    x.push_back(i);
  }
  int64_t r, j, temp;
  for (int64_t i = 0; i < k; ++i) {
    std::uniform_int_distribution<int64_t> dist{0, n-k+1+i};
    r = dist(rng); // get random numbers with PRNG
    if (H.count(r)) {
      std::uniform_int_distribution<int64_t> dist2{0, i};
      j = dist2(rng);
      temp = x[i];
      x[i] = x[j];
      x[j] = temp;
      H.insert(n-k+i);
    } else {
      x[i] = r;
      H.insert(r);
    }
  }
  sort(x.begin(), x.end());
}

int64_t ODS::Sample(int inStructureId, int64_t sampleSize, std::vector<EncOneBlock> &trustedM2, SortType sorttype) {
  int64_t N_prime = sampleSize;
  // double alpha = (!is_rec) ? ALPHA : _ALPHA;
  int64_t n_prime = ceil(1.0 * alpha * N_prime);
  int64_t boundary = ceil(1.0 * N_prime / B);
  int64_t j = 0, Msize;
  int is_tight = (sorttype == ODSTIGHT) ? 1 : 0;
  EncOneBlock *trustedM1 = new EncOneBlock[B];
  eServer.nonEnc = 1;
  std::vector<int64_t> sampleIdx;
  floydSampler(N_prime, n_prime, sampleIdx);
  for (int64_t i = 0; i < boundary; ++i) {
    if (is_tight) {
      Msize = std::min((int64_t)B, N_prime - i * B);
      eServer.opOneLinearScanBlock(i * B, trustedM1, Msize, inStructureId, 0, 0);
      while ((j < n_prime) && (sampleIdx[j] >= i * B) && (sampleIdx[j] < (i+1) * B)) {
        trustedM2.push_back(trustedM1[sampleIdx[j] % B]);
        j += 1;
      }
    } else if ((!is_tight) && (sampleIdx[j] >= i * B) && (sampleIdx[j] < (i+1) * B)) {
      Msize = std::min((int64_t)B, N_prime - i * B);
      eServer.opOneLinearScanBlock(i * B, trustedM1, Msize, inStructureId, 0, 0);
      while ((sampleIdx[j] >= i * B) && (sampleIdx[j] < (i+1) * B)) {
        trustedM2.push_back(trustedM1[sampleIdx[j] % B]);
        j += 1;
        if (j >= n_prime) break;
      }
      if (j >= n_prime) break;
    }
  }
  sort(trustedM2.begin(), trustedM2.end());
  delete [] trustedM1;
  return n_prime;
}

// get pivots from external stored samples
void ODS::ODSquantileCal(int sampleId, int64_t sampleSize, int sortedSampleId, std::vector<EncOneBlock>& pivots) {
  printf("In ODSquantileCal\n");
  std::vector<EncOneBlock> trustedM2;
  int64_t realNum = Sample(sampleId, sampleSize, trustedM2, ODSLOOSE);
  int sampleP = ceil((1 + beta + gamma) * sampleSize / M);
  quantileCal(sampleSize, trustedM2, realNum, sampleP);
  std::pair<int64_t, int> section = OneLevelPartition(sampleId, sampleSize, trustedM2, sampleP, sortedSampleId);
  int64_t sectionSize = section.first;
  int sectionNum = section.second;
  EncOneBlock *trustedM = new EncOneBlock[M];
  // Cal pivot index
  std::vector<int64_t> quantileIdx;
  for (int64_t i = 1; i < P; ++i) {
    quantileIdx.push_back(i * sampleSize / P);
  }
  int64_t size = ceil(1.0 * sampleSize / P);
  int64_t j = 0;
  int64_t k = 0, total = 0;
  for (int i = 0; i < sectionNum; ++i) {
    eServer.opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, sortedSampleId, 0, 0);
    k = eServer.moveDummy(trustedM, sectionSize);
    // TODO: Change to fully oblivious version
    Quick qsort(eServer, trustedM);
    qsort.quickSort(0, k-1);
    total += k;
    // Cal Level1 pivots
    while ((j < P-1) && (quantileIdx[j] < total)) {
      pivots.push_back(trustedM[quantileIdx[j]-(total-k)]);
      j += 1;
    }
  }
  EncOneBlock a, b;
  a.sortKey = std::numeric_limits<int>::min();
  a.primaryKey = std::numeric_limits<int>::min();
  b.sortKey = std::numeric_limits<int>::max();
  b.primaryKey = std::numeric_limits<int>::max();
  pivots.insert(pivots.begin(), a);
  pivots.push_back(b);
  delete [] trustedM;
}

void ODS::quantileCal(int inSize, std::vector<EncOneBlock> &samples, int64_t sampleSize, int p) {
  int64_t hatN, M_prime, r, p0;
  calParams(inSize, p, hatN, M_prime, r, p0);
  for (int i = 1; i < p0; ++i) {
    samples[i] = samples[i * sampleSize / p0];
  }
  samples[0].sortKey = std::numeric_limits<int>::min();
  samples[p0].sortKey = std::numeric_limits<int>::max();
  samples.resize(p0+1);
  samples.shrink_to_fit();
  return ;
}

void ODS::quantileCal2(std::vector<EncOneBlock> &samples, int64_t start, int64_t end, int p) {
  int64_t sampleSize = end - start;
  for (int64_t i = 1; i <= p; ++i) {
    samples[i-1] = samples[i * sampleSize / (p+1)];
  }
  samples.resize(p);
  samples.shrink_to_fit();
  return ;
}

int64_t ODS::partitionMulti(EncOneBlock *arr, int64_t low, int64_t high, EncOneBlock pivot) {
  int64_t i = low - 1;
  for (int64_t j = low; j < high + 1; ++j) {
    if (eServer.cmpHelper(&pivot, arr + j)) { 
      i += 1;
      eServer.swapRow(arr, i, j);
    }
  }
  return i;
}

void ODS::quickSortMulti(EncOneBlock *arr, int64_t low, int64_t high, std::vector<EncOneBlock> pivots, int left, int right, std::vector<int64_t> &partitionIdx) {
  int pivotIdx;
  int64_t mid;
  EncOneBlock pivot;
  if (right >= left) {
    pivotIdx = (left + right) >> 1;
    pivot = pivots[pivotIdx];
    mid = partitionMulti(arr, low, high, pivot);
    partitionIdx.push_back(mid);
    quickSortMulti(arr, low, mid, pivots, left, pivotIdx-1, partitionIdx);
    quickSortMulti(arr, mid+1, high, pivots, pivotIdx+1, right, partitionIdx);
  }
}

int64_t ODS::sumArray(bool *M, int64_t left, int64_t right) {
  int64_t total = 0;
  for (int64_t i = left; i < right; ++i) {
    total += M[i];
  }
  return total;
}

void ODS::OROffCompact(EncOneBlock *D, bool *M, int64_t left, int64_t right, int64_t z) {
  int64_t n = right - left;
  if (n == 1) return;
  int64_t n2 = n / 2;
  int64_t m = sumArray(M, left, left + n2);
  if (n == 2) {
    bool flag = ((1 - M[left]) * M[left + 1]) ^ (z % 2);
    if (flag) eServer.swapRow(D, left, left + 1);
  } else if (n > 2) {
    OROffCompact(D, M, left, left + n2, z % n2);
    OROffCompact(D, M, left + n2, left + n, (z + m % n2) % n2);
    bool s1 = (((z % n2) + m) >= n2) ? 1 : 0;
    bool s2 = (z >= n2) ? 1 : 0;
    bool s = s1 ^ s2;
    for (int64_t i = 0; i < n2; ++i) {
      bool s3 = (i >= ((z + m) % n2)) ? 1 : 0;
      bool b = s ^ s3;
      if (b) eServer.swapRow(D, left + i, left + i + n2);
    }
  }
}

void ODS::ORCompact(EncOneBlock *D, bool *M, int64_t left, int64_t right) {
  int64_t n = right - left;
  if (n <= 1) return;
  int64_t n1 = pow(2, floor(log2(n)));
  int64_t n2 = n - n1;
  int64_t m = sumArray(M, left, left + n2);
  ORCompact(D, M, left, left + n2);
  OROffCompact(D, M, left + n2, left + n, (n1 - n2 + m) % n1);
  // print(D, n);
  for (int64_t i = 0; i < n2; ++i) { // not 2^k mix
    bool b = (i >= m) ? 1 : 0;
    if (b) eServer.swapRow(D, left + i, left + i + n1);
  }
}

// #elem < pivot
int64_t ODS::assignM(EncOneBlock *arr, bool *M, int64_t left, int64_t right, EncOneBlock pivot) {
  int64_t total = 0;
  for (int64_t i = left; i < right; ++i) {
    M[i] = eServer.cmpHelper(&pivot, arr + i) ? 1 : 0;
    total += M[i];
  }
  return total;
}
// [low, high), mid - low = #ele in each section
// all p pivots, without dummy version
void ODS::obliviousPWayPartition(EncOneBlock *D, bool *M, int64_t low, int64_t high, std::vector<EncOneBlock> pivots, int left, int right, std::vector<int64_t> &partitionIdx) {
  int pivotIdx;
  int64_t mid;
  EncOneBlock pivot;
  if (right >= left) {
    pivotIdx = (left + right) >> 1;
    pivot = pivots[pivotIdx];
    mid = assignM(D, M, low, high, pivot);
    ORCompact(D, M, low, high);
    partitionIdx.push_back(mid+low);
    obliviousPWayPartition(D, M, low, low + mid, pivots, left, pivotIdx-1, partitionIdx);
    obliviousPWayPartition(D, M, low + mid, high, pivots, pivotIdx+1, right, partitionIdx);
  }
}

// TODO: Need correct parameters
void ODS::internalObliviousSort(EncOneBlock *D, int64_t left, int64_t right) {
  printf("In internalObliviousSort\n");
  int64_t M = right - left;
  int64_t hatM = M + ceil((beta + gamma) * M);
  int64_t n = ceil(1.0 * alpha * M);
  int r = ceil(log2(1.0 * M / smallM));
  int p = pow(2, r) - 1; // we need 2^r-1 pivots
  printf("level number: %d,pivots number: %d\n", r, p);
  // 1. get D's samples
  std::vector<int64_t> sampleIdx;
  std::vector<EncOneBlock> samples;
  floydSampler(M, n, sampleIdx);
  for (int64_t i = 0; i < sampleIdx.size(); ++i) {
    samples.push_back(D[sampleIdx[i]]); // replace index to real value
  }
  sort(samples.begin(), samples.end());
  // 2. get pivots
  printf("At step2\n");
  quantileCal2(samples, 0, samples.size(), p);
  std::vector<int64_t> partitionIdx;
  bool *indicator = new bool[hatM];
  // copy D to extended memory
  EncOneBlock *extD = new EncOneBlock[hatM];
  memcpy(extD, D, eServer.encOneBlockSize * M);
  eServer.setValue(extD + M, (hatM - M), DUMMY<int>());
  std::vector<int64_t> size0; // record for level even level
  std::vector<int64_t> size1; // record for level odd level
  // 3. get partitioned index
  // 3.1 level 0
  printf("At step3.1\n");
  EncOneBlock pivot0 = samples[p/2]; // pivot for level 0
  int64_t leftRealNum = assignM(extD, indicator, left, right, pivot0);
  printf("Before memset\n");
  memset(&indicator[right], 1, sizeof(bool) * (hatM/2-leftRealNum));
  memset(&indicator[right+hatM/2-leftRealNum], 0, sizeof(bool) * (hatM-hatM/2-(M-leftRealNum)));
  printf("Before ORCompact\n");
  ORCompact(extD, indicator, 0, hatM);
  size0.push_back(hatM/2);
  size0.push_back(hatM-hatM/2);
  // 3.2 > level 1
  printf("At step3.2\n");
  int64_t readStart, secSize, realNum, dummyNum, rightRealNum;
  EncOneBlock pivot;
  for (int64_t i = 1; i < r; ++i) { // index of level
    printf("At level %d / %d\n", i, r-1);
    readStart = 0; // record read start for each section
    std::vector<int64_t> readSize, writeSize;
    if (i % 2 == 1) { // readSize in size0
      size1.clear();
      readSize = size0;
    } else { // readSize in size1
      size0.clear();
      readSize = size1;
    }
    for (int64_t j = 0; j < pow(2, i); ++j) { // j: #last level sections
      printf("Section number: %d\n", j);
      realNum = eServer.moveDummy(&extD[readStart], readSize[j]);
      dummyNum = readSize[j] - realNum;
      pivot = samples[(2*j+1)*p/pow(2, i+1)];
      printf("Real number: %d, dummy number: %d\n", realNum, dummyNum);
      leftRealNum = assignM(extD, indicator, readStart, readStart+realNum, pivot);
      rightRealNum = realNum - leftRealNum;
      printf("Before memset, %d, %d\n", readSize[j]/2-leftRealNum, dummyNum-(readSize[j]/2-leftRealNum));
      memset(&indicator[readStart+realNum], 1, sizeof(bool) * (readSize[j]/2-leftRealNum));
      // TODO: negative error
      memset(&indicator[readStart+realNum+(readSize[j]/2-leftRealNum)], 0, sizeof(bool) * (dummyNum-(readSize[j]/2-leftRealNum)));
      printf("Before ORCompact\n");
      ORCompact(extD, indicator, readStart, readStart+readSize[j]);
      writeSize.push_back(readSize[j]/2);
      writeSize.push_back(readSize[j]-readSize[j]/2);
      // each section ending part
      readStart += readSize[j];
    }
    if (i % 2 == 1) { // writeSize in size1
      size1 = writeSize;
    } else { // writeSize in size0
      size0 = writeSize;
    }
  }
  printf("At final step\n");
  if ((r - 1) % 2 == 1) { // writeSize in size1
    partitionIdx = size1;
  } else { // writeSize in size0
    partitionIdx = size0;
  }
  for (int64_t i = 1; i < partitionIdx.size(); ++i) {
    partitionIdx[i] += partitionIdx[i-1];
  }
  // 4. get the right order of index
  printf("At step4\n");
  partitionIdx.insert(partitionIdx.begin(), 0);
  // partitionIdx.push_back(hatM);
  // 5. for each section, call bitonic sort, [)
  printf("At step5\n");
  int64_t writeStart = 0;
  for (int64_t i = 0; i < partitionIdx.size() - 1; ++i) {
    printf("Begin at: %d, number: %d\n", partitionIdx[i], partitionIdx[i+1]-partitionIdx[i]);
    realNum = eServer.moveDummy(&extD[partitionIdx[i]], partitionIdx[i+1]-partitionIdx[i]);
    Bitonic bisort(eServer);
    bisort.smallBitonicSort(extD, partitionIdx[i], realNum, 0);
    memcpy(&D[writeStart], &extD[partitionIdx[i]], eServer.encOneBlockSize * realNum);
    writeStart += realNum;
  }
  delete indicator;
  delete extD;
}

std::pair<int64_t, int> ODS::OneLevelPartition(int inStructureId, int64_t inSize, std::vector<EncOneBlock> &pivots, int p, int outId) {
  if (inSize <= M) {
    resultId = inStructureId;
    resultN = inSize;
  }
  printf("In OneLevelPartition\n");
  int64_t hatN, M_prime, r, p0;
  calParams(inSize, p, hatN, M_prime, r, p0);
  // quantileCal(samples, 0, sampleSize, p0);
  int64_t boundary1 = ceil(1.0 * inSize / M_prime);
  int64_t boundary2 = ceil(1.0 * M_prime / B);
  int64_t dataBoundary = boundary2 * B;
  int64_t smallSectionSize = M / p0;
  int64_t bucketSize0 = boundary1 * smallSectionSize;
  int64_t totalEncB = boundary1 * smallSectionSize * p0;
  freeAllocate(outId, outId, totalEncB);
  int64_t Msize1, index1, index2, writeBackNum;
  EncOneBlock *trustedM3 = new EncOneBlock[boundary2 * B];
  bool *indicator = new bool[boundary2 * B];
  // memset(trustedM3, DUMMY<int>(), eServer.encOneBlockSize * boundary2 * B);
  std::vector<int64_t> partitionIdx;
  fyShuffle(inStructureId, inSize, 1);
  for (int64_t i = 0; i < boundary1; ++i) {
    printf("Partition progress: %ld / %ld\n", i, boundary1-1);
    Msize1 = std::min(boundary2 * B, inSize - i * boundary2 * B);
    eServer.nonEnc = 1;
    eServer.opOneLinearScanBlock(i * boundary2 * B, trustedM3, Msize1, inStructureId, 0, 0);
    // TODO: Simplify only for one level version, no dummy, input only
    // eServer.moveDummy(trustedM3, dataBoundary);
    int64_t blockNum = Msize1;
    if (seclevel == FULLY) {
      obliviousPWayPartition(trustedM3, indicator, 0, blockNum, pivots, 1, pivots.size()-2, partitionIdx);
      sort(partitionIdx.begin(), partitionIdx.end());
      partitionIdx.insert(partitionIdx.begin(), 0);
      partitionIdx.push_back(blockNum);
      for (int64_t j = 0; j < partitionIdx.size()-1; ++j) {
        index1 = partitionIdx[j];
        index2 = partitionIdx[j+1];
        writeBackNum = index2 - index1;
        if (writeBackNum > smallSectionSize) {
          printf("Overflow in small section %ld: %ld > %ld\n", j, writeBackNum, smallSectionSize);
        }
        eServer.nonEnc = 0;
        eServer.opOneLinearScanBlock(j * bucketSize0 + i * smallSectionSize, &trustedM3[index1], writeBackNum, outId, 1, smallSectionSize - writeBackNum);
      }
    } else {
      quickSortMulti(trustedM3, 0, blockNum-1, pivots, 1, p0, partitionIdx);
      sort(partitionIdx.begin(), partitionIdx.end());
      partitionIdx.insert(partitionIdx.begin(), -1);
      for (int j = 0; j < p0; ++j) {
        index1 = partitionIdx[j]+1;
        index2 = partitionIdx[j+1];
        writeBackNum = index2 - index1 + 1;
        if (writeBackNum > smallSectionSize) {
          printf("Overflow in small section M/p0: %d > %d\n", writeBackNum, smallSectionSize);
        }
        eServer.nonEnc = 0;
        eServer.opOneLinearScanBlock(j * bucketSize0 + i * smallSectionSize, &trustedM3[index1], writeBackNum, outId, 1, smallSectionSize - writeBackNum);
      }
    }
    // memset(trustedM3, DUMMY<int>(), eServer.encOneBlockSize * boundary2 * B);
    partitionIdx.clear();
  }
  delete [] trustedM3;
  delete [] indicator;
  // mbedtls_aes_free(&aes);
  if (bucketSize0 > M) {
    printf("Each section size is greater than M, adjst parameters: %d, %d\n", bucketSize0, M);
  }
  return {bucketSize0, p0};
}

void ODS::ObliviousSort(int64_t inSize, SortType sorttype, int inputId, int outputId1, int outputId2) {
  printf("In ODS\n");
  EncOneBlock *trustedM;
  if (inSize <= M) {
    trustedM = new EncOneBlock[M];
    eServer.nonEnc = 1;
    eServer.opOneLinearScanBlock(0, trustedM, N, inputId, 0, 0);
    Quick qsort(eServer, trustedM);
    qsort.quickSort(0, inSize - 1);
    freeAllocate(outputId1, outputId1, inSize);
    eServer.nonEnc = 0;
    eServer.opOneLinearScanBlock(0, trustedM, inSize, outputId1, 1, 0);
    delete [] trustedM;
    resultId = outputId1;
    resultN = inSize;
  }
  std::vector<EncOneBlock> trustedM2;
  int64_t sampleSize;
  // step1. get samples & pivots
  if ((int64_t)ceil(alpha * N) < M) {
    printf("In memory samples\n");
    sampleSize = Sample(inputId, inSize, trustedM2, sorttype);
    quantileCal(inSize, trustedM2, sampleSize, P);
    printf("IOcost: %f, %f\n", IOcost/N*B, IOcost);
  } else {
    int64_t n_prime = ceil(1.0 * alpha * N);
    printf("External memory samples\n");
    sampleSize = OcallSample(inputId, sampleId, sortedSampleId, N, M, n_prime, 0);
    ODSquantileCal(sampleId, sampleSize, sortedSampleId, trustedM2);
    printf("IOcost: %f, %f\n", IOcost/N*B, IOcost);
  }
  // step2. partition
  std::pair<int64_t, int> section = OneLevelPartition(inputId, inSize, trustedM2, P, outputId1);
  int64_t sectionSize = section.first;
  int sectionNum = section.second;
  int64_t k;
  printf("IOcost: %f, %f\n", IOcost/N*B, IOcost);
  // step3. Final sort
  if (sorttype == ODSTIGHT) {
    printf("In Tight Final\n");
    freeAllocate(outputId2, outputId2, inSize);
    trustedM = new EncOneBlock[M];
    int64_t j = 0;
    for (int i = 0; i < sectionNum; ++i) {
      printf("Final progress: %ld / %ld\n", i, sectionNum-1);
      eServer.nonEnc = 0;
      eServer.opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outputId1, 0, 0);
      k = eServer.moveDummy(trustedM, sectionSize);
      if (seclevel == FULLY) {
        internalObliviousSort(trustedM, 0, k);
      } else {
        Quick qsort(eServer, trustedM);
        qsort.quickSort(0, k - 1);
      }
      eServer.nonEnc = 1;
      eServer.opOneLinearScanBlock(j, trustedM, k, outputId2, 1, 0);
      j += k;
    }
    delete [] trustedM;
    resultId = outputId2;
    resultN = N;
  } else if (sorttype == ODSLOOSE) {
    printf("In Loose Final\n");
    int64_t totalLevelSize = sectionNum * sectionSize;
    freeAllocate(outputId2, outputId2, totalLevelSize);
    trustedM = new EncOneBlock[M];
    for (int i = 0; i < sectionNum; ++i) {
      printf("Final progress: %ld / %ld\n", i, sectionNum-1);
      eServer.nonEnc = 0;
      eServer.opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outputId1, 0, 0);
      k = eServer.moveDummy(trustedM, sectionSize);
      if (seclevel == FULLY) {
        internalObliviousSort(trustedM, 0, k);
      } else {
        Quick qsort(eServer, trustedM);
        qsort.quickSort(0, k - 1);
      }
      eServer.nonEnc = 1;
      eServer.opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outputId2, 1, 0);
    }
    delete [] trustedM;
    resultId = outputId2;
    resultN = totalLevelSize;
  }
}