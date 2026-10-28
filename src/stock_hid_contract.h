#pragma once
#include "Engine/Game/Input/playstation_report.h"
#include <array>
#include <cstddef>
#include <cstdint>
namespace jishsfirstpersonmod {
// ReSkate 2.1.3 input_capture.cpp::InputCaptureHooks::HidRead. POD only.
struct StockHidRead { void* file{};void* buffer{};std::uint32_t capacity{};dingosdk::PlayStationPad kind{}; };
static_assert(sizeof(StockHidRead)==24 && offsetof(StockHidRead,buffer)==8 && offsetof(StockHidRead,capacity)==16 && offsetof(StockHidRead,kind)==20);
inline constexpr std::uintptr_t stock_hid_rva=0xf98990;
inline constexpr std::array<unsigned char,54> stock_hid_prefix{0x48,0x89,0x5c,0x24,0x8,0x55,0x56,0x57,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x50,0x8b,0xfa,0x48,0x8b,0xd9,0xe8,0x66,0xd9,0xff,0xff,0x8b,0xd7,0x48,0x8b,0xcb,0x84,0xc0,0x74,0xa,0xe8,0xb8,0x1c,0x0,0x0,0xe9,0x87,0x1,0x0,0x0,0xe8,0x3e,0xf9,0xff,0xff,0x45,0x33,0xff,0x44};
inline constexpr std::array<std::uintptr_t,3> stock_hid_callers{0xf97749,0xf97760,0xf98077};
template<class Read> bool verified_stock_hid(Read read,std::uintptr_t base) {
    std::array<unsigned char,54> actual{};
    if(!read(base+stock_hid_rva,actual.data(),actual.size()) || actual!=stock_hid_prefix) return false;
    for(auto caller:stock_hid_callers) {
        std::array<unsigned char,5> call{};if(!read(base+caller,call.data(),call.size()) || call[0]!=0xe8) return false;
        std::int32_t displacement{};std::memcpy(&displacement,call.data()+1,4);
        if(static_cast<std::intptr_t>(caller)+5+displacement!=stock_hid_rva) return false;
    }
    return true;
}
}
