<!--
SPDX-FileCopyrightText: 2026 lj970926

SPDX-License-Identifier: MIT
-->

# concurrency_book

`concurrency_book` is a small C++ concurrency playground for studying and
testing ideas from lock-free programming and the C++ memory model.

The code is intentionally compact. It is meant for learning and experimentation,
not as a production container or synchronization library.

## Requirements

- CMake 3.16 or newer
- A C++17-capable compiler
- A platform with standard C++ thread support

## Build

```bash
cmake -S . -B build
cmake --build build
```

The default build creates the shared library, unit-style test executables, and
the standalone memory-ordering examples under `scripts/`.

To configure without tests:

```bash
cmake -S . -B build -DCONCURRENCY_BOOK_BUILD_TESTS=OFF
cmake --build build
```

## Test

```bash
ctest --test-dir build --output-on-failure
```

Current tests cover:

- `spin_mutex_test`
- `lock_free_stack_test`
- `lock_free_stack_v2_test`
- `lock_free_stack_v3_test`
- `spsc_queue_test`
- `mpmc_queue_test`
- `thread_safe_stack_test`
- `parallel_quick_sort_test`

## Components

### SpinMutex

`include/spin_mutex.h` contains a minimal spin mutex based on
`std::atomic_flag`.

- `lock()` uses `std::memory_order_acquire`
- `unlock()` uses `std::memory_order_release`

### LockFreeStack

`include/lock_free_stack.h` contains a Treiber-stack-style example with deferred
node reclamation based on a count of threads currently inside `pop()`.

This version is useful for studying the basic idea of postponing deletion while
another thread may still be observing a removed node.

### LockFreeStackV2

`include/lock_free_stack_v2.h` contains a Treiber-stack-style implementation
that uses hazard pointers for node reclamation.

During `pop()`, the current head is first published through the calling thread's
hazard pointer. The stack then verifies that the head is still unchanged before
attempting the compare-exchange. This prevents another thread from deleting and
reusing the protected node address while it is still being inspected.

Removed nodes are either deleted immediately or placed on a pending reclamation
list until no hazard pointer references them.

Related files:

- `include/hp_manager.h`
- `src/hp_manager.cc`

### LockFreeStackV3

`include/lock_free_stack_v3.h` contains a Treiber-stack-style implementation
that reclaims nodes with a split reference-counting scheme.

Each node carries an internal atomic counter, and the head pointer carries an
external counter packed alongside the raw pointer. A popper first bumps the
external counter on the head to register interest, then races to swing the head
with compare-exchange. Losing threads release their claim through the internal
counter; the winning thread folds the remaining external claims into the
internal counter and deletes the node once both sides agree no other thread
still references it.

### SPSCQueue

`include/spsc_queue.h` contains a lock-free single-producer single-consumer
queue based on a sentinel (dummy) node.

`tail_` always points to an empty sentinel node. A `push()` writes data into
the current sentinel and appends a new sentinel before advancing `tail_`.
`pop()` reads `tail_` to detect whether a new node is available, then
reclaims the old head. The `tail_.store()` in `push()` synchronizes-with the
`tail_.load()` in `pop()`, ensuring the written data is visible to the
consumer before it is read.

`push()` and `pop()` must each be called from a single, dedicated thread.

### MPMCQueue

`include/mpmc_queue.h` contains a lock-free multi-producer multi-consumer queue
that extends the SPSC sentinel design with split reference counting for node
reclamation (Williams, *C++ Concurrency in Action*, Ch. 7).

Both `head_` and `tail_` hold a pointer packed with an external counter. A
thread first bumps the external counter to register interest, then races to
publish its work with compare-exchange: producers CAS the new value into the
sentinel's `data` slot, consumers CAS the head forward. Each node carries an
internal counter and a small count of how many external slots still point at
it; reclamation happens only after every external slot has been freed and the
internal counter has drained the in-flight references folded into it.

Unlike `SPSCQueue`, `push()` and `pop()` may be called from any number of
threads concurrently.

### ThreadSafeStack

`include/thread_safe_stack.h` contains a mutex-based thread-safe stack.

`push()` copies the value into a `std::shared_ptr` and stores it under a
`std::lock_guard`. `pop()` returns a `std::shared_ptr<T>` (or `nullptr` when
empty), which avoids the race between checking emptiness and using the popped
value — the caller holds a valid handle even if another thread concurrently
modifies the stack.

### ParallelQuickSort

`include/parallel_quick_sort.h` contains a parallel quicksort that partitions
work across a pool of worker threads.

The algorithm uses a Dutch-national-flag three-way partition (`< pivot`,
`== pivot`, `> pivot`) so that duplicate-heavy ranges degrade gracefully.
Sub-ranges smaller than a configurable threshold (`kMinChunkSize`) fall back to
`std::sort` to avoid per-element task overhead. The left sub-range is dispatched
as a pool task while the right sub-range is recursed on the current thread,
keeping the critical path short.

Related files:

- `include/thread_safe_stack.h` (used as the internal task queue)
- `tests/parallel_quick_sort_test.cpp`

## Memory-Ordering Examples

The `scripts/` directory contains standalone examples for observing C++ atomic
memory-ordering behavior:

- `relaxed_order_1.cc`: relaxed stores and loads across two atomic flags
- `relaxed_order_2.cc`: relaxed snapshots across several writer and reader threads
- `act_rel_trans.cc`: acquire-release synchronization through a handoff chain
- `seq_cst_order.cc`: sequential consistency with a single global order

After building, run them directly:

```bash
./build/scripts/relaxed_order_1
./build/scripts/relaxed_order_2
./build/scripts/act_rel_trans
./build/scripts/seq_cst_order
```

## License

This project is licensed under the MIT License. See `LICENSES/MIT.txt` for the
full license text.
