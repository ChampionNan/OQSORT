#ifndef OQ_SORT_H
#define OQ_SORT_H

#include "../shared.h"
#include <utility>

__uint128_t prf(__uint128_t a);
int64_t encrypt(int64_t index);
void pseudo_init(int64_t size);
void floydSampler(int64_t n, int64_t k, std::vector<int64_t> &x);
int64_t Sample(int inStructureId, int64_t sampleSize, std::vector<int64_t> &trustedM2, int is_tight, int is_rec);
void quantileCal(std::vector<int64_t> &samples, int64_t start, int64_t end, int64_t p);
int64_t partitionMulti(int64_t *arr, int64_t low, int64_t high, int64_t pivot);
void quickSortMulti(int64_t *arr, int64_t low, int64_t high, std::vector<int64_t> pivots, int64_t left, int64_t right, std::vector<int64_t> &partitionIdx);
std::pair<int64_t, int64_t> OneLevelPartition(int inStructureId, int64_t inSize, std::vector<int64_t> &samples, int64_t sampleSize, int64_t p, int outStructureId1, int is_rec);
int ObliviousTightSort(int inStructureId, int64_t inSize, int outStructureId1, int outStructureId2);
std::pair<int64_t, int64_t> ObliviousLooseSort(int inStructureId, int64_t inSize, int outStructureId1, int outStructureId2);

#endif // !OQ_SORT_H