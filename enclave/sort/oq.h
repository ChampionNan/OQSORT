#ifndef OQ_SORT_H
#define OQ_SORT_H

#include "../shared.h"
#include <utility>

int Hypergeometric(int NN, int Msize, int n_prime);
void shuffle(int *array, int n);
int SampleTight(int inStructureId, int samplesId, int *trustedM2);
int SampleLoose(int inStructureId, int samplesId);
int quantileCalT(int *samples, int start, int end, int p, int *trustedM1);
int quantileCalL(int sampleId, int start, int end, int p, int *trustedM1);
std::pair<int, int> MultiLevelPartitionT(int inStructureId, int *samples, int sampleSize, int p, int outStructureId1);
std::pair<int, int> MultiLevelPartitionL(int inStructureId, int sampleId, int sampleSize, int p, int outStructureId1);
int ObliviousTightSort(int inStructureId, int inSize, int sampleId, int outStructureId1, int outStructureId2);
std::pair<int, int> ObliviousLooseSort(int inStructureId, int inSize, int sampleId, int outStructureId1, int outStructureId2);


#endif // !OQ_SORT_H