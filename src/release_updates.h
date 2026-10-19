#pragma once
#include <filesystem>
#include <string>
#include <cstdint>
namespace jishsfirstpersonmod::release_updates {
struct Snapshot {
 std::string status="Update checker not started",installed,latest;
 std::uint64_t received{},total{};
 bool busy{},available{},ready{},armed{},automatic=true;
};
void start(const std::filesystem::path& companion,const std::filesystem::path& data);
Snapshot snapshot();void check();void download();void cancel();void later();void automatic(bool);
void install_after_exit();void open_folder();
}
