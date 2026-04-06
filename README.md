# Billion Row Challenge

I am using the setup provided by [dannyvankooten](https://github.com/dannyvankooten/1brc#submitting) to create my billion-row file and analyze it. All my attempts will be my journey learning concurrency.

## The Challenge

[challenge](https://1brc.dev/#the-challenge)

Find the min, mean, and max of each weather station from a file containing **1 billion rows** of temperature readings.

Input format:
```
Hamburg;12.0
Bulawayo;8.9
Palembang;38.8
```

---

## Prerequisites

- **g++** with C++11 support
- **gcc** (for building the sample data generator)
- A Unix-like environment (Linux / macOS)
- ~14 GB of free disk space for the full 1 billion row dataset

---

## Quick Start

### 1. Generate the Test Data

First, compile the sample data generator and create a measurements file:

```bash
# Compile the data generator
gcc -o create-sample create-sample.c -lm

# Generate 1 billion rows (~14 GB file)
./create-sample 1000000000

# This creates a file called "measurements.txt" in the project root
```

> **Tip:** For faster testing, generate a smaller sample first:
> ```bash
> ./create-sample 1000000   # 1 million rows
> ./create-sample 100000000 # 100 million rows
> ```

### 2. Build a Version

Use `make` to build the latest version (`src/main.cpp`) into `bin/main`:

```bash
make
```

To compile individual versions directly:

```bash
# v1 - Single threaded baseline
g++ -Wall -Wextra -pedantic -std=c++11 -O3 -o v1 v1.cpp

# v2 - Producer/consumer with circular buffer
g++ -Wall -Wextra -pedantic -std=c++11 -O3 -I./src/includes -o v2 v2.cpp src/classes/buffer.cpp

# v3 - Multithreaded with mmap (16 chunks)
g++ -Wall -Wextra -pedantic -std=c++11 -O3 -o v3 v3.cpp

# v4 - Optimised multithreaded with mmap (256 chunks)
g++ -Wall -Wextra -pedantic -std=c++11 -O3 -o v4 v4.cpp

# v5 - Allocation-free hot loop (requires C++17)
g++ -Wall -Wextra -pedantic -std=c++17 -O3 -o v5 v5.cpp
```

### 3. Run

Every version takes the measurements file as its only argument:

```bash
# Run the latest (built via make)
./bin/main measurements.txt

# Or run a specific version
./v1 measurements.txt
./v3 measurements.txt
./v4 measurements.txt
./v5 measurements.txt
```

### 4. Benchmark

Use the provided shell scripts to benchmark runs:

```bash
# Run the latest build 10 times with 20s cooldown between runs
bash run.sh

# Run all numbered versions (1-7) 5 times each and report averages
bash run-progressions.sh
```

---

## Build Options

| Command | Description |
|---------|-------------|
| `make` | Build `src/main.cpp` → `bin/main` with `-O3` optimisation |
| `make debug` | Build with debug symbols (`-g -O0 -DDEBUG`) |
| `make clean` | Remove the `bin/` directory |

---

## Project Structure

```
.
├── Makefile             # Build system for src/main.cpp
├── create-sample.c      # Generates the measurements.txt test data
├── run.sh               # Benchmark script (single version, 10 runs)
├── run-progressions.sh  # Benchmark script (all versions, 5 runs each)
├── v1.cpp               # Version 1 – single threaded
├── v2.cpp               # Version 2 – producer/consumer threads
├── v3.cpp               # Version 3 – multithreaded mmap (16 chunks)
├── v4.cpp               # Version 4 – optimised mmap (256 chunks)
├── v5.cpp               # Version 5 – allocation-free hot loop
└── src/
    ├── main.cpp          # Latest working version
    ├── classes/
    │   └── buffer.cpp    # Circular buffer (used by v2)
    └── includes/
        └── buffer.h      # Buffer header
```

---

## Version History

### v1 — Single Threaded Baseline (~5 min)

Single thread, calculate min/mean/max with an `unordered_map`. Reads the file line by line.

### v2 — Producer/Consumer Threads (slower than v1)

Trying to have one thread for parsing and one thread for processing with a circular buffer. Actually made it slower due to synchronisation overhead!

### v3 — Multithreaded with mmap (~25 sec)

Reverted back to the v1 approach but made it multithreaded by splitting the file into 16 chunks and processing them separately with `mmap`. Memory mapping was essential — without it, chunk-boundary seeking and line-by-line reading caused too much overhead or ran out of memory.

Chrono timestamps confirm the file processing is the bottleneck (24.9s of the total 25.0s).

### v4 — Optimised mmap (~5 sec)

Using `-O3` compiler optimisations and 256 chunks to better utilise available threads. Integer arithmetic for temperatures instead of floating point. Next steps: further architecture-specific tuning.

### v5 — Parsing & Threading Fixes (~20 sec on 16GB M1 Pro)

Cleaner version of v4 with real code-level improvements:

- **Inline `parseTemp()`** — parse temperature directly from raw `char*` into an `int`, eliminating `std::string` + `std::stoi()` × 1 billion rows
- **Pointer-range city name** — `std::string(start, end)` instead of `push_back()` per character
- **`std::thread::hardware_concurrency()`** — match actual CPU cores instead of spawning 256 threads
- **`madvise(MADV_SEQUENTIAL)`** — hint to kernel for aggressive prefetch
- **`.reserve(512)` on hash maps** — prevent rehashing (only ~400 stations)
- **Single hash lookup in combine step** — one `combined[name]` instead of four

---

## A Note on Memory: The Real Bottleneck

The original v4 claimed ~5 seconds, but that was on a machine with significantly more RAM. On a **16GB M1 Pro laptop** processing a **~13GB file**, every version is fundamentally **memory-bound**:

| Metric | Observation |
|--------|-------------|
| File size | ~13 GB |
| Available RAM | 16 GB |
| System time | ~14–18 seconds (kernel handling page faults) |
| CPU utilisation | ~250% of a possible 800% |

When `mmap` maps a 13GB file on a 16GB machine, there is almost no room for page cache. Every thread touching a different region of the file triggers **page faults** that block on disk I/O, and the kernel spends more time servicing those faults than the CPU spends processing data. This is visible in the timing output — the "Threads" phase dominates, and `time` reports massive system time.

### What is the Page Cache?

The OS never lets programs read directly from disk — it's far too slow. Instead, the kernel maintains a **page cache**: a region of RAM that holds recently-read chunks of files. When your program reads a file, the data is first copied from disk into the page cache, and then from the page cache into your program. If the same data is read again, it's served straight from RAM — no disk involved.

On a 32GB+ machine, the entire 13GB file fits in the page cache after the first read. Every subsequent access is a fast memory read. On a 16GB machine, the OS has ~3GB left over after the file mapping and system overhead — so it can only cache a small fraction of the file at a time, and must constantly evict old pages to make room for new ones.

### What is a Page Fault?

Memory is managed in **pages** (4KB chunks on most systems). When `mmap` maps a file, it doesn't actually load anything into RAM — it just sets up virtual address ranges. The first time your code touches a byte in a given page, the CPU raises a **page fault**: an interrupt that says *"this address isn't in physical RAM yet."*

The kernel then:
1. Suspends the thread that touched the address
2. Reads the 4KB page from disk (SSD) into physical RAM
3. Updates the page table so the virtual address now points to real memory
4. Resumes the thread

Each page fault takes **~10–100μs** depending on your SSD. That sounds tiny, but with 8 threads each reading different parts of a 13GB file, you trigger **millions of page faults**. The threads spend most of their time *suspended, waiting for disk I/O* rather than doing useful work. This is why `time` reports ~14–18s of **system time** (kernel handling faults) vs ~25s of **user time** (your actual code), and CPU utilisation sits at ~250% instead of the theoretical 800%.
