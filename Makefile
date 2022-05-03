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
	host/osorthost ./enclave/osortenc.signed

simulate:
	host/osorthost ./enclave/osortenc.signed --simulate