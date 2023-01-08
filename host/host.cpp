#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <cmath>
#include <cassert>
#include <cstdint>
#include <random>
// #include <boost/program_options.hpp>
#include <openenclave/host.h>

#include "../include/DataStore.h"
#include "../include/common.h"

#include "oqsort_u.h"

using namespace std;
using namespace chrono;
// namespace po = boost::program_options;

EncOneBlock *arrayAddr[NUM_STRUCTURES];
double IOcost = 0;

/* OCall functions */
void OcallSample(int inStructureId, int sampleId, int sortedSampleId, int64_t N, int64_t M, int64_t n_prime, int is_tight, int64_t *ret) {
  int64_t N_prime = N;
  int64_t boundary = ceil(1.0 * N_prime / M);
  int64_t realNum = 0;
  int64_t readStart = 0;
  EncOneBlock *trustedM1 = new EncOneBlock[M];
  int64_t m = 0, Msize;
  freeAllocate(sampleId, sampleId, n_prime);
  for (int64_t i = 0; i < boundary; ++i) {
    Msize = std::min(M, N - i * M);
    m = Hypergeometric(N_prime, Msize, n_prime);
    printf("Sampling progress: %ld / %ld, m: %ld\n", i, boundary-1, m);
    if (is_tight || (!is_tight && m > 0)) {
      memcpy(trustedM1, arrayAddr[inStructureId] + readStart, Msize * sizeof(EncOneBlock));
      readStart += Msize;
      shuffle(trustedM1, Msize);
      memcpy(arrayAddr[sampleId] + realNum, trustedM1, m * sizeof(EncOneBlock));
      realNum += m;
    }
  }
  delete [] trustedM1;
  *ret = realNum;
}

void ocall_print_string(const char *str) {
  /* Proxy/Bridge will check the length and null-terminate the input string to prevent buffer overflow. */
  printf("%s", str);
  fflush(stdout);
}

// index: Block Index, blockSize: bytes
void OcallRB(size_t index, int* buffer, size_t blockSize, int structureId) {
  // std::cout<< "In OcallRB\n";
  memcpy(buffer, (int*)(&((arrayAddr[structureId])[index])), blockSize);
  IOcost += 1;
}

// index: Block index, blockSize: bytes
void OcallWB(size_t index, int* buffer, size_t blockSize, int structureId) {
  // std::cout<< "In OcallWB\n";
  memcpy((int*)(&((arrayAddr[structureId])[index])), buffer, blockSize);
  IOcost += 1;
}

// allocate encB for outside memory
void freeAllocate(int structureIdM, int structureIdF, size_t size) {
  // 1. Free arrayAddr[structureId]
  if (arrayAddr[structureIdF]) {
    delete [] arrayAddr[structureIdF];
  }
  // 2. malloc new asked size (allocated in outside)
  if (size <= 0) {
    return;
  }
  EncOneBlock *addr = (EncOneBlock*)malloc(size * sizeof(EncOneBlock));
  memset(addr, DUMMY<int>(), size * sizeof(EncOneBlock));
  // 3. assign malloc address to arrayAddr
  arrayAddr[structureIdM] = addr;
  return ;
}

// shuffle data in B block size
void fyShuffle(int structureId, size_t size, int B) {
  if (size % B != 0) {
    printf("Error! Not B's time.\n"); // Still do the shuffling
  }
  int64_t total_blocks = size / B;
  EncOneBlock *trustedM3 = new EncOneBlock[B];
  int64_t k;
  int64_t eachSec = size / 100;
  int swapSize = sizeof(EncOneBlock) * B;
  std::random_device rd;
  std::mt19937 rng{rd()};
  // switch block i & block k
  for (int64_t i = total_blocks-1; i >= 0; i--) {
    if (i % eachSec == 0) {
      printf("Shuffle progress %ld / %ld\n", i, total_blocks-1);
    }
    std::uniform_int_distribution<int64_t> dist(0, i);
    k = dist(rng);
    memcpy(trustedM3, arrayAddr[structureId] + k * B, swapSize);
    memcpy(arrayAddr[structureId] + k * B, arrayAddr[structureId] + i * B, swapSize);
    memcpy(arrayAddr[structureId] + i * B, trustedM3, swapSize);
  }
  std::cout << "Finished floyd shuffle\n";
}

void readParams(InputType inputtype, int &datatype, int64_t &N, int64_t &M, int &B, int &sigma, int &sortId, double &alpha, double &beta, double &gamma, int &P, int &argc, const char* argv[]) {
  if (inputtype == BOOST) {
    cout << "Need to be done." << endl;
    return;
    /* TODO: Finish in the future
    auto vm = read_options(argc, argv);
    datatype = vm["datatype"].as<int>();
    M = ((vm["memory"].as<int64_t>()) << 20) / datatype;
    N = vm["c"].as<int>() * M;
    B = vm["block_size"].as<int>();
    sigma = vm["sigma"].as<int>();
    sortId = vm["sort_type"].as<int>();
    alpha = vm["alpha"].as<double>();
    beta = vm["beta"].as<double>();
    gamma = vm["gamma"].as<double>();
    P = vm["P"].as<int>();*/
  } else if (inputtype == SETINMAIN) {
    datatype = 4;
    M = (128 << 20) / 16; // (MB << 20) / 1 element bytes
    N = 200 * M;
    B = (4 << 10) / 16; // 4KB pagesize
    sigma = 40;
    // 0: OQSORT-Tight, 1: OQSORT-Loose, 2: bucketOSort, 3: bitonicSort
    sortId = 1;
    alpha = 0.02;
    beta = 0.04;
    gamma = 0.07;
    P = 228;
  }
}

int main(int argc, const char* argv[]) {
  int ret = 1;
  int *resId = new int;
  int *resN = new int;
  int FAN_OUT, BUCKET_SIZE;
  int inputId = 1;
  oe_result_t result;
  oe_enclave_t* enclave = NULL;
  high_resolution_clock::time_point start, end;
  seconds duration;
  // step1: init test numbers
  InputType inputtype = SETINMAIN;
  int datatype, B, sigma, sortId, P;
  int64_t N, M;
  double alpha, beta, gamma;
  readParams(inputtype, datatype, N, M, B, sigma, sortId, alpha, beta, gamma, P, argc, argv);
  double params[10] = {(double)sortId, (double)inputId, (double)N, (double)M, (double)B, (double)sigma, alpha, beta, gamma, (double)P};
  // step2: Create the enclave
  // result = oe_create_oqsort_enclave(argv[1], OE_ENCLAVE_TYPE_SGX, OE_ENCLAVE_FLAG_DEBUG, NULL, 0, &enclave);
  // transition_using_threads
  /*
  oe_enclave_setting_context_switchless_t switchless_setting = {
        1,  // number of host worker threads
        1}; // number of enclave worker threads.
  oe_enclave_setting_t settings[] = {{
        .setting_type = OE_ENCLAVE_SETTING_CONTEXT_SWITCHLESS,
        .u.context_switchless_setting = &switchless_setting,
    }};
  result = oe_create_oqsort_enclave(argv[1], OE_ENCLAVE_TYPE_SGX, 0, settings, OE_COUNTOF(settings), &enclave);
  */
  result = oe_create_oqsort_enclave(argv[1], OE_ENCLAVE_TYPE_SGX, 0, NULL, 0, &enclave);
  // 0: OQSORT-Tight, 1: OQSORT-Loose, 2: bucketOSort, 3: bitonicSort
  if (sortId == 3 && (N % B) != 0) {
    int64_t addi = addi = ((N / B) + 1) * B - N;
    N += addi;
  }
  DataStore data(arrayAddr, N, M, B);
  if (sortId == 2) {
    int64_t totalSize = calBucketSize(sigma, N, M, B);
    data.init(inputId, N);
    data.init(inputId + 1, totalSize);
    data.init(inputId + 2, totalSize);
  } else {
    data.init(inputId, N);
  }
  start = high_resolution_clock::now();
  callSort(enclave, resId, resN, params);
  end = high_resolution_clock::now();
  if (result != OE_OK) {
    fprintf(stderr, "Calling into enclave_hello failed: result=%u (%s)\n", result, oe_result_str(result));
    ret = -1;
  }
  // step4: std::cout execution time
  duration = duration_cast<seconds>(end - start);
  std::cout << "Time taken by sorting function: " << duration.count() << " seconds" << std::endl;
  printf("IOcost: %f, %f\n", IOcost/N*B, IOcost);
  // testEnc(arrayAddr, *resId, *resN);
  data.print(*resId, *resN, FILEOUT, data.filepath);
  // step5: exix part
  exit:
    if (enclave) {
      oe_terminate_enclave(enclave);
    }
    delete resId;
    delete resN;
    return ret;
}

/*
po::variables_map read_options(int argc, const char *argv[]) {
  int m, c;
  po::variables_map vm;
  try {
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help,H", "Show help message")
    ("memory,M", po::value<int64_t>()->default_value(8), "Internal memory size (MB)")
    ("c,c", po::value<int>()->default_value(16), "The value of N/M")
    ("block_size,B", po::value<int>()->default_value(4), "Block size (in terms of elements)")
    ("num_threads,T", po::value<int>()->default_value(4), "#threads, not suppoted in enclave yet")
    ("sigma,s", po::value<int>()->default_value(40), "Failure probability upper bound: 2^(-sigma)")
    ("alpha,a", po::value<double>()->default_value(-1), "Parameter for ODS")
    ("beta,b", po::value<double>()->default_value(-1), "Parameter for ODS")
    ("gamma,g", po::value<double>()->default_value(-1), "Parameter for ODS")
    ("P,P", po::value<int>()->default_value(1), "Parameter for ODS")
    ("sort_type,ST", po::value<int>()->default_value(1), "Selections for sorting type: 0: ODSTight, 1: ODSLoose, 2: bucketOSort, 3: bitonicSort, 4: mergeSort")
    ("datatype,DT", po::value<int>()->default_value(4), "#bytes for this kind of datatype, normally int32_t or int64_t");
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    if (vm.count("help") || vm.count("H")) {
      cout << desc << endl;
      exit(0);
    }
  } catch (exception &e) {
    cerr << "Error: " << e.what() << endl;
    exit(1);
  } catch (...) {
    cerr << "Exception of unknown type! \n";
    exit(-1);
  }
  return vm;
}
*/