#include "quick.h"
std::random_device dev2;
std::mt19937 rng2(dev2());

int64_t partition(int64_t *arr, int64_t low, int64_t high) {
  // TODO: random version
  // srand(unsigned(time(NULL)));
  std::uniform_int_distribution<int64_t> dist{low, high};
  int64_t randNum = dist(rng2);
  swapRow(arr + high, arr + randNum);
  int64_t *pivot = arr + high;
  int64_t i = low - 1;
  for (int64_t j = low; j <= high - 1; ++j) {
    if (cmpHelper(pivot, arr + j)) {
      i++;
      if (i != j) {
        swapRow(arr + i, arr + j);
      }
    }
  }
  if (i + 1 != high) {
    swapRow(arr + i + 1, arr + high);
  }
  return (i + 1);
}

int64_t partition(Bucket_x *arr, int64_t low, int64_t high) {
  std::uniform_int_distribution<int64_t> dist{low, high};
  int64_t randNum = dist(rng2);
  // int randNum = rand() % (high - low + 1) + low;
  swapRow(arr + high, arr + randNum);
  Bucket_x *pivot = arr + high;
  int64_t i = low - 1;
  for (int64_t j = low; j <= high - 1; ++j) {
    if (cmpHelper(pivot, arr + j)) {
      i++;
      if (i != j) {
        swapRow(arr + i, arr + j);
      }
    }
  }
  if (i + 1 != high) {
    swapRow(arr + i + 1, arr + high);
  }
  return (i + 1);
}


void quickSort(int64_t *arr, int64_t low, int64_t high) {
  if (high > low) {
    int64_t mid = partition(arr, low, high);
    quickSort(arr, low, mid - 1);
    quickSort(arr, mid + 1, high);
  }
}

void quickSort(Bucket_x *arr, int64_t low, int64_t high) {
  if (high > low) {
    // printf("In quickSort: %lu, %lu\n", low, high);
    int64_t mid = partition(arr, low, high);
    quickSort(arr, low, mid - 1);
    quickSort(arr, mid + 1, high);
  }
}