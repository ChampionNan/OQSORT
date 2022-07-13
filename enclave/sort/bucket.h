#ifndef BUCKET_SORT_H
#define BUCKET_SORT_H

#include "../shared.h"

void mergeSplitHelper(Bucket_x *inputBuffer, int inputBufferLen, int* numRow2, int outputId0, int outputId1, int iter, int* bucketAddr, int outputStructureId);
void mergeSplit(int inputStructureId, int outputStructureId, int inputId0, int inputId1, int outputId0, int outputId1, int* bucketAddr, int* numRow1, int* numRow2, int iter);
void kWayMergeSort(int inputStructureId, int outputStructureId, int* numRow1, int* numRow2, int* bucketAddr, int bucketSize);
void bucketSort(int inputStructureId, int bucketId, int size, int dataStart);
int bucketOSort(int structureId, int size);

#endif // !BUCKET_SORT_H
