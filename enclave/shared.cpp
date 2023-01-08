#include "shared.h"
#include "../include/common.h"

Heap::Heap(EnclaveServer &eServer, HeapNode *a, int64_t size, int64_t bsize) : eServer(eServer) {
  heapSize = size;
  harr = a;
  int64_t i = (heapSize - 1) / 2;
  batchSize = bsize;
  while (i >= 0) {
    Heapify(i);
    i --;
  }
}

void Heap::Heapify(int64_t i) {
  int64_t l = left(i);
  int64_t r = right(i);
  int64_t target = i;

  if (l < heapSize && eServer.cmpHelper(harr[i].data + harr[i].elemIdx % batchSize, harr[l].data + harr[l].elemIdx % batchSize)) {
    target = l;
  }
  if (r < heapSize && eServer.cmpHelper(harr[target].data + harr[target].elemIdx % batchSize, harr[r].data + harr[r].elemIdx % batchSize)) {
    target = r;
  }
  if (target != i) {
    swapHeapNode(&harr[i], &harr[target]);
    Heapify(target);
  }
}

int64_t Heap::left(int64_t i) {
  return (2 * i + 1);
}

int64_t Heap::right(int64_t i) {
  return (2 * i + 2);
}

void Heap::swapHeapNode(HeapNode *a, HeapNode *b) {
  HeapNode temp = *a;
  *a = *b;
  *b = temp;
}

HeapNode* Heap::getRoot() {
  return &harr[0];
}

int64_t Heap::getHeapSize() {
  return heapSize;
}

bool Heap::reduceSizeByOne() {
  free(harr[0].data);
  heapSize --;
  if (heapSize > 0) {
    harr[0] = harr[heapSize];
    Heapify(0);
    return true;
  } else {
    return false;
  }
}

void Heap::replaceRoot(HeapNode x) {
  harr[0] = x;
  Heapify(0);
}

EnclaveServer::EnclaveServer(int64_t N, int64_t M, int B, EncMode encmode) : N{N}, M{M}, B{B}, encmode{encmode} {
  encOneBlockSize = sizeof(EncOneBlock);
  IOcost = 0;
  const char *pers = "aes generate keygcm generate key";
  int ret;
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (unsigned char *)pers, strlen(pers))) != 0) {
    printf(" failed\n ! mbedtls_ctr_drbg_init returned -0x%04x\n", -ret);
    return;
  }
  if((ret = mbedtls_ctr_drbg_random(&ctr_drbg, key, 32)) != 0) {
    printf(" failed\n ! mbedtls_ctr_drbg_random returned -0x%04x\n", -ret);
    return ;
  }
}

// Invokes OCALL to display the enclave buffer to the terminal.
int EnclaveServer::printf(const char *fmt, ...) {
  char buf[BUFSIZ] = {'\0'};
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(buf, BUFSIZ, fmt, ap);
  va_end(ap);
  ocall_print_string(buf);
  return ret;
}

// Assume encSize = 16 * k
void EnclaveServer::ofb_encrypt(EncOneBlock* buffer, int encSize) {
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, key, 256);
  mbedtls_ctr_drbg_random(&ctr_drbg, (uint8_t*)(&(buffer->randomKey)), 4);
  iv_offset = 0;
  memset(iv, 0, sizeof(iv));
  mbedtls_aes_crypt_ofb(&aes, encSize, &iv_offset, iv, (uint8_t*)buffer, (uint8_t*)buffer);
  mbedtls_aes_free(&aes);
  return;
}

// Assume encSize = 16 * k
void EnclaveServer::ofb_decrypt(EncOneBlock* buffer, int encSize) {
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, key, 256);
  iv_offset1 = 0;
  memset(iv, 0, sizeof(iv));
  mbedtls_aes_crypt_ofb(&aes, encSize, &iv_offset1, iv, (uint8_t*)buffer, (uint8_t*)buffer);
  mbedtls_aes_free(&aes);
  return;
}

void EnclaveServer::gcm_encrypt(EncOneBlock* buffer, int encSize) {
  mbedtls_gcm_init(&gcm);
  mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, (const unsigned char*)key, 256);
  mbedtls_ctr_drbg_random(&ctr_drbg, (uint8_t*)(&(buffer->randomKey)), 4);
  memset(iv, 0, sizeof(iv));
  // Initialise the GCM cipher...
  mbedtls_gcm_starts(&gcm, MBEDTLS_GCM_ENCRYPT, iv, 16, NULL, 0);
  // Send the intialised cipher some data and store it...
  mbedtls_gcm_update(&gcm, encSize, (uint8_t*)buffer, (uint8_t*)buffer);
  // Free up the context.
  mbedtls_gcm_free(&gcm);
  return;
}

// Assume encSize = 16 * k
void EnclaveServer::gcm_decrypt(EncOneBlock* buffer, int encSize) {
  mbedtls_gcm_init(&gcm);
  mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, (const unsigned char*) key, 256);
  memset(iv, 0, sizeof(iv));
  mbedtls_gcm_starts(&gcm, MBEDTLS_GCM_DECRYPT, iv, 16, NULL, 0);
  mbedtls_gcm_update(&gcm, encSize, (uint8_t*)buffer, (uint8_t*)buffer);
  mbedtls_gcm_free(&gcm);
  return;
}

// startIdx: index of blocks, 
// pageSize: number of real data
void EnclaveServer::OcallReadPage(int64_t startIdx, EncOneBlock* buffer, int pageSize, int structureId) {
  // printf("In OcallReadPage\n");
  if (pageSize == 0) {
    return;
  }
  if (nonEnc) {
    // printf("Before Read nonEnc: \n");
    OcallRB(startIdx, (int*)buffer, encOneBlockSize * pageSize, structureId);
    IOcost += 1;
  } else {
    // printf("Before Read Enc: \n");
    OcallRB(startIdx, (int*)buffer, encOneBlockSize * pageSize, structureId);
    IOcost += 1;
    if (encmode == OFB) {
      for (int i = 0; i < pageSize; ++i) {
        ofb_decrypt(buffer + i, encOneBlockSize);
      }
    } else if (encmode ==GCM) {
      for (int i = 0; i < pageSize; ++i) {
        gcm_decrypt(buffer + i, encOneBlockSize);
      }
    }
  }
}
// startIdx: index of blocks, 
// pageSize: number of real data
void EnclaveServer::OcallWritePage(int64_t startIdx, EncOneBlock* buffer, int pageSize, int structureId) {
  // printf("In OcallWritePage\n");
  if (pageSize == 0) {
    return;
  }
  if (nonEnc) {
    OcallWB(startIdx, (int*)buffer, encOneBlockSize * pageSize, structureId);
    IOcost += 1;
  } else {
    if (encmode == OFB) {
      for (int i = 0; i < pageSize; ++i) {
        ofb_encrypt(buffer + i, encOneBlockSize);
      }
    } else if (encmode == GCM) {
      for (int i = 0; i < pageSize; ++i) {
        gcm_encrypt(buffer + i, encOneBlockSize);
      }
    }
    OcallWB(startIdx, (int*)buffer, encOneBlockSize * pageSize, structureId);
    IOcost += 1;
  }
}
// index: start index counted by elements (count from 0), elementNum: #elements
void EnclaveServer::opOneLinearScanBlock(int64_t index, EncOneBlock* block, int64_t elementNum, int structureId, int write, int64_t dummyNum) {
  // printf("In oponelinear scan block\n");
  if (elementNum + dummyNum == 0) {
    return ;
  }
  if (dummyNum < 0) {
    printf("Dummy padding error!\n");
    return ;
  }
  int64_t boundary = ceil(1.0 * elementNum / B);
  // printf("boundary: %d, remain: %d, B: %d\n", boundary, remain, B);
  int Msize;
  if (!write) { // read
    for (int64_t i = 0; i < boundary; ++i) {
      Msize = std::min((int64_t)B, elementNum - i * B);
      // printf("in Op Read, B: %d\n", Msize);
      OcallReadPage(index + i * B, &block[i * B], Msize, structureId);
    }
  } else { // write
    for (int64_t i = 0; i < boundary; ++i) {
       Msize = std::min((int64_t)B, elementNum - i * B);
      OcallWritePage(index + i * B, &block[i * B], Msize, structureId);
    }
    if (dummyNum > 0) {
      EncOneBlock *junk = new EncOneBlock[dummyNum];
      for (int64_t j = 0; j < dummyNum; ++j) {
        junk[j].sortKey = DUMMY<int>();
      }
      int64_t dummyBoundary = ceil(1.0 * dummyNum / B);
      for (int64_t j = 0; j < dummyBoundary; ++j) {
        Msize = std::min((int64_t)B, dummyNum - j * B);
        OcallWritePage(index + elementNum + j * B, &junk[j * B], Msize, structureId);
      }
      delete [] junk;
    }
  }
}

bool EnclaveServer::cmpHelper(EncOneBlock *a, EncOneBlock *b) {
  if (a->sortKey > b->sortKey) {
    return true;
  } else if (a->sortKey < b->sortKey) {
    return false;
  } else {
    if (a->primaryKey > b->primaryKey) {
      return true;
    } else if (a->primaryKey < b->primaryKey) {
      return false;
    } else {
      if (a->payLoad > b->payLoad) {
        return true;
      } else if (a->payLoad < b->payLoad) {
        return false;
      }
    }
  }
  return true; // equal
}

int64_t EnclaveServer::moveDummy(EncOneBlock *a, int64_t size) {
  int64_t i = 0;
  int64_t j = size - 1;
  while (1) {
    while (i < j && a[i].sortKey != DUMMY<int>()) i++;
    while (i < j && a[j].sortKey == DUMMY<int>()) j--;
    if (i >= j) break;
    swapRow(a, i, j);
  }
  return i;
}

void EnclaveServer::setValue(EncOneBlock *a, int64_t size, int value) {
  for (int64_t i = 0; i < size; ++i) {
    a[i].sortKey = value;
    a[i].primaryKey = value;
  }
}

void EnclaveServer::swapRow(EncOneBlock *a, int64_t i, int64_t j) {
  EncOneBlock *temp = new EncOneBlock;
  memmove(temp, a + i, encOneBlockSize);
  memmove(a + i, a + j, encOneBlockSize);
  memmove(a + j, temp, encOneBlockSize);
  delete temp;
}

int64_t EnclaveServer::Sample(int inStructureId, int sampleId, int sortedSampleId, int64_t N, int64_t M, int64_t n_prime, int is_tight) {
  int64_t sampleSize;
  OcallSample(inStructureId, sampleId, sortedSampleId, N, M, n_prime, is_tight, &sampleSize);
  return sampleSize;
}

int64_t EnclaveServer::greatestPowerOfTwoLessThan(double n) {
  int64_t k = 1;
  while (k > 0 && k < n) {
      k = k << 1;
   }
  return k >> 1;
}

int64_t EnclaveServer::smallestPowerOfKLargerThan(int64_t n, int k) {
  int64_t num = 1;
  while (num > 0 && num < n) {
    num = num * k;
  }
  return num;
}