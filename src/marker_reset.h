#pragma once
#include <array>
#include <cstdint>
#include <cmath>
namespace jishsfirstpersonmod {
// Serialized by the stick runtime mutex; no game-memory access in this policy.
class MarkerReset {
public:
    using Owner=std::array<std::uintptr_t,4>;
    Owner owner{};
    std::uint64_t generation{};
    bool teleport{},pending{},neutral{};
private:
    bool positioned{};
    std::array<float,3> position{};
public:
    void clear() { const auto next=generation+1;*this={};generation=next; }
    bool observe(Owner current,bool flag,bool valid_position,std::array<float,3> point,bool continue_episode=false) {
        if(current!=owner) {
            const bool carry=pending && continue_episode;
            clear();owner=current;pending=carry;
        }
        if(!owner[0] || !owner[1] || !owner[2] || !owner[3]) return false;
        bool moved=false;
        if(valid_position) {
            if(positioned) {float distance=0;for(unsigned i=0;i<3;++i) distance+=(point[i]-position[i])*(point[i]-position[i]);moved=distance>25;}
            position=point;positioned=true;
        }
        const bool begin=(flag && !teleport) || (moved && !teleport && !pending);
        teleport=flag;
        if(begin) {++generation;pending=true;neutral=false;}
        return begin;
    }
    bool accept_native(Owner current,bool valid) {
        if(current!=owner || !valid || teleport || !pending) return false;
        pending=false;neutral=true;return true;
    }
    bool take_neutral(bool eligible,bool centred) {
        if(!neutral || !eligible || !centred) return false;
        neutral=false;return true;
    }
    bool permits(std::uint64_t token) const {return token==generation && !teleport && !pending && !neutral;}
};
}
