#pragma once
#include "msc_presence_policy.h"
#include <Windows.h>
#include <atomic>
#include <charconv>
#include <cstring>
namespace msc_presence {
// Steam's documented server-list API, using the same packed ABI as upstream
// steam_server_browser.cpp. No Steam initialization/callback pump is replaced.
class Discovery {
#pragma pack(push,4)
    struct Item {
        struct {std::uint16_t connection,query;std::uint32_t ip;} address;
        int ping;bool responded,skip;char directory[32],map[32],description[64];
        std::uint32_t app;int players,maximum,bots;bool password,secure;
        std::uint32_t played;int version;char name[64],tags[128];std::uint64_t id;
    };
#pragma pack(pop)
    static_assert(sizeof(Item)==372);
    struct Filter {char key[256],value[256];};
    class Response {
    public:
        virtual void ServerResponded(void*,int) {}
        virtual void ServerFailedToRespond(void*,int) {}
        virtual void RefreshComplete(void*,int) {done=true;}
        std::atomic<bool> done{};
    } response_;
    void* servers_{};void* handle_{};
    void* (*get_servers_)(){};
    void* (*request_)(void*,std::uint32_t,Filter**,std::uint32_t,Response*){};
    Item* (*details_)(void*,void*,int){};
    int (*count_)(void*,void*){};
    bool (*refreshing_)(void*,void*){};
    void (*release_)(void*,void*){};
    Session key_{};std::uint64_t started_{},next_{};bool supported_{};
    Filter filter_{"gametagsand","reskate,jfp1"};
    template<class T>static T symbol(HMODULE module,const char* name){return reinterpret_cast<T>(GetProcAddress(module,name));}
    bool open() {
        if(servers_)return true;
        const auto module=GetModuleHandleW(L"steam_api64.dll");if(!module)return false;
        get_servers_=symbol<decltype(get_servers_)>(module,"SteamAPI_SteamMatchmakingServers_v002");
        request_=symbol<decltype(request_)>(module,"SteamAPI_ISteamMatchmakingServers_RequestInternetServerList");
        details_=symbol<decltype(details_)>(module,"SteamAPI_ISteamMatchmakingServers_GetServerDetails");
        count_=symbol<decltype(count_)>(module,"SteamAPI_ISteamMatchmakingServers_GetServerCount");
        refreshing_=symbol<decltype(refreshing_)>(module,"SteamAPI_ISteamMatchmakingServers_IsRefreshing");
        release_=symbol<decltype(release_)>(module,"SteamAPI_ISteamMatchmakingServers_ReleaseRequest");
        if(!get_servers_ || !request_ || !details_ || !count_ || !refreshing_ || !release_)return false;
        servers_=get_servers_();return servers_!=nullptr;
    }
public:
    void reset() {
        if(handle_ && release_)release_(servers_,handle_);
        handle_=nullptr;key_={};supported_=false;next_=0;
    }
    bool update(Session key,std::uint64_t now) {
        if(key!=key_) {reset();key_=key;}
        // The capability belongs to this live authenticated server/session.
        // Do not compete with ReSkate's browser by polling the master list
        // after discovery. A reconnect or prefix loss invalidates it.
        if(supported_)return true;
        if(!key.host || !key.secret || !open())return false;
        if(!handle_ && now>=next_) {
            response_.done=false;Filter* filters=&filter_;
            handle_=request_(servers_,3354750,&filters,1,&response_);started_=now;next_=now+45000;
        }
        if(handle_) {
            const auto count=count_(servers_,handle_);
            if(count>=0 && count<=4096)for(int i=0;i<count;++i) {
                const auto* row=details_(servers_,handle_,i);
                if(!row || row->id!=key.host || row->app!=3354750)continue;
                std::size_t length{};while(length<sizeof(row->tags) && row->tags[length])++length;
                supported_=metadata_matches({row->tags,length},key.secret);
                if(supported_)break;
            }
            if(supported_ || response_.done || !refreshing_(servers_,handle_) || now-started_>=10000) {
                release_(servers_,handle_);handle_=nullptr;
            }
        }
        return supported_;
    }
};
}
