
# Multi-Processing and Multi-Threading Word Frequency Analyzer

## Overview

This project analyzes the **top 10 most frequent words** in a large dataset (`enwik8`) using three different approaches:

- **Naïve Approach** (no parallelism)
- **Multiprocessing Approach** (using multiple processes)
- **Multithreading Approach** (using multiple threads)

It measures and compares execution times for each approach to evaluate the benefits of parallelism on multi-core systems.

---

## Files

| File | Description |
|:-----|:------------|
| `naive.c` | Naïve approach code (no parallelism) |
| `multiprocessing_code.c` | Multiprocessing code (using multiple child processes) |
| `multithreading_code.c` | Multithreading code (using pthreads) |
| `report.pdf` | Full project report with analysis, Amdahl’s law, results, and conclusions |
| `Project_Description.pdf` | Official project instructions and requirements |

---

## Problem Statement

Find the **top 10 most frequent words** in the `enwik8` dataset:  
📄 Dataset: [enwik8 (HuggingFace)](https://huggingface.co/datasets/LTCB/enwik8)

Compare the three approaches by measuring execution time for:

- 2, 4, 6, 8 processes/threads.
- Analyze results using **Amdahl's Law**.
- Discuss performance differences and draw conclusions.

---


## Requirements

- **GCC** Compiler
- **Linux OS** (Virtual Machine or Native)
- **C Language**
- Shared memory APIs (`shm_open`, `mmap`) for multiprocessing
- POSIX threads (`pthread`) for multithreading

---

## Results Summary

| Approach | Execution Time |
|:---------|:---------------|
| Naïve | 45 minutes |
| Multiprocessing (2 processes) | 27 minutes |
| Multiprocessing (4 processes) | 16.19 minutes |
| Multiprocessing (6 processes) | 15.1 minutes |
| Multiprocessing (8 processes) | 12.2 minutes |
| Multithreading (2 threads) | 28 seconds |
| Multithreading (4 threads) | 16.7 seconds |
| Multithreading (6 threads) | 16.3 seconds |
| Multithreading (8 threads) | 15.2 seconds |

✅ **Multithreading** achieved the best performance.

---

## Computer Environment

- Processor: Intel Core i5, 8th Gen @ ~3.2GHz  
- Memory: 4GB  
- OS: Linux (Virtual Machine with 5 allocated cores)  
- Programming Language: C  
- IDE/Environment: Linux Terminal  




