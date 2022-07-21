# OQSORT

## 1. Overview

This repo is used for implementing oqsort, bucketOSort, bitonic sort etc in TEE using openenclave.

## 2. Environment

- Setting Openenclave Environment
  - `. /opt/openenclave/share/openenclave/openenclaverc`
- Sgx oegdb Debug Command
  - `/opt/openenclave/bin/oegdb -arg ./host/oqsorthost ./enclave/oqsortenc.signed `

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

- config: NumHeapPages=262144, NumStackPages=8192, NumTCS=1
- $Z=max(B, (40+log(n)) * 6)$
- x: Test Failed, xx: segfault, limited memory, v: test pass on non-enclave environment
- A/B: server data / local data
- bucket sort 100w

  | B(bytes) \ M/B | 2   | 3   | 4   | 5   | 6   | 7   |
  | -------------- | --- | --- | --- | --- | --- | --- |
  | 256 \* 8       | 18s | x   | x   | x   | x   | x   |
  | 512 \* 8       | 7s  | 6s  | 5s  | x   | x   | x   |
  | 1000 \* 8      | 6s  | 5s  | 5s  | 5s  | 6s  | 4s  |
  | 2000 \* 8      | 5s  | 5s  | 4s  | 5s  | 4s  | 5s  |
  | 3000 \* 8      | 5s  | 4s  | 4s  | 5s  | 4s  | 5s  |
  | 5000 \* 8      | 5s  | 4s  | 4s  | 4s  | 4s  | 5s  |
  | 7000 \* 8      | 5s  | 5s  | 4s  | 4s  | 4s  | 4s  |
  | 10000 \* 8     | 5s  | 4s  | 4s  | 4s  | 4s  | 4s  |

- bucket sort 500w

| B(bytes) \ M/B | 2    | 3    | 4    | 5    | 6    | 7    |
| -------------- | ---- | ---- | ---- | ---- | ---- | ---- |
| 256 \* 8       | xx/v | xx/x |      |      |      |      |
| 512 \* 8       | xx/v | xx/x |      |      |      |      |
| 1000 \* 8      | xx/v | xx/v | xx/v | xx/v | xx/x |      |
| 2000 \* 8      | 84s  | 26s  | xx/v | xx/v | xx/v | xx/v |
| 3000 \* 8      | 31s  | 27s  | 23s  | 35s  | 27s  | xx/v |
| 5000 \* 8      | 29s  | 24s  | 24s  | 23s  | 28s  | 22s  |
| 7000 \* 8      | 29s  | 24s  | 24s  | 23s  | 28s  | 23s  |
| 10000 \* 8     | 27s  | 25s  | 22s  | 23s  | 22s  | 23s  |

- bucket sort 1000w

| B(bytes) \ M/B | 2    | 3    | 4    | 5   | 6   | 7    |
| -------------- | ---- | ---- | ---- | --- | --- | ---- |
| 256 \* 8       | xx/v | xx/x |      |     |     |      |
| 512 \* 8       | xx/v | xx/v | xx/x |     |     |      |
| 1000 \* 8      | xx/v | xx/v | xx/x |     |     |      |
| 2000 \* 8      | xx/v | xx/v | xx/v |     |     |      |
| 3000 \* 8      | 169s | xx/v |      |     |     |      |
| 5000 \* 8      | 62s  | 53s  | 47s  | 68s | 53s | xx/v |
| 7000 \* 8      | 63s  | 53s  | 47s  | 46s | 53s | xx   |
| 10000 \* 8     | 58s  | 48s  | 48s  | 46s | 54s | 45s  |

## 6. Version & Key commit

- v0.1 (Finish bitonic sort): bd6e19b8f56a56f2a1a992f4c97bcea36aca9f9c
- v0.2 (Finish bucket sort, primarily): 5c816db25c5ab2459efa2c22f1e6fca57e5d8203
- v0.3 (Finish bucket sort, 10w+256): 9fa159914cda99e5be47f6eb4caed0bab013924d
- v0.4 (latest): 475199efaf061591a2234bfec68150112b400476
- v0.5 (100w ok): 858935643980c7092078bf3e512eae81fa5b842d
- v0.6 (500w+3000, reconstruct): 67480c89060c7f6afcb28c0384600cf1f1972dd4
- v0.7 (1000w+10000): eff1ced37e4feedb88afedcf5c15703fab55ac3c
- v0.8 (1000w+10000+Reconstruct): 0f014ef78019d76db0a7a6f83a9c7af4e9723c11
