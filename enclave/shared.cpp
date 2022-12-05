#include "shared.h"

// unsigned char key2[16];
unsigned char key2[] = {0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
mbedtls_aes_context aes2;
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_entropy_context entropy;
size_t iv_offset, iv_offset1;

mbedtls_gcm_context aes3;

Heap::Heap(HeapNode *a, int size, int bsize) {
  heapSize = size;
  harr = a;
  int i = (heapSize - 1) / 2;
  batchSize = bsize;
  while (i >= 0) {
    Heapify(i);
    i --;
  }
}

void Heap::Heapify(int i) {
  int l = left(i);
  int r = right(i);
  int target = i;

  if (l < heapSize && cmpHelper(harr[i].data + harr[i].elemIdx % batchSize, harr[l].data + harr[l].elemIdx % batchSize)) {
    target = l;
  }
  if (r < heapSize && cmpHelper(harr[target].data + harr[target].elemIdx % batchSize, harr[r].data + harr[r].elemIdx % batchSize)) {
    target = r;
  }
  if (target != i) {
    swapHeapNode(&harr[i], &harr[target]);
    Heapify(target);
  }
}

int Heap::left(int i) {
  return (2 * i + 1);
}

int Heap::right(int i) {
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

int Heap::getHeapSize() {
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

/*
 * printf:
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
int printf(const char *fmt, ...) {
  char buf[BUFSIZ] = {'\0'};
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(buf, BUFSIZ, fmt, ap);
  va_end(ap);
  ocall_print_string(buf);
  return ret;
}

int greatestPowerOfTwoLessThan(double n) {
  int k = 1;
  while (k > 0 && k < n) {
    k = k << 1;
  }
  return k >> 1;
}

int smallestPowerOfKLargerThan(int n, int k) {
  int num = 1;
  while (num > 0 && num < n) {
    num = num * k;
  }
  return num;
}

void aes_init() {
  mbedtls_aes_init(&aes2); // 1.
  // mbedtls_gcm_init(&aes2); // 2.
  char *pers = "aes2 generate key";
  int ret;
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (unsigned char *) pers, strlen(pers))) != 0) {
    printf(" failed\n ! mbedtls_ctr_drbg_init returned -0x%04x\n", -ret);
    return;
  }
  if((ret = mbedtls_ctr_drbg_random(&ctr_drbg, key2, 16)) != 0) {
    printf(" failed\n ! mbedtls_ctr_drbg_random returned -0x%04x\n", -ret);
    return ;
  }
  mbedtls_aes_setkey_enc(&aes2, key2, 128); // 1.
  // mbedtls_gcm_setkey(&aes2, MBEDTLS_CIPHER_ID_AES, key2, 128); // 2.
}

// Assume blockSize = 16 * k
void cbc_encrypt(EncBlock* buffer, int blockSize) {
  // std::cout<< "In cbc_encrypt\n";
  mbedtls_aes_init(&aes2);
  mbedtls_aes_setkey_enc(&aes2, key2, 256);
  mbedtls_ctr_drbg_random(&ctr_drbg, (uint8_t*)(&(buffer->iv)), 16);
  // // std::cout<< "memcpy iv\n";
  unsigned char iv[16];
  iv_offset = 0;
  memcpy(iv, (uint8_t*)(&(buffer->iv)), 16);
  mbedtls_aes_crypt_ofb(&aes2, blockSize, &iv_offset, (uint8_t*)(&(buffer->iv)), (uint8_t*)buffer, (uint8_t*)buffer);
  memcpy((uint8_t*)(&(buffer->iv)), iv, 16);
  mbedtls_aes_free(&aes2);
  return;
}

// Assume blockSize = 16 * k
void cbc_decrypt(EncBlock* buffer, int blockSize) {
  // std::cout<< "In cbc_decrypt\n";
  // mbedtls_aes_crypt_cfb8(&aes2, MBEDTLS_AES_DECRYPT, blockSize, (uint8_t*)(&(buffer->iv)), (uint8_t*)buffer, (uint8_t*)buffer);
  mbedtls_aes_init(&aes2);
  mbedtls_aes_setkey_enc(&aes2, key2, 256);
  iv_offset1 = 0;
  mbedtls_aes_crypt_ofb(&aes2, blockSize, &iv_offset1, (uint8_t*)(&(buffer->iv)), (uint8_t*)buffer, (uint8_t*)buffer);
  mbedtls_aes_free(&aes2);
  return;
}

void gcm_encrypt(EncBlock* buffer, int blockSize) {
  mbedtls_gcm_init( &aes3 );
  mbedtls_gcm_setkey( &aes3, MBEDTLS_CIPHER_ID_AES, (const unsigned char*) key2, 128);
  mbedtls_ctr_drbg_random(&ctr_drbg, (uint8_t*)(&(buffer->iv)), 16);
  unsigned char iv[16];
  memcpy(iv, (uint8_t*)(&(buffer->iv)), 16);
  // Initialise the GCM cipher...
  mbedtls_gcm_starts(&aes3, MBEDTLS_GCM_ENCRYPT, (const unsigned char*)iv, sizeof(iv),NULL, 0);
  // Send the intialised cipher some data and store it...
  mbedtls_gcm_update(&aes3, blockSize, (uint8_t*)buffer, (uint8_t*)buffer);
  memcpy((uint8_t*)(&(buffer->iv)), iv, 16);
  // Free up the context.
  mbedtls_gcm_free( &aes3 );
  return;
}

// Assume blockSize = 16 * k
void gcm_decrypt(EncBlock* buffer, int blockSize) {
  mbedtls_gcm_init( &aes3 );
  mbedtls_gcm_setkey(&aes3, MBEDTLS_CIPHER_ID_AES, (const unsigned char*) key2, 128);
  mbedtls_gcm_starts(&aes3, MBEDTLS_GCM_DECRYPT, (uint8_t*)(&(buffer->iv)), sizeof(buffer->iv), NULL, 0);
  mbedtls_gcm_update(&aes3, blockSize, (uint8_t*)buffer, (uint8_t*)buffer);
  mbedtls_gcm_free( &aes3 );
  return;
}

// startIdx: index of blocks, offset: data offset in the block, blockSize: bytes of real data
void OcallReadBlock(int startIdx, int offset, int* buffer, int blockSize, int structureId) {
  if (blockSize == 0) {
    return;
  }
  // // std::cout<< "In OcallReadBlock\n";
  EncBlock *readBuffer = (EncBlock*)malloc(sizeof(EncBlock));
  if (nonEnc) {
    OcallRB(startIdx, (int*)readBuffer, sizeof(EncBlock), structureId);
    int *a = (int*)readBuffer;
    memcpy(buffer, (int*)readBuffer+offset*(structureSize[structureId]/sizeof(int)), blockSize);
  } else {
    OcallRB(startIdx, (int*)readBuffer, sizeof(EncBlock), structureId);
    cbc_decrypt(readBuffer, sizeof(int)*BLOCK_DATA_SIZE);
    // gcm_decrypt(readBuffer, sizeof(int)*BLOCK_DATA_SIZE); 
    memcpy(buffer, (int*)readBuffer+offset*(structureSize[structureId]/sizeof(int)), blockSize);
  }
  free(readBuffer);
}
// startIdx: index of blocks, offset(int): data offset in the block, blockSize: bytes of real data
void OcallWriteBlock(int startIdx, int offset, int* buffer, int blockSize, int structureId) {
  // std::cout<< "In OcallWriteBlock\n";
  if (blockSize == 0) {
    return;
  }
  EncBlock* writeBuf = (EncBlock*)malloc(sizeof(EncBlock));
  if (nonEnc) {
    OcallWB(startIdx, offset*(structureSize[structureId]/sizeof(int)), buffer, blockSize, structureId);
  } else {
    if (offset == 0) { // could write the whole block
      // printf("In OcallWriteBlock1\n");
      memcpy((int*)writeBuf, buffer, blockSize);
      cbc_encrypt(writeBuf, sizeof(int)*BLOCK_DATA_SIZE);
      // gcm_encrypt(writeBuf, sizeof(int)*BLOCK_DATA_SIZE); 
      OcallWB(startIdx, 0, (int*)writeBuf, sizeof(EncBlock), structureId);
    } else { // read&decrypt first, later write&encrypt
      // printf("In OcallWriteBlock2\n");
      OcallRB(startIdx, (int*)writeBuf, sizeof(EncBlock), structureId);
      cbc_decrypt(writeBuf, sizeof(int)*BLOCK_DATA_SIZE);
      // gcm_decrypt(writeBuf, sizeof(int)*BLOCK_DATA_SIZE); 
      memcpy((int*)writeBuf+offset*(structureSize[structureId]/sizeof(int)), buffer, blockSize);
      cbc_encrypt(writeBuf, sizeof(int)*BLOCK_DATA_SIZE);
      // gcm_encrypt(writeBuf, sizeof(int)*BLOCK_DATA_SIZE); 
      OcallWB(startIdx, 0, (int*)writeBuf, sizeof(EncBlock), structureId);
    }
  }
  free(writeBuf);
}
// index: start index counted by elements (count from 0), blockSize: #elements
void opOneLinearScanBlock(int index, int* block, int blockSize, int structureId, int write, int dummyNum) {
  // std::cout<< "In opOneLinearScanBlock\n";
  if (blockSize + dummyNum == 0) {
    return ;
  }
  if (dummyNum < 0) {
    printf("Dummy padding error!");
    return ;
  }
  int multi = structureSize[structureId] / sizeof(int);
  int B = BLOCK_DATA_SIZE / multi;
  int startBIdx = index / B;
  int startOffset = index % B;
  int firstSize = B - startOffset;
  int boundary = ceil(1.0 * (blockSize - firstSize) / B) + 1;
  int opStart = 0;
  int Msize, offset = 0;
  if (!write) {
    // // // std::cout<< "In reading B: \n";
    for (int i = 0; i < boundary; ++i) {
      if (i != 0 && (i != boundary - 1)) {
        Msize = B;
      } else if (i == 0) {
        Msize = firstSize;
      } else {
        Msize = blockSize - opStart;
      }
      offset = (i == 0) ? startOffset : 0;
      // printf("Start Idx: %d, offset: %d, opstart: %d, Msize: %d\n", startBIdx + i, offset, opStart, Msize);
      OcallReadBlock(startBIdx + i, offset, &block[opStart * multi], Msize * structureSize[structureId], structureId);
      opStart += Msize;
    }
  } else {
    // printf("In opwriteReal\n");
    for (int i = 0; i < boundary; ++i) {
      if (i != 0 && (i != boundary - 1)) {
        Msize = B;
      } else if (i == 0) {
        Msize = firstSize;
      } else {
        Msize = blockSize - opStart;
      }
      offset = (i == 0) ? startOffset : 0;
      OcallWriteBlock(startBIdx + i, offset, &block[opStart * multi], Msize * structureSize[structureId], structureId);
      opStart += Msize;
    }
    if (dummyNum > 0) {
      // printf("In opwriteDummy\n");
      int *junk = (int*)malloc(dummyNum * multi * sizeof(int));
      for (int j = 0; j < dummyNum * multi; ++j) {
        junk[j] = DUMMY;
      }
      int dummyStart = (index + blockSize) / B;
      int dummyOffset = (index + blockSize) % B;
      int dummyFirstSize = B - dummyOffset;
      int dummyBoundary = ceil(1.0 * (dummyNum - dummyFirstSize) / B) + 1;
      int dummyOpStart = 0;
      for (int j = 0; j < dummyBoundary; ++j) {
        if (j != 0 && (j != dummyBoundary - 1)) {
          Msize = B;
        } else if (j == 0) {
          Msize = dummyFirstSize;
        } else {
          Msize = dummyNum - dummyOpStart;
        }
        offset = (j == 0) ? dummyOffset : 0;
        OcallWriteBlock(dummyStart + j, offset, &junk[dummyOpStart * multi], Msize * structureSize[structureId], structureId);
        dummyOpStart += Msize; 
      }
    }
  }
}

bool cmpHelper(int *a, int *b) {
  return (*a > *b) ? true : false;
}

bool cmpHelper(Bucket_x *a, Bucket_x *b) {
  return (a->x > b->x) ? true : false;
}
/*
int moveDummy(int *a, int size) {
  // k: #elem != DUMMY
  int k = 0;
  for (int i = 0; i < size; ++i) {
    if (a[i] != DUMMY) {
      if (i != k) {
        swapRow(&a[i], &a[k++]);
      } else {
        k++;
      }
    }
  }
  return k;
}*/

int moveDummy(int *a, int size) {
  int i = 0;
  int j = size - 1;
  while (1) {
    while (i < j && a[i] != DUMMY) i++;
    while (i < j && a[j] == DUMMY) j--;
    if (i >= j) break;
    swapRow(&a[i], &a[j]);
  }
  return i;
}

void swapRow(int *a, int *b) {
  int *temp = (int*)malloc(sizeof(int));
  memmove(temp, a, sizeof(int));
  memmove(a, b, sizeof(int));
  memmove(b, temp, sizeof(int));
  free(temp);
}

void swapRow(Bucket_x *a, Bucket_x *b) {
  Bucket_x *temp = (Bucket_x*)malloc(sizeof(Bucket_x));
  memmove(temp, a, sizeof(Bucket_x));
  memmove(a, b, sizeof(Bucket_x));
  memmove(b, temp, sizeof(Bucket_x));
  free(temp);
}

