#pragma once
#include "proxy_digest.h"
#include <string_view>
namespace jishsfirstpersonmod::updates {
inline constexpr std::string_view legacy_proxy_sha256="d77068ccd3d489a5649982c18de7d5e2fbd208fbb4f311382f05e81f1864e13d";
// Retain published multi-loader digests here when the bundled loader changes.
// Recognized loaders stay installed across First Person updates.
inline constexpr std::string_view published_multi_loader_sha256[]={"3117aad80316d4c8f3e9120b979d5a3bd26a6d47542f77d53fb16b0239d6ba1d",
    "438e671de5910e04e1af3ade92f3e72f14cbf12fb63df23676863794a43a0559"};
inline bool multi_mod_loader(std::string_view hash) {
    if(hash==bundled_proxy_sha256) return true;
    for(auto known:published_multi_loader_sha256) if(hash==known) return true;
    return false;
}
inline bool supported_update_loader(std::string_view hash) {
    return hash==legacy_proxy_sha256 || multi_mod_loader(hash);
}
}
