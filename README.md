# High-concurrency Caching System

A high-performance, **thread-safe cache system** implemented in **C++**, supporting multiple **page replacement algorithms** (LRU, LFU, ARC).  
This project focuses on **multi-threading optimizations**, **cache hit rate improvements**, and **adaptive replacement strategies**.

---

## Features

### Core Algorithms
- **LRU (Least Recently Used)** — Evicts items that haven’t been accessed recently.
- **LFU (Least Frequently Used)** — Evicts items with the lowest access frequency.
- **ARC (Adaptive Replacement Cache)** — Dynamically balances between LRU and LFU behavior.

### Optimizations

#### LRU Optimizations
- **LRU-Sharding**: Improves concurrency under high multi-threaded workloads.  
- **LRU-K**: Prevents hot data from being replaced by cold data to reduce cache pollution.

#### LFU Optimizations
- **LFU-Sharding**: Enhances parallel access efficiency.  
- **Max Average Frequency Control**: Avoids outdated hot data occupying cache space.

---

## Environment

- **Operating System:** Ubuntu 22.04 LTS  
- **Build System:** CMake  
- **Compiler:** g++ / clang++

---

## Build & Run

```bash
# Create build directory
mkdir build && cd build

# Generate build files
cmake ..

# Build project
make

# Run executable
./main