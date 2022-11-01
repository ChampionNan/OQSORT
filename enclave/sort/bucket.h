#ifndef BUCKET_SORT_H
#define BUCKET_SORT_H

#include "../shared.h"

void initMerge(int64_t size);
bool isTargetIterK(int64_t randomKey, int64_t iter, int64_t k, int64_t num);
void mergeSplitHelper(Bucket_x *inputBuffer, int64_t* numRow1, int64_t* numRow2, int64_t* inputId, int64_t* outputId, int64_t iter, int64_t k, int64_t* bucketAddr, int64_t outputStructureId);
void mergeSplit(int inputStructureId, int outputStructureId, int64_t *inputId, int64_t *outputId, int64_t k, int64_t* bucketAddr, int64_t* numRow1, int64_t* numRow2, int64_t iter);
void kWayMergeSort(int inputStructureId, int outputStructureId, int64_t* numRow1, int64_t* bucketAddr, int64_t bucketNum);
void bucketSort(int inputStructureId, int64_t size, int64_t dataStart);
int bucketOSort(int structureId, int64_t size);

#endif // !BUCKET_SORT_H
