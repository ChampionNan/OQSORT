#include "shared.h"
#include "common.h"

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
  encDataSize = encOneBlockSize - 16; 
  char *pers = "aes generate keygcm generate key";
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

// Assume encSize = 16 * k
void EnclaveServer::ofb_encrypt(EncOneBlock* buffer, int encSize) {
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, key, 256);
  mbedtls_ctr_drbg_random(&ctr_drbg, (uint8_t*)(&(buffer->iv)), 16);
  unsigned char iv[16];
  iv_offset = 0;
  memcpy(iv, (uint8_t*)(&(buffer->iv)), 16);
  mbedtls_aes_crypt_ofb(&aes, encSize, &iv_offset, (uint8_t*)(&(buffer->iv)), (uint8_t*)buffer, (uint8_t*)buffer);
  memcpy((uint8_t*)(&(buffer->iv)), iv, 16);
  mbedtls_aes_free(&aes);
  return;
}

// Assume encSize = 16 * k
void EnclaveServer::ofb_decrypt(EncOneBlock* buffer, int encSize) {
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, key, 256);
  iv_offset1 = 0;
  mbedtls_aes_crypt_ofb(&aes, encSize, &iv_offset1, (uint8_t*)(&(buffer->iv)), (uint8_t*)buffer, (uint8_t*)buffer);
  mbedtls_aes_free(&aes);
  return;
}

void EnclaveServer::gcm_encrypt(EncOneBlock* buffer, int encSize) {
  mbedtls_gcm_init(&gcm);
  mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, (const unsigned char*)key, 256);
  mbedtls_ctr_drbg_random(&ctr_drbg, (uint8_t*)(&(buffer->iv)), 16);
  unsigned char iv[16];
  memcpy(iv, (uint8_t*)(&(buffer->iv)), 16);
  // Initialise the GCM cipher...
  mbedtls_gcm_starts(&gcm, MBEDTLS_GCM_ENCRYPT, (const unsigned char*)iv, 16);
  // Send the intialised cipher some data and store it...
  size_t realOutSize;
  mbedtls_gcm_update(&gcm, (uint8_t*)buffer, encSize, (uint8_t*)buffer, encSize, &realOutSize);
  memcpy((uint8_t*)(&(buffer->iv)), iv, 16);
  // Free up the context.
  mbedtls_gcm_free(&gcm);
  return;
}

// Assume encSize = 16 * k
void EnclaveServer::gcm_decrypt(EncOneBlock* buffer, int encSize) {
  mbedtls_gcm_init(&gcm);
  mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, (const unsigned char*) key, 256);
  mbedtls_gcm_starts(&gcm, MBEDTLS_GCM_DECRYPT, (uint8_t*)(&(buffer->iv)), 16);
  size_t realOutSize;
  mbedtls_gcm_update(&gcm, (uint8_t*)buffer, encSize, (uint8_t*)buffer, encSize, &realOutSize);
  mbedtls_gcm_free(&gcm);
  return;
}

// startIdx: index of blocks, 
// pageSize: number of real data
void EnclaveServer::OcallReadPage(int64_t startIdx, EncOneBlock* buffer, int pageSize, int structureId) {
  if (pageSize == 0) {
    return;
  }
  if (nonEnc) {
    OcallRB(startIdx, (int*)buffer, encOneBlockSize * pageSize, structureId);
  } else {
    OcallRB(startIdx, (int*)buffer, encOneBlockSize * pageSize, structureId);
    if (encmode == OFB) {
      for (int i = 0; i < pageSize; ++i) {
        ofb_decrypt(buffer + i, encDataSize);
      }
    } else if (encmode ==GCM) {
      for (int i = 0; i < pageSize; ++i) {
        gcm_decrypt(buffer + i, encDataSize);
      }
    }
  }
}
// startIdx: index of blocks, 
// pageSize: number of real data
void EnclaveServer::OcallWritePage(int64_t startIdx, EncOneBlock* buffer, int pageSize, int structureId) {
  // std::cout<< "In OcallWriteBlock\n";
  if (pageSize == 0) {
    return;
  }
  if (nonEnc) {
    OcallWB(startIdx, (int*)buffer, encOneBlockSize * pageSize, structureId);
  } else {
    if (encmode == OFB) {
      for (int i = 0; i < pageSize; ++i) {
        ofb_encrypt(buffer + i, encDataSize);
      }
    } else if (encmode == GCM) {
      for (int i = 0; i < pageSize; ++i) {
        gcm_encrypt(buffer + i, encDataSize);
      }
    }
    OcallWB(startIdx, (int*)buffer, encOneBlockSize * pageSize, structureId);
  }
}
// index: start index counted by elements (count from 0), elementNum: #elements
void EnclaveServer::opOneLinearScanBlock(int64_t index, EncOneBlock* block, int64_t elementNum, int structureId, int write, int64_t dummyNum) {
  if (elementNum + dummyNum == 0) {
    return ;
  }
  if (dummyNum < 0) {
    printf("Dummy padding error!");
    return ;
  }
  int64_t remain = elementNum % B;
  int64_t boundary = ceil(1.0 * elementNum / B);
  int Msize;
  if (!write) { // read
    for (int64_t i = 0; i < boundary; ++i) {
      Msize = (i == boundary - 1) ? remain : B;
      OcallReadPage(index + i * B, &block[i * B], Msize, structureId);
    }
  } else { // write
    for (int64_t i = 0; i < boundary; ++i) {
      Msize = (i == boundary - 1) ? remain : B;
      OcallWritePage(index + i * B, &block[i * B], Msize, structureId);
    }
    if (dummyNum > 0) {
      EncOneBlock *junk = new EncOneBlock[dummyNum];
      for (int64_t j = 0; j < dummyNum; ++j) {
        junk[j].sortKey = DUMMY<int>();
      }
      int64_t dummyBoundary = ceil(1.0 * dummyNum / B);
      remain = dummyNum % B;
      for (int64_t j = 0; j < dummyBoundary; ++j) {
        Msize = (j == dummyBoundary - 1) ? remain : B;
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
      } else {
        if (a->randomKey > b->randomKey) {
          return true;
        } else if (a->randomKey < b->randomKey) {
          return false;
        }
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

void EnclaveServer::swapRow(EncOneBlock *a, int64_t i, int64_t j) {
  EncOneBlock *temp = new EncOneBlock;
  memmove(temp, a + i, encOneBlockSize);
  memmove(a + i, a + j, encOneBlockSize);
  memmove(a + j, temp, encOneBlockSize);
  delete temp;
}