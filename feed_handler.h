#pragma once
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <iostream>
#include <x86intrin.h>

#include "itch_protocol.h"
#include "spsc_ring_buffer.h"
#include "alpha_engine.h"

using TickQueue = SPSCRingBuffer<AlphaTick, 65536>;

inline void producer_feed_handler(TickQueue& queue, std::atomic<bool>& running, size_t total_messages) {
    std::vector<uint8_t> binary_stream(sizeof(AddOrderMessage) * total_messages);
    uint8_t* ptr = binary_stream.data();

    for (size_t i = 0; i < total_messages; ++i) {
        AddOrderMessage msg{};
        msg.header.message_type = 'A';
        msg.order_ref_num = i;
        msg.buy_sell_indicator = (i % 2 == 0) ? 'B' : 'S';
        msg.shares = 100 + (i % 50) * 10;
        msg.price = 1500000 + (i % 100) * 100;
        std::memcpy(msg.stock, "AAPL    ", 8);

        std::memcpy(ptr, &msg, sizeof(AddOrderMessage));
        ptr += sizeof(AddOrderMessage);
    }

    const uint8_t* stream_ptr = binary_stream.data();
    for (size_t i = 0; i < total_messages; ++i) {
        const auto* itch_msg = reinterpret_cast<const AddOrderMessage*>(stream_ptr);

        if (itch_msg->header.message_type == 'A') {
            AlphaTick tick;
            tick.ingress_cycles = __rdtsc();
            tick.signed_volume = (itch_msg->buy_sell_indicator == 'B') ? itch_msg->shares : -static_cast<double>(itch_msg->shares);
            tick.price = itch_msg->price / 10000.0;

            while (!queue.push(tick)) {
                std::this_thread::yield();
            }
        }
        stream_ptr += sizeof(AddOrderMessage);
    }

    running.store(false);
}

inline void consumer_alpha_engine(TickQueue& queue, std::atomic<bool>& running, std::vector<uint64_t>& cycle_records) {
    double volume_buf[4] = {0};
    double price_buf[4] = {0};
    size_t buf_idx = 0;

    AlphaTick tick;
    while (true) {
        if (queue.pop(tick)) {
            volume_buf[buf_idx] = tick.signed_volume;
            price_buf[buf_idx] = tick.price;
            buf_idx++;

            if (buf_idx == 4) {
                double signal = VectorizedAlphaEngine::compute_order_flow_imbalance(volume_buf, price_buf);
                uint64_t egress_cycles = __rdtsc();

                if (signal == 0.0000001) std::cout << signal;

                if (egress_cycles > tick.ingress_cycles) {
                    cycle_records.push_back(egress_cycles - tick.ingress_cycles);
                }
                buf_idx = 0;
            }
        } else if (!running.load()) {
            // Drain remaining queue items before exiting
            while (queue.pop(tick)) {
                volume_buf[buf_idx] = tick.signed_volume;
                price_buf[buf_idx] = tick.price;
                buf_idx++;
                if (buf_idx == 4) {
                    double signal = VectorizedAlphaEngine::compute_order_flow_imbalance(volume_buf, price_buf);
                    uint64_t egress_cycles = __rdtsc();
                    if (egress_cycles > tick.ingress_cycles) {
                        cycle_records.push_back(egress_cycles - tick.ingress_cycles);
                    }
                    buf_idx = 0;
                }
            }
            break;
        } else {
            std::this_thread::yield();
        }
    }
}