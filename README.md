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
- x: Test Failed, xx: segfault, limited memory, v: test pass

- bucket sort 100w

| FAN OUT \ Z(bytes) | 323 \* 8 |
| ------------------ | -------- |
| 2                  | 8s       |
| 3                  | 7s       |
| 4                  | 17s      |
| 5                  | 15s      |
| 6                  | 7s       |
| 7                  | 18s      |
| 8                  | 33s      |
| 9                  | 6s       |
| 10                 | 6s       |
| 11                 | 17s      |
| 12                 | 32s      |
| 13                 | 30s      |
| 14                 | 36s      |
| 15                 | 42s      |
| 16                 | 48s      |
| 17                 | 58s      |
| 18                 | 70s      |
| 19                 | 6s       |
| 20                 | 6s       |

- bucket sort 500w
  | FAN OUT \ Z(bytes) | 333 \* 8 |
  | -------------- | --- |
  | 2 | 270s |
  | 3 | 229s |
  | 4 | 348s |
  | 5 | 365s |
  | 6 | 227s |
  | 7 | 409s |
  | 8 | 145s |
  | 9 | 151s |
  | 10 | 312s |
  | 11 | 409s |
  | 12 | 520s |
  | 13 | 647s |
  | 14 | 158s |
  | 15 | 178s |
  | 16 | 185s |
  | 17 | 222s |
  | 18 | 253s |
  | 19 | 280s |
  | 20 | 315s |

- bucket sort 1000w

| FAN OUT \ Z(bytes) | 337 \* 8 |
| ------------------ | -------- |
| 2                  | 906s     |
| 3                  | 1357s    |
| 4                  | 684s     |
| 5                  | 720s     |
| 6                  | 1378s    |
| 7                  | 800s     |
| 8                  | 1169s    |
| 9                  | x        |
| 10                 | 618s     |
| 11                 | 792s     |
| 12                 | 978s     |
| 13                 | 1183s    |
| 14                 | x        |
| 15                 | x        |
| 16                 | 364s     |
| 17                 | 446s     |
| 18                 | 494s     |
| 19                 | 525s     |
| 20                 | 576s     |

## 6. Version & Key commit

- v0.1 (Finish bitonic sort): bd6e19b8f56a56f2a1a992f4c97bcea36aca9f9c
- v0.2 (Finish bucket sort, primarily): 5c816db25c5ab2459efa2c22f1e6fca57e5d8203
- v0.3 (Finish bucket sort, 10w+256): 9fa159914cda99e5be47f6eb4caed0bab013924d
- v0.4 (latest): 475199efaf061591a2234bfec68150112b400476
- v0.5 (100w ok): 858935643980c7092078bf3e512eae81fa5b842d
- v0.6 (500w+3000, reconstruct): 67480c89060c7f6afcb28c0384600cf1f1972dd4
- v0.7 (1000w+10000): eff1ced37e4feedb88afedcf5c15703fab55ac3c
- v0.8 (1000w+10000+Reconstruct): 0f014ef78019d76db0a7a6f83a9c7af4e9723c11
