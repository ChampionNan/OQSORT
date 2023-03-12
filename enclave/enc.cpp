// #include "enc.h"
#include "bitonic.h"
#include "bucket.h"
#include "quick.h"
#include "oq.h"
#include "shared.h"

#include <ctime>

void callSort(int *resId, int *resN, double *params) {
  int sortId = params[0];
  int inputId = params[1];
  int64_t N = params[2];
  int64_t M = params[3];
  int B = params[4];
  double sigma = params[5];
  double alpha = params[6];
  double beta = params[7];
  double gamma = params[8];
  int P = params[9];
  int SSD = params[10];
  EncMode encmode = GCM; // GCM, OFB
  SecLevel seclevel = FULLY; // FULLY, PARTIAL
  EnclaveServer eServer(N, M, B, encmode, SSD);

  if (sortId == 0) { // ODS-Tight
    ODS odsTight(eServer, alpha, beta, gamma, P, 1, seclevel, inputId + 2);
    odsTight.ObliviousSort(N, ODSTIGHT, inputId, inputId + 1, inputId);
    *resId = odsTight.resultId;
    *resN = odsTight.resultN;
  } else if (sortId == 1) { // ODS_Loose
    ODS odsLoose(eServer, alpha, beta, gamma, P, 0, seclevel, inputId + 2);
    odsLoose.ObliviousSort(N, ODSLOOSE, inputId, inputId + 1, inputId);
    *resId = odsLoose.resultId;
    *resN = odsLoose.resultN;
  } else if (sortId == 2) { // bucket oblivious sort
    clock_t start, end;
    Bucket bksort(eServer, inputId - 1);
    start = time(NULL);
    *resId = bksort.bucketOSort();
    end = time(NULL);
    *resN = N;
    printf("Computation Time: %lf, IOtime: %lf, IOcost: %lf\n", (double)(end-start-eServer.getIOtime()), eServer.getIOtime(), eServer.getIOcost()*B/N);
  } else if (sortId == 3) {
    int64_t size = N / B;
    Bitonic bisort(eServer, inputId, 0, size);
    bisort.bitonicSort(0, size, 0);
    *resId = inputId;
    *resN = N;
  } else if (sortId == -1) {
    clock_t start, end;
    start = time(NULL);
    fyShuffle(inputId, N, 1);
    end = time(NULL);
    printf("Time: %lf\n", (double)(end-start));
  } else {
    // NOTE: Used for test
    // FIXME: not correct, should do OCall
    clock_t start, end;
    eServer.nonEnc = 0; // do the encryption
    int n = (4 << 10) / sizeof(EncOneBlock);
    int times = (1 << 21); // one for ncryption & one for decryption
    EncOneBlock *a = new EncOneBlock[n];
    freeAllocate(0, 0, n, 0);
    start = time(NULL);
    for (int i = 0; i < times; ++i) {
      eServer.opOneLinearScanBlock(0, a, n, 0, 0, 0);
      eServer.opOneLinearScanBlock(0, a, n, 0, 1, 0);
    }
    end = time(NULL);
    delete [] a;
    printf("Time: %lf\n", (double)(end-start));
  }
}