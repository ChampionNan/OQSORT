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
  for (i = 0; i < size; i++) {
    if(structureSize[structureId] == 4) {
      // int
      int *addr = (int*)arrayAddr[structureId];
      printf("%d ", addr[i]);
    } else if (structureSize[structureId] == 8) {
      //Bucket
      Bucket_x *addr = (Bucket_x*)arrayAddr[structureId];
      printf("(%d, %d), ", addr[i].x, addr[i].key);
    }
  }
  printf("\n");
}

void test(int **arrayAddr, int structureId, int size) {
  int pass = 1;
  int i;
  // print(structureId);
  for (i = 1; i < size; i++) {
    pass &= (((Bucket_x*)arrayAddr[structureId])[i-1].x <= ((Bucket_x*)arrayAddr[structureId])[i].x);
    // std::cout<<"i, pass: "<<i<<" "<<pass<<std::endl;
  }
  printf(" TEST %s\n",(pass) ? "PASSed" : "FAILed");
}