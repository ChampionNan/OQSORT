# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.
include ../../config.mk

OE_CRYPTO_LIB := mbedtls

CFLAGS=$(shell pkg-config oeenclave-$(C_COMPILER) --cflags)
CXXFLAGS=$(shell pkg-config oeenclave-$(CXX_COMPILER) --cflags)
LDFLAGS=$(shell pkg-config oeenclave-$(CXX_COMPILER) --libs)
INCDIR=$(shell pkg-config oeenclave-$(C_COMPILER) --variable=includedir)
CRYPTO_LDFLAGS=$(shell pkg-config oeenclave-$(COMPILER) --variable=${OE_CRYPTO_LIB}libs)

all:
	$(MAKE) build
	$(MAKE) keys
	$(MAKE) sign

build:
	@ echo "Compilers used: $(CC), $(CXX)"
	oeedger8r ../osort.edl --trusted \
		--search-path $(INCDIR) \
		--search-path $(INCDIR)/openenclave/edl/sgx
	@ echo "FLAGS: $(LDFLAGS) $(CRYPTO_LDFLAGS)"
	$(CXX) -g -c $(CXXFLAGS) -DOE_API_VERSION=2 -std=c++11 enc.cpp
	$(CC) -g -c $(CFLAGS) -DOE_API_VERSION=2 osort_t.c
	$(CXX) -o osortenc enc.o osort_t.o $(LDFLAGS) $(CRYPTO_LDFLAGS)

sign:
	oesign sign -e osortenc -c osort.conf -k private.pem

clean:
	rm -f enc.o osortenc osortenc.signed private.pem public.pem osort_t.* osort_args.h

keys:
	openssl genrsa -out private.pem -3 3072
	openssl rsa -in private.pem -pubout -out public.pem