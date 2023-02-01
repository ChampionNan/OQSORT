#ifndef COMMON_STRUCTURE_H
#define COMMON_STRUCTURE_H

#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <random>
#include <cmath>

#define NUM_STRUCTURES 10
#define MEM_IN_ENCLAVE 5
#define PAYLOAD 29 // 1, 29

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

enum InputType {
  BOOST, 
  SETINMAIN
};

enum OutputType {
  TERMINAL, 
  FILEOUT
};

enum EncMode {
  OFB,
  GCM
};

enum SecLevel {
  FULLY,
  PARTIAL
};

struct Bucket_x {
  int x;
  int key;
};

struct EncOneBlock {
  int sortKey;    // used for sorting 
  int primaryKey; // tie-breaker when soryKey equals
  int payLoad[PAYLOAD];
  // bucket sort random key, also used for dummy flag
  int randomKey;  

  EncOneBlock() {
    sortKey = DUMMY<int>();
  }
  friend EncOneBlock operator*(const int &flag, const EncOneBlock &y) {
    EncOneBlock res;
    res.sortKey = flag * y.sortKey;
    res.primaryKey = flag * y.primaryKey;
    // res.payLoad = flag * y.payLoad;
    for (int i = 0; i < PAYLOAD; ++i) {
      res.payLoad[i] = flag * y.payLoad[i];
    }
    res.randomKey = flag * y.randomKey;
    return res;
  }
  friend EncOneBlock operator+(const EncOneBlock &x, const EncOneBlock &y) {
    EncOneBlock res;
    res.sortKey = x.sortKey + y.sortKey;
    res.primaryKey = x.primaryKey + y.primaryKey;
    // res.payLoad = x.payLoad + y.payLoad;
    for (int i = 0; i < PAYLOAD; ++i) {
      res.payLoad[i] = x.payLoad[i] + y.payLoad[i];
    }
    res.randomKey = x.randomKey + y.randomKey;
    return res;
  }
  friend bool operator<(const EncOneBlock &a, const EncOneBlock &b) {
    if (a.sortKey < b.sortKey) {
      return true;
    } else if (a.sortKey > b.sortKey) {
      return false;
    } else if (a.primaryKey < b.primaryKey) {
      return true;
    } else if (a.primaryKey > b.primaryKey) {
      return false;
    }
    return true; // equal
  }

  bool operator=(const EncOneBlock &a) {
    sortKey = a.sortKey;
    primaryKey = a.primaryKey;
    for (int i = 0; i < PAYLOAD; ++i) {
      payLoad[i] = a.payLoad[i];
    }
    randomKey = a.randomKey;
    return true;
  }
};

extern double IOcost;
extern EncOneBlock *arrayAddr[NUM_STRUCTURES]; 

int64_t greatestPowerOfTwoLessThan(double n);
int64_t smallestPowerOfKLargerThan(int64_t n, int k);
int64_t calBucketSize(int sigma, int64_t N, int64_t M, int B);
int64_t ceil_divide(int64_t n, int64_t q);
void shuffle(EncOneBlock *a, int64_t sampleSize);
int64_t Hypergeometric(int64_t &N, int64_t M, int64_t &n);

#endif // !COMMON_STRUCTURE_H