// #include "enc.h"
#include "sort/bitonic.h"
#include "sort/bucket.h"
#include "sort/quick.h"
#include "sort/oq.h"


// trusted function
void callSort(int sortId, int structureId, int paddedSize, int *resId, int *resN) {
  if (sortId == 0) {
    *resId = ObliviousTightSort(structureId, paddedSize, structureId + 1, structureId + 2, structureId + 3, structureId + 4, structureId + 5);
  } else if (sortId == 1) {
    *resId = ObliviousLooseSort(structureId, paddedSize, structureId + 1, structureId + 2, structureId + 3, structureId + 4, structureId + 5, resN);
  } else if (sortId == 2) {
     *resId = bucketOSort(structureId, paddedSize);
  } else if (sortId == 3) {
    int size = paddedSize / BLOCK_DATA_SIZE;
    int *row1 = (int*)malloc(BLOCK_DATA_SIZE * sizeof(int));
    int *row2 = (int*)malloc(BLOCK_DATA_SIZE * sizeof(int));
    bitonicSort(structureId, 0, size, 0, row1, row2);
    free(row1);
    free(row2);
  }
}
