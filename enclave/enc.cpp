// #include "enc.h"
#include "sort/bitonic.h"
#include "sort/bucket.h"
#include "sort/quick.h"
#include "sort/merge.h"
#include "sort/oq.h"
#include "shared.h"

double ALPHA, BETA, P;
int N, M, BLOCK_DATA_SIZE;
int is_tight;

void testIO(int IOnum, int inId) {
  int totalB = IOnum / 2;
  int *buffer = (int*)malloc(sizeof(int) * BLOCK_DATA_SIZE);
  freeAllocate(inId, inId, totalB*BLOCK_DATA_SIZE);
  // std::random_device dev;
  // std::mt19937 rng(dev());
  // std::uniform_int_distribution<int> dist{0, totalB-1};
  int index;
  aes_init();
  nonEnc = 0;
  for (int i = 0; i < totalB; ++i) {
    // index = dist(rng);
    opOneLinearScanBlock(i * BLOCK_DATA_SIZE, buffer, BLOCK_DATA_SIZE, inId, 0, 0);
    opOneLinearScanBlock(i * BLOCK_DATA_SIZE, buffer, BLOCK_DATA_SIZE, inId, 1, 0);
  }/*
  for (int i = 0; i < totalB; ++i) {
    // index = dist(rng);
    opOneLinearScanBlock(i * BLOCK_DATA_SIZE, buffer, BLOCK_DATA_SIZE, inId, 1, 0);
  }*/
  free(buffer);
}

// trusted function
void callSort(int sortId, int structureId, int paddedSize, int *resId, int *resN, double *params) {
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
      std::pair<int, int> ans = ObliviousLooseSort(structureId, paddedSize, structureId + 1, structureId);
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
    *resId = merge_sort(structureId, structureId+1);
  } else if (sortId == 5) {
    testIO(10000000, structureId);
  }
}

