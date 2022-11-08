#include "merge.h"
#include "bucket.h"

int merge_sort(int inStructureId, int outStructureId) {
  int bucketNum = ceil(1.0 * N / M);
  printf("Bucket Number: %d\n", bucketNum);
  int setSize = floor(1.0 * M / (bucketNum + 1));
  initMerge(setSize);
  printf("HEAP_NODE_SIZE: %d\n", setSize);
  int avg = 1.0*N/bucketNum;
  int remainder = 1.0*N - avg*bucketNum;
  int *bucketAddr = (int*)malloc(bucketNum * sizeof(int));
  int *elements = (int*)malloc(bucketNum * sizeof(int));
  int each, total = 0;
  printf("sizeof bucket_x: %lu\n", sizeof(Bucket_x));
  for (int i = 0; i < bucketNum; ++i) {
    each = avg + ((i < remainder) ? 1 : 0);
    elements[i] = each;
    bucketAddr[i] = total;
    printf("total: %d\n", bucketAddr[i]);
    total += each;
  }
  // Bucket_x *arr = (Bucket_x*)malloc(M * sizeof(Bucket_x));
  for (int i = 0; i < bucketNum; ++i) {
    printf("bucketSort %d, elements: %d, bucketAddr: %d\n", i, elements[i], bucketAddr[i]);
    bucketSort(inStructureId, elements[i], bucketAddr[i]);
  }
  printf("Begin merge sort\n");
  kWayMergeSort(inStructureId, outStructureId, elements, bucketAddr, bucketNum);
  free(bucketAddr);
  free(elements);
  return outStructureId;
}