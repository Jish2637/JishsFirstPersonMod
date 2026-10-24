#pragma once
#include <cstdint>
#include <optional>
#include <string_view>
#include <charconv>
namespace msc_presence {
inline bool enabled_setting(bool initialized,bool master,bool requested,bool spectator) {
    return (initialized ? master:requested) || spectator;
}
inline constexpr std::string_view prefix="-MSC-Midnight Skate Club-";
inline bool server_name(std::string_view name) {return name.starts_with(prefix);}
inline bool metadata_matches(std::string_view tags,std::uint64_t secret) {
    bool capability{},key{},reskate{};
    while(!tags.empty()) {
        const auto end=tags.find(',');const auto token=tags.substr(0,end);
        tags=end==std::string_view::npos ? std::string_view{}:tags.substr(end+1);
        if(token=="reskate")reskate=true;
        if(token=="jfp1")capability=true;
        if(token.starts_with("k")) {
            std::uint64_t value{};const auto t=token.substr(1);const auto parsed=std::from_chars(t.data(),t.data()+t.size(),value,16);
            key=parsed.ec==std::errc{} && parsed.ptr==t.data()+t.size() && value==secret;
        }
        if(token.starts_with("n"))break;
    }
    return reskate && capability && key && secret;
}
struct Session {std::uint64_t host{},epoch{},secret{},host_epoch{};bool operator==(const Session&) const=default;};
class Gate {
    Session session_{};
    bool confirmed_{},qualified_{},sent_{},value_{},name_allowed_{};
    std::uint64_t last_send_{};
public:
    void reset() {*this={};}
    void observe(Session session,bool authenticated,std::string_view name) {
        if(!session.host || !session.epoch) {reset();return;}
        if(session!=session_) {reset();session_=session;}
        name_allowed_=name.empty() || server_name(name);
        if(!name_allowed_) {confirmed_=qualified_=sent_=false;return;}
        qualified_=authenticated && server_name(name);
    }
    void capability(Session session,bool supported) {
        if(session==session_ && session_.host && name_allowed_)confirmed_=supported;
    }
    std::optional<bool> due(bool enabled,std::uint64_t now) const {
        if(!confirmed_ || !qualified_)return {};
        if(!sent_ || value_!=enabled || (enabled && now-last_send_>=5000))return enabled;
        return {};
    }
    void sent(bool enabled,std::uint64_t now) {sent_=true;value_=enabled;last_send_=now;}
    bool active() const {return confirmed_ && qualified_;}
    bool eligible() const {return qualified_;}
    Session session() const {return session_;}
};
inline bool dispatch_allowed(const Gate& gate,Session current,Session queued,std::string_view command,std::string_view queued_command) {
    return gate.active() && gate.session()==current && current==queued && !queued_command.empty() && command==queued_command;
}
}
