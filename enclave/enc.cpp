// #include "enc.h"
#include "bitonic.h"
#include "bucket.h"
#include "quick.h"
#include "oq.h"
#include "shared.h"

#include <ctime>

void printfEnc(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}

void callSort(int *resId, int *resN, int *address, double *params) {
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
  EnclaveServer eServer(N, M, B, sigma, encmode, SSD);
  eServer.nonEnc = 1;
  clock_t start, end;
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
    printf("Bucket Oblivious Sort\n");
    Bucket bksort(eServer, inputId);
    start = time(NULL);
    *resId = bksort.bucketOSort();
    end = time(NULL);
    printfEnc("Computation Time: %lf, IOtime: %lf, IOcost: %lf\n", (double)(end-start-eServer.getIOtime()), eServer.getIOtime(), eServer.getIOcost()*B/N);
    *resN = N;
  } else if (sortId == 3) {
    int64_t size = N / B;
    Bitonic bisort(eServer, inputId, 0, size);
    start = time(NULL);
    bisort.bitonicSort(0, size, 0);
    end = time(NULL);
    printfEnc("Computation Time: %lf, IOtime: %lf, IOcost: %lf\n", (double)(end-start-eServer.getIOtime()), eServer.getIOtime(), eServer.getIOcost()*B/N);
    *resId = inputId;
    *resN = N;
  } else {
    // NOTE: Used for test
  }
}
