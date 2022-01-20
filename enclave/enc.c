#include <stdio.h>
#include "hello_t.h"

oe_result_t enclave_hello(int* number) {
  oe_result_t oe_result_value, method_result_value;
  fprintf(stdout, "This is oputput from enclave called from host: %d\n", number[0]);
  char string[20] = {"this is string"};
  // oe_result_value = host_hello(&method_result_value, string);
  oe_result_value = OE_OK;
  method_result_value = OE_OK; 
  if (oe_result_value != OE_OK) {
    fprintf(stderr,
            "Call to host_hello failed: result=%u (%s)\n",
            oe_result_value,
            oe_result_str(oe_result_value));
  } else {
    if (method_result_value != OE_OK) {
      oe_result_value = method_result_value;
    }
  }
  return oe_result_value;
}

