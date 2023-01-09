#ifndef QUICK_SORT_H
#define QUICK_SORT_H

#include "shared.h"

class Quick {
  public:
    Quick(EnclaveServer &eServer, EncOneBlock *arr);
    int partition(int64_t low, int64_t high);
    void quickSort(int64_t low, int64_t high);
  private:
    EnclaveServer eServer;
    EncOneBlock *arr;
    std::random_device rd;
    std::mt19937 rng{rd()};
};

class QuickV {
  public:
    QuickV(EnclaveServer &eServer);
    int64_t partition(std::vector<EncOneBlock> &arr, int64_t low, int64_t high);
    void quickSort(std::vector<EncOneBlock> &arr, int64_t low, int64_t high);
  private:
    EnclaveServer eServer;
    std::random_device rd;
    std::mt19937 rng{rd()};
};

#endif // !QUICK_SORT_H
