// #include "enc.h"
#include "sort/bitonic.h"
#include "sort/bucket.h"
#include "sort/quick.h"
#include "sort/merge.h"
#include "sort/oq.h"
#include "shared.h"

double ALPHA, BETA, P;
int64_t N, M, BLOCK_DATA_SIZE;
int is_tight;

// trusted function
void callSort(int sortId, int structureId, int64_t paddedSize, int *resId, int64_t *resN, double *params) {
  // TODO: Utilize Memory alloction -- structureId
  N = params[0]; M = params[1]; BLOCK_DATA_SIZE = params[2];
  ALPHA = params[3];BETA = params[4];P = params[5];
  if (sortId == 0) {
    if (paddedSize / M <= 128) {
      is_tight = 1;
      *resId = ObliviousTightSort(structureId, paddedSize, structureId + 1, structureId);
    }
  } else if (sortId == 1) {
    if (paddedSize / M <= 128) {
      is_tight = 0;
      std::pair<int, int64_t> ans = ObliviousLooseSort(structureId, paddedSize, structureId + 1, structureId);
      *resId = ans.first;
      *resN = ans.second;
    }
  } else if (sortId == 2) {
     *resId = bucketOSort(structureId, paddedSize);
  } else if (sortId == 3) {
    int size = paddedSize / BLOCK_DATA_SIZE;
    int64_t *row1 = (int64_t*)malloc(BLOCK_DATA_SIZE * sizeof(int64_t));
    int64_t *row2 = (int64_t*)malloc(BLOCK_DATA_SIZE * sizeof(int64_t));
    bitonicSort(structureId, 0, size, 0, row1, row2);
    free(row1);
    free(row2);
  } else if (sortId == 4) {
    *resId = merge_sort(structureId, structureId+1);
  }
}

