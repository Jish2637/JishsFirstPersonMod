#pragma once
#include "native_211.h"
namespace jishsfirstpersonmod::embedded211 {
inline constexpr native211::Entry draw{"draw",0x1021d00,"4c8bdc5553565741544157498d6ba14881ecf8000000410f2973b8450f294b88"};
inline constexpr native211::Entry skater_page{"skater_page",0x10229e0,"48895c24205556574883ec50488b05cd81b4004833c4488944244848c7442428"};
inline constexpr native211::Entry begin_card{"begin_card",0xfd0d40,"48895c2408488974241048897c24184c8974242055488bec4883ec60803d3d42"};
inline constexpr native211::Entry end_card{"end_card",0xfd6560,"4883ec28803d35ead10000744af30f100d7b7db800488d4c2430f30f590d4216"};
inline constexpr native211::Entry field{"field",0xfd6650,"48895c24084889742410574883ec404d85c00f29742430488d1d977559000f29"};
inline constexpr native211::Entry note{"note",0xfd7a40,"40534883ec20488bd9ba96989eff33c9e81bdd2a000f57c0e8d3df2a0033d248"};
inline constexpr native211::Entry slider_float{"slider_float",0xfd9690,"40555356574156488d6c24d94881ecf0000000488b051615b9004833c4488945"};
inline constexpr native211::Entry toggle_row{"toggle_row",0xfd9dc0,"488bc4555356574154415541564157488da8c8feffff4881ecf80100000f2970"};
inline constexpr native211::Entry context_getter{"context",0x1279dc0,"488b051135b100c3"};
inline constexpr native211::Entry mem_alloc{"mem_alloc",0x127eb30,"40534883ec20488b05e3518e00488bd9488b1599e7b000ffd04c8b0d88e7b000"};
inline constexpr native211::Entry mem_free{"mem_free",0x127ebe0,"4c8bc94885c9746d4c8b05e9e6b0004d85c07461490fbf90c0270000498d88c4"};
inline constexpr native211::Entry entries[]{draw,skater_page,begin_card,end_card,field,note,slider_float,toggle_row,context_getter,mem_alloc,mem_free};
inline constexpr std::uintptr_t context_slot=0x1d8d2d8,alloc_slot=0x1b63d20,free_slot=0x1b63d28,userdata_slot=0x1d8d2e0;
inline constexpr std::size_t state_context=2744,state_menu=28936,state_window=720,menu_page=0,menu_tab=1884;
}
