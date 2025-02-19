# Round-Robin CPU Scheduling Simulator

## Table of Contents

1. [Introduction](#introduction)
2. [Key Features](#key-features)
3. [File Structure](#file-structure)
4. [Implementation Details](#implementation-details)
5. [Building and Running](#building-and-running)
6. [Usage Example](#usage-example)
7. [Output Format](#output-format)
8. [Notes and Assumptions](#notes-and-assumptions)
9. [License](#license)

---

## Introduction

This repository contains my implementation of a **Round-Robin (RR) CPU Scheduling Simulator**. Round-robin scheduling is a popular CPU scheduling algorithm that assigns time slices (or quanta) to processes in a cyclic manner. If a process doesn’t complete within its assigned quantum, it is placed at the back of the ready queue.

The core goal of this simulator is to:

1. Compute the **start time** and **finish time** for each CPU-bound process.
2. Generate a **condensed execution order** of processes, indicating which process ran at each time step (or `-1` when the CPU is idle).

---

## Key Features

- **Flexible Time Quantum**: Pass any positive integer as the time slice.
- **Condensed Execution Order**: Only store up to a specified maximum number of sequence entries (helpful for large simulations).
- **Automatic Fast-Forward**: The simulator can skip idle or repetitive CPU states to speed up the computation when possible.
- **Simple Implementation**: Uses C++ standard library features such as `std::queue` for the ready queue.

---

## File Structure

1. **`scheduler.cpp`**  
   - Contains the main function `simulate_rr()` where I’ve implemented the round-robin logic.

2. **`scheduler.h`**  
   - Declares the `Process` structure and the `simulate_rr()` function signature.

3. **`common.cpp`** and **`common.h`**  
   - Provide helper functions and utility code.

4. **`main.cpp`**  
   - A basic driver program that demonstrates how to read input, call `simulate_rr()`, and display results.

5. **`Makefile`**  
   - Used to compile and build the project.

---

## Implementation Details

### `simulate_rr()` Function

```cpp
void simulate_rr(
    int64_t quantum,
    int64_t max_seq_len,
    std::vector<Process> & processes,
    std::vector<int> & seq
);
```

- **Parameters**:
  - `quantum`: Length of each time slice.
  - `max_seq_len`: Maximum length of the execution sequence to record.
  - `processes`: A list of `Process` structures, each containing:
    - `id`  
    - `arrival_time`  
    - `burst`  
    - `start_time` (computed)  
    - `finish_time` (computed)
  - `seq`: A vector storing the ID of the process running at each time step (or `-1` when idle).  

- **Logic**:
  - Sorts and inserts processes into a **ready queue** (or direct CPU assignment) based on their arrival times.
  - Keeps track of **remaining CPU bursts** for each process, updating them after each quantum.
  - If a process completes, it is removed from the queue, and `finish_time` is recorded.
  - If a process uses up its quantum but isn’t done, it is placed at the back of the queue.
  - Skips over large stretches of idle CPU or continuous running with no arrivals to optimize performance.

---

## Building and Running

1. **Clone or Download** the repository:
   ```bash
   git clone <REPO_URL>
   cd scheduler
   ```

2. **Compile** using the provided Makefile:
   ```bash
   make
   ```
   This creates an executable named `scheduler`.

3. **Run** the simulator:
   ```bash
   ./scheduler <quantum> <max_seq_len> < input_file.txt
   ```
   - `<quantum>`: Time slice length.
   - `<max_seq_len>`: The maximum entries of execution history to record.
   - `input_file.txt`: File containing process details:
     ```
     arrival_time burst_length
     arrival_time burst_length
     ...
     ```

---

## Usage Example

Suppose you have a file called `test1.txt`:

```txt
1 10
3 5
5 3
```

This means:
- **Process 0**: Arrives at time 1, needs 10 units of CPU.
- **Process 1**: Arrives at time 3, needs 5 units of CPU.
- **Process 2**: Arrives at time 5, needs 3 units of CPU.

**Run** the simulator with a time slice of 3 and a max sequence length of 20:

```bash
./scheduler 3 20 < test1.txt
```

**Output** could look like this (example of correct round-robin scheduling):

```
seq = [-1, 0, 1, 0, 2, 1, 0]
+------------------------+-------------------+-------------------+-------------------+-------------------+
| Id | Arrival | Burst | Start | Finish |
+------------------------+-------------------+-------------------+-------------------+-------------------+
|  0 |   1     |  10   |   1   |   19   |
|  1 |   3     |   5   |   4   |   15   |
|  2 |   5     |   3   |  10   |   13   |
+------------------------+-------------------+-------------------+-------------------+-------------------+
```

- `-1` indicates the CPU was idle from time 0 to 1.
- The table shows each process’s final start and finish times.

---

## Output Format

- **`seq`**: Shows the execution order as a list of process IDs or `-1` (idle CPU). Limited by `max_seq_len`.
- **Process Table**: Displays each process’s:
  - **ID**
  - **Arrival time**
  - **CPU burst**
  - **Start time**
  - **Finish time**

---

## Notes and Assumptions

- **Arrival Ordering**: Processes in `input_file.txt` are implicitly assigned IDs in ascending order (first line = ID 0, second line = ID 1, etc.).
- **Pure CPU-Bound**: Each process is assumed to never block for I/O or require additional waiting.
- **Skipping Time**: The code may skip through large idle or continuous running periods to optimize runtime.
- **Quantum Handling**: If a process remains after using its quantum, it is placed at the back of the ready queue.

---
