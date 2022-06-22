include ../../config.mk

CFLAGS=$(shell pkg-config oehost-$(C_COMPILER) --cflags)
CXXFLAGS=$(shell pkg-config oehost-$(CXX_COMPILER) --cflags)
LDFLAGS=$(shell pkg-config oehost-$(CXX_COMPILER) --libs)
INCDIR=$(shell pkg-config oehost-$(COMPILER) --variable=includedir)

all:
	$(MAKE) build

build:
	@ echo "Compiler used: $(CC), $(CXX)"
	oeedger8r ../oqsort.edl --untrusted \
		--search-path $(INCDIR) \
		--search-path $(INCDIR)/openenclave/edl/sgx
	$(CXX) -g -c $(CXXFLAGS) host.cpp
	$(CC) -g -c $(CFLAGS) oqsort_u.c
	$(CXX) -o oqsorthost oqsort_u.o host.o $(LDFLAGS)

clean:
	rm -f hellohost host.o oqsort_*