#pragma once
#include "Engine/Game/Skater/first_person_spring.h"
#include <optional>
#include <algorithm>
#include <cmath>
namespace jishsfirstpersonmod {
using CollisionPoint=dingosdk::first_person::Vec3;
struct CollisionSweep { bool valid{};float fraction{1}; };
struct CollisionEscape {
    CollisionPoint head{};
    // Physical head up, right, forward in world space.
    std::array<CollisionPoint,3> axes{{{0,1,0},{1,0,0},{0,0,-1}}};
};
struct BailCollisionInfo {
    double obstruction_seconds{};float head_distance{};
    unsigned escape_attempts{},escape_queries{};
    bool query_valid{};float initial_fraction=1;
    const char* escape_result="none";
    const char* fallback_reason="none";
};
// Native sweeps protect the camera sphere. During uncontrolled falls a clear
// live head target can replace trapped history, without changing head rotation.
class BailCollision {
    CollisionPoint safe_{},stall_anchor_{};double time_{},stall_since_{};
    bool ready_{},blocked_{},stall_tracking_{};
    bool obstructed_{};double obstruction_since_{};unsigned escape_cursor_{};
    BailCollisionInfo info_{};
public:
    static constexpr float radius=.10f,clearance=.01f;
    void reset() { *this={}; }
    bool blocked() const { return blocked_; }
    BailCollisionInfo info() const { return info_; }
    static bool owns_camera(bool bail,bool recovery) { return bail || recovery; }
    template<class Query> CollisionPoint resolve(CollisionPoint desired,CollisionPoint seed,double now,Query query,
                                                bool recovering=false,const CollisionPoint* normal_target=nullptr,double return_remaining=-1,bool head_locked=false,const CollisionEscape* escape=nullptr) {
        using namespace dingosdk::first_person;
        const auto scaled=[](CollisionPoint point,float factor) { for(auto& value:point) value*=factor;return point; };
        if(!std::isfinite(now)) { reset();return desired; }
        if(ready_ && (now<time_ || now-time_>.25 || length(subtract(desired,safe_))>5)) reset();
        if(!ready_) { safe_=seed;time_=now;ready_=true; }
        const float dt=static_cast<float>(std::clamp(now-time_,0.,.05));time_=now;
        info_={};
        if(escape) {
            bool valid=true;
            for(auto value:escape->head) valid=valid && std::isfinite(value);
            for(const auto& axis:escape->axes) {
                for(auto value:axis) valid=valid && std::isfinite(value);
                const float norm=length(axis);valid=valid && norm>.9f && norm<1.1f;
            }
            if(!valid) escape=nullptr;
        }
        if(!escape) { obstructed_=false;obstruction_since_=0;escape_cursor_=0; }
        const auto clear_obstruction=[&]() {obstructed_=false;obstruction_since_=0;escape_cursor_=0;};
        const auto rescue=[&]() {
            if(!obstructed_) {obstructed_=true;obstruction_since_=now;}
            info_.obstruction_seconds=std::max(0.,now-obstruction_since_);
            info_.head_distance=length(subtract(safe_,escape->head));
            // Always retry the current head, then scan live head-relative
            // candidates. No candidate or clearance result survives a frame.
            while(info_.escape_queries<=3) {
                const bool at_head=info_.escape_attempts==0;
                auto candidate=escape->head;
                if(!at_head) {
                    const unsigned index=escape_cursor_++%12;
                    const float distance=index<6 ? .12f:.24f;
                    candidate=add(candidate,scaled(escape->axes[(index%6)/2],index%2 ? -distance:distance));
                }
                ++info_.escape_attempts;
                bool clear=true;
                for(const auto& axis:escape->axes) {
                    ++info_.escape_queries;
                    const auto probe=query(subtract(candidate,scaled(axis,.01f)),add(candidate,scaled(axis,.01f)),radius+clearance);
                    if(!probe.valid || !std::isfinite(probe.fraction) || probe.fraction!=1.f) {clear=false;break;}
                }
                if(clear) {
                    safe_=candidate;blocked_=!at_head;stall_tracking_=false;
                    info_.escape_result=at_head ? "head":"nearby";
                    info_.head_distance=length(subtract(safe_,escape->head));
                    clear_obstruction();return safe_;
                }
            }
            // Staying with the player takes priority over indefinite collision
            // separation. This explicit last resort can briefly clip geometry.
            const bool distant=info_.obstruction_seconds>=.05 && info_.head_distance>.25f;
            if(distant || info_.obstruction_seconds>=.1-1e-9) {
                safe_=escape->head;blocked_=true;stall_tracking_=false;
                info_.fallback_reason=distant ? "head_distance":"time_limit";
                info_.head_distance=0;
            }
            return safe_;
        };
        const auto sweep=[&](CollisionPoint from,CollisionPoint to)->std::optional<CollisionPoint> {
            const auto delta=subtract(to,from);const float distance=length(delta);
            if(distance<.00001f) return from;
            const auto result=query(from,to,radius+clearance);
            if(!result.valid || !std::isfinite(result.fraction) || result.fraction<0 || result.fraction>1) return {};
            const float fraction=result.fraction<1 ? std::max(0.f,result.fraction-.002f/distance):1.f;
            return add(from,scaled(delta,fraction));
        };
        const auto recover_stall=[&]() {
            if(!recovering || !normal_target || length(subtract(*normal_target,safe_))<.10f) {
                stall_tracking_=false;return safe_;
            }
            if(!stall_tracking_ || length(subtract(safe_,stall_anchor_))>.002f) {
                stall_tracking_=true;stall_anchor_=safe_;stall_since_=now;return safe_;
            }
            if(now-stall_since_<.050) return safe_;
            // The ordinary controlled camera target is assumed clear. Query
            // toward the trapped position, stopping on the target's clear side.
            // Never interpolate through the embedded part of that segment.
            if(const auto recovered=sweep(*normal_target,safe_)) {
                safe_=length(subtract(*recovered,safe_))>.002f ? *recovered:*normal_target;
                stall_tracking_=false;blocked_=true;
            }
            return safe_;
        };
        // The direct sweep below records native immediate-contact evidence.
        const auto direct=[&]()->std::optional<CollisionPoint> {
            const auto delta=subtract(desired,safe_);const float distance=length(delta);
            if(distance<.00001f) return safe_;
            const auto hit=query(safe_,desired,radius+clearance);
            info_.query_valid=hit.valid && std::isfinite(hit.fraction) && hit.fraction>=0 && hit.fraction<=1;
            info_.initial_fraction=hit.fraction;
            if(!info_.query_valid) return {};
            const float fraction=hit.fraction<1 ? std::max(0.f,hit.fraction-.002f/distance):1.f;
            return add(safe_,scaled(delta,fraction));
        }();
        if(!direct) { blocked_=true;stall_tracking_=false;return escape ? rescue():safe_; }
        bool stationary_blocked=false;
        if(escape && length(subtract(safe_,desired))<.00001f) {
            const auto& axis=escape->axes[0];
            const auto probe=query(subtract(safe_,scaled(axis,.01f)),add(safe_,scaled(axis,.01f)),radius+clearance);
            info_.query_valid=probe.valid && std::isfinite(probe.fraction) && probe.fraction>=0 && probe.fraction<=1;
            info_.initial_fraction=probe.fraction;
            stationary_blocked=!info_.query_valid || probe.fraction<1;
        }
        const bool obstructed=stationary_blocked || length(subtract(*direct,desired))>.0001f;
        if(obstructed) {
            safe_=*direct;
            // Axis sweeps allow movement along floors/walls without needing an
            // unverified native hit-normal layout. Every intermediate move is swept.
            for(unsigned axis=0;axis<3;++axis) {
                auto slide=safe_;slide[axis]=desired[axis];
                if(const auto checked=sweep(safe_,slide)) safe_=*checked;
            }
            blocked_=true;
            if(escape) return rescue();
            if(head_locked) {
                const float distance=length(subtract(desired,safe_));
                if(distance>.02f) {
                    // A reverse sweep starts at the live head target. Positive
                    // clear travel proves that sphere is outside the surface;
                    // reattach there instead of keeping the old camera behind
                    // a wall the ragdoll has already passed. Zero/invalid hits
                    // never authorize relocation into an embedded head target.
                    const auto reverse=query(desired,safe_,radius+clearance);
                    if(reverse.valid && std::isfinite(reverse.fraction) && reverse.fraction<=1 &&
                       reverse.fraction*distance>.002f) {
                        safe_=desired;blocked_=stall_tracking_=false;
                    }
                }
                return safe_;
            }
            return recover_stall();
        }
        clear_obstruction();
        if(escape) info_.head_distance=length(subtract(desired,escape->head));
        auto next=desired;
        if(!blocked_ || head_locked) { safe_=desired;blocked_=stall_tracking_=false;return safe_; }
        if(blocked_) {
            // Tighten the tail as the 100ms position handoff approaches, so a
            // successful return does not leave a lag for the release to snap.
            const float tau=recovering && return_remaining>=0 ? std::min(.03f,std::max(.001f,float(return_remaining)*.3f)):
                recovering ? .03f:.08f;
            const float alpha=dt>0 ? 1-std::exp(-dt/tau):0;
            next=add(safe_,scaled(subtract(desired,safe_),alpha));
        }
        if(const auto checked=sweep(safe_,next)) safe_=*checked;
        if(length(subtract(safe_,desired))<.001f) blocked_=false;
        return recover_stall();
    }
};
void initialize_bail_collision(std::uintptr_t base) noexcept;
void observe_collision_engine_thread() noexcept;
bool bail_collision_available() noexcept;
std::uintptr_t bail_collision_world() noexcept;
CollisionSweep native_bail_sweep(std::uintptr_t world,CollisionPoint from,CollisionPoint to,float radius) noexcept;
}
