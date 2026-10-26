#include "stock_gamepad.h"
#include <Windows.h>
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace jishsfirstpersonmod;
void check(bool value,const char* why) {if(!value) throw std::runtime_error(why);}
int main() try {
    struct Guarded { StockGamepad pad;std::array<unsigned char,12> adjacent; } guarded;
    guarded.adjacent.fill(0xa5);
    native211::RecordingCapture capture;
    guarded.pad.buttons=1;
    filter_recording_gamepad(guarded.pad,capture,false,true);
    for(auto byte:guarded.adjacent) check(byte==0xa5,"capture overwrote caller memory after byte 20");
    StockGamepad neutral;
    check(std::memcmp(&guarded.pad,&neutral,sizeof(neutral))==0,"capture did not clear native input");
    for(int channel=0;channel<7;++channel) {
        StockGamepad input;
        switch(channel) {
        case 0:input.buttons=0x8000;break;
        case 1:input.left_trigger=255;break;
        case 2:input.right_trigger=255;break;
        case 3:input.left_x=-32768;break;
        case 4:input.left_y=32767;break;
        case 5:input.right_x=32767;break;
        case 6:input.right_y=-32768;break;
        }
        check(stock_gamepad_held(input),"native physical input not recognized");
        const auto physical=input;
        filter_recording_gamepad(input,capture,true,true);
        check(std::memcmp(&input,&physical,sizeof(input))==0,"raw binding sampler was suppressed");
        filter_recording_gamepad(input,capture,false,false);
        check(!stock_gamepad_held(input),"held input leaked after capture ended");
    }
    StockGamepad metadata;metadata.state=metadata.extra=0xffffffffu;
    check(!stock_gamepad_held(metadata),"metadata mistaken for held controls");
    filter_recording_gamepad(metadata,capture,false,false);
    check(metadata.state==0xffffffffu,"neutral release did not end suppression");
    metadata.buttons=1;filter_recording_gamepad(metadata,capture,false,false);
    check(metadata.buttons==1,"normal gameplay remained suppressed");
    // A caller may allocate exactly 20 bytes at the end of a committed page.
    SYSTEM_INFO info{};GetSystemInfo(&info);
    auto* pages=static_cast<unsigned char*>(VirtualAlloc(nullptr,info.dwPageSize*2,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    check(pages!=nullptr,"guard-page allocation failed");
    DWORD old{};check(VirtualProtect(pages+info.dwPageSize,info.dwPageSize,PAGE_NOACCESS,&old)!=0,"guard-page protection failed");
    auto* exact=new(pages+info.dwPageSize-sizeof(StockGamepad)) StockGamepad{};
    exact->buttons=1;filter_recording_gamepad(*exact,capture,false,true);
    check(exact->buttons==0,"exact-size capture failed");
    VirtualFree(pages,0,MEM_RELEASE);
    std::cout<<"20-byte ABI, adjacent canary/guard page, raw reads and release latch passed\n";
    return 0;
} catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
