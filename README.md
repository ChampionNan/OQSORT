# OQSORT

## 1. Overview

This repo is used for implementing oqsort, bucketOSort, bitonic sort etc in TEE using openenclave.

## 2. Environment

- `. /opt/openenclave/share/openenclave/openenclaverc`
- `/opt/openenclave/bin/oegdb -arg ./host/oqsorthost ./enclave/oqsortenc.signed `

### 2.1 Hardware

- CPU:
  - Intel(R) Xeon(R) E-2288G CPU @ 3.70GHz
  - cache size: 16384 KB
- SGX:
  - Linux SGX 5.11.0-1025-azure

### 2.2 Software

- System:
  - Linux: Ubuntu 20.04.1
- SDK:
  - Open Encalve SDK

## 3. Building

```
$ make clean
$ make
$ make run
```

## 4. Structure Details

## 5. Version & Key commit

-
