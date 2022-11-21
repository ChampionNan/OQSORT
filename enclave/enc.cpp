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
  printf("In testIO\n");
  int totalB = IOnum;
  int *buffer = (int*)malloc(sizeof(int) * BLOCK_DATA_SIZE);
  freeAllocate(inId, inId, totalB*BLOCK_DATA_SIZE);
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<int> dist{0, totalB-1};
  int index;
  // aes_init();
  nonEnc = 1;
  for (int i = 0; i < totalB; ++i) {
    // index = dist(rng);
    opOneLinearScanBlock(i * BLOCK_DATA_SIZE, buffer, BLOCK_DATA_SIZE, inId, 0, 0);
  }
  free(buffer);
}

// IOnum: # EncBlock 2200
void testEncDec(int IOnum) {
  printf("In testEncDec\n");
  int onePassEncNum = IOnum;
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<int> dist{0, onePassEncNum-1};
  int randx = dist(rng);
  // aes_init();
  EncBlock x;
  x.x = randx;
  int size = sizeof(EncBlock)/2;
  for (int i = 0; i < onePassEncNum; ++i) {
    // gcm_encrypt(&x, size);
    // cbc_encrypt(&x, size); 
    gcm_decrypt(&x, size);
    // cbc_decrypt(&x, size);
    x.x = dist(rng);
  }
}

void testPageFault() {
  int pagesize = 4096;
  uint8_t *page;
  uint8_t *a = (uint8_t*)malloc(pagesize);
  for (int i = 0; i < 100000; ++i) {
    uint8_t *page = (uint8_t*)malloc(pagesize);
    memset(page, i, 4096);
    memcpy(a, page, 4096);
  }
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
    testIO(128, structureId);
  } else if (sortId == 6) {
    testEncDec(256);
  } else if (sortId == 7) {
    testPageFault();
  }
}