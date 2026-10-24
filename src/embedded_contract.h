#pragma once
#include "native_211.h"
namespace jishsfirstpersonmod::embedded211 {
inline constexpr native211::Entry draw{"draw",0xfa0ec0,"4c8bdc5553565741544157498d6ba14881ecf8000000410f2973b8450f294b88"};
inline constexpr native211::Entry skater_page{"skater_page",0xfa1ba0,"48895c24205556574883ec50488b054d56ae004833c4488944244848c7442428"};
inline constexpr native211::Entry begin_card{"begin_card",0xf56160,"48895c2408488974241048897c24184c8974242055488bec4883ec60803dfd25"};
inline constexpr native211::Entry end_card{"end_card",0xf5b6d0,"4883ec28803da5d0c90000744af30f100d2bf2b100488d4c2430f30f590d1202"};
inline constexpr native211::Entry field{"field",0xf5b7c0,"48895c24084889742410574883ec404d85c00f29742430488d1d276455000f29"};
inline constexpr native211::Entry note{"note",0xf5cbb0,"40534883ec20488bd9ba96989eff33c9e8db6a27000f57c0e8936d270033d248"};
inline constexpr native211::Entry slider_float{"slider_float",0xf5e890,"40555356574156488d6c24d94881ecf0000000488b055689b2004833c4488945"};
inline constexpr native211::Entry toggle_row{"toggle_row",0xf5efe0,"488bc4555356574154415541564157488da8c8feffff4881ecf80100000f2970"};
inline constexpr native211::Entry context_getter{"context",0x11c7cf0,"488b05498dac00c3"};
inline constexpr native211::Entry mem_alloc{"mem_alloc",0x11cca60,"40534883ec20488b05d3388b00488bd9488b15d13fac00ffd04c8b0dc03fac00"};
inline constexpr native211::Entry mem_free{"mem_free",0x11ccb10,"4c8bc94885c9746d4c8b05213fac004d85c07461490fbf90c0270000498d88c4"};
inline constexpr native211::Entry entries[]{draw,skater_page,begin_card,end_card,field,note,slider_float,toggle_row,context_getter,mem_alloc,mem_free};
inline constexpr std::uintptr_t context_slot=0x1c90a40,alloc_slot=0x1a80340,free_slot=0x1a80348,userdata_slot=0x1c90a48;
inline constexpr std::size_t state_context=1720,state_menu=27912,state_window=720,menu_page=0,menu_tab=1884;
}
