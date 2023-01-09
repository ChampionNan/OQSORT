#include "quick.h"

Quick::Quick(EnclaveServer &eServer, EncOneBlock *arr) : eServer{eServer}, arr{arr} {}

int Quick::partition(int64_t low, int64_t high) {
  std::uniform_int_distribution<int64_t> dist{low, high};
  int64_t randNum = dist(rng);
  eServer.swapRow(arr, high, randNum);
  EncOneBlock *pivot = arr + high;
  int64_t i = low - 1;
  for (int64_t j = low; j <= high - 1; ++j) {
    if (eServer.cmpHelper(pivot, arr + j)) {
      i++;
      if (i != j) {
        eServer.swapRow(arr, i, j);
      }
    }
  }
  if (i + 1 != high) {
    eServer.swapRow(arr, i + 1, high);
  }
  return (i + 1);
}

void Quick::quickSort(int64_t low, int64_t high) {
  if (high > low) {
    int64_t mid = partition(low, high);
    quickSort(low, mid - 1);
    quickSort(mid + 1, high);
  }
}

QuickV::QuickV(EnclaveServer &eSErver) : eServer{eServer} {}

int64_t QuickV::partition(std::vector<EncOneBlock> &arr, int64_t low, int64_t high) {
  std::uniform_int_distribution<int64_t> dist{low, high};
  int64_t randNum = dist(rng);
  eServer.swap(arr, high, randNum);
  EncOneBlock pivot = arr[high];
  int64_t i = low - 1;
  for (int64_t j = low; j <= high - 1; ++j) {
    if (eServer.cmpHelper(&pivot, &arr[j])) {
      i++;
      if (i != j) {
        eServer.swap(arr, i, j);
      }
    }
  }
  if (i + 1 != high) {
    eServer.swap(arr, i + 1, high);
  }
  return (i + 1);
}

void QuickV::quickSort(std::vector<EncOneBlock> &arr, int64_t low, int64_t high) {
  if (high > low) {
    int64_t mid = partition(arr, low, high);
    quickSort(arr, low, mid - 1);
    quickSort(arr, mid + 1, high);
  }
}