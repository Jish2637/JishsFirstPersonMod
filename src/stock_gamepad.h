#pragma once
#include "native_211.h"
#include <cstddef>
#include <cstdint>
namespace jishsfirstpersonmod {
// ReSkate 2.1.2 copies 16 bytes plus one 32-bit word into the caller's buffer.
struct StockGamepad {
    std::uint16_t buttons{};
    std::uint8_t left_trigger{},right_trigger{};
    std::int16_t left_x{},left_y{},right_x{},right_y{};
    std::uint32_t state{},extra{};
};
static_assert(sizeof(StockGamepad)==20);
static_assert(offsetof(StockGamepad,buttons)==0);
static_assert(offsetof(StockGamepad,left_trigger)==2 && offsetof(StockGamepad,right_trigger)==3);
static_assert(offsetof(StockGamepad,left_x)==4 && offsetof(StockGamepad,left_y)==6);
static_assert(offsetof(StockGamepad,right_x)==8 && offsetof(StockGamepad,right_y)==10);
static_assert(offsetof(StockGamepad,state)==12 && offsetof(StockGamepad,extra)==16);
inline bool stock_gamepad_held(const StockGamepad& input) {
    const auto deflected=[](std::int16_t axis) {return axis>8192 || axis< -8192;};
    return input.buttons || input.left_trigger>63 || input.right_trigger>63 ||
        deflected(input.left_x) || deflected(input.left_y) ||
        deflected(input.right_x) || deflected(input.right_y);
}
inline void filter_recording_gamepad(StockGamepad& input,native211::RecordingCapture& capture,
                                     bool raw,bool recording) {
    if(capture.consume(raw,recording,stock_gamepad_held(input))) input={};
}
}
