#pragma once
#include "native_211.h"
namespace jishsfirstpersonmod::embedded211 {
inline constexpr native211::Entry draw{"draw",0x1200ca0,"4c8bdc5553565741544157498d6ba14881ecf8000000410f2973b8450f294b88"};
inline constexpr native211::Entry skater_page{"skater_page",0x1201980,"48895c24205556574883ec60488b056d10bb004833c44889442450498be8488b"};
inline constexpr native211::Entry begin_card{"begin_card",0x10d95b0,"48895c2408488974241048897c24184c8974242055488bec4883ec60803ddded"};
inline constexpr native211::Entry end_card{"end_card",0x10df000,"4883ec28803da593e60000744af30f100dbb6fcc00488d4c2430f30f590dc2d1"};
inline constexpr native211::Entry field{"field",0x10df0f0,"48895c24084889742410574883ec40488bf20f297424304d85c00f297c242048"};
inline constexpr native211::Entry note{"note",0x10e0890,"40534883ec20488bd9ba96989eff33c9e8eb6d3a000f57c0e8a3703a0033d248"};
inline constexpr native211::Entry slider_float{"slider_float",0x10e23d0,"40555357488d6c24c94881ece0000000488b051906cd004833c448894527488b"};
inline constexpr native211::Entry toggle_row{"toggle_row",0x10e2990,"488bc44889581848895010488948085556574154415541564157488d68b14881"};
inline constexpr native211::Entry context_getter{"context",0x147bd60,"488b054963b600c3"};
inline constexpr native211::Entry mem_alloc{"mem_alloc",0x1480b30,"40534883ec20488b157b15b600488bd9ff1502b092004c8b0d6315b6004c8bc0"};
inline constexpr native211::Entry mem_free{"mem_free",0x1480be0,"4c8bc94885c9746d4c8b05c114b6004d85c07461490fbf90c0270000498d88c4"};
inline constexpr native211::Entry entries[]{draw,skater_page,begin_card,end_card,field,note,slider_float,toggle_row,context_getter,mem_alloc,mem_free};
inline constexpr std::uintptr_t context_slot=0x1fe20b0,alloc_slot=0x1dabb48,free_slot=0x1dabb50,userdata_slot=0x1fe20b8;
inline constexpr std::size_t state_context=2912,state_menu=29104,state_window=728,menu_page=0,menu_tab=1884;
}
