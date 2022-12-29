#ifndef COMMON_STRUCTURE_H
#define COMMON_STRUCTURE_H

#include <cstdint>
#include <limits>
#include <cmath>

template<typename T>
constexpr T DUMMY() {
    return std::numeric_limits<T>::max();
}

enum SortType {
  ODSTIGHT,
  ODSLOOSE,
  BUCKET,
  BITONIC
};

enum OutputType {
  TERMINAL, 
  FILEOUT
};

enum EncMode {
  OFB,
  GCM
};

struct Bucket_x {
  int x;
  int key;
};

struct EncOneBlock {
  int sortKey;    // used for sorting 
  int primaryKey; // tie-breaker when soryKey equals
  int payLoad;
  int key;        // bucket sort random key
  __uint128_t iv; // used for encryption & decryption
  EncOneBlock() {
    sortKey = DUMMY<int>(); // TODO: ?
  };
};

// TODO: set up these two
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

#endif // !COMMON_STRUCTURE_H