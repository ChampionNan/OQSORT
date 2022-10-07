#ifndef OQ_SORT_H
#define OQ_SORT_H

#include "../shared.h"
#include <utility>

int Hypergeometric(int NN, int Msize, int n_prime);
void shuffle(int *array, int n);
void floydSampler(int n, int k, std::vector<int> &x);
int Sample(int inStructureId, int sampleSize, std::vector<int> &trustedM2, int is_tight, int is_rec);
void SampleRec(int inStructureId, int sampleId, int sortedSampleId, int is_tight, std::vector<std::vector<int>>& pivots);
void quantileCal(std::vector<int> &samples, int start, int end, int p);
int partitionMulti(int *arr, int low, int high, int pivot);
void quickSortMulti(int *arr, int low, int high, std::vector<int> pivots, int left, int right, std::vector<int> &partitionIdx);
std::pair<int, int> OneLevelPartition(int inStructureId, int inSize, std::vector<int> &samples, int sampleSize, int p, int outStructureId1, int is_rec);
std::pair<int, int> TwoLevelPartition(int inStructureId, std::vector<std::vector<int>>& pivots, int p, int outStructureId1, int outStructureId2);
int ObliviousTightSort(int inStructureId, int inSize, int outStructureId1, int outStructureId2, int is_tight);
int ObliviousTightSort2(int inStructureId, int inSize, int sampleId, int sortedSampleId, int outStructureId1, int outStructureId2);
std::pair<int, int> ObliviousLooseSort(int inStructureId, int inSize, int outStructureId1, int outStructureId2, int is_tight);
std::pair<int, int> ObliviousLooseSort2(int inStructureId, int inSize, int sampleId, int sortedSampleId, int outStructureId1, int outStructureId2);
void ObliviousLooseSortRec(int sampleId, int sampleSize, int sortedSampleId, std::vector<std::vector<int>>& pivots);


#endif // !OQ_SORT_H