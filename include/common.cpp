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


