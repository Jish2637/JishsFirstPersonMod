#include "native_head_look.h"
#include "board_look.h"
#include <iostream>
#include <stdexcept>
using namespace jishsfirstpersonmod;
void check(bool v,const char* why){if(!v)throw std::runtime_error(why);}
int main(){try {
    check(!native_head_look_passthrough(false,true,true,false,false,false,false),"default/off switch passed input");
    check(native_head_look_passthrough(true,true,true,false,false,false,false),"camera-only test did not pass physical input");
    check(!native_head_look_passthrough(true,false,true,false,false,false,false),"neutral-wait capture passed input");
    check(!native_head_look_passthrough(true,true,false,false,false,false,false),"disabled look passed input");
    check(!native_head_look_passthrough(true,true,true,false,true,true,false),"character-turning mode passed input");
    check(!native_head_look_passthrough(true,true,true,false,true,false,false),"temporarily ineligible character mode passed input");
    check(native_head_look_passthrough(true,true,true,true,true,false,false),"compatibility effective camera-only mode rejected");
    check(!native_head_look_passthrough(true,true,true,true,true,true,false),"active facing request passed input");
    check(!native_head_look_passthrough(true,true,true,false,false,false,true),"bail recovery passed input");
    // An empty stick policy preserves physical axes; the existing default masks
    // off-board axes, and board policies retain their modifier and held axes.
    StickAxes physical{.375f,-.625f};
    StickInputPolicy pass;
    auto axes=pass.captures(0) ? pass.axes:physical;
    check(axes.x==physical.x && axes.y==physical.y,"physical axes rescaled/changed");
    StickInputPolicy ordinary{~0u,{}};
    check(ordinary.captures(0) && ordinary.axes.x==0 && ordinary.axes.y==0,"off-mode baseline filtering changed");
    StickInputPolicy board{0x100,{.25f,-.5f}};
    check(board.captures(0x100) && !board.captures(0) && board.axes.x==.25f && board.axes.y==-.5f,"board policy changed");
    std::cout<<"Default-off, camera-only/compatibility, active aim, disabled/turning/recovery exclusions and physical-axis preservation PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
