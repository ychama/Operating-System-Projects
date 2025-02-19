# Best-Fit Dynamic Partition Memory Allocation Simulator

## Table of Contents
1. [Introduction](#introduction)
2. [Project Goals](#project-goals)
3. [Key Concepts](#key-concepts)
4. [Implementation Details](#implementation-details)
5. [How to Build and Run](#how-to-build-and-run)
6. [Example Usage](#example-usage)
7. [Notes](#notes)
8. [License](#license)

---

## Introduction
This repository contains my personal implementation of a **best-fit dynamic partition memory allocation** simulator. I wanted to mimic how `malloc()` and `free()` work at a conceptual level, focusing on:

- **Partitioning** memory into free and occupied blocks.
- **Allocating** memory using the *best-fit* strategy.
- **Deallocating** memory by freeing and merging adjacent free blocks.

Although this is a simplified model, it demonstrates core ideas behind dynamic memory management.

---

## Project Goals
- **Practice memory partition management** using lists or sets to track free blocks.
- **Simulate “on-demand” memory growth** in multiples of a configurable page size.
- **Learn how to split free blocks on allocation** and **merge adjacent free blocks** on deallocation.
- **Keep track of requested pages** and the **largest free partition** at the end of the simulation.

---

## Key Concepts

### Partitions
Each chunk of memory is represented by a `Partion` structure:
```cpp
struct Partion {
  long tag;      // Identifies the partition (occupied if >= 0, free if -1)
  long size;     // Size of the partition in bytes
  int64_t addr;  // Starting address of the partition
};
```

- **`tag == -1`**: The partition is free.
- **`tag >= 0`**: The partition is occupied and references the `tag` provided by an allocation request.

### Page Size
All memory requests expand the available space in multiples of a configurable **`page_size`**. If an allocation does not fit in any existing free partition, the simulator “requests” enough pages from the OS to satisfy that request (plus any remainder to match a page boundary).

### Best-Fit Allocation
- I use a **sorted data structure** (a `std::set` with a custom comparator) to keep track of free partitions by **increasing size**.
- When an **allocation request** arrives:
  1. Find the smallest free block that can satisfy the request (best-fit).
  2. If not found, request more memory from the OS (in multiples of `page_size`).
  3. **Split** the chosen free block if it’s larger than needed.

### Deallocation and Merging
- A **deallocation request** frees all partitions with the matching `tag`.
- Freed partitions are marked with **`tag = -1`** and then merged with adjacent free partitions to maintain **maximal contiguous free space**.

### Tracking Statistics
At the end of the simulation, I compute two metrics:
1. **`n_pages_requested`**: Total pages acquired from the OS.  
2. **`max_free_partition_size`**: The size of the largest free partition left.

---

## Implementation Details

### Core Structures and Methods

1. **`Simulator` Class**
   - Maintains:
     - A `std::list<Partion>` of all partitions in ascending order of addresses.
     - A `std::set<PartitionRef, scmp>` that indexes **free** partitions by size (and then by address).
     - A `std::unordered_map<long, std::vector<PartitionRef>>` to quickly find all partitions of a given `tag`.
   - **Allocation** (`allocate(tag, size)`):
     1. Find a best-fit free partition using `free_blocks.lower_bound(...)`.
     2. If not found, request enough pages to accommodate the new block.
     3. Insert or modify partitions accordingly, and update data structures.
   - **Deallocation** (`deallocate(tag)`):
     1. Mark each partition with the matching `tag` as free.
     2. Merge with adjacent free blocks if possible.
   - **Statistics** (`getStats(result)`):
     1. Set `result.n_pages_requested` to total pages requested so far.
     2. Find the largest free partition size for `result.max_free_partition_size`.

2. **`mem_sim(page_size, requests, result)`**
   - Creates a `Simulator` instance with the given `page_size`.
   - Starts with one empty partition (size = 0, `tag = -1`).
   - Iterates over each request:
     - If `tag >= 0`, call `allocate(tag, size)`.
     - If `tag < 0`, call `deallocate(-tag)`.
   - Extracts final stats into `result`.

---

## How to Build and Run

1. **Clone** this project:
   ```bash
   git clone <YOUR_REPOSITORY_URL>
   cd memsim
   ```

2. **Compile** using the provided `Makefile`:
   ```bash
   make
   ```
   This produces an executable named **`memsim`**.

3. **Run** the simulator:
   ```bash
   ./memsim <page_size> < input_file
   ```
   - **`page_size`**: Any positive integer specifying the page size in bytes.
   - **`input_file`**: Text file with allocation (`tag size`) or deallocation (`-tag`) lines.

---

## Example Usage

### Sample Input
```
5 100
-5
-6
1 100
2 20
1 100
2 30
1 100
2 40
1 100
-2
2 21
-1
3 220
3 759
3 1
3 5900
```
- **`5 100`** → Allocate 100 bytes with tag 5.
- **`-5`** → Deallocate all partitions with tag 5.
- **`-6`** → Deallocate all partitions with tag 6 (if any).
- … and so on.

### Running
```bash
g++ -O2 -Wall memsim.cpp -o memsim
./memsim 1000 < test_input.txt
```

### Possible Output
```
pages requested: 7
largest free partition: 99
```

---

## Notes
- **Efficiency**:  
  - I rely on `std::set` to keep free blocks in ascending order by size. This supports quick lookups for best-fit but is still `O(log n)` for insert/delete.  
  - Merging adjacent free blocks is straightforward in `O(1)` time, since I maintain a doubly-linked list of all partitions in ascending address order.
- **Memory Limits**:  
  - The simulator handles large requests and page sizes up to 1,000,000.  
  - For extremely large inputs (up to 1,000,000 requests), the data structure choices keep the simulation efficient enough.

---