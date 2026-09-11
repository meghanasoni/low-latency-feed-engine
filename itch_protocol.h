#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct ITCHHeader {
    uint8_t message_type;
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6]; 
};

struct AddOrderMessage {
    ITCHHeader header;       
    uint64_t order_ref_num;
    char buy_sell_indicator; 
    uint32_t shares;
    char stock[8];
    uint32_t price;          
};
#pragma pack(pop)

struct alignas(64) AlphaTick {
    uint64_t ingress_cycles;
    double signed_volume;
    double price;
};