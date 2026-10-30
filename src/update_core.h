#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <array>
#include <cstdint>
namespace jishsfirstpersonmod::updates {
inline constexpr char supported_update_runtime_sha256[]="ed8f07863bcc3de1d83371de0f90b368ca55602d793d1bd84dbbff44b6d76801";
using Bytes=std::vector<unsigned char>;
inline constexpr std::size_t archive_limit=64*1024*1024,expanded_limit=128*1024*1024;
struct HttpsRequest {std::wstring host,target;};
HttpsRequest https_request(const std::wstring&);
struct Release {std::string version,tag,url,digest;std::uint64_t size{};};
std::array<unsigned,3> version(const std::string&);
std::string installed_version(const std::filesystem::path& root);
Release parse_release(const std::string& json);
bool newer(const Release&,const std::string& installed);
std::string sha256(const Bytes&);
Bytes read_file(const std::filesystem::path&,std::size_t limit=archive_limit);
void safe_path(const std::filesystem::path&);
void write_atomic(const std::filesystem::path&,const Bytes&);
void write_text(const std::filesystem::path&,const std::string&);
std::map<std::string,Bytes> unpack(const Bytes&,const Release&);
void install(const std::filesystem::path& root,const std::filesystem::path& job,const Release&,
             const std::string& expected_proxy,std::size_t fail_after=static_cast<std::size_t>(-1), bool preserve_proxy=false);
void recover(const std::filesystem::path& root,const std::filesystem::path& job);
}
