#include "marker_native_reset.h"
// Compile against the pinned OFFICIAL native math, not the legacy mod copy.
#include "reference/first_person_spring_211.h"
#include <iostream>
#include <stdexcept>
using namespace dingosdk::first_person;
using jishsfirstpersonmod::MarkerNativeReset;
void check(bool v,const char* why) {if(!v)throw std::runtime_error(why);}
float error(const Matrix& a,const Matrix& b) {
    const auto x=orientation(a),y=orientation(b);float dot=0;for(unsigned i=0;i<4;++i)dot+=x[i]*y[i];return 1-std::abs(dot);
}
int main() {try {
    static_assert(sizeof(Settings)==64);
    Matrix old{},head{};old[15]=head[15]=1;
    write(old,{0,0,0,1},{0,1.7f,0});write(head,{0,.70710678f,0,.70710678f},{0,1.7f,0});
    Settings saved;saved.enabled=true;saved.stabilize=true;saved.smoothing=100;saved.offset={.1f,.2f,.3f};saved.rotation={5,0,0};
    FrameSpring native;native.begin(1);native.sample(old,saved,{});
    native.begin(1.016);const auto stale=native.sample(head,saved,{});
    Settings temporary=saved;temporary.enabled=false;temporary.stabilize=false;temporary.follow_flips=false;
    const auto desired=target(head,temporary);
    check(error(stale,desired)>.01f,"fixture did not reproduce retained native heading");
    MarkerNativeReset reset;reset.observe(4,true,true,1.016);check(!reset.ready(),"teleport counted as fresh frame");
    reset.observe(4,true,false,1.032);check(!reset.ready(),"first tick accepted previous animation capture");
    native.begin(1.032);
    check(reset.reset_sample(true,1.032),"native sample was not reset");
    native.sample(old,temporary,{}); // tick still has old capture
    const auto render=native.sample(head,temporary,{}); // same frame_start!
    check(error(render,desired)<.00001f,"same-frame render reintroduced previous transform");
    reset.observe(4,true,false,1.032);check(!reset.ready(),"render counted as another frame");
    reset.observe(4,true,false,1.048);check(reset.ready(),"second native frame not ready");
    native.begin(1.048);const auto fresh=native.sample(head,temporary,{});reset.completed(1.048);
    check(error(fresh,desired)<.00001f,"native restart did not attach to new head");
    check(reset.reset_sample(false,1.048),"completion-frame render reused old frame_start");
    check(!reset.reset_sample(false,1.064),"native preferences not restored next frame");
    native.begin(1.064);const auto restored=native.sample(head,saved,{});
    check(error(restored,desired)<.00001f,"saved smoothing restored stale native heading");
    for(bool remembered:{false,true}) for(bool active:{false,true}) {
        const bool keep=remembered && active;
        Settings glance;glance.rotation[1]=keep ? 60.f:0.f;
        const auto displayed=target(fresh,glance);
        check((error(displayed,fresh)>.01f)==keep,"inactive/disabled glance retained last transform");
        check(std::abs(displayed[12]-fresh[12])<.00001f,"glance retained old world position");
    }
    reset.observe(5,true,false,2);check(!reset.ready(),"new marker reused previous fresh-frame confirmation");
    std::cout<<"Official native stale spring reproduction, first-frame capture quarantine, tick/render resets, preference restoration and active-only relative glance PASS\n";
    return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}}
