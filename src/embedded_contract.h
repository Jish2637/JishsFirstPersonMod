#pragma once
#include "native_211.h"
namespace jishsfirstpersonmod::embedded211 {
inline constexpr native211::Entry draw{"draw",0x104e0d0,"4c8bdc5553565741544157498d6ba14881ecf8000000410f2973b8450f294b88"};
inline constexpr native211::Entry skater_page{"skater_page",0x104edb0,"48895c24205556574883ec50488b053dc0b5004833c4488944244848c7442428"};
inline constexpr native211::Entry begin_card{"begin_card",0xffcbd0,"48895c2408488974241048897c24184c8974242055488bec4883ec60803dcd8c"};
inline constexpr native211::Entry end_card{"end_card",0x1002910,"4883ec28803da52fd90000744af30f100d0bbcb900488d4c2430f30f590d0ad0"};
inline constexpr native211::Entry field{"field",0x1002a00,"48895c24084889742410574883ec404d85c00f29742430488d1d17285a000f29"};
inline constexpr native211::Entry note{"note",0x1003df0,"40534883ec20488bd9ba96989eff33c9e8eb6d2b000f57c0e8a3702b0033d248"};
inline constexpr native211::Entry slider_float{"slider_float",0x1005a40,"40555356574156488d6c24d94881ecf0000000488b05a653ba004833c4488945"};
inline constexpr native211::Entry toggle_row{"toggle_row",0x1006170,"488bc4555356574154415541564157488da8c8feffff4881ecf80100000f2970"};
inline constexpr native211::Entry context_getter{"context",0x12af1a0,"488b0571eab700c3"};
inline constexpr native211::Entry mem_alloc{"mem_alloc",0x12b3fb0,"40534883ec20488b05b3ff8e00488bd9488b15599cb700ffd04c8b0d489cb700"};
inline constexpr native211::Entry mem_free{"mem_free",0x12b4060,"4c8bc94885c9746d4c8b05a99bb7004d85c07461490fbf90c0270000498d88c4"};
inline constexpr native211::Entry entries[]{draw,skater_page,begin_card,end_card,field,note,slider_float,toggle_row,context_getter,mem_alloc,mem_free};
inline constexpr std::uintptr_t context_slot=0x1e2dc18,alloc_slot=0x1ba3f70,free_slot=0x1ba3f78,userdata_slot=0x1e2dc20;
inline constexpr std::size_t state_context=2752,state_menu=28944,state_window=728,menu_page=0,menu_tab=1884;
}
