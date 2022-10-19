// #include "enc.h"
#include "sort/bitonic.h"
#include "sort/bucket.h"
#include "sort/quick.h"
#include "sort/merge.h"
#include "sort/oq.h"
#include "shared.h"

double ALPHA, BETA, P, _ALPHA, _BETA, _P;
int N, M, BLOCK_DATA_SIZE;

// trusted function
void callSort(int sortId, int structureId, int paddedSize, int *resId, int *resN, double *params) {
  // TODO: Utilize Memory alloction -- structureId
  N = (int)params[0]; M = (int)params[1]; BLOCK_DATA_SIZE = (int)params[2];
  ALPHA = params[3];BETA = params[4];P = params[5];
  _ALPHA = params[6];_BETA = params[7];_P = params[8];
  if (sortId == 0) {
    if (paddedSize / M < 100) {
      *resId = ObliviousTightSort(structureId, paddedSize, structureId + 1, structureId, 1);
    } else {
      *resId = ObliviousTightSort2(structureId, paddedSize, structureId+1, structureId+2, structureId+1, structureId);
    }
  } else if (sortId == 1) {
    if (paddedSize / M < 100) {
      std::pair<int, int> ans = ObliviousLooseSort(structureId, paddedSize, structureId + 1, structureId, 0);
      *resId = ans.first;
      *resN = ans.second;
    } else {
      std::pair<int, int> ans = ObliviousLooseSort2(structureId, paddedSize, structureId + 1, structureId + 2, structureId + 1, structureId);
      *resId = ans.first;
      *resN = ans.second;
    }
  } else if (sortId == 2) {
     *resId = bucketOSort(structureId, paddedSize);
  } else if (sortId == 3) {
    int size = paddedSize / BLOCK_DATA_SIZE;
    int *row1 = (int*)malloc(BLOCK_DATA_SIZE * sizeof(int));
    int *row2 = (int*)malloc(BLOCK_DATA_SIZE * sizeof(int));
    bitonicSort(structureId, 0, size, 0, row1, row2);
    free(row1);
    free(row2);
  } else if (sortId == 4) {
    *resId = merge_sort(structureId, structureId+1, paddedSize);
  }
}

