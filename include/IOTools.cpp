#include "IOTools.h"
#include "../include/common.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <fstream>
#include <algorithm>

int64_t greatestPowerOfTwoLessThan(double n) {
    int64_t k = 1;
    while (k > 0 && k < n) {
        k = k << 1;
    }
    return k >> 1;
}

int64_t smallestPowerOfKLargerThan(int64_t n, int64_t k) {
  int64_t num = 1;
  while (num > 0 && num < n) {
    num = num * k;
  }
  return num;
}

void swapRow(int64_t *a, int64_t *b) {
  int64_t *temp = (int64_t*)malloc(sizeof(int64_t));
  memmove(temp, a, sizeof(int64_t));
  memmove(a, b, sizeof(int64_t));
  memmove(b, temp, sizeof(int64_t));
  free(temp);
}

void init(int64_t **arrayAddr, int structureId, int64_t size) {
  int64_t i;
  if (structureSize[structureId] == 8) {
    int64_t *addr = (int64_t*)arrayAddr[structureId];
    for (i = 0; i < size; i++) {
      addr[i] = (size - i);
    }
  } else if (structureSize[structureId] == 16) {
    Bucket_x *addr = (Bucket_x*)arrayAddr[structureId];
    for (i = 0; i < size; ++i) {
      // TODO: size overflow, become negative
      addr[i].x = size - i;
    }
  }
}

void print(int64_t **arrayAddr, int structureId, int64_t size) {
  int64_t i;
  std::ofstream fout("/home/chenbingnan/mysamples/OQSORT/out.txt");
  if(structureSize[structureId] == 8) {
    int64_t *addr = (int64_t*)arrayAddr[structureId];
    for (i = 0; i < size; i++) {
      // printf("%d ", addr[i]);
      fout << addr[i] << " ";
      if ((i != 0) && (i % 8 == 0)) {
        // printf("\n");
        fout << std::endl;
      }
    }
  } else if (structureSize[structureId] == 16) {
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
void test(int64_t **arrayAddr, int structureId, int64_t size) {
  int pass = 1;
  int64_t i;
  // print(structureId);
  if(structureSize[structureId] == 4) {
    for (i = 1; i < size; i++) {
      pass &= ((arrayAddr[structureId])[i-1] <= (arrayAddr[structureId])[i]);
      if (!pass) {
        printf("%ld, %ld\n", (arrayAddr[structureId])[i-1], (arrayAddr[structureId])[i]);
        break;
      }
      if ((arrayAddr[structureId])[i] == 0) {
        pass = 0;
        break;
      }
    }
  } else if (structureSize[structureId] == 8) {
    for (i = 1; i < size; i++) {
      pass &= (((Bucket_x*)arrayAddr[structureId])[i-1].x <= ((Bucket_x*)arrayAddr[structureId])[i].x);
      if (!pass) {

      }
      if (((Bucket_x*)arrayAddr[structureId])[i].x == 0) {
        pass = 0;
        break;
      }
    }
  }
  printf(" TEST %s\n",(pass) ? "PASSed" : "FAILed");
}

void testWithDummy(int64_t **arrayAddr, int structureId, int64_t size) {
  int64_t i = 0;
  int64_t j = 0;
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
