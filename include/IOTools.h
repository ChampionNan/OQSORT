#ifndef IOTOOLS_H
#define IOTOOLS_H

int smallestPowerOfTwoLargerThan(int n);
void init(int **arrayAddr, int structurId, int size);
void print(int **arrayAddr, int structureId, int size);
void test(int **arrayAddr, int structureId, int size);
void ocall_print_string(const char *str);

#endif // !IOTOOLS_H
