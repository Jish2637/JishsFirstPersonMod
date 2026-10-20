#pragma once
#include <cstdint>
#include <cmath>
namespace jishsfirstpersonmod {
// The native tick can use its previous animation capture. Wait for a second
// distinct native frame, and reset every sample of the completion frame too:
// FrameSpring render samples share frame_start with the earlier tick sample.
class MarkerNativeReset {
    std::uint64_t generation_{};
    unsigned frames_{};
    double frame_{},completed_frame_{};
    bool have_frame_{},completed_{};
public:
    void observe(std::uint64_t generation,bool pending,bool teleport,double frame) {
        if(generation!=generation_) {*this={};generation_=generation;}
        if(!pending || teleport || !std::isfinite(frame)) return;
        if(!have_frame_ || frame!=frame_) {frame_=frame;have_frame_=true;++frames_;}
    }
    bool ready() const {return frames_>=2;}
    bool reset_sample(bool pending,double frame) const {return pending || (completed_ && frame==completed_frame_);}
    void completed(double frame) {completed_=true;completed_frame_=frame;}
};
}
