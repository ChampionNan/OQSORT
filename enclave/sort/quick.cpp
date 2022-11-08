#include "quick.h"
std::random_device dev2;
std::mt19937 rng2(dev2());

int partition(int *arr, int low, int high) {
  // TODO: random version
  // srand(unsigned(time(NULL)));
  std::uniform_int_distribution<int> dist{low, high};
  int randNum = dist(rng2);
  swapRow(arr + high, arr + randNum);
  int *pivot = arr + high;
  int i = low - 1;
  for (int j = low; j <= high - 1; ++j) {
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

int partition(Bucket_x *arr, int low, int high) {
  std::uniform_int_distribution<int> dist{low, high};
  int randNum = dist(rng2);
  // int randNum = rand() % (high - low + 1) + low;
  swapRow(arr + high, arr + randNum);
  Bucket_x *pivot = arr + high;
  int i = low - 1;
  for (int j = low; j <= high - 1; ++j) {
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


void quickSort(int *arr, int low, int high) {
  if (high > low) {
    int mid = partition(arr, low, high);
    quickSort(arr, low, mid - 1);
    quickSort(arr, mid + 1, high);
  }
}

void quickSort(Bucket_x *arr, int low, int high) {
  if (high > low) {
    // printf("In quickSort: %lu, %lu\n", low, high);
    int mid = partition(arr, low, high);
    quickSort(arr, low, mid - 1);
    quickSort(arr, mid + 1, high);
  }
}