#include "bucket.h"
#include "quick.h"
#include "oq.h"

#include <mbedtls/aes.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

mbedtls_aes_context aes;
unsigned char key[16];
int64_t base;
int64_t max_num;
int ROUND = 3;
std::random_device dev;
std::mt19937 rng(dev()); 

__uint128_t prf(__uint128_t a) {
  unsigned char input[16] = {0};
  unsigned char encrypt_output[16] = {0};
  for (int i = 0; i < 16; ++i) {
    input[i] = (a >> (120 - i * 8)) & 0xFF;
  }
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input, encrypt_output);
  __uint128_t res = 0;
  for (int i = 0; i < 16; ++i) {
    res |= encrypt_output[i] << (120 - i * 8);
  }
  return res;
}

int64_t encrypt(int64_t index) {
  int64_t l = index / (1 << base);
  int64_t r = index % (1 << base);
  __uint128_t e;
  int64_t temp, i = 1;
  while (i <= ROUND) {
    e = prf((r << 16 * 8 - base) + i);
    temp = r;
    r = l ^ (e >> 16 * 8 - base);
    l = temp;
    i += 1;
  }
  return (l << base) + r;
}

void pseudo_init(int64_t size) {
  mbedtls_aes_init(&aes);
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_entropy_context entropy;
  char *pers = "aes generate key";
  int ret;
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (unsigned char *) pers, strlen(pers))) != 0) {
    printf(" failed\n ! mbedtls_ctr_drbg_init returned -0x%04x\n", -ret);
    return;
  }
  if((ret = mbedtls_ctr_drbg_random(&ctr_drbg, key, 16)) != 0) {
    printf(" failed\n ! mbedtls_ctr_drbg_random returned -0x%04x\n", -ret);
    return ;
  }
  mbedtls_aes_setkey_enc(&aes, key, 128);
  base = ceil(1.0 * log2(size) / 2);
  max_num = 1 << 2 * base;
}

void floydSampler(int64_t n, int64_t k, std::vector<int64_t> &x) {
  std::unordered_set<int64_t> H;
  for (int64_t i = n - k; i < n; ++i) {
    x.push_back(i);
  }
  std::random_device dev;
  std::mt19937 rng(dev()); 
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

int64_t Sample(int inStructureId, int64_t sampleSize, std::vector<int64_t> &trustedM2, int is_tight, int is_rec=0) {
  printf("In sample\n");
  int64_t N_prime = sampleSize;
  // double alpha = (!is_rec) ? ALPHA : _ALPHA;
  double alpha = ALPHA;
  int64_t n_prime = ceil(1.0 * alpha * N_prime);
  int64_t boundary = ceil(1.0 * N_prime / BLOCK_DATA_SIZE);
  int64_t j = 0, Msize;
  int64_t *trustedM1 = (int64_t*)malloc(BLOCK_DATA_SIZE * sizeof(int64_t));
  std::vector<int64_t> sampleIdx;
  floydSampler(N_prime, n_prime, sampleIdx);
  for (int64_t i = 0; i < boundary; ++i) {
    if (is_tight) {
      Msize = std::min((int64_t)BLOCK_DATA_SIZE, N_prime - i * BLOCK_DATA_SIZE);
      opOneLinearScanBlock(i * BLOCK_DATA_SIZE, trustedM1, Msize, inStructureId, 0, 0);
      while ((j < n_prime) && (sampleIdx[j] >= i * BLOCK_DATA_SIZE) && (sampleIdx[j] < (i+1) * BLOCK_DATA_SIZE)) {
        trustedM2.push_back(trustedM1[sampleIdx[j] % BLOCK_DATA_SIZE]);
        j += 1;
      }
    } else if ((!is_tight) && (sampleIdx[j] >= i * BLOCK_DATA_SIZE) && (sampleIdx[j] < (i+1) * BLOCK_DATA_SIZE)) {
      Msize = std::min((int64_t)BLOCK_DATA_SIZE, N_prime - i * BLOCK_DATA_SIZE);
      opOneLinearScanBlock(i * BLOCK_DATA_SIZE, trustedM1, Msize, inStructureId, 0, 0);
      while ((sampleIdx[j] >= i * BLOCK_DATA_SIZE) && (sampleIdx[j] < (i+1) * BLOCK_DATA_SIZE)) {
        trustedM2.push_back(trustedM1[sampleIdx[j] % BLOCK_DATA_SIZE]);
        j += 1;
        if (j >= n_prime) break;
      }
      if (j >= n_prime) break;
    }
  }
  sort(trustedM2.begin(), trustedM2.end());
  return n_prime;
}

void quantileCal(std::vector<int64_t> &samples, int64_t start, int64_t end, int64_t p) {
  int64_t sampleSize = end - start;
  for (int64_t i = 1; i < p; ++i) {
    samples[i] = samples[i * sampleSize / p];
  }
  samples[0] = INT_MIN;
  samples[p] = INT_MAX;
  samples.resize(p+1);
  samples.shrink_to_fit();
  return ;
}

int64_t partitionMulti(int64_t *arr, int64_t low, int64_t high, int64_t pivot) {
  int64_t i = low - 1;
  for (int64_t j = low; j < high + 1; ++j) {
    if (pivot > arr[j]) {
      i += 1;
      swapRow(arr + i, arr + j);
    }
  }
  return i;
}

void quickSortMulti(int64_t *arr, int64_t low, int64_t high, std::vector<int64_t> pivots, int64_t left, int64_t right, std::vector<int64_t> &partitionIdx) {
  int64_t pivotIdx, pivot, mid;
  if (right >= left) {
    pivotIdx = (left + right) >> 1;
    pivot = pivots[pivotIdx];
    mid = partitionMulti(arr, low, high, pivot);
    partitionIdx.push_back(mid);
    quickSortMulti(arr, low, mid, pivots, left, pivotIdx-1, partitionIdx);
    quickSortMulti(arr, mid+1, high, pivots, pivotIdx+1, right, partitionIdx);
  }
}

std::pair<int64_t, int64_t> OneLevelPartition(int inStructureId, int64_t inSize, std::vector<int64_t> &samples, int64_t sampleSize, int64_t p, int outStructureId1, int is_rec) {
  if (inSize <= M) {
    return {inSize, 1};
  }
  printf("In Onelevel Partition\n");
  // double beta = (!is_rec) ? BETA : _BETA;
  double beta = BETA;
  int64_t hatN = ceil(1.0 * (1 + 2 * beta) * inSize);
  int64_t M_prime = ceil(1.0 * M / (1 + 2 * beta));
  int64_t r = ceil(1.0 * log(hatN / M) / log(p));
  int64_t p0 = ceil(1.0 * hatN / (M * pow(p, r - 1)));
  quantileCal(samples, 0, sampleSize, p0);
  int64_t boundary1 = ceil(1.0 * inSize / M_prime);
  int64_t boundary2 = ceil(1.0 * M_prime / BLOCK_DATA_SIZE);
  int64_t dataBoundary = boundary2 * BLOCK_DATA_SIZE;
  int64_t smallSectionSize = M / p0;
  int64_t bucketSize0 = boundary1 * smallSectionSize;
  freeAllocate(outStructureId1, outStructureId1, boundary1 * smallSectionSize * p0);
  
  int64_t Msize1, Msize2, index1, index2, writeBackNum;
  int64_t total_blocks = ceil(1.0 * inSize / BLOCK_DATA_SIZE);
  int64_t *trustedM3 = (int64_t*)malloc(sizeof(int64_t) * boundary2 * BLOCK_DATA_SIZE);
  memset(trustedM3, DUMMY, sizeof(int64_t) * boundary2 * BLOCK_DATA_SIZE);
  int64_t *shuffleB = (int64_t*)malloc(sizeof(int64_t) * BLOCK_DATA_SIZE);
  std::vector<int64_t> partitionIdx;
  // Finish FFSEM implementation in c++
  pseudo_init(total_blocks);
  int64_t index_range = max_num;
  int64_t k = 0, read_index;
  for (int64_t i = 0; i < boundary1; ++i) {
    for (int64_t j = 0; j < boundary2; ++j) {
      read_index = encrypt(k);
      while (read_index >= total_blocks) {
        k += 1;
        if (k == index_range) {
          k = -1;
          break;
        }
        read_index = encrypt(k);
      }
      if (k == -1) {
        break;
      }
      Msize1 = std::min((int64_t)BLOCK_DATA_SIZE, inSize - read_index * BLOCK_DATA_SIZE);
      opOneLinearScanBlock(read_index * BLOCK_DATA_SIZE, &trustedM3[j*BLOCK_DATA_SIZE], Msize1, inStructureId, 0, 0);
      k += 1;
      if (k == index_range) {
        break;
      }
    }
    int64_t blockNum = moveDummy(trustedM3, dataBoundary);
    quickSortMulti(trustedM3, 0, blockNum-1, samples, 1, p0, partitionIdx);
    sort(partitionIdx.begin(), partitionIdx.end());
    partitionIdx.insert(partitionIdx.begin(), -1);
    for (int64_t j = 0; j < p0; ++j) {
      index1 = partitionIdx[j]+1;
      index2 = partitionIdx[j+1];
      writeBackNum = index2 - index1 + 1;
      if (writeBackNum > smallSectionSize) {
        printf("Overflow in small section M/p0: %ld > %ld\n", writeBackNum, smallSectionSize);
      }
      opOneLinearScanBlock(j * bucketSize0 + i * smallSectionSize, &trustedM3[index1], writeBackNum, outStructureId1, 1, smallSectionSize - writeBackNum);
    }
    memset(trustedM3, DUMMY, sizeof(int64_t) * boundary2 * BLOCK_DATA_SIZE);
    partitionIdx.clear();
  }
  free(trustedM3);
  free(shuffleB);
  if (bucketSize0 > M) {
    printf("Each section size is greater than M, adjst parameters: %ld, %ld", bucketSize0, M);
  }
  return {bucketSize0, p0};
}

int ObliviousTightSort(int inStructureId, int64_t inSize, int outStructureId1, int outStructureId2) {
  int64_t *trustedM;
  printf("In ObliviousTightSort\n");
  if (inSize <= M) {
    trustedM = (int64_t*)malloc(sizeof(int64_t) * M);
    opOneLinearScanBlock(0, trustedM, inSize, inStructureId, 0, 0);
    quickSort(trustedM, 0, inSize - 1);
    freeAllocate(outStructureId1, outStructureId1, inSize);
    opOneLinearScanBlock(0, trustedM, inSize, outStructureId1, 1, 0);
    free(trustedM);
    return outStructureId1;
  }
  std::vector<int64_t> trustedM2;
  int64_t realNum = Sample(inStructureId, inSize, trustedM2, is_tight);
  // printf("Till Sample IOcost: %f\n", 1.0*IOcost/N*BLOCK_DATA_SIZE);
  std::pair<int64_t, int64_t> section= OneLevelPartition(inStructureId, inSize, trustedM2, realNum, P, outStructureId1, 0);
  // printf("Till Partition IOcost: %f\n", 1.0*IOcost/N*BLOCK_DATA_SIZE);
  int64_t sectionSize = section.first;
  int64_t sectionNum = section.second;
  // TODO: IN order to reduce memory, can replace outStructureId2 with inStructureId
  freeAllocate(outStructureId2, outStructureId2, inSize);
  trustedM = (int64_t*)malloc(sizeof(int64_t) * M);
  int64_t j = 0, k;
  printf("In final\n");
  for (int64_t i = 0; i < sectionNum; ++i) {
    opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outStructureId1, 0, 0);
    k = moveDummy(trustedM, sectionSize);
    quickSort(trustedM, 0, k-1);
    opOneLinearScanBlock(j, trustedM, k, outStructureId2, 1, 0);
    j += k;
  }
  // printf("Till Final IOcost: %f\n", 1.0*IOcost/N*BLOCK_DATA_SIZE);
  free(trustedM);
  return outStructureId2;
}

std::pair<int64_t, int64_t> ObliviousLooseSort(int inStructureId, int64_t inSize, int outStructureId1, int outStructureId2) {
  printf("In ObliviousLooseSort\n");
  int64_t *trustedM;
  if (inSize <= M) {
    trustedM = (int64_t*)malloc(sizeof(int64_t) * M);
    opOneLinearScanBlock(0, trustedM, inSize, inStructureId, 0, 0);
    quickSort(trustedM, 0, inSize - 1);
    opOneLinearScanBlock(0, trustedM, inSize, outStructureId1, 1, 0);
    free(trustedM);
    return {outStructureId1, inSize};
  }
  std::vector<int64_t> trustedM2;
  int64_t realNum = Sample(inStructureId, inSize, trustedM2, is_tight);
  // printf("Till Sample IOcost: %f\n", 1.0*IOcost/N*BLOCK_DATA_SIZE);
  std::pair<int64_t, int64_t> section = OneLevelPartition(inStructureId, inSize, trustedM2, realNum, P, outStructureId1, 0);
  // printf("Till Partition IOcost: %f\n", 1.0*IOcost/N*BLOCK_DATA_SIZE);
  int64_t sectionSize = section.first;
  int64_t sectionNum = section.second;
  int64_t totalLevelSize = sectionNum * sectionSize;
  int64_t k;
  freeAllocate(outStructureId2, outStructureId2, totalLevelSize);
  trustedM = (int64_t*)malloc(sizeof(int64_t) * M);
  for (int64_t i = 0; i < sectionNum; ++i) {
    opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outStructureId1, 0, 0);
    k = moveDummy(trustedM, sectionSize);
    quickSort(trustedM, 0, k - 1);
    opOneLinearScanBlock(i * sectionSize, trustedM, sectionSize, outStructureId2, 1, 0);
  }
  // printf("Till Final IOcost: %f\n", 1.0*IOcost/N*BLOCK_DATA_SIZE);
  return {outStructureId2, totalLevelSize};
}