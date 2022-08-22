#ifndef OQ_SORT_H
#define OQ_SORT_H

#include "../shared.h"
#include <utility>

int Hypergeometric(int NN, int Msize, double n_prime);
void shuffle(int *array, int n);
int SampleTight(int inStructureId, int samplesId);
int SampleLoose(int inStructureId, int samplesId);
int quantileCal(int sampleId, int start, int end, int p, int *trustedM1);
int ProcessL(int LIdIn, int LIdOut, int lsize);
std::pair<int, int> MultiLevelPartition(int inStructureId, int sampleId, int LIdIn, int LIdOut, int sampleSize, int p, int outStructureId1);
int ObliviousTightSort(int inStructureId, int inSize, int sampleId, int LIdIn, int LIdOut, int outStructureId1, int outStructureId2);
int ObliviousLooseSort(int inStructureId, int inSize, int sampleId, int LIdIn, int LIdOut, int outStructureId1, int outStructureId2, int *resN);

#endif // !OQ_SORT_H