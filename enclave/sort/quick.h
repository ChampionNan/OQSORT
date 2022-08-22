#ifndef QUICK_SORT_H
#define QUICK_SORT_H

#include "../shared.h"

int partition(int *arr, int low, int high);
int partition(Bucket_x *arr, int low, int high);
void quickSort(int *arr, int low, int high);
void quickSort(Bucket_x *arr, int low, int high);

#endif // !QUICK_SORT_H
