#ifndef COMMON_STRUCTURE_H
#define COMMON_STRUCTURE_H

// #include <cstdio>
#include "definitions.h"
#include <cstdint>

typedef struct {
  int64_t x;
  int64_t key;
} Bucket_x;

// TODO: set up structure size
const int structureSize[NUM_STRUCTURES] = {sizeof(int64_t),
  2 * sizeof(int64_t), 2 * sizeof(int64_t),
  sizeof(int64_t), sizeof(int64_t), sizeof(int64_t), sizeof(int64_t)};

// Print Message
#define DBGprint(...) { \
  fprintf(stderr, "%s: Line %d:\t", __FILE__, __LINE__); \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n"); \
}

#endif // !COMMON_STRUCTURE_H