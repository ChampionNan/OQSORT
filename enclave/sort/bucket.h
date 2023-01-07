#ifndef BUCKET_SORT_H
#define BUCKET_SORT_H

#include "../shared.h"
#include <random>

class Bucket {
  public:
    Bucket(EnclaveServer &eServer, int inputId);
    bool isTargetIterK(int randomKey, int iter, int64_t k, int64_t num);
    void mergeSplitHelper(EncOneBlock *inputBuffer, int64_t* numRow1, int64_t* numRow2, int64_t* inputId, int64_t* outputId, int iter, int64_t k, int64_t* bucketAddr, int outputStructureId);
    void mergeSplit(int inputStructureId, int outputStructureId, int64_t *inputId, int64_t *outputId, int64_t k, int64_t* bucketAddr, int64_t* numRow1, int64_t* numRow2, int iter);
    void kWayMergeSort(int inputStructureId, int outputStructureId, int64_t* numRow1, int64_t* bucketAddr, int64_t bucketNum);
    void bucketSort(int inputStructureId, int64_t size, int64_t dataStart);
    int bucketOSort();

  private:
    EnclaveServer eServer;
    int64_t N, M;
    int B;
    int inStructureId, outId1, outId2;
    int HEAP_NODE_SIZE;
    int MERGE_BATCH_SIZE;
    int FAN_OUT;
    int64_t Z; // bucketSize
    int64_t bucketNum;
    int ranBinAssignIters;
    int64_t k1, oldK;
    std::random_device rd;
    std::mt19937 rng{rd()};
};

#endif // !BUCKET_SORT_H
