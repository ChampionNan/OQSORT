#include "bucket.h"
#include "quick.h"
#include "bitonic.h"
#include "oq.h"

#include <algorithm>


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
  // p0 = ceil(1.0 * hatN / (M * pow(p, r - 1)));
  p0 = p;
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
  delete [] trustedM1;
  // NOTE: QuickV qvsort(eServer); qvsort.quickSort(trustedM2, 0, trustedM2.size()-1);
  Bitonic bisort(eServer);
  bisort.smallBitonicSort(trustedM2, 0, trustedM2.size());
  return n_prime;
}

int64_t ODS::Hypergeometric(int64_t &N, int64_t M, int64_t &n) {
  int64_t m = 0;
  std::uniform_real_distribution<double> dist(0.0, 1.0);
  double rate = double(n) / N;
  for (int64_t j = 0; j < M; ++j) {
    if (dist(rng) < rate) {
      m += 1;
      n -= 1;
    }
    N -= 1;
    rate = double(n) / double(N);
  }
  return m;
}

int64_t ODS::SampleEx(int inStructureId, int sampleId) {
  if ((N % M) || (M % B)) {
    // NOTE: N%M, M%B constant
    printf("Error N, M, B setting! \n");
    return -1;
  }
  int64_t N_prime = N;
  int64_t n_prime = ceil(1.0 * alpha * N_prime);
  int64_t boundary = N_prime / M;
  int64_t eachM = ceil(2 * alpha * M);
  int64_t realNum = 0, eachMSize;
  EncOneBlock *trustedM1 = new EncOneBlock[M];
  bool *indicator = new bool[M];
  int64_t m = 0;
  EncOneBlock dummyB;
  eServer.setDummy(&dummyB, 1);
  freeAllocate(sampleId, sampleId, eachM * boundary, eServer.SSD);
  std::binomial_distribution<int> binom(B, alpha);
  for (int64_t i = 0; i < boundary; ++i) {
    printf("SampleEx progress: %ld / %ld\n", i, boundary-1);
    // each memory load
    // eServer.setDummy(trustedM1, M);
    for (int64_t j = 0; j < M / B; ++j) {
      // each Block Size
      int num_to_sample = binom(rng);
      eServer.opOneLinearScanBlock(i*M+j*B, &trustedM1[j*B], B, inStructureId, 0, 0);
      if (seclevel == FULLY) {
        for (int k = 0; k < B; ++k) {
          int flag = (k < num_to_sample) ? 1 : 0;
          int sampleStart = flag ? k : (num_to_sample - 1);
          EncOneBlock tmp = trustedM1[j*B+sampleStart];
          std::uniform_int_distribution<int> unif(sampleStart, num_to_sample-1);
          int l = unif(rng);
          dummyB.primaryKey = eServer.tie_breaker++;
          trustedM1[j*B+k] = flag * trustedM1[j*B+l] + (!flag) * dummyB;
          trustedM1[j*B+l] = flag * tmp + (!flag) * trustedM1[j*B+l];
        }
      } else {
        for (int k = 0; k < num_to_sample; ++k) {
          EncOneBlock tmp = trustedM1[j*B+k];
          std::uniform_int_distribution<int> unif(k, num_to_sample-1);
          int l = unif(rng);
          trustedM1[j*B+k] = trustedM1[j*B+l];
          trustedM1[j*B+l] = tmp;
        }
        eServer.setDummy(&trustedM1[j*B+num_to_sample], B-num_to_sample);
      }
    }
    if (seclevel == FULLY) {
      int flag;
      eachMSize = 0;
      for (int64_t k = 0; k < M; ++k) {
        flag = (trustedM1[k].sortKey != DUMMY<int>()) ? 1 : 0;
        indicator[k] = 1 * flag;
        eachMSize += 1 * flag;
      }
      ORCompact(trustedM1, indicator, 0, M);
    } else {
      // seclevel=PARTIAL
      eachMSize = eServer.moveDummy(trustedM1, M);
    }
    if (eachMSize > eachM) {
      // sample number > 2 * alpha * M
      printf("Data overflow when sampling %ld, %ld\n", eachMSize, eachM);
      return -1;
    }
    eServer.opOneLinearScanBlock(i * eachM, trustedM1, eachM, sampleId, 1, 0);
    realNum += eachMSize;
  }
  delete [] trustedM1;
  delete [] indicator;
  return realNum;
}

// get pivots from external stored samples
void ODS::ODSquantileCal(int sampleId, int64_t sampleSize, int64_t xDummySampleSize, int sortedSampleId, std::vector<EncOneBlock>& pivots) {
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
    quantileIdx.push_back(i * xDummySampleSize / P);
  }
  printf("ODSquantileCal before sort\n");
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
  // NOTE: If it is proper
  a.sortKey = std::numeric_limits<int>::min();
  a.primaryKey = std::numeric_limits<int>::min();
  b.sortKey = std::numeric_limits<int>::max();
  b.primaryKey = std::numeric_limits<int>::max();
  pivots.insert(pivots.begin(), a);
  pivots.push_back(b);
  delete [] trustedM;
}

void ODS::quantileCal(int64_t inSize, std::vector<EncOneBlock> &samples, int64_t sampleSize, int p) {
  int64_t hatN, M_prime, r, p0;
  calParams(inSize, p, hatN, M_prime, r, p0);
  for (int64_t i = 1; i < p0; ++i) {
    samples[i] = samples[i * sampleSize / p0];
  }
  samples[0].sortKey = std::numeric_limits<int>::min();
  samples[0].primaryKey = std::numeric_limits<int>::min();
  samples[p0].sortKey = std::numeric_limits<int>::max();
  samples[p0].primaryKey = std::numeric_limits<int>::max();
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
    // if (flag) eServer.regswap(&D[left], &D[left + 1]);
    eServer.oswap128((uint128_t*)&D[left], (uint128_t*)&D[left + 1], flag);
  } else if (n > 2) {
    OROffCompact(D, M, left, left + n2, z % n2);
    OROffCompact(D, M, left + n2, left + n, (z + m % n2) % n2);
    bool s1 = (((z % n2) + m) >= n2) ? 1 : 0;
    bool s2 = (z >= n2) ? 1 : 0;
    bool s = s1 ^ s2;
    for (int64_t i = 0; i < n2; ++i) {
      bool s3 = (i >= ((z + m) % n2)) ? 1 : 0;
      bool b = s ^ s3;
      // if (b) eServer.regswap(&D[left + i], &D[left + i + n2]);
      eServer.oswap128((uint128_t*)&D[left + i], (uint128_t*)&D[left + i + n2], b);
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
    // if (b) eServer.regswap(&D[left + i], &D[left + i + n1]);
    eServer.oswap128((uint128_t*)&D[left + i], (uint128_t*)&D[left + i + n1], b);
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
  freeAllocate(outId, outId, totalEncB, eServer.SSD);
  int64_t Msize1, index1, index2, writeBackNum;
  EncOneBlock *trustedM3 = new EncOneBlock[boundary2 * B];
  bool *indicator = new bool[boundary2 * B];
  // memset(trustedM3, DUMMY<int>(), eServer.encOneBlockSize * boundary2 * B);
  // Failure probability
  if (bucketSize0 > M) {
    printf("Each section size is greater than M, adjst parameters: %ld, %ld\n", bucketSize0, M);
  }
  std::vector<int64_t> partitionIdx;
  if (!eServer.SSD) {
    fyShuffle(inStructureId, inSize, 1);
  }
  // used for psedo-random permutation
  int total_blocks = ceil(1.0 * inSize / B);
  eServer.base = ceil(1.0 * log2(total_blocks) / 2);
  eServer.max_num = 1 << 2 * eServer.base;
  int64_t index_range = eServer.max_num;
  int64_t k = 0, read_index;
  for (int64_t i = 0; i < boundary1; ++i) {
    printf("Partition progress: %ld / %ld\n", i, boundary1-1);
    Msize1 = std::min(boundary2 * B, inSize - i * boundary2 * B);
    if (!eServer.SSD) {
      eServer.opOneLinearScanBlock(i * boundary2 * B, trustedM3, Msize1, inStructureId, 0, 0);
    } else {
      int64_t b2 = ceil(1.0 * Msize1 / B);
      for (int64_t j = 0; j < b2; ++j) {
        read_index = eServer.encrypt(k);
        while (read_index >= total_blocks) {
          k += 1;
          if (k == index_range) {
            k = -1;
            break;
          }
          read_index = eServer.encrypt(k);
        }
        if (k == -1) break;
        eServer.opOneLinearScanBlock(read_index * B, &trustedM3[j*B], std::min((int64_t)B, inSize - read_index * B), inStructureId, 0, 0);
        k += 1;
        if (k == index_range) break;
      }
    }
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
          printf("Overflow in small section M/p0: %ld > %ld\n", writeBackNum, smallSectionSize);
        }
        eServer.opOneLinearScanBlock(j * bucketSize0 + i * smallSectionSize, &trustedM3[index1], writeBackNum, outId, 1, smallSectionSize - writeBackNum);
      }
    }
    // memset(trustedM3, DUMMY<int>(), eServer.encOneBlockSize * boundary2 * B);
    partitionIdx.clear();
  }
  delete [] trustedM3;
  delete [] indicator;
  // mbedtls_aes_free(&aes);
  return {bucketSize0, p0};
}

void ODS::ObliviousSort(int64_t inSize, SortType sorttype, int inputId, int outputId1, int outputId2) {
  printf("In ODS\n");
  EncOneBlock *trustedM;
  eServer.nonEnc = 0;
  if (inSize < M) {
    trustedM = new EncOneBlock[M];
    eServer.opOneLinearScanBlock(0, trustedM, N, inputId, 0, 0);
    Quick qsort(eServer, trustedM);
    qsort.quickSort(0, inSize - 1);
    freeAllocate(outputId1, outputId1, inSize, eServer.SSD);
    eServer.opOneLinearScanBlock(0, trustedM, inSize, outputId1, 1, 0);
    delete [] trustedM;
    resultId = outputId1;
    resultN = inSize;
  }
  clock_t startS, startP, startF, end;
  std::vector<EncOneBlock> trustedM2;
  int64_t sampleSize, xDummySampleSize;
  // step1. get samples & pivots
  if ((int64_t)ceil(alpha * N) < M) {
    printf("In memory samples\n");
    startS = time(NULL);
    sampleSize = Sample(inputId, inSize, trustedM2, sorttype);
    quantileCal(inSize, trustedM2, sampleSize, P);
  } else {
    int64_t n_prime = ceil(1.0 * alpha * N);
    printf("External memory samples\n");
    startS = time(NULL);
    if (sorttype == ODSLOOSE) {
      sampleSize = eServer.Sample(inputId, sampleId, N, M, n_prime);
      xDummySampleSize = sampleSize;
    } else {
      xDummySampleSize = SampleEx(inputId, sampleId);
      sampleSize = ceil(2 * alpha * M) * (N / M);
    }
    ODSquantileCal(sampleId, sampleSize, xDummySampleSize, sortedSampleId, trustedM2);
    freeAllocate(sampleId, sampleId, 0, eServer.SSD);
    freeAllocate(sortedSampleId, sortedSampleId, 0, eServer.SSD);
  }
  // step2. partition
  startP = time(NULL);
  printf("Sampling Time: %lf, IOtime: %lf, IOcost: %lf\n", (double)(startP-startS-eServer.getIOtime()), eServer.getIOtime(), eServer.getIOcost()*B/N);
  eServer.IOtime = 0;
  eServer.IOcost = 0;
  std::pair<int64_t, int> section = OneLevelPartition(inputId, inSize, trustedM2, P, outputId1);
  int64_t sectionSize = section.first;
  int sectionNum = section.second;
  int64_t k;
  // step3. Final sort
  startF = time(NULL);
  printf("Partition Time: %lf, IOtime: %lf, IOcost: %lf\n", (double)(startF-startP-eServer.getIOtime()), eServer.getIOtime(), eServer.getIOcost()*B/N);
  // printf("SecSize: %ld\n", sectionSize);
  eServer.IOtime = 0;
  eServer.IOcost = 0;
  if (sorttype == ODSTIGHT) {
    printf("In Tight Final\n");
    freeAllocate(outputId2, outputId2, inSize, eServer.SSD);
    trustedM = new EncOneBlock[M];
    int64_t j = 0;
    startF = time(NULL);
    for (int i = 0; i < sectionNum; ++i) {
      printf("Final progress: %d / %d\n", i, sectionNum-1);
      eServer.opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outputId1, 0, 0);
      k = eServer.moveDummy(trustedM, sectionSize);
      if (seclevel == FULLY) {
        // internalObliviousSort(trustedM, 0, k);
        Bitonic bisort(eServer);
        bisort.smallBitonicSort(trustedM, 0, k, 0);
        // smallBitonicSort(eServer, trustedM, 0, k, 0);
      } else {
        Quick qsort(eServer, trustedM);
        qsort.quickSort(0, k - 1);
      }
      eServer.opOneLinearScanBlock(j, trustedM, k, outputId2, 1, 0);
      j += k;
    }
    delete [] trustedM;
    resultId = outputId2;
    resultN = N;
  } else if (sorttype == ODSLOOSE) {
    printf("In Loose Final\n");
    int64_t totalLevelSize = sectionNum * sectionSize;
    freeAllocate(outputId2, outputId2, totalLevelSize, eServer.SSD);
    trustedM = new EncOneBlock[M];
    startF = time(NULL);
    for (int i = 0; i < sectionNum; ++i) {
      printf("Final progress: %d / %d\n", i, sectionNum-1);
      eServer.opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outputId1, 0, 0);
      k = eServer.moveDummy(trustedM, sectionSize);
      if (seclevel == FULLY) {
        // internalObliviousSort(trustedM, 0, k);
        Bitonic bisort(eServer);
        bisort.smallBitonicSort(trustedM, 0, k, 0);
        // smallBitonicSort(eServer, trustedM, 0, k, 0);
      } else {
        Quick qsort(eServer, trustedM);
        qsort.quickSort(0, k - 1);
      }
      eServer.opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outputId2, 1, 0);
    }
    delete [] trustedM;
    resultId = outputId2;
    resultN = totalLevelSize;
  }
  end = time(NULL);
  printf("Final Time: %lf, IOtime: %lf, IOcost: %lf\n", (double)(end-startF-eServer.getIOtime()), eServer.getIOtime(), eServer.getIOcost()*B/N);
  eServer.IOtime = 0;
}