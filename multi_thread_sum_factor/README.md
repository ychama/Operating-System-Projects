# Multi-Threaded Factor Sum

## Important Concepts

- **`pthread_barrier_t`**: Used to synchronize multiple threads at specific points in the program. In this project, each thread waits at a barrier after reading the next number (serial phase) and again after doing factor checks (parallel phase). This ensures orderly progression through the workload.

- **Thread “Cancellation” / Short-Circuiting**: If any thread finds a smaller factor, the other threads can skip further checks for that same number. This prevents unnecessary work once we have discovered a divisor.

---

## Table of Contents

1. [Introduction](#introduction)  
2. [Project Overview](#project-overview)  
3. [Key Features](#key-features)  
4. [Implementation Details](#implementation-details)  
5. [Building and Usage](#building-and-usage)  
6. [Example](#example)  
7. [Performance Considerations](#performance-considerations)  
8. [License](#license)  

---

## Introduction

This project demonstrates how to convert a single-threaded program into a **multi-threaded** version to **speed up** the process of finding the smallest non-trivial factors of numbers. Specifically, the program:

- Reads a list of **64-bit integers** (`[0..2^63-2]`) from **standard input**.
- **Ignores** any numbers less than 2 or prime numbers.
- For all **composite** numbers, calculates the **smallest factor** (≥ 2).
- Sums these smallest factors and prints the final **sum** to **standard output**.

---

## Project Overview

Originally, the single-threaded version processed each integer in a straightforward manner:
1. Check if the number is less than 2 or prime.
2. If composite, find its smallest factor.

This could be **time-consuming** for large inputs or numbers that require many checks. The new approach:
- Uses **`n_threads`** worker threads to distribute the computation.
- Aims to achieve near-optimal **speedup** proportional to the number of available CPU cores.

---

## Key Features

1. **Multi-Threaded Factorization**  
   - Each number is split into partial ranges for the threads to inspect in parallel.

2. **Flexible Thread Count**  
   - The program accepts one command-line argument, **`n_threads`**, to specify the number of threads.

3. **Load Balancing**  
   - Partitioning work so no single thread is disproportionately loaded with “hard” or large numbers.

4. **Thread-Safe Summation & Early Cancellation**  
   - Uses atomic operations and a global minimum to short-circuit the search for a given number once a smaller factor is found.

5. **Automatic Fallback for Single Thread**  
   - If `n_threads == 1`, it runs the **original** single-threaded code.

---

## Implementation Details

1. **Main Function**  
   - **`sum_factors(int n_threads)`**  
     - Spawns `n_threads` threads (if `n_threads > 1`), each running a **thread start function** that cycles between serial and parallel phases.
     - If `n_threads == 1`, it simply falls back to the single-threaded version.

2. **Synchronization Using `pthread_barrier_t`**  
   - **Barriers** coordinate the threads to ensure:
     - All threads complete the serial phase (e.g., reading the next number, setting up search ranges) before any thread starts the parallel factor search.
     - All threads complete the parallel search before the next iteration.

3. **Parallel Factor-Finding**  
   - Each thread checks a **subrange** of potential factors. Once any thread finds a factor, the others can skip further checks.

4. **Data Structures**  
   - `tasks[]`: An array of structures holding **thread IDs**, **start/end** ranges, and the **original number**.  
   - **Global Variables**:  
     - `global_sum`: Accumulated sum of the smallest factors.  
     - `min_divisor_found`: Atomic variable storing the smallest divisor found for the current number.  
     - `global_finished`: Signals no more numbers are available.  
     - `initial_check`: Indicates if the current number can be immediately skipped (prime or < 2).

---

## Building and Usage

1. **Clone** the repository:
   ```bash
   git clone 
   cd factor-sum
   ```

2. **Compile**:
   ```bash
   make
   ```
   This generates the `sumFactors` executable.

3. **Run**:
   ```bash
   ./sumFactors <n_threads> < input_file
   ```
   - `<n_threads>`: Number of threads to use (e.g., 4).
   - `<input_file>`: A file containing a list of 64-bit integers.

The program outputs:
```
Sum of divisors = <result>
```

---

## Example

Consider an example input file `example.txt`:

```
0 3 19 25
4012009 165 1033
```

- **0** is ignored (smaller than 2).
- **3, 19, and 1033** are prime.
- **25** has smallest non-trivial factor **5**.
- **4012009** has factor **2003** (4012009 = 2003 × 2003).
- **165** has factor **3**.

When running with 1 thread:
```bash
./sumFactors 1 < example.txt
Using 1 thread.
Sum of divisors = 2011
```
Because `5 + 2003 + 3 = 2011`.

For more speedup, try:
```bash
./sumFactors 4 < example.txt
```
The result stays the same but should run faster for large/complex inputs.

---

## Performance Considerations

- **Balanced Work Distribution**:  
  - Parallelizing the factor checks ensures multiple threads split heavy computations.
  - Large numbers require more checks, so an **early cancellation** mechanism halts other threads as soon as a small factor is found.

- **Edge Cases**:
  - If only a single number is in the input, multi-threading can only help if that single number is large enough to benefit from parallel factor searching.
  - If there are many trivial or prime numbers, the overhead of threading might not provide large gains.

---