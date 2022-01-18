include ../../config.mk

CFLAGS=$(shell pkg-config oehost-$(COMPILER) --cflags)
LDFLAGS=$(shell pkg-config oehost-$(COMPILER) --libs)
INCDIR=$(shell pkg-config oehost-$(COMPILER) --variable=includedir)

all:
	$(MAKE) build

build:
	@ echo "Compiler used: $(CC), $(CXX)"
	oeedger8r ../hello.edl --untrusted \
		--search-path $(INCDIR) \
		--search-path $(INCDIR)/openenclave/edl/sgx
	$(CC) -g -c $(CFLAGS) host.c
	$(CC) -g -c $(CFLAGS) hello_u.c
	$(CC) -o hellohost hello_u.o host.o $(LDFLAGS)

clean:
	rm -f hellohost host.o hello_u.o hello_u.c hello_u.h hello_args.h