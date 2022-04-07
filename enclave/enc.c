#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "hello_t.h"

const int SIZE[14] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};

oe_result_t enclave_hello(int* number) {
  oe_result_t oe_result_value, method_result_value;
  // fprintf(stdout, "This is oputput from enclave called from host: %d\n", number[0]);
  // char string[20] = {"this is string"};
  // oe_result_value = host_hello(&method_result_value, string);
  
  int res = 0;
  for (int i = 0; i < SIZE[10]*256; ++i) {
    res += number[i];
  }
  oe_result_value = OE_OK; 
  // number = (int*)oe_host_malloc(sizeof(int) * SIZE[10] * 256);
  if (number == NULL) {
    oe_result_value = OE_OUT_OF_MEMORY;
    goto exit; 
  }
  method_result_value = OE_OK; 
  if (oe_result_value != OE_OK) {
    fprintf(stderr,
            "Call to host_hello failed: result=%u (%s)\n",
            oe_result_value,
            oe_result_str(oe_result_value));
    goto exit;
  } else {
    if (method_result_value != OE_OK) {
      oe_result_value = method_result_value;
    }
  }
exit:
  return (int)oe_result_value;
}

