#include "common.h"

int64_t greatestPowerOfTwoLessThan(double n) {
    int64_t k = 1;
    while (k > 0 && k < n) {
        k = k << 1;
    }
    return k >> 1;
}

int64_t smallestPowerOfKLargerThan(int64_t n, int k) {
  int64_t num = 1;
  while (num > 0 && num < n) {
    num = num * k;
  }
  return num;
}

int64_t calBucketSize(int sigma, int64_t N, int64_t M, int B) {
  double kappa = 1.0 * sigma * 0.693147;
  double Z = 6 * (kappa + log(2.0 * N));
  Z = 6 * (kappa + log(2.0 * N / Z));
  Z = ceil(Z / B) * B;
  double thresh = 1.0 * M / Z;
  int FAN_OUT = greatestPowerOfTwoLessThan(thresh)/2;
  int64_t bucketNum = smallestPowerOfKLargerThan(ceil(2.0 * N / Z), 2);
  int64_t bucketSize = bucketNum * Z;
  return bucketSize;
}

int64_t ceil_divide(int64_t n, int64_t q) {
  int64_t p = n / q;
  if (n % q == 0) {
    return p;
  }
  return p + 1;
}

void shuffle(EncOneBlock *a, int64_t sampleSize) {
  std::random_device rd;
  std::mt19937 rng{rd()};
  for (int64_t k = 0; k < sampleSize; ++k) {
    std::uniform_int_distribution<int64_t> unif(k, sampleSize - 1);
    int64_t l = unif(rng);
    EncOneBlock temp = a[k];
    a[k] = a[l];
    a[l] = temp;
  }
}

int64_t Hypergeometric(int64_t &N, int64_t M, int64_t &n) {
  int64_t m = 0;
  std::random_device rd;
  std::mt19937 rng{rd()};
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

int64_t OcallSample(int inStructureId, int sampleId, int sortedSampleId, int64_t N, int64_t M, int64_t n_prime, int is_tight) {
  int64_t N_prime = N;
  int64_t boundary = ceil(1.0 * N_prime / M);
  int64_t realNum = 0;
  int64_t readStart = 0;
  EncOneBlock *trustedM1 = new EncOneBlock[M];
  int64_t m = 0, Msize;
  freeAllocate(sampleId, sampleId, n_prime);
  for (int64_t i = 0; i < boundary; ++i) {
    Msize = std::min(M, N - i * M);
    m = Hypergeometric(N_prime, Msize, n_prime);
    printf("Sampling progress: %ld / %ld, m: %d\n", i, boundary-1, m);
    if (is_tight || (!is_tight && m > 0)) {
      memcpy(trustedM1, arrayAddr[inStructureId] + readStart, Msize * sizeof(EncOneBlock));
      readStart += Msize;
      shuffle(trustedM1, Msize);
      memcpy(arrayAddr[sampleId] + realNum, trustedM1, m * sizeof(EncOneBlock));
      realNum += m;
    }
  }
  delete [] trustedM1;
  return realNum;
}

/* OCall functions */
void ocall_print_string(const char *str) {
  /* Proxy/Bridge will check the length and null-terminate the input string to prevent buffer overflow. */
  printf("%s", str);
  fflush(stdout);
}

// index: Block Index, blockSize: bytes
void OcallRB(int64_t index, int* buffer, size_t blockSize, int structureId) {
  // std::cout<< "In OcallRB\n";
  memcpy(buffer, (int*)(&((arrayAddr[structureId])[index])), blockSize);
  IOcost += 1;
}

// index: Block index, blockSize: bytes
void OcallWB(int64_t index, int* buffer, size_t blockSize, int structureId) {
  // std::cout<< "In OcallWB\n";
  memcpy((int*)(&((arrayAddr[structureId])[index])), buffer, blockSize);
  IOcost += 1;
}

// allocate encB for outside memory
void freeAllocate(int structureIdM, int structureIdF, size_t size) {
  // 1. Free arrayAddr[structureId]
  if (arrayAddr[structureIdF]) {
    delete [] arrayAddr[structureIdF];
  }
  // 2. malloc new asked size (allocated in outside)
  if (size <= 0) {
    return;
  }
  EncOneBlock *addr = (EncOneBlock*)malloc(size * sizeof(EncOneBlock));
  memset(addr, DUMMY<int>(), size * sizeof(EncOneBlock));
  // 3. assign malloc address to arrayAddr
  arrayAddr[structureIdM] = addr;
  return ;
}

// shuffle data in B block size
void fyShuffle(int structureId, size_t size, int B) {
  if (size % B != 0) {
    printf("Error! Not B's time.\n"); // Still do the shuffling
  }
  int64_t total_blocks = size / B;
  EncOneBlock *trustedM3 = new EncOneBlock[B];
  int64_t k;
  int64_t eachSec = size / 100;
  int swapSize = sizeof(EncOneBlock) * B;
  std::random_device rd;
  std::mt19937 rng{rd()};
  // switch block i & block k
  for (int64_t i = total_blocks-1; i >= 0; i--) {
    if (i % eachSec == 0) {
      printf("Shuffle progress %ld / %ld\n", i, total_blocks-1);
    }
    std::uniform_int_distribution<int64_t> dist(0, i);
    k = dist(rng);
    memcpy(trustedM3, arrayAddr[structureId] + k * B, swapSize);
    memcpy(arrayAddr[structureId] + k * B, arrayAddr[structureId] + i * B, swapSize);
    memcpy(arrayAddr[structureId] + i * B, trustedM3, swapSize);
  }
  std::cout << "Finished floyd shuffle\n";
}

