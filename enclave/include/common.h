#ifndef COMMON_STRUCTURE_H
#define COMMON_STRUCTURE_H
#include <iostream> 

typedef struct {
  int x;
  int key;
} Bucket_x;

int structureSize[NUM_STRUCTURES] = {sizeof(int), sizeof(Bucket_x), sizeof(Bucket_x)};

#endif // !COMMON_STRUCTURE_H