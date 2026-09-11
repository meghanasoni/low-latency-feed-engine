# Zero-Copy ITCH 5.0 Feed Handler & AVX-256 Alpha Engine

An end-to-end, ultra-low latency C++20 market data feed handler and real-time vectorized alpha calculation engine. Built for high-frequency trading infrastructure, this engine ingests synthetic NASDAQ ITCH 5.0 binary protocol streams and computes Order Flow Imbalance (OFI) signals with zero dynamic heap allocations on the execution hot path.

## Key Technical Features

* **Zero-Copy Ingestion:** Reinterpret-casts packed binary messages (`#pragma pack(push, 1)`) directly from memory streams, bypassing string parsing and heap allocations.
* **Lock-Free Concurrency:** Single-Producer Single-Consumer (SPSC) ring buffer with `std::atomic` acquire-release memory ordering.
* **Cache Line Padding:** Explicit 64-byte struct alignment (`alignas(64)`) to eliminate cross-core false sharing between consumer and producer threads.
* **SIMD Vectorization:** Parallelized signal calculation evaluating 4 double-precision price/volume ticks simultaneously using 256-bit AVX intrinsics (`__m256d`).
* **Cycle-Accurate Profiling:** Hardware-level profiling using the x86 `__rdtsc()` register to measure tick-to-signal cycle counts.

## System Architecture

```text
Binary ITCH 5.0 Stream ---> Lock-Free SPSC Ring Buffer ---> AVX-256 SIMD Alpha Engine ---> Microsecond Metrics