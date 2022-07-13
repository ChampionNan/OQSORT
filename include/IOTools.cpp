#include "IOTools.h"
#include "../include/common.h"

#include <cstdio>

int smallestPowerOfTwoLargerThan(int n) {
  int k = 1;
  while (k > 0 && k < n) {
    k = k << 1;
  }
  return k;
}

void init(int **arrayAddr, int structurId, int size) {
  int i;
  int *addr = (int*)arrayAddr[structurId];
  for (i = 0; i < size; i++) {
    addr[i] = (size - i);
  }
}


void print(int **arrayAddr, int structureId, int size) {
  int i;
  if(structureSize[structureId] == 4) {
    int *addr = (int*)arrayAddr[structureId];
    for (i = 0; i < size; i++) {
      printf("%d ", addr[i]);
    }
  } else if (structureSize[structureId] == 8) {
    Bucket_x *addr = (Bucket_x*)arrayAddr[structureId];
    for (i = 0; i < size; i++) {
      printf("(%d, %d) ", addr[i].x, addr[i].key);
    } 
  }
  printf("\n");
}

// TODO: change nt types
void test(int **arrayAddr, int structureId, int size) {
  int pass = 1;
  int i;
  // print(structureId);
  if(structureSize[structureId] == 4) {
    for (i = 1; i < size; i++) {
      pass &= ((arrayAddr[structureId])[i-1] <= (arrayAddr[structureId])[i]);
      if ((arrayAddr[structureId])[i] == 0) {
        pass = 0;
        break;
      }
    }
  } else if (structureSize[structureId] == 8) {
    for (i = 1; i < size; i++) {
      pass &= (((Bucket_x*)arrayAddr[structureId])[i-1].x <= ((Bucket_x*)arrayAddr[structureId])[i].x);
      if (((Bucket_x*)arrayAddr[structureId])[i].x == 0) {
        pass = 0;
        break;
      }
    }
  }
  printf(" TEST %s\n",(pass) ? "PASSed" : "FAILed");
}
