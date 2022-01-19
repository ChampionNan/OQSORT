#include <openenclave/host.h>
#include <stdio.h>
#include <time.h>

#include "hello_u.h"

const int SIZE = 100000;

oe_result_t host_hello(char* this_is_a_string) {
  fprintf(stdout, "This is output from host called from enclave: %s\n", this_is_a_string);
  return OE_OK;
}

int main(int argc, const char* argv[]) {
  oe_result_t result;
  oe_result_t method_return;
  int ret = 1;
  oe_enclave_t* enclave = NULL;
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
  clock_t start, end;
  char string[SIZE] = {"abc"};
  start = clock();
  result = enclave_hello(enclave,
                         &method_return,
                         string);
  end = clock();
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
  fprintf(stdout, "Passing 4 * %d bytes size using %f time\n", SIZE, (double)(end-start)/CLOCKS_PER_SEC);
  exit:
    if (enclave) 
      oe_terminate_enclave(enclave);
    
    return ret;
}
