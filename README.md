# OSORT

## 1. Overview
This repo is used for testing the execution time to pass different size of data inside an SGX (in), do some calculations, and copy it back outside. 

## 2. Environment
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
$ make
  or 
$ make build
```

## 4. Testcase
### 4.1 Passing data
We pass different size data from the host to the enclave using pointer, and return to the host. We record the time in the host for running 20000 passing data iterations. 

### 4.2 Passing data and calculate
We pass different size data from the host to the enclave using pointer, add all the data and return to the host. We record the time in the host for running 20000 passing data iterations. 

### 4.3 Results

data size = [1, 2, 4, 8, 16, 32, 64, 128, 256] KB

1. Execution time about different data size

- Overview

<img src='./pic/KB2.png'>

- Details

<img src='./pic/KB1.png'>

2. Average execution time about different data size

<img src='./pic/KB3.png'>

3. Data 

| data size (KB) / running time (s) | pass | pass & calculate |
| :--- | :---: | :---: |
| 1 | 1.258899 | 1.421846 |
| 2 | 1.287418 | 1.616117 |
| 4 | 1.280107 | 1.921882 |
| 8 | 1.398573 | 2.673572 |
| 16 | 1.576942 | 4.274649 |
| 32 | 1.916218 | 7.221779 |
| 64 | 2.796984 | 12.790657 |
| 128 | 4.590082 | 24.379629 |
| 256 | 7.576735 | 48.610560 |









