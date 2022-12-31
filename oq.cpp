#include "bucket.h"
#include "quick.h"
#include "oq.h"

ODS::ODS(EnclaveServer &eServer, SortType sorttype, int inputId, double alpha, double beta, double gamma, int P, int is_tight) : eServer{eServer}, sorttype{sorttype}, inputId{inputId}, alpha{alpha}, beta{beta}, gamma{gamma}, P{P}, is_tight{is_tight} {
  N = eServer.N;
  M = eServer.M;
  B = eServer.B;
  outputId1 = inputId + 1;
  outputId2 = inputId;
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

int64_t ODS::Sample(int inStructureId, int64_t sampleSize, std::vector<EncOneBlock> &trustedM2) {
  printf("In sample\n");
  int64_t N_prime = sampleSize;
  // double alpha = (!is_rec) ? ALPHA : _ALPHA;
  int64_t n_prime = ceil(1.0 * alpha * N_prime);
  int64_t boundary = ceil(1.0 * N_prime / B);
  int64_t j = 0, Msize;
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

void ODS::quantileCal(std::vector<EncOneBlock> &samples, int64_t start, int64_t end, int p) {
  int64_t sampleSize = end - start;
  for (int i = 1; i < p; ++i) {
    samples[i] = samples[i * sampleSize / p];
  }
  samples[0].sortKey = INT_MIN;
  samples[p].sortKey = INT_MAX;
  samples.resize(p+1);
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

std::pair<int64_t, int> ODS::OneLevelPartition(int inStructureId, int64_t inSize, std::vector<EncOneBlock> &samples, int64_t sampleSize, int p, int outId) {
  if (inSize <= M) {
    resultId = inStructureId;
    resultN = inSize;
  }
  printf("In OneLevelPartition\n");
  int64_t hatN = ceil(1.0 * (1 + beta + gamma) * inSize);
  int64_t M_prime = ceil(1.0 * M / (1 + 2 * beta));
  int r = ceil(1.0 * log(hatN / M) / log(p));
  int p0 = ceil(1.0 * hatN / (M * pow(p, r - 1)));
  quantileCal(samples, 0, sampleSize, p0);
  int64_t boundary1 = ceil(1.0 * inSize / M_prime);
  int64_t boundary2 = ceil(1.0 * M_prime / B);
  int64_t dataBoundary = boundary2 * B;
  int64_t smallSectionSize = M / p0;
  int64_t bucketSize0 = boundary1 * smallSectionSize;
  int64_t totalEncB = boundary1 * smallSectionSize * p0;
  freeAllocate(outId, outId, totalEncB);
  
  int64_t Msize1, index1, index2, writeBackNum;
  EncOneBlock *trustedM3 = new EncOneBlock[boundary2 * B];
  memset(trustedM3, DUMMY<int>(), eServer.encOneBlockSize * boundary2 * B);
  std::vector<int64_t> partitionIdx;
  // TODO: ? change B to 1
  fyShuffle(inStructureId, inSize, B);
  for (int64_t i = 0; i < boundary1; ++i) {
    // Read one M' memory block after fisher-yates shuffle
    printf("OneLevel %d/%d\n", i, boundary1);
    Msize1 = std::min(boundary2 * B, inSize - i * boundary2 * B);
    eServer.nonEnc = 1;
    eServer.opOneLinearScanBlock(i * boundary2 * B, trustedM3, Msize1, inStructureId, 0, 0);
    int64_t blockNum = eServer.moveDummy(trustedM3, dataBoundary);
    quickSortMulti(trustedM3, 0, blockNum-1, samples, 1, p0, partitionIdx);
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
    memset(trustedM3, DUMMY<int>(), eServer.encOneBlockSize * boundary2 * B);
    partitionIdx.clear();
  }
  delete [] trustedM3;
  // mbedtls_aes_free(&aes);
  if (bucketSize0 > M) {
    printf("Each section size is greater than M, adjst parameters: %d, %d", bucketSize0, M);
  }
  return {bucketSize0, p0};
}

void ODS::ObliviousSort(int64_t inSize) {
  EncOneBlock *trustedM;
  printf("In ObliviousTightSort\n");
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
  int64_t realNum = Sample(inputId, inSize, trustedM2);
  std::pair<int64_t, int> section = OneLevelPartition(inputId, inSize, trustedM2, realNum, P, outputId1);
  int64_t sectionSize = section.first;
  int sectionNum = section.second;
  int64_t k;
  if (sorttype == ODSTIGHT) {
    freeAllocate(outputId2, outputId2, inSize);
    trustedM = new EncOneBlock[M];
    int64_t j = 0;
    printf("In final\n");
    for (int i = 0; i < sectionNum; ++i) {
      eServer.nonEnc = 0;
      eServer.opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outputId1, 0, 0);
      k = eServer.moveDummy(trustedM, sectionSize);
      Quick qsort(eServer, trustedM);
      qsort.quickSort(0, k-1);
      eServer.nonEnc = 1;
      eServer.opOneLinearScanBlock(j, trustedM, k, outputId2, 1, 0);
      j += k;
    }
    delete [] trustedM;
    resultId = outputId2;
    resultN = N;
  } else if (sorttype == ODSLOOSE) {
    int64_t totalLevelSize = sectionNum * sectionSize;
    freeAllocate(outputId2, outputId2, totalLevelSize);
    trustedM = new EncOneBlock[M];
    for (int i = 0; i < sectionNum; ++i) {
      eServer.nonEnc = 0;
      eServer.opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outputId1, 0, 0);
      k = eServer.moveDummy(trustedM, sectionSize);
      Quick qsort(eServer, trustedM);
      qsort.quickSort(0, k - 1);
      eServer.nonEnc = 1;
      eServer.opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outputId2, 1, 0);
    }
    delete [] trustedM;
    resultId = outputId2;
    resultN = totalLevelSize;
  }
}