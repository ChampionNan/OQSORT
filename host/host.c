#include <openenclave/host.h>
#include <stdio.h>
#include <time.h>

#include "hello_u.h"

const int SIZE = 256 * 1024;
const int TIMES = 200000;

oe_result_t host_hello(int* number) {
  fprintf(stdout, "This is output from host called from enclave: %d\n", number[0]);
  return OE_OK;
}

int main(int argc, const char* argv[]) {
  oe_result_t result;
  oe_result_t method_return;
  int ret = 1;
  oe_enclave_t* enclave = NULL;
  FILE *f = fopen("running_time.txt", "w+");
  if (argc != 2) {
    goto exit;
  }
  // Create the enclave
  result = oe_create_hello_enclave(
    argv[1], OE_ENCLAVE_TYPE_SGX, 0, NULL, 0, &enclave);
  if (result != OE_OK) {
    fprintf(stderr,
            "oe_create_hello_enclave(): result=%u (%s)\n",
            result,
            oe_result_str(result));
    goto exit;
  }
  // Call into the enclave
  clock_t start_out, end_out, start_in, end_in;
  int string[SIZE];
  
  srand(time(0));
  for (int i = 0; i < SIZE; ++i) {
    string[i] = rand();
  }
  
  start_out = clock();
  int i = 0;
  do {
    // start_in = clock();
    result = enclave_hello(enclave,
                         &method_return,
                         string);
    // end_in = clock();
    ++i;
  } while (result == OE_OK && i <= TIMES);

  end_out = clock();
  if (result != OE_OK) {
    fprintf(stderr,
            "Calling into enclave_hello failed: result=%u (%s)\n",
            result,
            oe_result_str(result));
    goto exit;
  } else {
    if (method_return != OE_OK) {
      goto exit;
    }
  }

  ret = 0;
  fprintf(stdout, "Passing %d KB size using %f time for %d loops\n", SIZE/256, (double)(end_out-start_out)/CLOCKS_PER_SEC, TIMES);
  exit:
    if (enclave) 
      oe_terminate_enclave(enclave);
    
    return ret;
}
