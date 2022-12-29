#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <cmath>
#include <cassert>
#include <cstdint>
#include <random>
#include <boost/program_options.hpp>
#include <openenclave/host.h>

#include "../include/DataStore.h"
#include "../include/common.h"

#include "oqsort_u.h"

#define NUM_STRUCTURES 10
#define MEM_IN_ENCLAVE 5

using namespace std;
using namespace chrono;
namespace po = boost::program_options;

EncOneBlock *arrayAddr[NUM_STRUCTURES];
double IOcost = 0;

/* OCall functions */
void ocall_print_string(const char *str) {
  /* Proxy/Bridge will check the length and null-terminate the input string to prevent buffer overflow. */
  printf("%s", str);
  fflush(stdout);
}

// index: Block Index, blockSize: bytes
void OcallRB(int64_t index, int* buffer, size_t blockSize, int structureId) {
  // std::cout<< "In OcallRB\n";
  memcpy(buffer, (int*)(&((arrayAddr[structureId])[index])), blockSize);
  IOcost += 1;
}

// index: Block index, blockSize: bytes
void OcallWB(int64_t index, int* buffer, size_t blockSize, int structureId) {
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

// support int version only
void fyShuffle(int structureId, size_t size, int B) {
  if (size % B != 0) {
    printf("Error! Not B's time.\n");
    return ;
  }
  int64_t total_blocks = size / B;
  EncOneBlock *trustedM3 = new EncOneBlock;
  int64_t k;
  std::random_device dev;
  std::mt19937 rng(dev()); 
  // switch block i & block k
  for (int64_t i = total_blocks-1; i >= 0; i--) {
    std::uniform_int_distribution<int64_t> dist(0, i);
    k = dist(rng);
    memcpy(trustedM3, arrayAddr[structureId] + k, sizeof(EncOneBlock));
    memcpy(arrayAddr[structureId] + k, arrayAddr[structureId] + i, sizeof(EncOneBlock));
    memcpy(arrayAddr[structureId] + i, trustedM3, sizeof(EncOneBlock));
  }
  std::cout << "Finished floyd shuffle\n";
}

po::variables_map read_options(int argc, char *argv[]) {
  int m, c;
  po::variables_map vm;
  try {
    po::option_description desc("Allowed options");
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
    ("sort_type,ST", po::value<int>->default_value(1), "Selections for sorting type: 0: ODSTight, 1: ODSLoose, 2: bucketOSort, 3: bitonicSort, 4: mergeSort")
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

int main(int argc, const char* argv[]) {
  int ret = 1;
  int *resId = new int;
  int *resN = new int;
  int FAN_OUT, BUCKET_SIZE;
  int inputId = 1;
  oe_enclave_t* enclave = NULL;
  high_resolution_clock::time_point start, end;
  milliseconds duration;
  // step1: init test numbers
  auto vm = read_options(argc, argv);
  int datatype = vm["datatype"].as<int>();
  int64_t M = ((vm["memory"].as<int64_t>()) << 20) / datatype;
  int64_t N = vm["c"].as<int>() * M;
  int B = vm["block_size"].as<int>();
  int sigma = vm["sigma"].as<int>();
  int sortId = vm["sort_type"].as<int>();
  double alpha = vm["alpha"].as<double>();
  double beta = vm["beta"].as<double>();
  double gamma = vm["gamma"].as<double>();
  int P = vm["P"].as<int>();

  double params[10] = {sortId, inputId, N, M, B, sigma, alpha, beta, gamma, P};
  // step2: Create the enclave
  // result = oe_create_oqsort_enclave(argv[1], OE_ENCLAVE_TYPE_SGX, OE_ENCLAVE_FLAG_DEBUG, NULL, 0, &enclave);
  // transition_using_threads
  oe_enclave_setting_context_switchless_t switchless_setting = {
        1,  // number of host worker threads
        1}; // number of enclave worker threads.
  oe_enclave_setting_t settings[] = {{
        .setting_type = OE_ENCLAVE_SETTING_CONTEXT_SWITCHLESS,
        .u.context_switchless_setting = &switchless_setting,
    }};
  result = oe_create_oqsort_enclave(argv[1], OE_ENCLAVE_TYPE_SGX, 0, settings, OE_COUNTOF(settings), &enclave);
  // result = oe_create_oqsort_enclave(argv[1], OE_ENCLAVE_TYPE_SGX, 0, NULL, 0, &enclave);
  // 0: OQSORT-Tight, 1: OQSORT-Loose, 2: bucketOSort, 3: bitonicSort
  if (sortId == 3 && (N % B) != 0) {
    int64_t addi = addi = ((N / B) + 1) * B - N;
    N += addi;
  }
  DataStore data(N, M, B);
  start = high_resolution_clock::now();
  if (sortId == 2) {
    int64_t totalSize = calBucketSize(sigma, N, M, B);
    data.init(inputId, N);
    data.init(inputId + 1, totalSize);
    data.init(inputId + 2, totalSize);
  } else {
    data.init(inputId, N);
  }
  callSort(enclave, resId, resN, params);
  end = high_resolution_clock::now();
  if (result != OE_OK) {
    fprintf(stderr, "Calling into enclave_hello failed: result=%u (%s)\n", result, oe_result_str(result));
    ret = -1;
  }
  // step4: std::cout execution time
  duration = duration_cast<milliseconds>(end - start);
  std::cout << "Time taken by sorting function: " << duration.count() << " miliseconds" << std::endl;
  printf("IOcost: %f, %f\n", IOcost/N*B, IOcost);
  // testEnc(arrayAddr, *resId, *resN);
  data.print(*resId, *resN);
  // step5: exix part
  exit:
    if (enclave) {
      oe_terminate_enclave(enclave);
    }
    delete resId;
    delete resN;
    return ret;
}