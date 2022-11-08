#ifndef OQ_SORT_H
#define OQ_SORT_H

#include "../shared.h"
#include <utility>

__uint128_t prf(__uint128_t a);
int encrypt(int index);
void pseudo_init(int size);
void floydSampler(int n, int k, std::vector<int> &x);
int Sample(int inStructureId, int sampleSize, std::vector<int> &trustedM2, int is_tight, int is_rec);
void quantileCal(std::vector<int> &samples, int start, int end, int p);
int partitionMulti(int *arr, int low, int high, int pivot);
void quickSortMulti(int *arr, int low, int high, std::vector<int> pivots, int left, int right, std::vector<int> &partitionIdx);
std::pair<int, int> OneLevelPartition(int inStructureId, int inSize, std::vector<int> &samples, int sampleSize, int p, int outStructureId1, int is_rec);
int ObliviousTightSort(int inStructureId, int inSize, int outStructureId1, int outStructureId2);
std::pair<int, int> ObliviousLooseSort(int inStructureId, int inSize, int outStructureId1, int outStructureId2);

#endif // !OQ_SORT_H