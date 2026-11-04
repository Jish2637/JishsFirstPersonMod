#pragma once
#include "native_211.h"
namespace jishsfirstpersonmod::embedded211 {
inline constexpr native211::Entry draw{"draw",0x105bf60,"4c8bdc5553565741544157498d6ba14881ecf8000000410f2973b8450f294b88"};
inline constexpr native211::Entry skater_page{"skater_page",0x105cc40,"48895c24205556574883ec50488b052d22b6004833c4488944244848c7442428"};
inline constexpr native211::Entry begin_card{"begin_card",0x1009430,"48895c2408488974241048897c24184c8974242055488bec4883ec60803dbd07"};
inline constexpr native211::Entry end_card{"end_card",0x100f4d0,"4883ec28803d35a7d90000744af30f100d9b30ba00488d4c2430f30f590d4a24"};
inline constexpr native211::Entry field{"field",0x100f5c0,"48895c24084889742410574883ec404d85c00f29742430488d1d577c5a000f29"};
inline constexpr native211::Entry note{"note",0x10109b0,"40534883ec20488bd9ba96989eff33c9e8abb82b000f57c0e863bb2b0033d248"};
inline constexpr native211::Entry slider_float{"slider_float",0x1012600,"40555356574156488d6c24d94881ecf0000000488b0566c8ba004833c4488945"};
inline constexpr native211::Entry toggle_row{"toggle_row",0x1012d30,"488bc4555356574154415541564157488da8c8feffff4881ecf80100000f2970"};
inline constexpr native211::Entry context_getter{"context",0x12c0820,"488b054117b800c3"};
inline constexpr native211::Entry mem_alloc{"mem_alloc",0x12c5630,"40534883ec20488b0583298f00488bd9488b1529c9b700ffd04c8b0d18c9b700"};
inline constexpr native211::Entry mem_free{"mem_free",0x12c56e0,"4c8bc94885c9746d4c8b0579c8b7004d85c07461490fbf90c0270000498d88c4"};
inline constexpr native211::Entry entries[]{draw,skater_page,begin_card,end_card,field,note,slider_float,toggle_row,context_getter,mem_alloc,mem_free};
inline constexpr std::uintptr_t context_slot=0x1e41f68,alloc_slot=0x1bb7fc0,free_slot=0x1bb7fc8,userdata_slot=0x1e41f70;
inline constexpr std::size_t state_context=2752,state_menu=28968,state_window=728,menu_page=0,menu_tab=1884;
}
