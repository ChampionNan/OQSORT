# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.
include ../../config.mk

CRYPTO_LDFLAGS := $(shell pkg-config oeenclave-$(COMPILER) --variable=${OE_CRYPTO_LIB}libs)

ifeq ($(LVI_MITIGATION), ControlFlow)
		ifeq ($(LVI_MITIGATION_BINDIR),)
			$(error LVI_MITIGATION_BINDIR is not set)
		endif
		# Only run once. 
		ifeq (,$(findstring $(LVI_MITIGATION_BINDIR),$(CC)))		CC := $(LVI_MITIGATION_BINDIR)/$(CC)
		endif
		COMPILER := $(COMPILER)-lvi-cfg
		CRYPTO_LDFLAGS := $(shell pkjg-config oeenclave-$(COMPILER) --variable=${OE_CRYPTO_LIB}libslvicfg)
endif

CFLAGS=$(shell pkg-config oeenclave-$(COMPILER) --cflags)
LDFLAGS=$(shell pkg-config oeenclave-$(COMPILER) --libs)
INCDIR=$(shell pkg-config oeenclave-$(COMPILER) --variable=includedir)

all:
	$(MAKE) build
	$(MAKE) keys
	$(MAKE) sign

build:
	@ echo "Compilers used: $(CC), $(CXX)"
	oeedger8r ../hello.edl --trusted \
		--search-path $(INCDIR) \
		--search-path $(INCDIR)/openenclave/edl/sgx
	$(CC) -g -c $(CFLAGS) -DOE_API_VERSION=2 enc.c -o enc.o
	$(CC) -g -c $(CFLAGS) -DOE_API_VERSION=2 hello_t.c -o hello_t.o
	$(CC) -o helloenc hello_t.o enc.o $(LDFLAGS) $(CRYPTO_LDFLAGS)

sign:
	oesign sign -e helloenc -c hello.conf -k private.pem

clean:
	rm -f enc.o helloenc helloenc.signed private.pem public.pem hello_t.o hello_t.h hello_t.c hello_args.h

keys:
	openssl genrsa -out private.pem -3 3072
	openssl rsa -in private.pem -pubout -out public.pem