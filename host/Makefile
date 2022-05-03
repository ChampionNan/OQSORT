include ../../config.mk

CFLAGS=$(shell pkg-config oehost-$(C_COMPILER) --cflags)
CXXFLAGS=$(shell pkg-config oehost-$(CXX_COMPILER) --cflags)
LDFLAGS=$(shell pkg-config oehost-$(CXX_COMPILER) --libs)
INCDIR=$(shell pkg-config oehost-$(COMPILER) --variable=includedir)

all:
	$(MAKE) build

build:
	@ echo "Compiler used: $(CC), $(CXX)"
	oeedger8r ../osort.edl --untrusted \
		--search-path $(INCDIR) \
		--search-path $(INCDIR)/openenclave/edl/sgx
	$(CXX) -g -c $(CXXFLAGS) host.cpp
	$(CC) -g -c $(CFLAGS) osort_u.c
	$(CXX) -o osorthost osort_u.o host.o $(LDFLAGS)

clean:
	rm -f hellohost host.o osort_*