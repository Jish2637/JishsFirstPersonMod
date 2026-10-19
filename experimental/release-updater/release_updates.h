#pragma once
#include <string>
namespace jishsfirstpersonmod::release_updates {
struct Snapshot { std::string status, version, compatible_build; bool available{}, busy{}; };
Snapshot snapshot();
void check();
void download();
}
