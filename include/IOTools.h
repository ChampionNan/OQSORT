#ifndef IOTOOLS_H
#define IOTOOLS_H

#include <cstdint>

int greatestPowerOfTwoLessThan(double n);
int smallestPowerOfKLargerThan(int n, int k);
void swapRow(int *a, int *b);
void init(int **arrayAddr, int structureId, int size);
void print(int **arrayAddr, int structureId, int size);
void printEnc(int **arrayAddr, int structureId, int size);
void test(int **arrayAddr, int structureId, int size);
void testEnc(int **arrayAddr, int structureId, int size);
void testWithDummy(int **arrayAddr, int structureId, int size);
void initEnc(int **arrayAddr, int structureId, int size);

#endif // !IOTOOLS_H
