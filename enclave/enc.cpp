#include "enc.h"

#include "sort/bitonic.h"
#include "sort/bucket.h"
#include "sort/quick.h"


// trusted function
void callSort(int sortId, int structureId, int paddedSize, int *resId) {
  // bitonic sort
  if (sortId == 1) {
     *resId = bucketOSort(structureId, paddedSize);
  }
  if (sortId == 3) {
    int size = paddedSize / BLOCK_DATA_SIZE;
    int *row1 = (int*)oe_malloc(BLOCK_DATA_SIZE * sizeof(int));
    int *row2 = (int*)oe_malloc(BLOCK_DATA_SIZE * sizeof(int));
    bitonicSort(structureId, 0, size, 0, row1, row2);
    oe_free(row1);
    oe_free(row2);
  }
}
