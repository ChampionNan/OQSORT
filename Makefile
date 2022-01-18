.PHONY: all build clean run simulate

OE_CRYPTO_LIB := mbedtls
export OE_CRYPTO_LIB

all: build

build:
	$(MAKE) -C enclave
	$(MAKE) -C host

clean: 
	$(MAKE) -C enclave clean
	$(MAKE) -C host clean

run:
	host/hellohost ./enclave/helloenc.signed

simulate:
	host/hellohost ./enclave/helloenc.signed --simulate