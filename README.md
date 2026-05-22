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
