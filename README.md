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

- bucket sort

100w, 500w, 1000w, corresponding to the first, the second and the third column

| M/Z \ Z(bytes) | 323 \* 8 | 333 \* 8 | 337 \* 8 |
| -------------- | -------- | -------- | -------- |
| 2              | 8s       | 43s      | 95s      |
| 3              | 6s       | 40s      | 228s     |
| 4              | 7s       | 41s      | 76s      |
| 5              | 7s       | 41s      | 76s      |
| 6              | 6s       | 35s      | 311s     |
| 7              | 7s       | 44s      | 79s      |
| 8              | 8s       | 32s      | 214s     |
| 9              | 5s       | 32s      | 510s     |
| 10             | 6s       | 41s      | 76s      |
| 11             | 6s       | 56s      | 101s     |
| 12             | 7s       | 93s      | 169s     |
| 13             | 7s       | 145s     | 257s     |
| 14             | 8s       | 32s      | 376s     |
| 15             | 9s       | 34s      | 1918s    |
| 16             | 10s      | 36s      | 69s      |
| 17             | 11s      | 38s      | 71s      |
| 18             | 12s      | 39s      | 74s      |
| 19             | 5s       | 42s      | 78s      |
| 20             | 5s       | 53s      | 96s      |

- oqsort (Time, IOcost)

| B,type/N(10^4) | 500          | 600          | 700          | 800          | 900          | 1000          |
| -------------- | ------------ | ------------ | ------------ | ------------ | ------------ | ------------- |
| 4, T           | 50s,13168870 | 59s,15635865 | 69s,18241884 | 80s,20625353 | 90s,23203449 | 99s,25781679  |
| 4, L           | 51s,12460507 | 60s,14533611 | 72s,17152972 | 82s,19268224 | 92s,21678412 | 100s,24066255 |
| 5, T           | 43s,11407451 | 52s,13422070 | 60s,15659092 | 70s,17718363 | 77s,19933057 | 85s,21925366  |
| 5, L           | 45s,10991854 | 54s,12790012 | 62s,14919311 | 71s,16698155 | 78s,18751674 | 87s,20649795  |
| 6, T           | 40s,10478649 | 48s,12240840 | 55s,14151276 | 62s,16024585 | 71s,17860908 | 78s,19845496  |
| 6, L           | 42s,10248146 | 49s,11824088 | 57s,13530172 | 65s,15375684 | 72s,17022945 | 81s,18910147  |
| 7, T           | x            | 44s,11352124 | 51s,13021773 | 59s,14755054 | 65s,16456341 | 72s,18284837  |
| 7, L           | x            | 46s,11126143 | 54s,12666213 | 60s,14198127 | 67s,15826555 | 74s,17596466  |
| 8, T           | x            | x            | 50s,12385467 | 56s,13932539 | 62s,15549002 | 68s,17137622  |
| 8, L           | x            | x            | 52s,12191353 | 58s,13581347 | 64s,15146543 | 70s,16541910  |

## 6. Version & Key commit

- v0.1 (Finish bitonic sort): bd6e19b8f56a56f2a1a992f4c97bcea36aca9f9c
- v0.2 (Finish bucket sort, primarily): 5c816db25c5ab2459efa2c22f1e6fca57e5d8203
- v0.3 (Finish bucket sort, 10w+256): 9fa159914cda99e5be47f6eb4caed0bab013924d
- v0.4 (latest): 475199efaf061591a2234bfec68150112b400476
- v0.5 (100w ok): 858935643980c7092078bf3e512eae81fa5b842d
- v0.6 (500w+3000, reconstruct): 67480c89060c7f6afcb28c0384600cf1f1972dd4
- v0.7 (1000w+10000): eff1ced37e4feedb88afedcf5c15703fab55ac3c
- v0.8 (1000w+10000+Reconstruct): 0f014ef78019d76db0a7a6f83a9c7af4e9723c11
