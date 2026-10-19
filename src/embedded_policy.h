#pragma once
#include <cstdint>
namespace jishsfirstpersonmod {
struct EmbeddedVisibility {
    bool enabled{},failed{},menu{},focused{};int page{},tab{};
    std::uint64_t now{},last_draw{};
    bool page_visible() const {return enabled && !failed && menu && page==3 && tab==1;}
    bool capture(bool recording) const {return recording && focused && page_visible() && now-last_draw<250;}
};
}
