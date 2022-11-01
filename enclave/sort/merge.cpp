#include "merge.h"
#include "bucket.h"

int merge_sort(int inStructureId, int outStructureId, int size) {
  printf("insize: %d, M: %d\n", size, M);
  int bucketNum = ceil(1.0 * size / M);
  printf("Bucket Number: %d\n", bucketNum);
  int setSize = floor(1.0 * M / (bucketNum + 1));
  initMerge(size);
  printf("HEAP_NODE_SIZE: %d\n", setSize);
  int avg = size / bucketNum;
  int remainder = size % bucketNum;
  int *bucketAddr = (int*)malloc(bucketNum * sizeof(int));
  int *elements = (int*)malloc(bucketNum * sizeof(int));
  int each, total = 0;
  for (int i = 0; i < bucketNum; ++i) {
    each = avg + ((i < remainder) ? 1 : 0);
    elements[i] = each;
    bucketAddr[i] = total;
    total += each;
  }
  Bucket_x *arr = (Bucket_x*)malloc(M * sizeof(Bucket_x));
  printf("Before internal sort\n");
  for (int i = 0; i < bucketNum; ++i) {
    printf("Processing bucket%d", i);
    bucketSort(inStructureId, elements[i], bucketAddr[i]);
  }
  printf("Before kwaymergesort\n");
  kWayMergeSort(inStructureId, outStructureId, elements, bucketAddr, bucketNum);
  return outStructureId;
}