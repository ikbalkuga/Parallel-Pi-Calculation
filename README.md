# Parallel Pi Calculation in C

A C project that approximates π using the Leibniz series with serial and parallel implementations using POSIX threads (Pthreads).

## Features

- Serial and parallel implementations
- Configurable iteration count and thread count
- Input validation and error handling
- Workload distribution across threads
- Performance benchmarking
- Automated invalid-input tests

## How It Works

The program approximates π using the Leibniz series:

$$
\pi = 4 \sum_{i=0}^{n-1} \frac{(-1)^i}{2i+1}
$$

The serial version calculates the sum sequentially, while the parallel version distributes the work among multiple threads.

## Requirements

- Linux or WSL
- GCC
- GNU Make
- POSIX threads
- `/usr/bin/time` for benchmarking

## Build

Compile both implementations:

```bash
make
```

Compiler flags: `-std=c11 -Wall -Wextra -Wpedantic -Werror -O2`

## Usage

**Serial:**

```bash
./pi_serial 1000000
```

**Parallel (4 threads):**

```bash
./pi_parallel 1000000 4
```

## Benchmarking

Run the benchmark with 3 billion iterations:

```bash
./benchmark.sh 3000000000
```

The script compares serial execution with parallel runs using 1, 2, 4, and 8 threads. Results are saved to `benchmark_results.md`.

*Note: Benchmark results may vary depending on hardware and system load.*

## Error-Handling Tests

Run the automated invalid-input tests:

```bash
make test-errors
```

The tests cover missing arguments, invalid inputs, zero iterations, and invalid thread counts.

## Clean

Remove compiled executables and generated benchmark results:

```bash
make clean
```

## Notes

This project demonstrates parallel computation, thread management, and performance comparison in C.
