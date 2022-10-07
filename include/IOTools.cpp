#include "IOTools.h"
#include "../include/common.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <fstream>
#include <algorithm>

int smallestPowerOfKLargerThan(int n, int k) {
  int num = 1;
  while (num > 0 && num < n) {
    num = num * k;
  }
  return num;
}

void swapRow(int *a, int *b) {
  int *temp = (int*)malloc(sizeof(int));
  memmove(temp, a, sizeof(int));
  memmove(a, b, sizeof(int));
  memmove(b, temp, sizeof(int));
  free(temp);
}

void init(int **arrayAddr, int structurId, int size) {
  int i;
  int *addr = (int*)arrayAddr[structurId];
  for (i = 0; i < size; i++) {
    addr[i] = (size - i);
  }
  /*
  for(i = size - 1; i >= 1; --i) {
    swapRow(addr + i, addr + (rand() % i));
  }*/
}

void print(int **arrayAddr, int structureId, int size) {
  int i;
  std::ofstream fout("/home/chenbingnan/mysamples/OQSORT/out.txt");
  if(structureSize[structureId] == 4) {
    int *addr = (int*)arrayAddr[structureId];
    for (i = 0; i < size; i++) {
      // printf("%d ", addr[i]);
      fout << addr[i] << " ";
      if ((i != 0) && (i % 5 == 0)) {
        // printf("\n");
        fout << std::endl;
      }
    }
  } else if (structureSize[structureId] == 8) {
    Bucket_x *addr = (Bucket_x*)arrayAddr[structureId];
    for (i = 0; i < size; i++) {
      // printf("(%d, %d) ", addr[i].x, addr[i].key);
      fout << "(" << addr[i].x << ", " << addr[i].key << ") ";
      if ((i != 0) && (i % 5 == 0)) {
        // printf("\n");
        fout << std::endl;
      }
    }
  }
  // printf("\n");
  fout << std::endl;
  fout.close();
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

void testWithDummy(int **arrayAddr, int structureId, int size) {
  int i = 0;
  int j = 0;
  // print(structureId);
  if(structureSize[structureId] == 4) {
    for (i = 0; i < size; ++i) {
      if ((arrayAddr[structureId])[i] != DUMMY) {
        break;
      }
    }
    if (i == size - 1) {
      printf(" TEST PASSed\n");
      return;
    }
    while (i < size && j < size) {
      for (j = i + 1; j < size; ++j) {
        if ((arrayAddr[structureId])[j] != DUMMY) {
          break;
        }
      }
      if (j == size) { // Only 1 element not dummy
        printf(" TEST PASSed\n");
        return;
      }
      if ((arrayAddr[structureId])[i] <= (arrayAddr[structureId])[j]) {
        i = j;
      } else {
        printf(" TEST FAILed\n");
        return;
      }
    }
    printf(" TEST PASSed\n");
    return;
  } else if (structureSize[structureId] == 8) {
    for (i = 0; i < size; ++i) {
      if (((Bucket_x*)arrayAddr[structureId])[i].x != DUMMY) {
        break;
      }
    }
    if (i == size) { // All elements are dummy
      printf(" TEST PASSed\n");
      return;
    }
    while (i < size) {
      for (j = i + 1; j < size; ++i) {
        if (((Bucket_x*)arrayAddr[structureId])[j].x != DUMMY) {
          break;
        }
      }
      if (j == size) { // Only 1 element not dummy
        printf(" TEST PASSed\n");
        return;
      }
      if (((Bucket_x*)arrayAddr[structureId])[i].x <= ((Bucket_x*)arrayAddr[structureId])[j].x) {
        i = j;
      } else {
        printf(" TEST FAILed\n");
        return;
      }
    }
  }
}
