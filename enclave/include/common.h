#ifndef COMMON_STRUCTURE_H
#define COMMON_STRUCTURE_H

#include <cstdio>
#include "definitions.h"

typedef struct {
  int x;
  int key;
} Bucket_x;

// size_t structureSize[NUM_STRUCTURES] = {(size_t)sizeof(int), (size_t)sizeof(Bucket_x), (size_t)sizeof(Bucket_x)};
int structureSize[NUM_STRUCTURES] = {sizeof(int), sizeof(Bucket_x), sizeof(Bucket_x)};

// Print Message
#define DBGprint(...) { \
  fprintf(stderr, "%s: Line %d:\t", __FILE__, __LINE__); \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n"); \
}

#endif // !COMMON_STRUCTURE_H