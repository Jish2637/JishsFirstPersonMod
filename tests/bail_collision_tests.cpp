#include "bail_collision.h"
#include "bail_collision_contract.h"
#include "bail_view.h"
#include "settings.h"
#include <iostream>
#include <fstream>
#include <limits>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path module_directory() { return std::filesystem::current_path(); }
std::filesystem::path data_directory() { return module_directory(); }
}
using namespace jishsfirstpersonmod;
void check(bool value,const char* message) { if(!value) { std::cerr<<message<<'\n';std::exit(1); } }
bool close_enough(float a,float b) { return std::abs(a-b)<.0001f; }
int main() {
    BailCollision collision;
    unsigned calls{};
    auto floor=[&](CollisionPoint from,CollisionPoint to,float radius) {
        ++calls;check(close_enough(radius,.11f),"Incorrect camera clearance");
        if(to[1]<radius && to[1]<from[1]) return CollisionSweep{true,std::clamp((from[1]-radius)/(from[1]-to[1]),0.f,1.f)};
        return CollisionSweep{true,1};
    };
    auto free=[](auto,auto,float) { return CollisionSweep{true,1}; };
    auto point=collision.resolve({0,1,0},{0,2,0},1,free);
    check(close_enough(point[1],1) && !collision.blocked(),"Unobstructed camera changed");
    point=collision.resolve({1,-1,1},{0,2,0},1.016,floor);
    check(point[1]>=.11f && close_enough(point[0],1) && close_enough(point[2],1),"Floor penetration or blocked lateral movement");
    for(int i=0;i<15;++i) {
        point=collision.resolve({1+float(i)*.01f,-1,1},{},1.032+i*.016,floor);
        check(point[1]>=.11f,"Repeated contact entered floor");
    }
    check(calls<90,"Unbounded per-frame queries");
    const auto held=point;
    point=collision.resolve({3,2,1},{},1.272,[](auto,auto,float) { return CollisionSweep{}; });
    check(point==held,"Failed query moved camera");
    point=collision.resolve({3,2,1},{},1.288,free);
    check(point[1]>held[1] && point[1]<2,"Collision release was not smoothed");
    for(int i=0;i<50;++i) point=collision.resolve({3,2,1},{},1.304+i*.016,free);
    check(!collision.blocked() && close_enough(point[1],2),"Return did not finish");
    collision.reset();
    auto corner=[](CollisionPoint from,CollisionPoint to,float radius) {
        float fraction=1;
        for(unsigned i=0;i<2;++i) if(to[i]<radius && to[i]<from[i]) fraction=std::min(fraction,std::clamp((from[i]-radius)/(from[i]-to[i]),0.f,1.f));
        return CollisionSweep{true,fraction};
    };
    point=collision.resolve({-1,-1,2},{1,1,0},3,corner);
    check(point[0]>=.11f && point[1]>=.11f && close_enough(point[2],2),"Corner penetration");
    collision.reset();
    point=collision.resolve({0,1,0},{0,2,0},4,[](auto,auto,float) { return CollisionSweep{true,std::numeric_limits<float>::quiet_NaN()}; });
    check(close_enough(point[1],2),"Invalid hit moved camera");
    point=collision.resolve({20,1,0},{20,2,0},5,free);
    check(close_enough(point[0],20) && close_enough(point[1],1),"Teleport retained collision history");
    using namespace dingosdk::first_person;
    HeadPose head;head.valid=true;head.head=head.root=Matrix{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    BailView bail;auto native=head.head;bail.remember(native,native,head,true);
    CollisionPoint player{};
    bail.apply(native,head,true,false,6,false,true,&player);
    bail.accept_displayed_position({0,.5f,0});
    const auto recovered=bail.finish(native,head,&player,6,false);
    check(close_enough(recovered[13],0),"Collision pinned the independent get-up target");
    // Reproduce an immediate floor hit in both direct and axis forward casts.
    collision.reset();unsigned reverse_calls{};const CollisionPoint standing{0,1.8f,0};
    auto stuck_floor=[&](CollisionPoint from,CollisionPoint to,float radius) {
        if(from[1]<=.111f) return CollisionSweep{true,0};
        if(to[1]<radius && to[1]<from[1]) {
            ++reverse_calls;return CollisionSweep{true,(from[1]-radius)/(from[1]-to[1])};
        }
        return CollisionSweep{true,1};
    };
    collision.resolve(standing,{0,.10f,0},10,stuck_floor,true,&standing);
    point=collision.resolve(standing,{},10.049,stuck_floor,true,&standing);
    check(reverse_calls==0 && close_enough(point[1],.1f),"Reverse escape ran before 50ms");
    point=collision.resolve(standing,{},10.051,stuck_floor,true,&standing);
    check(reverse_calls==1 && point[1]>.111f,"Stuck camera did not escape on the clear side");
    point=collision.resolve(standing,{},10.067,stuck_floor,true,&standing);
    check(point[1]>.112f && point[1]<standing[1],"Escape did not resume smoothed return");
    collision.reset();reverse_calls=0;
    for(int i=0;i<20;++i) collision.resolve(standing,{0,.1f,0},11+i*.016,stuck_floor,false,&standing);
    check(reverse_calls==0,"Reverse escape ran before normal control returned");
    collision.reset();
    auto failed_reverse=[](CollisionPoint from,auto,float) {
        return from[1]>.111f ? CollisionSweep{}:CollisionSweep{true,0};
    };
    collision.resolve(standing,{0,.1f,0},12,failed_reverse,true,&standing);
    point=collision.resolve(standing,{},12.16,failed_reverse,true,&standing);
    check(close_enough(point[1],.1f),"Failed reverse query moved the camera");
    // During a fall the live head must not wait for controllable recovery.
    collision.reset();reverse_calls=0;
    point=collision.resolve(standing,{0,.1f,0},12.3,stuck_floor,false,nullptr,-1,true);
    check(point==standing && !collision.blocked(),"Uncontrolled fall stayed stuck after its head cleared the floor");
    check(reverse_calls==1,"Head reattachment did not verify the reverse path");
    collision.reset();
    point=collision.resolve({0,.05f,0},{0,.12f,0},12.4,stuck_floor,false,nullptr,-1,true);
    check(point[1]>=.11f && point[1]>.05f,"Embedded head target bypassed collision");
    collision.reset();
    point=collision.resolve(standing,{0,.1f,0},12.5,failed_reverse,false,nullptr,-1,true);
    check(close_enough(point[1],.1f),"Invalid reverse cast reattached into an unverified target");
    collision.reset();
    collision.resolve(standing,{0,.1f,0},12.6,[](auto,auto,float){return CollisionSweep{true,0};},false,nullptr,-1,true);
    point=collision.resolve(standing,{},12.616,free,false,nullptr,-1,true);
    check(point==standing,"Clear head target retained collision-release lag");
    collision.reset();unsigned head_queries=0;
    auto wall=[&](CollisionPoint from,CollisionPoint to,float) {
        ++head_queries;
        if((from[0]<1 && to[0]>1) || (from[0]>1 && to[0]<1))
            return CollisionSweep{true,(1-from[0])/(to[0]-from[0])};
        return CollisionSweep{true,1};
    };
    point=collision.resolve({2,1,0},{0,1,0},12.64,wall,false,nullptr,-1,true);
    check(point==CollisionPoint{2,1,0} && head_queries<=5,"Camera remained on the old side of a wall after its head crossed");
    for(float invalid:{-1.f,2.f,std::numeric_limits<float>::quiet_NaN()}) {
        collision.reset();
        auto bad_reverse=[&](CollisionPoint from,auto,float) {
            return from[1]>.111f ? CollisionSweep{true,invalid}:CollisionSweep{true,0};
        };
        point=collision.resolve(standing,{0,.1f,0},12.66,bad_reverse,false,nullptr,-1,true);
        check(close_enough(point[1],.1f),"Malformed reverse fraction authorized head relocation");
    }
    // Rotation follows all live head axes even when collision holds position.
    BailView falling;HeadPose tilted=head;
    falling.remember(native,native,head,true);
    collision.reset();
    for(int i=0;i<12;++i) {
        write(tilted.head,from_rotation_vector({i*.31f,i*-.47f,i*.23f}),{0,.05f,0});
        auto camera=falling.apply(native,tilted,true,false,12.7+i*.016);
        const auto rotation=orientation(camera);
        auto safe=collision.resolve({camera[12],camera[13],camera[14]},{0,.12f,0},12.7+i*.016,stuck_floor,false,nullptr,-1,true);
        for(unsigned axis=0;axis<3;++axis) camera[12+axis]=safe[axis];
        check(length(rotation_vector(multiply(conjugate(rotation),orientation(camera))))<.00001f,
            "Collision changed camera rotation");
        const auto forward=rotate(orientation(camera),{0,0,-1});
        const auto face=rotate(orientation(tilted.head),{0,1,0});
        check(length(subtract(forward,face))<.00001f,"Falling camera did not face with the head");
    }

    // Controlled return is faster, but still sweeps and smooths every move.
    BailCollision slow,fast;
    const auto blocked=[](auto,auto,float) { return CollisionSweep{true,0}; };
    slow.resolve(standing,{0,.1f,0},13,blocked);
    fast.resolve(standing,{0,.1f,0},13,blocked);
    const auto slow_point=slow.resolve(standing,{},13.016,free);
    const auto fast_point=fast.resolve(standing,{},13.016,free,true,&standing);
    check(fast_point[1]>slow_point[1] && fast_point[1]<standing[1],"Controlled return was not faster and smooth");
    // End-to-end bail/recovery ownership: repeated zero hits cannot extend it.
    BailView getup;
    native[13]=head.head[13]=head.root[13]=1.8f;
    getup.remember(native,native,head,true);
    head.head[13]=.1f;
    auto output=getup.apply(native,head,true,false,20,false,true,&player);
    output=getup.finish(output,head,&player,20,false);
    getup.accept_displayed_position({0,.1f,0});
    output=getup.apply(native,head,false,true,20.01,true,false,&player);
    output=getup.finish(output,head,&player,20.01,true);
    check(getup.detached(),"Recovery transition not active");
    getup.accept_displayed_position({0,.1f,0});
    output=getup.apply(native,head,false,true,20.16,true,false,&player);
    output=getup.finish(output,head,&player,20.16,true);
    check(output[13]>.1f,"Desired recovery trajectory was pinned to floor");
    check(!getup.collision_owned(false) && getup.detached(),"Rotation retained collision after 100ms positional return");
    check(close_enough(output[13],1.8f),"Position did not settle within 100ms");
    check(!getup.collision_owned(true),"Lingering bail flag reacquired the released sphere");
    getup.accept_displayed_position({0,.1f,0});
    output=getup.apply(native,head,false,true,20.32,true,false,&player);
    output=getup.finish(output,head,&player,20.32,true);
    check(!getup.detached() && !BailCollision::owns_camera(false,getup.detached()),"Collision ownership outlived recovery");
    check(close_enough(output[13],1.8f),"Recovery did not return to ordinary camera");
    collision.reset();check(!collision.blocked(),"Finished recovery retained blocked history");
    check(BailCollision::owns_camera(true,false),"A fresh bail did not acquire collision");
    // Full position path: the head rises while uncontrolled, then walks away
    // during controlled release. Repeated immediate contacts must not leave a
    // large final jump when successful reverse recovery reaches the clear side.
    BailView moving;BailCollision moving_collision;
    head.head[12]=native[12]=0;head.head[13]=native[13]=1.8f;
    auto floor_display=native;floor_display[13]=.1f;
    moving.remember(floor_display,native,head,true);
    head.head[13]=.1f;
    output=moving.apply(native,head,true,false,30,false,true,&player);
    output=moving.finish(output,head,&player,30,false);
    moving.accept_displayed_position({0,.1f,0});
    head.head[13]=.8f;
    output=moving.apply(native,head,true,false,30.016,false,true,&player);
    output=moving.finish(output,head,&player,30.016,false);
    check(output[13]>.1f && output[13]<.8f,"Getting-up position did not follow rising live head");
    moving.accept_displayed_position({0,.1f,0});
    CollisionPoint last_displayed{0,.1f,0};
    for(int i=0;i<=7;++i) {
        const double now=30.032+i*.016;
        native[12]=head.head[12]=float(i)*.03f;head.head[13]=native[13]=1.8f;
        output=moving.apply(native,head,false,true,now,true,false,&player);
        const CollisionPoint normal{native[12],native[13],native[14]};
        output=moving.finish(output,head,&player,now,true,&normal);
        CollisionPoint desired{output[12],output[13],output[14]};
        if(moving.collision_owned(false)) {
            desired=moving_collision.resolve(desired,last_displayed,now,stuck_floor,true,&normal,moving.position_return_remaining(now));
            moving.accept_displayed_position(desired);
        } else {
            check(length(subtract(last_displayed,normal))<.06f,"Successful collision return snapped on positional handoff");
            moving_collision.reset();
        }
        last_displayed=desired;
    }
    check(moving.detached() && !moving.collision_owned(false),"Position and rotation release were not independent");
    check(close_enough(last_displayed[0],native[12]) && close_enough(last_displayed[1],1.8f),"Released camera stopped following moving normal target");
    moving.apply(native,head,true,false,30.160,false,true,&player,true);
    check(moving.collision_owned(true),"Repeated bail did not reacquire collision");
    // New bounded escape path, exercised without trusting a clear initial seed.
    CollisionEscape escape;escape.head={0,.02f,0};
    auto escape_step=[&](CollisionPoint desired,CollisionPoint seed,double time,auto query,bool recovering=false) {
        unsigned queries=0;
        const auto counted=[&](auto from,auto to,float radius) {++queries;return query(from,to,radius);};
        const auto result=collision.resolve(desired,seed,time,counted,recovering,nullptr,-1,false,&escape);
        check(collision.info().escape_queries<=6 && queries<=10,"Escape exceeded six additional native queries");
        return result;
    };
    auto occupied_floor=[](CollisionPoint from,CollisionPoint to,float radius) {
        if(from[1]<radius) return CollisionSweep{true,0};
        if(to[1]<radius) return CollisionSweep{true,(from[1]-radius)/(from[1]-to[1])};
        return CollisionSweep{true,1};
    };
    collision.reset();
    point=escape_step(escape.head,escape.head,40,occupied_floor);
    check(close_enough(point[1],.14f) && std::string_view(collision.info().escape_result)=="nearby",
        "Simultaneously embedded camera and head did not find the near-head escape");
    check(close_enough(collision.info().head_distance,.12f),"Escape head distance incorrect");
    // The same path applies while standing up, even before controllable input.
    collision.reset();point=escape_step(escape.head,escape.head,40.1,occupied_floor,true);
    check(close_enough(point[1],.14f),"Stand-up did not use bounded escape");
    // A farther candidate is searched only after the nearer candidates fail.
    collision.reset();escape.head={0,0,0};
    auto deep_surface=[](CollisionPoint from,CollisionPoint to,float) {
        return CollisionSweep{true,from[1]>.22f && to[1]>.22f ? 1.f:0.f};
    };
    for(int i=0;i<4;++i) point=escape_step(escape.head,{},41+i*.016,deep_surface);
    check(close_enough(point[1],.24f),"24 cm candidate was not reached with bounded scanning");
    // Candidate axes and centers must be rebuilt from the current live head.
    collision.reset();escape.head={1,0,0};escape.axes={CollisionPoint{1,0,0},CollisionPoint{0,1,0},CollisionPoint{0,0,1}};
    auto side_surface=[](CollisionPoint from,CollisionPoint to,float) {
        return CollisionSweep{true,from[0]>1.10f && to[0]>1.10f ? 1.f:0.f};
    };
    point=escape_step(escape.head,escape.head,42,side_surface);
    check(close_enough(point[0],1.12f) && close_enough(point[1],0),"Escape ignored head-relative axes");
    escape.head={1.04f,0,0};
    point=escape_step(escape.head,{},42.016,side_surface);
    check(close_enough(point[0],1.16f),"Escape candidate retained a stale head center");
    escape={};escape.head={0,.02f,0};
    // Total enclosure has an explicit deadline, including malformed queries.
    for(float fraction:{0.f,-1.f,2.f,std::numeric_limits<float>::quiet_NaN()}) {
        collision.reset();
        auto enclosure=[&](auto,auto,float){return CollisionSweep{true,fraction};};
        point=escape_step(escape.head,{0,.2f,0},43,enclosure);
        point=escape_step(escape.head,{},43.099,enclosure);
        check(close_enough(point[1],.2f),"Head fallback ran before its deadline");
        point=escape_step(escape.head,{},43.1,enclosure);
        check(point==escape.head && std::string_view(collision.info().fallback_reason)=="time_limit",
            "Enclosed camera missed the 100 ms head fallback");
    }
    collision.reset();
    auto unavailable=[](auto,auto,float){return CollisionSweep{};};
    escape_step(escape.head,{0,.2f,0},44,unavailable);
    point=escape_step(escape.head,{},44.1,unavailable);
    check(point==escape.head && std::string_view(collision.info().escape_result)=="none",
        "Invalid queries looked clear or prevented explicit clipping fallback");
    // Distance fallback requires persistent obstruction, not one hard-impact frame.
    collision.reset();escape.head={0,1,0};
    auto enclosed=[](auto,auto,float){return CollisionSweep{true,0};};
    point=escape_step(escape.head,{},45,enclosed);
    check(point!=escape.head,"Distance fallback ran on the initial contact");
    point=escape_step(escape.head,{},45.051,enclosed);
    check(point==escape.head && std::string_view(collision.info().fallback_reason)=="head_distance",
        "Persistent separation over 25 cm did not return to the head");
    // Tangential movement cannot keep restarting the obstruction clock.
    collision.reset();escape.head={0,.2f,0};
    auto sliding=[](CollisionPoint from,CollisionPoint to,float) {
        return CollisionSweep{true,from[1]==0 && to[1]==0 ? 1.f:0.f};
    };
    for(int i=0;i<=10;++i) {
        escape.head[0]=i*.01f;
        point=escape_step(escape.head,{},46+i*.01,sliding);
    }
    check(point==escape.head && collision.info().obstruction_seconds>=.099,
        "Sliding along an object indefinitely postponed fallback");
    // Passing two axes is insufficient: a thin obstruction on the third
    // rejects the candidate, and two complete candidates consume exactly six.
    collision.reset();escape={};escape.head={0,.2f,0};
    auto third_axis=[](CollisionPoint from,CollisionPoint to,float) {
        const auto delta=subtract(to,from);
        const bool short_probe=std::abs(length(delta)-.02f)<.0001f;
        return CollisionSweep{true,short_probe && std::abs(delta[2])<.001f ? 1.f:.5f};
    };
    escape_step(escape.head,{},46.2,third_axis);
    check(collision.info().escape_queries==6 && std::string_view(collision.info().escape_result)=="none",
        "A partially clear candidate was accepted or exceeded the search budget");
    // A moving object can engulf an unchanged camera position.
    collision.reset();escape.head={0,0,0};
    escape_step(escape.head,escape.head,47,free);
    escape_step(escape.head,escape.head,47.01,enclosed);
    point=escape_step(escape.head,escape.head,47.11,enclosed);
    check(std::string_view(collision.info().fallback_reason)=="time_limit",
        "Stationary camera overlap was not rechecked");
    point=escape_step(escape.head,{},47.126,free);
    check(point==escape.head && !collision.blocked() && collision.info().obstruction_seconds==0,
        "Camera did not resume normal collision after the object cleared");
    // Episode resets, invalid axes, observation gaps and teleports discard timers.
    collision.reset();escape.head={0,.2f,0};escape_step(escape.head,{},48,enclosed);
    collision.reset();escape_step(escape.head,{},48.1,enclosed);
    check(collision.info().obstruction_seconds==0,"Explicit reset retained obstruction timer");
    escape_step(escape.head,{},48.5,enclosed);
    check(collision.info().obstruction_seconds==0,"Observation gap retained obstruction timer");
    escape.head={20,.2f,0};escape_step(escape.head,{20,0,0},48.516,enclosed);
    check(collision.info().obstruction_seconds==0,"Teleport retained obstruction timer");
    escape.axes[0]={0,0,0};escape_step(escape.head,{},48.6,enclosed);
    check(collision.info().escape_queries==0,"Invalid head axes were queried");
    const auto path=std::filesystem::current_path()/L"bail-collision-test-data"/L"settings.ini";
    std::filesystem::create_directories(path.parent_path());
    { std::ofstream file(path);file<<"third_person_bailing=0\n"; }
    check(read_config(path).bail_camera_collision,"Old settings did not default collision on");
    SettingsStore store(path,Config{});Command command;command.kind=CommandKind::bail_collision;command.enabled=false;
    check(store.submit(command,100),"Collision toggle command failed");store.flush_due(100);
    if(store.snapshot().unsaved) std::cerr<<store.snapshot().feedback<<'\n';
    check(!store.snapshot().unsaved,"Collision settings save failed");
    check(!read_config(path).bail_camera_collision,"Collision setting did not persist");
    std::filesystem::remove(path);
    check(!bail_collision_contract::verified([](auto,void*,auto) { return false; },0),"Unsupported build accepted");
    std::cout<<"Collision resolver, native rejection, persistence and recovery passed\n";
}
