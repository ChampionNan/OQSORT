#ifndef BUCKET_SORT_H
#define BUCKET_SORT_H

#include "../shared.h"

class Bucket {
  public:
    Bucket(EnclaveServer &eServer, int inputId);
    bool isTargetIterK(int randomKey, int iter, int k, int num);
    void mergeSplitHelper(EncOneBlock *inputBuffer, int* numRow1, int* numRow2, int* inputId, int* outputId, int iter, int k, int* bucketAddr, int outputStructureId);
    void mergeSplit(int inputStructureId, int outputStructureId, int *inputId, int *outputId, int k, int* bucketAddr, int* numRow1, int* numRow2, int iter);
    void kWayMergeSort(int inputStructureId, int outputStructureId, int* numRow1, int* bucketAddr, int64_t bucketNum);
    void bucketSort(int inputStructureId, int64_t size, int64_t dataStart);
    int bucketOSort();

  private:
    EnclaveServer &eServer;
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
    std::random_device dev;
    std::mt19937 rng(dev());
}

#endif // !BUCKET_SORT_H
