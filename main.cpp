#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include <iomanip>
#include <memory>

#include "itch_protocol.h"
#include "spsc_ring_buffer.h"
#include "feed_handler.h"

int main() {
    constexpr size_t kTotalTicks = 1'000'000;
    
    // Allocate 4 MB buffer on HEAP instead of Stack
    auto queue_ptr = std::make_unique<TickQueue>();
    TickQueue& queue = *queue_ptr;

    std::atomic<bool> running{true};
    std::vector<uint64_t> cycle_records;
    cycle_records.reserve(kTotalTicks / 4);

    std::cout << "Starting Modular High-Frequency ITCH 5.0 Pipeline Benchmark..." << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    std::thread consumer(consumer_alpha_engine, std::ref(queue), std::ref(running), std::ref(cycle_records));
    std::thread producer(producer_feed_handler, std::ref(queue), std::ref(running), kTotalTicks);

    producer.join();
    consumer.join();

    auto end_time = std::chrono::high_resolution_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    std::sort(cycle_records.begin(), cycle_records.end());
    uint64_t p50_cycles = cycle_records[cycle_records.size() * 0.50];
    uint64_t p90_cycles = cycle_records[cycle_records.size() * 0.90];
    uint64_t p99_cycles = cycle_records[cycle_records.size() * 0.99];

    constexpr double kGhz = 3.0; 

    std::cout << "\n==================================================\n";
    std::cout << "           BENCHMARK RESULTS & METRICS            \n";
    std::cout << "==================================================\n";
    std::cout << " Ticks Processed    : " << kTotalTicks << "\n";
    std::cout << " Total Runtime      : " << std::fixed << std::setprecision(2) << total_ms << " ms\n";
    std::cout << " Throughput         : " << static_cast<size_t>(kTotalTicks / (total_ms / 1000.0)) << " ticks/sec\n";
    std::cout << "--------------------------------------------------\n";
    std::cout << " Tick-to-Signal Latency Waterfall (CPU Cycles / ~ns):\n";
    std::cout << "   P50 (Median)     : " << p50_cycles << " cycles (" << static_cast<uint64_t>(p50_cycles / kGhz) << " ns)\n";
    std::cout << "   P90              : " << p90_cycles << " cycles (" << static_cast<uint64_t>(p90_cycles / kGhz) << " ns)\n";
    std::cout << "   P99              : " << p99_cycles << " cycles (" << static_cast<uint64_t>(p99_cycles / kGhz) << " ns)\n";
    std::cout << "==================================================\n";

    return 0;
}