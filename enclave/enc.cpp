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
  EncMode encmode = GCM; // GCM, OFB
  SecLevel seclevel = FULLY; // FULLY, PARTIAL
  EnclaveServer eServer(N, M, B, encmode);

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
  } else {
    // NOTE: Used for test
    eServer.nonEnc = 0;
    int64_t size = 500000; 
    clock_t start, end;
    EncOneBlock *trustedM = new EncOneBlock[size];
    for (int i = 0; i < size; ++i) {
      trustedM[i].sortKey = size - i;
      trustedM[i].primaryKey = i;
    }
    start = time(NULL);
    // eServer.oswap128(ta, tb, 1);
    Bitonic bisort(eServer);
    bisort.smallBitonicSort(trustedM, 0, size, 0);
    // Quick qsort(eServer, trustedM);
    // qsort.quickSort(0, size);
    end = time(NULL);
    for (int i = 0; i < 50; ++i) {
      printf("%d ", trustedM[i].sortKey);
    }
    printf("\n");
    delete [] trustedM;
    printf("Sort time: %lf\n", (double)(end-start));
  }
}