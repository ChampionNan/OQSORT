#ifndef OQ_SORT_H
#define OQ_SORT_H

#include "shared.h"
#include <utility>

class ODS {
  public:
    ODS(EnclaveServer &eServer, double alpha, double beta, double gamma, int P, int is_tight, SecLevel seclevel, int sampleId);
    void calParams(int64_t inSize, int p, int64_t &hatN, int64_t &M_prime, int64_t &r, int64_t &p0);
    void floydSampler(int64_t n, int64_t k, std::vector<int64_t> &x);
    int64_t Sample(int inStructureId, int64_t sampleSize, std::vector<EncOneBlock> &trustedM2, SortType sorttype);
    void ODSquantileCal(int sampleId, int64_t sampleSize, int sortedSampleId, std::vector<EncOneBlock>& pivots);
    void quantileCal(int64_t inSize, std::vector<EncOneBlock> &samples, int64_t sampleSize, int p);
    void quantileCal2(std::vector<EncOneBlock> &samples, int64_t start, int64_t end, int p);
    int64_t partitionMulti(EncOneBlock *arr, int64_t low, int64_t high, EncOneBlock pivot);
    void quickSortMulti(EncOneBlock *arr, int64_t low, int64_t high, std::vector<EncOneBlock> pivots, int left, int right, std::vector<int64_t> &partitionIdx);
    int64_t sumArray(bool *M, int64_t left, int64_t right);
    void OROffCompact(EncOneBlock *D, bool *M, int64_t left, int64_t right, int64_t z);
    void ORCompact(EncOneBlock *D, bool *M, int64_t left, int64_t right);
    int64_t assignM(EncOneBlock *arr, bool *M, int64_t left, int64_t right, EncOneBlock pivot);
    void obliviousPWayPartition(EncOneBlock *D, bool *M, int64_t low, int64_t high, std::vector<EncOneBlock> pivots, int left, int right, std::vector<int64_t> &partitionIdx);
    void internalObliviousSort(EncOneBlock *D, int64_t left, int64_t right);
    std::pair<int64_t, int> OneLevelPartition(int inStructureId, int64_t inSize, std::vector<EncOneBlock> &samples, int p, int ouputId1);
    void ObliviousSort(int64_t inSize, SortType sorttype, int inputId, int outputId1, int outputId2);

  public:
    int resultId;
    int64_t resultN;

  private:
    EnclaveServer eServer;
    int64_t N, M, smallM;
    int B;
    int is_rec, is_tight;
    double alpha, beta, gamma;
    int P;
    int sampleId, sortedSampleId;
    SecLevel seclevel;
    std::random_device rd;
    std::mt19937 rng{rd()};
};

#endif // !OQ_SORT_H