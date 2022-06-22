#ifndef _ENC_H_
#define _ENC_H_

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <cassert>
#include <cmath>
#include <string.h>

#include "./include/common.h"

// Function Declaration
int greatestPowerOfTwoLessThan(int n);
int smallestPowerOfTwoLargerThan(int n);
void smallBitonicMerge(int *a, int start, int size, int flipped);
void smallBitonicSort(int *a, int start, int size, int flipped);
void opOneLinearScanBlock(int index, int* block, size_t blockSize, int structureId, int write);
void bitonicMerge(int structureId, int start, int size, int flipped, int* row1, int* row2);
void bitonicSort(int structureId, int start, int size, int flipped, int* row1, int* row2);
int callSort(int sortId, int structureId);

void padWithDummy(int structureId, int start, int realNum);
bool isTargetBitOne(int randomKey, int iter);
void mergeSplitHelper(Bucket_x *inputBuffer, int inputBufferLen, int *numRow2, int outputId0, int outputId1, int iter, int* bucketAddr, int outputStructureId);
void mergeSplit(int inputStructureId, int outputStructureId, int inputId0, int inputId1, int outputId0, int outputId1, int* bucketAddr, int* numRow1, int* numRow2, int iter);
void kWayMergeSort(int inputStructureId, int outputStructureId, int* numRow1, int* numRow2,int* bucketAddr, int bucketSize);
void swapRow(Bucket_x *a, Bucket_x *b);
bool cmpHelper(Bucket_x *a, Bucket_x *b);
int partition(Bucket_x *arr, int low, int high);
void quickSort(Bucket_x *arr, int low, int high);
void bucketSort(int inputStructureId, int bucketId, int size, int dataStart);
int bucketOSort(int structureId, int size);

#endif // !_ENC_H_