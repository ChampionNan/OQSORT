#ifndef COMMON_STRUCTURE_H
#define COMMON_STRUCTURE_H

// #include <cstdio>
#include "definitions.h"
#include <cstdint>

typedef struct {
  int x;
  int key;
} Bucket_x;

typedef struct {
  uint32_t x[ENCB_SIZE];
  __uint128_t iv;
} EncBlock;

// BOS: 0, 1, 2; ODS: 3, 4
const int structureSize[NUM_STRUCTURES] = {sizeof(int), sizeof(Bucket_x), sizeof(Bucket_x), sizeof(int), sizeof(int)};

// Print Message
#define DBGprint(...) { \
  fprintf(stderr, "%s: Line %d:\t", __FILE__, __LINE__); \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n"); \
}

#endif // !COMMON_STRUCTURE_H