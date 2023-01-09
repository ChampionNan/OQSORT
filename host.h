#ifndef HOST_H
#define HOST_H

// used for OCall funcitons

void freeAllocate(int structureIdM, int structureIdF, size_t size);
int64_t OcallSample(int inStructureId, int sampleId, int sortedSampleId, int64_t N, int64_t M, int64_t n_prime, int is_tight);
void ocall_print_string(const char *str);
void OcallRB(int64_t index, int* buffer, size_t blockSize, int structureId);
void OcallWB(int64_t index, int* buffer, size_t blockSize, int structureId);
void fyShuffle(int structureId, size_t size, int B);

#endif