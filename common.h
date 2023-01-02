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

struct Bucket_x {
  int x;
  int key;
};

struct EncOneBlock {
  int sortKey;    // used for sorting 
  int primaryKey; // tie-breaker when soryKey equals
  int payLoad;
  int randomKey;  // bucket sort random key
  __uint128_t iv; // used for encryption & decryption

  EncOneBlock() {
    sortKey = DUMMY<int>();
  }
  friend EncOneBlock operator*(const int &flag, const EncOneBlock &y) {
    EncOneBlock res;
    res.sortKey = flag * y.sortKey;
    res.primaryKey = flag * y.primaryKey;
    res.payLoad = flag * y.payLoad;
    res.randomKey = flag * y.randomKey;
    res.iv = flag * y.iv;
    return res;
  }
  friend EncOneBlock operator+(const EncOneBlock &x, const EncOneBlock &y) {
    EncOneBlock res;
    res.sortKey = x.sortKey + y.sortKey;
    res.primaryKey = x.primaryKey + y.primaryKey;
    res.payLoad = x.payLoad + y.payLoad;
    res.randomKey = x.randomKey + y.randomKey;
    res.iv = x.iv + y.iv;
    return res;
  }
  friend bool operator<(const EncOneBlock &a, const EncOneBlock &b) {
    if (a.sortKey < b.sortKey) {
      return true;
    } else if (a.sortKey > b.sortKey) {
      return false;
    } else {
      if (a.primaryKey < b.primaryKey) {
        return true;
      } else if (a.primaryKey > b.primaryKey) {
        return false;
      } else {
        if (a.payLoad < b.payLoad) {
          return true;
        } else if (a.payLoad > b.payLoad) {
          return false;
        } else {
          if (a.randomKey < b.randomKey) {
            return true;
          } else if (a.randomKey > b.randomKey) {
            return false;
          }
        }
      }
    }
    return true; // equal
  }
};

extern double IOcost;
extern EncOneBlock *arrayAddr[NUM_STRUCTURES]; 

int64_t greatestPowerOfTwoLessThan(double n);
int64_t smallestPowerOfKLargerThan(int64_t n, int k);
int64_t calBucketSize(int sigma, int64_t N, int64_t M, int B);
int64_t ceil_divide(int64_t n, int64_t q);
void ocall_print_string(const char *str);
void OcallRB(int64_t index, int* buffer, size_t blockSize, int structureId);
void OcallWB(int64_t index, int* buffer, size_t blockSize, int structureId);
void freeAllocate(int structureIdM, int structureIdF, size_t size);
void fyShuffle(int structureId, size_t size, int B);

#endif // !COMMON_STRUCTURE_H