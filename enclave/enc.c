#include <stdio.h>
#include "hello_t.h"

oe_result_t enclave_hello(char* this_is_a_string) {
  oe_result_t oe_result_value, method_result_value;
  oe_result_value = host_hello(&method_result_value, "this is a string");
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

