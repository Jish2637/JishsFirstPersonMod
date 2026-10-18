#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
namespace jishsfirstpersonmod::spectator {
// ReSkate 2.1.0 only. Official MSVC /MT release layout, checked against the
// shipped image by tools/audit_spectator_bridge.py. Never invoke its STL ABI.
inline constexpr std::uintptr_t party_getter=0xa73510,party_pointer=0x17a06f8;
inline constexpr std::uintptr_t remote_getter=0xa4ad20,remote_pointer=0x17a0560;
inline constexpr std::uintptr_t render_hook=0xa4af10,party_transform_return=0xbdba2f,party_fov_return=0xbdba4f;
inline constexpr std::string_view party_signature="4883ec288b0d1ec7e80065488b042558000000ba80ff0000488b04c88b04023905cbd1d2007f2b488b05bad1d2004883";
inline constexpr std::string_view remote_signature="48895c2410574883ec208b0d084feb0065488b042558000000488b3cc8b880ff00008b043839051d58d5007f34b8100000004869043878030000480305ff57d5";
inline constexpr std::string_view render_signature="48895c240848896c2410565741564883ec204c8bf2488bf1e8330400004c8b40";
inline constexpr std::string_view transform_signature="498b87200f0000488b542450498bceffd0f30f108c2490020000410f2fc876110f2ff1760c488b0d76aec400e8b15d0000";
inline constexpr std::uintptr_t selected_offset=0x3850,epoch_offset=0x3858,published_offset=0x10;
inline constexpr std::size_t players=250,record_stride=152,remote_stride=888,position_stride=56;
struct Identity {
    std::uint64_t id{},epoch{},generation{};
    std::uintptr_t entity{},component{},context{};std::size_t slot{};
    bool operator==(const Identity&) const=default;
};
struct Target { Identity identity;std::uintptr_t remote{},holder{},buffer{}; };
template<class Read> bool signature(Read read,std::uintptr_t address,std::string_view hex) {
    std::array<unsigned char,128> bytes{};
    if(hex.size()%2 || hex.size()/2>bytes.size() || !read(address,bytes.data(),hex.size()/2)) return false;
    auto nibble=[](char c) { return c<='9' ? c-'0':c-'a'+10; };
    for(std::size_t i=0;i<hex.size()/2;++i) if(bytes[i]!=(nibble(hex[2*i])*16+nibble(hex[2*i+1])))return false;
    return true;
}
template<class Read> bool verify_contract(Read read,std::uintptr_t stock) {
    return signature(read,stock+party_getter,party_signature) && signature(read,stock+remote_getter,remote_signature) &&
        signature(read,stock+render_hook,render_signature) && signature(read,stock+party_transform_return-17,transform_signature);
}
template<class Read> class Memory {
    Read read_;
public:
    explicit Memory(Read read):read_(read) {}
    template<class T> T get(std::uintptr_t base,std::uintptr_t offset=0) const {
        T out{};
        if(base<0x10000 || base>0x7ffffffeffffULL || offset>0x7ffffffeffffULL-base ||
            sizeof(T)>0x7ffffffeffffULL-base-offset || !read_(base+offset,&out,sizeof(out)))
            throw std::runtime_error("Watched player memory is unavailable.");
        return out;
    }
};
template<class Read> std::optional<Target> read_target(Read read,std::uintptr_t stock,std::uintptr_t context,
    std::uint64_t now,std::string& reason) {
    try {
        Memory memory(read);Target result;
        const auto party=memory.template get<std::uintptr_t>(stock,party_pointer);
        const auto id=memory.template get<std::uint64_t>(party,selected_offset);
        if(!id) { reason="ReSkate is not following a party player.";return {}; }
        const auto epoch=memory.template get<std::uint64_t>(party,epoch_offset);
        const auto published=memory.template get<std::uintptr_t>(party,published_offset);
        const auto records=memory.template get<std::array<unsigned char,players*record_stride>>(published,8);
        auto field=[&]<class T>(std::size_t i,std::size_t offset) { T out{};std::memcpy(&out,records.data()+i*record_stride+offset,sizeof(out));return out; };
        unsigned matches{};
        for(std::size_t i=0;i<players;++i) {
            if(field.template operator()<std::uint64_t>(i,0)!=id)continue;
            if(++matches>1)throw std::runtime_error("Watched player identity is ambiguous.");
            if(field.template operator()<unsigned char>(i,48) || !field.template operator()<unsigned char>(i,50) ||
                !field.template operator()<std::uint64_t>(i,88) || field.template operator()<std::uint64_t>(i,8)!=epoch)
                throw std::runtime_error("Watched player left or rejoined.");
            result.identity.id=id;result.identity.epoch=epoch;result.identity.slot=field.template operator()<std::size_t>(i,56);
        }
        if(matches!=1 || result.identity.slot>=players-1)throw std::runtime_error("Watched player slot is unavailable.");
        const auto position=party+192+result.identity.slot*position_stride;
        const auto sampled=memory.template get<std::uint64_t>(position,8);
        if(!memory.template get<unsigned char>(position) || sampled>now || now-sampled>250)
            throw std::runtime_error("Watched player pose is stale.");
        const auto remotes=memory.template get<std::uintptr_t>(stock,remote_pointer);
        if(remotes<0x10000 || remotes>0x7ffffffeffffULL-(players-1)*remote_stride)
            throw std::runtime_error("Remote actor storage is unavailable.");
        result.remote=remotes+result.identity.slot*remote_stride;
        result.identity.entity=memory.template get<std::uintptr_t>(result.remote);
        result.identity.component=memory.template get<std::uintptr_t>(result.remote,8);
        result.identity.context=memory.template get<std::uintptr_t>(result.remote,24);
        result.identity.generation=memory.template get<std::uint64_t>(result.remote,264);
        if(!context || result.identity.context!=context || !result.identity.entity || !result.identity.component || !result.identity.generation)
            throw std::runtime_error("Watched player actor is unavailable on this map.");
        result.holder=memory.template get<std::uintptr_t>(result.identity.component,0xa0);
        result.buffer=memory.template get<std::uintptr_t>(result.remote,256);
        if(!result.holder || !result.buffer)throw std::runtime_error("Watched player animation is unavailable.");
        if(memory.template get<std::uint64_t>(party,selected_offset)!=id || memory.template get<std::uint64_t>(party,epoch_offset)!=epoch ||
            memory.template get<std::uintptr_t>(party,published_offset)!=published)
            throw std::runtime_error("Watched player changed during capture.");
        reason.clear();return result;
    } catch(const std::exception& e) { reason=e.what();return {}; }
}
template<class Read> bool same_actor(Read read,const Target& target) {
    try {
        Memory memory(read);
        return memory.template get<std::uintptr_t>(target.remote)==target.identity.entity &&
            memory.template get<std::uintptr_t>(target.remote,8)==target.identity.component &&
            memory.template get<std::uintptr_t>(target.remote,24)==target.identity.context &&
            memory.template get<std::uint64_t>(target.remote,264)==target.identity.generation &&
            memory.template get<std::uintptr_t>(target.identity.component,0xa0)==target.holder;
    } catch(...) { return false; }
}
}
