// #include "enc.h"
#include "bitonic.h"
#include "bucket.h"
#include "quick.h"
#include "oq.h"
#include "shared.h"
#include "enc.h"

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
  EncMode GCM;
  EnclaveServer eServer(N, M, B, GCM);

  if (sortId == 0) { // ODS-Tight
    ODS odsTight(eServer, ODSTIGHT, inputId, alpha, beta, gamma, P, 1);
    odsTight.ObliviousSort(N);
    *resId = odsTight.resultId;
    *resN = odsTight.resultN;
  } else if (sortId == 1) { // ODS_Loose
    ODS odsLoose(eServer, ODSTIGHT, inputId, alpha, beta, gamma, P, 0);
    odsLoose.ObliviousSort(N);
    *resId = odsLoose.resultId;
    *resN = odsLoose.resultN;
  } else if (sortId == 2) { // bucket oblivious sort
    Bucket bksort(eServer, inputId - 1);
    *resId = bksort.bucketOSort();
    *resN = N;
  } else if (sortId == 3) {
    int64_t size = N / B;
    Bitonic bisort(eServer, inputId, 0, size);
    bisort.bitonicSort(0, size, 0);
    *resId = inputId;
    *resN = N;
  }
}