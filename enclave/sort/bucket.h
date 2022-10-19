#ifndef BUCKET_SORT_H
#define BUCKET_SORT_H

#include "../shared.h"

bool isTargetIterK(int randomKey, int iter, int k, int num);
void mergeSplitHelper(Bucket_x *inputBuffer, int* numRow1, int* numRow2, int* inputId, int* outputId, int iter, int k, int* bucketAddr, int outputStructureId);
void mergeSplit(int inputStructureId, int outputStructureId, int *inputId, int *outputId, int k, int* bucketAddr, int* numRow1, int* numRow2, int iter);
void kWayMergeSort(int inputStructureId, int outputStructureId, int* numRow1, int* bucketAddr, int bucketSize);
void bucketSort(int inputStructureId, int size, int dataStart);
int bucketOSort(int structureId, int size);

#endif // !BUCKET_SORT_H
