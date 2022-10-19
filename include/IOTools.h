#ifndef IOTOOLS_H
#define IOTOOLS_H

int smallestPowerOfKLargerThan(int n, int k);
int greatestPowerOfTwoLessThan(double n);
void swapRow(int *a, int *b);
void init(int **arrayAddr, int structurId, int size);
void print(int **arrayAddr, int structureId, int size);
void test(int **arrayAddr, int structureId, int size);
void testWithDummy(int **arrayAddr, int structureId, int size);

#endif // !IOTOOLS_H
