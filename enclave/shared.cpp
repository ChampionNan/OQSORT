#include "shared.h"

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

EnclaveServer::EnclaveServer(int64_t N, int64_t M, int B, int sigma, EncMode encmode, int SSD) : N{N}, M{M}, B{B}, sigma{sigma}, encmode{encmode}, SSD{SSD} {
  encOneBlockSize = sizeof(EncOneBlock);
  IOcost = 0;
  IOtime = 0;
  countSwap = 0;
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

double EnclaveServer::getIOcost() { return IOcost; }

double EnclaveServer::getIOtime() { return IOtime; }

double EnclaveServer::getSwapNum() { return countSwap; }

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

__uint128_t EnclaveServer::prf(__uint128_t a) {
  unsigned char input[16] = {0};
  unsigned char encrypt_output[16] = {0};
  for (int i = 0; i < 16; ++i) {
    input[i] = (a >> (120 - i * 8)) & 0xFF;
  }
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, key, 256);
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input, encrypt_output);
  mbedtls_aes_free(&aes);
  __uint128_t res = 0;
  for (int i = 0; i < 16; ++i) {
    res |= encrypt_output[i] << (120 - i * 8);
  }
  return res;
}

int64_t EnclaveServer::encrypt(int64_t index) {
  int64_t l = index / (1 << base);
  int64_t r = index % (1 << base);
  __uint128_t e;
  int64_t temp, i = 1;
  while (i <= ROUND) {
    e = prf((r << (16 * 8 - base)) + i);
    temp = r;
    r = l ^ (e >> (16 * 8 - base));
    l = temp;
    i += 1;
  }
  return (l << base) + r;
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
    OcallRB(startIdx, (int*)buffer, encOneBlockSize * pageSize, structureId, SSD);
    // IOcost += 1;
  } else {
    // printf("Before Read Enc: \n");
    OcallRB(startIdx, (int*)buffer, encOneBlockSize * pageSize, structureId, SSD);
    // IOcost += 1;
    if (encmode == OFB) {
      for (int i = 0; i < pageSize; ++i) {
        ofb_decrypt(buffer + i, encOneBlockSize);
      }
    } else if (encmode == GCM) {
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
    OcallWB(startIdx, (int*)buffer, encOneBlockSize * pageSize, structureId, SSD);
    // IOcost += 1;
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
    OcallWB(startIdx, (int*)buffer, encOneBlockSize * pageSize, structureId, SSD);
    // IOcost += 1;
  }
}
// index: start index counted by elements (count from 0), elementNum: #elements
void EnclaveServer::opOneLinearScanBlock(int64_t index, EncOneBlock* block, int64_t elementNum, int structureId, int write, int64_t dummyNum) {
  // printf("In oponelinear scan block\n");
  IOstart = time(NULL);
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
    IOcost += boundary;
    for (int64_t i = 0; i < boundary; ++i) {
      Msize = std::min((int64_t)B, elementNum - i * B);
      // printf("in Op Read, B: %d\n", Msize);
      OcallReadPage(index + i * B, &block[i * B], Msize, structureId);
    }
  } else { // write
    IOcost += boundary;
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
      IOcost += dummyBoundary;
      for (int64_t j = 0; j < dummyBoundary; ++j) {
        Msize = std::min((int64_t)B, dummyNum - j * B);
        OcallWritePage(index + elementNum + j * B, &junk[j * B], Msize, structureId);
      }
      delete [] junk;
    }
  }
  IOend = time(NULL);
  IOtime += IOend - IOstart;
}

bool EnclaveServer::cmpHelper(EncOneBlock *a, EncOneBlock *b) {
  if (a->sortKey > b->sortKey) {
    return true;
  } else if (a->sortKey < b->sortKey) {
    return false;
  } else if (a->primaryKey >= b->primaryKey) {
    return true;
  } else if (a->primaryKey < b->primaryKey) {
    return false;
  }
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

// TODO: random number influence running time
void EnclaveServer::setDummy(EncOneBlock *a, int64_t size) {
  if (size <= 0) {
    return ;
  }
  std::random_device rd;
  std::mt19937 rng{rd()};
  std::uniform_int_distribution<int> dist{std::numeric_limits<int>::min(), std::numeric_limits<int>::max()};
  for (int64_t i = 0; i < size; ++i) {
    a[i].sortKey = DUMMY<int>();
    a[i].primaryKey = dist(rng); // dist(rng); // tie_breaker++;
  }
}

void EnclaveServer::swapRow(EncOneBlock *a, int64_t i, int64_t j) {
  EncOneBlock *temp = new EncOneBlock;
  memmove(temp, a + i, encOneBlockSize);
  memmove(a + i, a + j, encOneBlockSize);
  memmove(a + j, temp, encOneBlockSize);
  delete temp;
}

void EnclaveServer::swap(std::vector<EncOneBlock> &arr, int64_t i, int64_t j) {
  EncOneBlock *temp = new EncOneBlock;
  memcpy(temp, &arr[i], encOneBlockSize);
  memcpy(&arr[i], &arr[j], encOneBlockSize);
  memcpy(&arr[j], temp, encOneBlockSize);
  delete temp;
}

void EnclaveServer::oswap(EncOneBlock *a, EncOneBlock *b, bool cond) {
  int mask = ~((int)cond - 1);
  *a = *a ^ *b;
  *b = *b ^ (*a & mask);
  *a = *a ^ *b;
}

void EnclaveServer::oswap128(uint128_t *a, uint128_t *b, bool cond) {
  // countSwap += 1;
  uint128_t mask = ~((uint128_t)cond - 1);
  *a = *a ^ *b;
  *b = *b ^ (*a & mask);
  *a = *a ^ *b;
  *(a+1) = *(a+1) ^ *(b+1);
  *(b+1) = *(b+1) ^ (*(a+1) & mask);
  *(a+1) = *(a+1) ^ *(b+1);
  *(a+2) = *(a+2) ^ *(b+2);
  *(b+2) = *(b+2) ^ (*(a+2) & mask);
  *(a+2) = *(a+2) ^ *(b+2);
  *(a+3) = *(a+3) ^ *(b+3);
  *(b+3) = *(b+3) ^ (*(a+3) & mask);
  *(a+3) = *(a+3) ^ *(b+3);
  *(a+4) = *(a+4) ^ *(b+4);
  *(b+4) = *(b+4) ^ (*(a+4) & mask);
  *(a+4) = *(a+4) ^ *(b+4);
  *(a+5) = *(a+5) ^ *(b+5);
  *(b+5) = *(b+5) ^ (*(a+5) & mask);
  *(a+5) = *(a+5) ^ *(b+5);
  *(a+6) = *(a+6) ^ *(b+6);
  *(b+6) = *(b+6) ^ (*(a+6) & mask);
  *(a+6) = *(a+6) ^ *(b+6);
  *(a+7) = *(a+7) ^ *(b+7);
  *(b+7) = *(b+7) ^ (*(a+7) & mask);
  *(a+7) = *(a+7) ^ *(b+7);
}

void EnclaveServer::regswap(EncOneBlock *a, EncOneBlock *b) {
  int numB = encOneBlockSize / 4;
  int Bsize;
  int tmp;
  for (int i = 0; i < numB; ++i) {
    tmp = ((int*)a)[i];
    ((int*)a)[i] = ((int*)b)[i];
    ((int*)b)[i] = tmp;
  }
}

int64_t EnclaveServer::greatestPowerOfTwoLessThan(double n) {
  int64_t k = 1;
  while (k > 0 && k < n) {
    k = k << 1;
  }
  return k >> 1;
}

int64_t EnclaveServer::greatestPowerOfTwoLessThan(int64_t n) {
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

int64_t EnclaveServer::Sample(int inStructureId, int sampleId, int64_t N, int64_t M, int64_t n_prime) {
  int64_t sampleSize;
  OcallSample(inStructureId, sampleId, N, M, n_prime, SSD, &sampleSize);
  return sampleSize;
}
