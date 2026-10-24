#pragma once
#include "head_attachment.h"
#include "bail_collision.h"
#include <cstdint>
namespace jishsfirstpersonmod {
using CameraMatrix=dingosdk::first_person::Matrix;
struct BailViewInfo {
    // 0: finished, 1: uncontrolled, 2: controllable recovery, 3: getting up.
    int phase{};bool blocked{},attached{},calibrated{};dingosdk::first_person::Vec3 offset{};
    dingosdk::first_person::Quat target_rotation{0,0,0,1};
    float recovery_yaw{},recovery_pitch{};
    bool getup_locked{};
    float getup_heading{};
    int getup_camera_phase{}; // 0 normal, 1 smoothed head follow, 2 blending, 3 reattached
    float getup_blend{};dingosdk::first_person::Vec3 head_target{};
    BailCollisionInfo collision;
};
class BailView {
    using Quat=dingosdk::first_person::Quat;using Vec3=dingosdk::first_person::Vec3;
    CameraMatrix last_native_{};
    Quat rotation_offset_{0,0,0,1};Vec3 offset_{};
    Quat displayed_{0,0,0,1},getup_heading_{0,0,0,1};Vec3 getup_tilt_{};
    bool displayed_ready_{},getting_up_{};double getup_started_{};float last_yaw_{};
    Vec3 displayed_position_{},getup_position_{},last_player_{};double position_time_{};
    Quat blend_rotation_{0,0,0,1};bool blending_{},reattached_{},player_ready_{},position_returned_{};double blend_started_{};
    bool ready_{},calibrated_{};unsigned wipeout_request_{};
    double time_{};
    int phase_{};BailViewInfo info_{};
    static Vec3 position(const CameraMatrix& pose) { return {pose[12],pose[13],pose[14]}; }
public:
    void reset() { *this={}; }
    BailViewInfo info() const { return info_; }
    // Clearing get-up flags is the normal animation-to-Ground handoff, not
    // evidence of a fresh tumble. Only a new validated wipeout request edge
    // may interrupt a held/blending get-up episode.
    bool observe_wipeout_request(bool verified,unsigned requested) {
        if(!verified) return false;
        const bool renewed=getup_episode() && (requested&~wipeout_request_)!=0;
        wipeout_request_=requested;return renewed;
    }
    void remember(const CameraMatrix& displayed,const CameraMatrix& native,const HeadPose& head,bool normal) {
        using namespace dingosdk::first_person;
        displayed_=orientation(displayed);displayed_position_=position(displayed);displayed_ready_=true;
        if(normal && !phase_ && head.valid) {
            const auto delta=subtract(position(native),position(head.head));
            if(length(delta)<=.4f) {
                const auto inverse=conjugate(orientation(head.head));
                offset_=rotate(inverse,delta);
                rotation_offset_=normalized(multiply(inverse,orientation(native)));calibrated_=true;
            }
        }
        info_.calibrated=calibrated_;info_.offset=offset_;
    }
    CameraMatrix attach(CameraMatrix target,const HeadPose& head) {
        using namespace dingosdk::first_person;
        info_.attached=head.valid && calibrated_;
        if(info_.attached) {
            const auto where=add(position(head.head),rotate(orientation(head.head),offset_));
            write(target,orientation(target),where);
        }
        info_.calibrated=calibrated_;info_.offset=offset_;return target;
    }
    CameraMatrix attach_full(CameraMatrix target,const HeadPose& head) const {
        using namespace dingosdk::first_person;
        if(head.valid && calibrated_) write(target,normalized(multiply(orientation(head.head),rotation_offset_)),
            add(position(head.head),rotate(orientation(head.head),offset_)));
        return target;
    }
    CameraMatrix apply(CameraMatrix native,const HeadPose& head,bool bail,bool /*normal*/,double now,bool controllable=false,bool getup_requested=false,const Vec3* player=nullptr,bool renewed=false) {
        using namespace dingosdk::first_person;
        if(!std::isfinite(now)) { reset();return native; }
        const bool discontinuity=ready_ && (now<time_ || now-time_>.25 || length(subtract(position(native),position(last_native_)))>5);
        if(discontinuity || (player && player_ready_ && length(subtract(*player,last_player_))>5)) reset();
        if(player) { last_player_=*player;player_ready_=true; }
        if(renewed) { getting_up_=blending_=reattached_=position_returned_=false; }
        ready_=true;time_=now;last_native_=native;
        if(!bail && !getting_up_) { reattached_=false;position_returned_=false; }
        if(getting_up_ && (!head.valid || !calibrated_ || !player)) { reset();return native; }
        if(bail && getup_requested && !getting_up_ && !reattached_ && player && head.valid && calibrated_) {
            getting_up_=true;getup_started_=now;
            getup_position_=displayed_ready_ ? displayed_position_:position(native);position_time_=now;
            const auto previous=displayed_ready_ ? displayed_:orientation(native);
            const auto back=rotate(previous,{0,0,1});
            // Near-vertical head poses have no stable heading; keep the last one.
            if(std::hypot(back[0],back[2])>.2f) last_yaw_=std::atan2(back[0],back[2]);
            getup_heading_={0,std::sin(last_yaw_/2),0,std::cos(last_yaw_/2)};
            getup_tilt_=rotation_vector(multiply(conjugate(getup_heading_),previous));
        }
        phase_=bail ? (controllable ? 2:getting_up_ ? 3:1):0;
        if(phase_) {
            auto output=orientation(native);
            auto where=position(native);
            const bool attached=head.valid && calibrated_;
            if(attached) {
                where=add(position(head.head),rotate(orientation(head.head),offset_));
                output=normalized(multiply(orientation(head.head),rotation_offset_));
            }
            if(!getting_up_ && !controllable && head.valid)
                output=head_camera_rotation(head);
            if(getting_up_ && !controllable && bail && head.valid && calibrated_) {
                where=getup_position_;
                const float decay=static_cast<float>(std::exp(-(now-getup_started_)/.12));
                auto tilt=getup_tilt_;for(auto& value:tilt) value*=decay;
                output=normalized(multiply(getup_heading_,from_rotation_vector(tilt)));
            }
            write(native,output,where);
            if(!getting_up_) { displayed_=output;displayed_position_=where;displayed_ready_=true; }
            const auto back=rotate(output,{0,0,1});
            if(!getting_up_ && std::hypot(back[0],back[2])>.2f) last_yaw_=std::atan2(back[0],back[2]);
            info_={phase_,phase_!=2,attached,calibrated_,offset_,output,0,0,getting_up_,last_yaw_*180/std::numbers::pi_v<float>};
        } else info_={0,false,false,calibrated_,offset_};
        return native;
    }
    // Called after look composition, before any ordinary handoff. This owns the
    // whole smoothed head-follow return, including frames after the bail latch clears.
    bool detached() const { return getting_up_; }
    // Rotation continues blending after position has returned. It must not
    // retain or reacquire the floor sphere during that remaining rotation.
    bool collision_owned(bool bail) const { return !position_returned_ && (bail || getting_up_); }
    double position_return_remaining(double now) const { return blending_ ? std::max(0.,.1-(now-blend_started_)):.1; }
    bool getup_episode() const { return getting_up_ || reattached_; }
    void accept_displayed_position(const Vec3& position) {
        displayed_position_=position;
        // Collision corrects the displayed camera, not the independent desired
        // get-up trajectory. Otherwise each correction pins the next target.
    }
    CameraMatrix finish(CameraMatrix target,const HeadPose& head,const Vec3* player,double now,bool control,const Vec3* normal_position=nullptr) {
        using namespace dingosdk::first_person;
        info_.head_target=head.valid && calibrated_ ? add(position(head.head),rotate(orientation(head.head),offset_)):position(target);
        if(getting_up_) {
            if(!player || !head.valid || !calibrated_) { reset();return target; }
            if(control && !blending_) {
                blending_=true;position_returned_=false;blend_started_=now;position_time_=now;getup_position_=displayed_position_;blend_rotation_=displayed_;
            }
            if(blending_ && !control) { getting_up_=blending_=position_returned_=false;return target; }
            const float dt=static_cast<float>(std::max(0.,now-position_time_));position_time_=now;
            // Follow the live calibrated head through stand-up. Once native
            // control returns, converge to the ordinary camera position so
            // releasing this override cannot jump between different targets.
            const auto position_target=control ? (normal_position ? *normal_position:position(target)):info_.head_target;
            const float t=blending_ ? std::clamp(float((now-blend_started_)/.3),0.f,1.f):0.f;
            const float weight=t*t*(3-2*t);
            const float position_t=blending_ ? std::clamp(float((now-blend_started_)/.1),0.f,1.f):0.f;
            const float position_weight=position_t*position_t*(3-2*position_t);
            const float tau=.05f*(1-position_weight);
            const float alpha=position_t>=1 ? 1:dt<=0 ? 0:tau<=.0001f ? 1:1-std::exp(-dt/tau);
            for(unsigned i=0;i<3;++i) getup_position_[i]+=(position_target[i]-getup_position_[i])*alpha;
            if(blending_ && position_t>=1) position_returned_=true;
            if(blending_) {
                auto angle=rotation_vector(multiply(conjugate(blend_rotation_),orientation(target)));
                for(auto& v:angle) v*=weight;
                write(target,normalized(multiply(blend_rotation_,from_rotation_vector(angle))),getup_position_);
                info_.getup_camera_phase=2;info_.getup_blend=t;info_.attached=false;
                if(t>=1) { getting_up_=blending_=false;reattached_=true;info_.getup_camera_phase=3;info_.attached=true; }
            } else {
                auto tilt=getup_tilt_;const float decay=static_cast<float>(std::exp(-(now-getup_started_)/.12));
                for(auto& value:tilt) value*=decay;
                write(target,normalized(multiply(getup_heading_,from_rotation_vector(tilt))),getup_position_);
                info_.getup_camera_phase=1;info_.attached=false;
            }
        } else if(reattached_) { info_.getup_camera_phase=3;info_.getup_blend=1; }
        info_.getup_locked=getting_up_;displayed_=orientation(target);displayed_position_=position(target);displayed_ready_=true;
        return target;
    }
};
}
