#pragma once
#include "native_211.h"
namespace jishsfirstpersonmod::embedded211 {
inline constexpr native211::Entry draw{"draw",0xe537b0,"4c8bdc5553565741544157498d6ba14881ecf8000000410f2973b8450f294b88"};
inline constexpr native211::Entry skater_page{"skater_page",0xe54f80,"48895c24205556574883ec50488b052d1892004833c4488944244848c7442428"};
inline constexpr native211::Entry begin_card{"begin_card",0xe0b7d0,"48895c2408488974241048897c24184c8974242055488bec4883ec60803df592"};
inline constexpr native211::Entry end_card{"end_card",0xe10970,"4883ec28803d6d41a50000744af30f100dbbac9500488d4c2430f30f590d6244"};
inline constexpr native211::Entry field{"field",0xe10a60,"48895c24084889742410574883ec404d85c00f29742430488d1d678140000f29"};
inline constexpr native211::Entry note{"note",0xe11510,"40534883ec20488bd9ba96989eff33c9e80b6a1c000f57c0e8c36c1c0033d248"};
inline constexpr native211::Entry slider_float{"slider_float",0xe12de0,"488bc4488958184889682056574154415641574881ecc00000000f2970c80f29"};
inline constexpr native211::Entry toggle_row{"toggle_row",0xe13700,"488bc4555356574154415541564157488da8c8feffff4881ecf80100000f2970"};
inline constexpr native211::Entry context_getter{"context",0xfcc5b0,"488b0581069300c3"};
inline constexpr native211::Entry mem_alloc{"mem_alloc",0xfd12f0,"40534883ec20488b0583fe7900488bd9488b1539b99200ffd04c8b0d28b99200"};
inline constexpr native211::Entry mem_free{"mem_free",0xfd13a0,"4c8bc94885c9746d4c8b0589b892004d85c07461490fbf90c0270000498d88c4"};
inline constexpr native211::Entry entries[]{draw,skater_page,begin_card,end_card,field,note,slider_float,toggle_row,context_getter,mem_alloc,mem_free};
inline constexpr std::uintptr_t context_slot=0x18fcc38,alloc_slot=0x1771180,free_slot=0x1771188,userdata_slot=0x18fcc40;
inline constexpr std::size_t state_context=1448,state_menu=27664,state_window=704,menu_page=0,menu_tab=1884;
}
