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
$sudo dd if=/dev/zero of=/swapfile bs=1G count=4
$sudo mkswap /swapfile
$sudo swapon /swapfile
$grep SwapTotal /proc/meminfo
```

### 2.1 Hardware

- CPU:
  - Intel(R) Xeon(R) E-2288G CPU @ 3.70GHz
  - cache size: 16384 KB
- SGX:
  - Linux SGX 5.11.0-1025-azure
  - Extended feature bits (EAX=07H, ECX=0H)
  - eax: 0 ebx: 9c6fbd ecx: 40000000 edx: 400
  - sgx available: 1
  - sgx launch control: 1
  - CPUID Leaf 12H, Sub-Leaf 0 of Intel SGX Capabilities (EAX=12H,ECX=0)
  - eax: 1 ebx: 0 ecx: 0 edx: 241f
  - sgx 1 supported: 1
  - sgx 2 supported: 0
  - MaxEnclaveSize_Not64: 1f
  - MaxEnclaveSize_64: 24

### 2.2 Software

- System:

  - Linux: Ubuntu 20.04.1

- SDK:
  - Open Encalve SDK

## 3. Building

```
$ cd OQSORT
$ make clean
$ make
$ make run
```

## 4. Structure Details

- `enclave/*`: codes running in encclave
  - `sort/*`: different sorting algorithms
  - `enc.*`: enclave main file
  - `shared.*`: contain common functions used in sortings
- `host/*`: host main file
- `include/*`: common structures & definitions
- `oqsort.edl`: declarations about ECALLs & OCALLs
- `gdb.cfg`: breakpoints used in oegdb

## 5. Test Results

- config: Debug=0, NumHeapPages=100000, NumStackPages=10000, NumTCS=1, swap=4G
- $Z=max(B, (40+log(n)) * 6)$
- x: Test Failed, xx: segfault, limited memory, v: test pass

- bucket sort 100w

| FAN OUT \ Z(bytes) | 323 \* 8 |
| ------------------ | -------- |
| 2                  | 8s       |
| 3                  | 6s       |
| 4                  | 7s       |
| 5                  | 7s       |
| 6                  | 6s       |
| 7                  | 7s       |
| 8                  | 8s       |
| 9                  | 5s       |
| 10                 | 6s       |
| 11                 | 6s       |
| 12                 | 7s       |
| 13                 | 7s       |
| 14                 | 8s       |
| 15                 | 9s       |
| 16                 | 10s      |
| 17                 | 11s      |
| 18                 | 12s      |
| 19                 | 5s       |
| 20                 | 5s       |

- bucket sort 500w
  | FAN OUT \ Z(bytes) | 333 \* 8 |
  | -------------- | --- |
  | 2 | 43s |
  | 3 | 40s |
  | 4 | 41s |
  | 5 | 41s |
  | 6 | 35s |
  | 7 | 44s |
  | 8 | 32s |
  | 9 | 32s |
  | 10 | 41s |
  | 11 | 56s |
  | 12 | 93s |
  | 13 | 145s |
  | 14 | 32s |
  | 15 | 34s |
  | 16 | 36s |
  | 17 | 38s |
  | 18 | 39s |
  | 19 | 42s |
  | 20 | 53s |

- bucket sort 1000w

| FAN OUT \ Z(bytes) | 337 \* 8 |
| ------------------ | -------- |
| 2                  | 95s      |
| 3                  | 228s     |
| 4                  | 76s      |
| 5                  | 76s      |
| 6                  | 311s     |
| 7                  | 79s      |
| 8                  | 214s     |
| 9                  | 510s     |
| 10                 | 76s      |
| 11                 | 101s     |
| 12                 | 169s     |
| 13                 | 257s     |
| 14                 | 376s     |
| 15                 | 1918s    |
| 16                 | 69s      |
| 17                 | 71s      |
| 18                 | 74s      |
| 19                 | 78s      |
| 20                 | 96s      |

## 6. Version & Key commit

- v0.1 (Finish bitonic sort): bd6e19b8f56a56f2a1a992f4c97bcea36aca9f9c
- v0.2 (Finish bucket sort, primarily): 5c816db25c5ab2459efa2c22f1e6fca57e5d8203
- v0.3 (Finish bucket sort, 10w+256): 9fa159914cda99e5be47f6eb4caed0bab013924d
- v0.4 (latest): 475199efaf061591a2234bfec68150112b400476
- v0.5 (100w ok): 858935643980c7092078bf3e512eae81fa5b842d
- v0.6 (500w+3000, reconstruct): 67480c89060c7f6afcb28c0384600cf1f1972dd4
- v0.7 (1000w+10000): eff1ced37e4feedb88afedcf5c15703fab55ac3c
- v0.8 (1000w+10000+Reconstruct): 0f014ef78019d76db0a7a6f83a9c7af4e9723c11
