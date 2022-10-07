.PHONY: all build clean run simulate

OE_CRYPTO_LIB := mbedtls
export OE_CRYPTO_LIB

all: build

build:
	python3 calParams.py
	$(MAKE) -C enclave
	$(MAKE) -C host

clean: 
	$(MAKE) -C enclave clean
	$(MAKE) -C host clean

run:
	host/oqsorthost ./enclave/oqsortenc.signed

simulate:
	host/oqsorthost ./enclave/oqsortenc.signed --simulate