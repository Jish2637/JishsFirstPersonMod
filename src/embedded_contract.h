#pragma once
#include "native_211.h"
namespace jishsfirstpersonmod::embedded211 {
inline constexpr native211::Entry draw{"draw",0x10a7820,"488bc44889582055565741564157488d68a14881ec000100000f2970c80f2978"};
inline constexpr native211::Entry skater_page{"skater_page",0x10a8700,"4c8bdc49895b205556574883ec60488b05ebd9b6004833c4488944245048c744"};
inline constexpr native211::Entry begin_card{"begin_card",0x1030110,"488bc4555356574154415541564157488da828ffffff4881ec980100000f2970"};
inline constexpr native211::Entry end_card{"end_card",0x10361a0,"48895c241048896c241856574154415641574881ec800000000f297424700f29"};
inline constexpr native211::Entry field{"field",0x1037020,"48895c24084889742410574883ec700f29742460488d1deaa15c000f297c2450"};
inline constexpr native211::Entry note{"note",0x10398b0,"40534883ec20488bd9ba96989eff33c9e82b7f2d000f57c0e8e3812d0033d248"};
inline constexpr native211::Entry slider_float{"slider_float",0x103d1a0,"488bc4535556574154415641574881ecc00000000f2970b80f2978a8440f2940"};
inline constexpr native211::Entry toggle_row{"toggle_row",0x103db60,"4c8bdc554157498dabd8feffff4881ec18020000488b058585bd004833c44889"};
inline constexpr native211::Entry context_getter{"context",0x1305d20,"488b0561fbbf00c3"};
inline constexpr native211::Entry mem_alloc{"mem_alloc",0x130abb0,"40534883ec20488b0583469000488bd9488b15c9acbf00ffd04c8b0db8acbf00"};
inline constexpr native211::Entry mem_free{"mem_free",0x130ac60,"4c8bc94885c9746d4c8b0519acbf004d85c07461490fbf90c0270000498d88c4"};
inline constexpr native211::Entry entries[]{draw,skater_page,begin_card,end_card,field,note,slider_float,toggle_row,context_getter,mem_alloc,mem_free};
inline constexpr std::uintptr_t context_slot=0x1f05888,alloc_slot=0x1c0f240,free_slot=0x1c0f248,userdata_slot=0x1f05890;
inline constexpr std::size_t state_context=2752,state_menu=29064,state_window=728,menu_page=0,menu_tab=1884;
}
