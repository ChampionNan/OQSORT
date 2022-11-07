#include "test.h"

void testIO(int64_t IOnum, int inId, int outId) {
  int64_t totalB = IOnum / BLOCK_DATA_SIZE / 2;
  int64_t *buffer = (int64_t*)malloc(sizeof(int64_t) * BLOCK_DATA_SIZE);
  freeAllocate(inId, inId, N);
  freeAllocate(outId, outId, N);
  for (int64_t i = 0; i < totalB; ++i) {
    opOneLinearScanBlock(i * BLOCK_DATA_SIZE, buffer, BLOCK_DATA_SIZE, inId, 0, 0);
  }
  for (int64_t i = 0; i < totalB; ++i) {
    opOneLinearScanBlock(i * BLOCK_DATA_SIZE, buffer, BLOCK_DATA_SIZE, outId, 1, 0);
  }
}