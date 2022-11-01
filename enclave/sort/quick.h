#ifndef QUICK_SORT_H
#define QUICK_SORT_H

#include "../shared.h"

int64_t partition(int64_t *arr, int64_t low, int64_t high);
void quickSort(int64_t *arr, int64_t low, int64_t high);
int64_t partition(Bucket_x *arr, int64_t low, int64_t high);
void quickSort(Bucket_x *arr, int64_t low, int64_t high);

#endif // !QUICK_SORT_H
