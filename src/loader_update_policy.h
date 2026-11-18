#pragma once
#include "proxy_digest.h"
#include <string_view>
namespace jishsfirstpersonmod::updates {
inline constexpr std::string_view legacy_proxy_sha256="d77068ccd3d489a5649982c18de7d5e2fbd208fbb4f311382f05e81f1864e13d";
// Retain published multi-loader digests here when the bundled loader changes.
// Recognized loaders stay installed across First Person updates.
inline constexpr std::string_view published_multi_loader_sha256[]={"3117aad80316d4c8f3e9120b979d5a3bd26a6d47542f77d53fb16b0239d6ba1d",
    "438e671de5910e04e1af3ade92f3e72f14cbf12fb63df23676863794a43a0559",
    "6a10539aeba7ea970eab5b07d86f03ae482571b008d3ed48e0f01e3f7aa2a4cf",
    "dea9da975df19c171c39f151ec6aed98b8d58dd903c8db43916036d0aa51f1c7",
    "7f5a0d8f2771480808b4f835fc7047bf234d452537610562b381447908ada428",
    "388ba4e79c2c108deabe67e24fcb57e731bbbbe04f1064a6f204e86df473585c",
    "afc50fa4647c02d8dd523dde19ecbba6be22f00daf5c3ac0fac39407c2b5868e",
    "5d7ae0cba2dc7280afe7d4a740d4cc19130d02392d2ec691a75b98127d800901",
    "8208090dc1b8de8eac07845e686b1a1afc04938784a4488509a8c1c3fd5259bc"};
inline bool multi_mod_loader(std::string_view hash) {
    if(hash==bundled_proxy_sha256) return true;
    for(auto known:published_multi_loader_sha256) if(hash==known) return true;
    return false;
}
inline bool supported_update_loader(std::string_view hash) {
    return hash==legacy_proxy_sha256 || multi_mod_loader(hash);
}
}
