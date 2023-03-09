# OQSORT

## 1. Overview

This repo is used for implementing oqsort, bucketOSort, bitonic sort etc in TEE using openenclave.

## 2. Environment

- Setting Openenclave Environment
  - `. /opt/openenclave/share/openenclave/openenclaverc`
- Sgx oegdb Debug Command
  - `/opt/openenclave/bin/oegdb -arg ./host/oqsorthost ./enclave/oqsortenc.signed `
- swap file

```sh
$sudo swapoff -a
$sudo dd if=/dev/zero of=/swapfile bs=1G count=21
$sudo mkswap /swapfile
$sudo swapon /swapfile
$grep SwapTotal /proc/meminfo
```

### 2.1 Hardware

- CPU:
  - 72 Intel(R) Xeon(R) Gold 6354 CPU @ 3.00GHz
  - cache size: 16384 KB
- SGX:
  - Extended feature bits (EAX=07H, ECX=0H)
    eax: 2 ebx: f3bfb7ef ecx: 40417f5e edx: bc040412
    sgx available: 1
    sgx launch control: 1
  - CPUID Leaf 12H, Sub-Leaf 0 of Intel SGX Capabilities (EAX=12H,ECX=0)
    eax: 403 ebx: 1 ecx: 0 edx: 381f
    sgx 1 supported: 1
    sgx 2 supported: 1
    MaxEnclaveSize_Not64: 1f
    MaxEnclaveSize_64: 38
  - CPUID Leaf 12H, Sub-Leaf 2 of Intel SGX Capabilities (EAX=12H,ECX=2)
    eax: c00001 ebx: 80 ecx: 7ec00002 edx: 0
    size of EPC section in Processor Reserved Memory, 2028 M
  - CPUID Leaf 12H, Sub-Leaf 3 of Intel SGX Capabilities (EAX=12H,ECX=3)
    eax: c00001 ebx: 100 ecx: 7f400002 edx: 0
    size of EPC section in Processor Reserved Memory, 2036 M

### 2.2 Software

- System:

  - Linux: Ubuntu 20.04.1

- SDK:
  - Open Encalve SDK

## 3. Building

### 3.1 
```
$ make clean
$ make
$ make run
```

### 3.2 Docker
```
$ docker build -t openenclave .
$ docker run --device /dev/sgx_enclave --device /dev/sgx_provision --name bchenba_openenclave --mount type=bind,source=/home/bchenba/OQSORT,target=/OQSORT -dit openenclave
$ docker exec -it bchenba_openenclave bash
$ . /opt/openenclave/share/openenclave/openenclaverc
  or
$ cd /opt/openenclave/share/openenclave 
$ source openenclaverc
... same as 3.1
```

## 4. Structure Details

- `enclave/*`: codes running in encclave
  - `sort*`: different sorting algorithms
  - `enc.*`: enclave main file
  - `shared.*`: contain common functions used in sortings
- `host/*`: host main file
- `include/*`: common structures & definitions
- `oqsort.edl`: declarations about ECALLs & OCALLs

