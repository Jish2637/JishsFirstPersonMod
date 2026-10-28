#include "bridge_resolver.h"
#include "owned_hooks.h"
#include <fstream>
#include <iostream>
#include <cstring>
namespace jishsfirstpersonmod { void log(const std::string&) noexcept {} }
using namespace jishsfirstpersonmod;
using namespace compatibility;
void check(bool value,const char* name) { if(!value) throw std::runtime_error(name); }
Image load(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);check(bool(file),"fixture missing");
    std::vector<unsigned char> raw((std::istreambuf_iterator<char>(file)),{});
    check(raw.size()>4096,"fixture too short");
    IMAGE_DOS_HEADER dos{};std::memcpy(&dos,raw.data(),sizeof(dos));
    check(dos.e_lfanew>0 && static_cast<std::size_t>(dos.e_lfanew)+sizeof(IMAGE_NT_HEADERS64)<raw.size(),"invalid fixture");
    IMAGE_NT_HEADERS64 nt{};std::memcpy(&nt,raw.data()+dos.e_lfanew,sizeof(nt));
    Image im;im.bytes.resize(nt.OptionalHeader.SizeOfImage);std::memcpy(im.bytes.data(),raw.data(),nt.OptionalHeader.SizeOfHeaders);
    const auto table=dos.e_lfanew+sizeof(DWORD)+sizeof(IMAGE_FILE_HEADER)+nt.FileHeader.SizeOfOptionalHeader;
    for(unsigned i=0;i<nt.FileHeader.NumberOfSections;++i) {
        IMAGE_SECTION_HEADER s{};std::memcpy(&s,raw.data()+table+i*sizeof(s),sizeof(s));
        check(s.PointerToRawData+s.SizeOfRawData<=raw.size() && s.VirtualAddress+s.SizeOfRawData<=im.bytes.size(),"invalid fixture section");
        std::memcpy(im.bytes.data()+s.VirtualAddress,raw.data()+s.PointerToRawData,s.SizeOfRawData);
    }
    std::string error;check(im.parse(error),error.c_str());return im;
}
std::array<unsigned char,32> hash_of(const char* hex) {
    std::array<unsigned char,32> hash{};
    for(unsigned i=0;i<32;++i) hash[i]=static_cast<unsigned char>(std::stoul(std::string(hex+i*2,2),nullptr,16));return hash;
}
int main(int argc,char** argv) {
    try {
        Image invalid;invalid.bytes.resize(4096);std::string error;check(!invalid.parse(error),"invalid PE accepted");
        check(!stock_contract::selected,"unverified profile selected by default");
        check(camera_readiness(0,0,10,0,20)==Readiness::waiting,"delayed initialization rejected");
        check(camera_readiness(1,10,10,20,20)==Readiness::ready,"ready slots rejected");
        check(camera_readiness(1,11,10,20,20)==Readiness::incompatible,"changed game target accepted");
        check(camera_readiness(2,10,10,20,20)==Readiness::incompatible,"invalid initialized flag accepted");
        for(unsigned failed_stage=0;failed_stage<8;++failed_stage) {
            dingosdk::OwnedHookLedger ledger;std::vector<void*> removed;
            for(unsigned stage=0;stage<failed_stage;++stage) ledger.remember(reinterpret_cast<void*>(static_cast<std::uintptr_t>(stage+1)));
            check(ledger.rollback([&](void* target) { removed.push_back(target);return dingosdk::HookOk; }),"rollback failed");
            check(removed.size()==failed_stage && ledger.size()==0,"rollback missed a completed stage");
            for(unsigned j=0;j<removed.size();++j) check(reinterpret_cast<std::uintptr_t>(removed[j])==failed_stage-j,"rollback order differs");
        }
        dingosdk::OwnedHookLedger retained;retained.remember(reinterpret_cast<void*>(1));
        check(!retained.rollback([](void*) { return dingosdk::HookProtectionFailed; }) && retained.size()==1,"failed detachment lost ownership");
        void* volatile slot=reinterpret_cast<void*>(2);
        check(dingosdk::restore_owned_slot(&slot,reinterpret_cast<void*>(2),reinterpret_cast<void*>(1)) && slot==reinterpret_cast<void*>(1),"owned callback did not restore");
        slot=reinterpret_cast<void*>(3);
        check(!dingosdk::restore_owned_slot(&slot,reinterpret_cast<void*>(2),reinterpret_cast<void*>(1)) && slot==reinterpret_cast<void*>(3),"foreign callback overwritten during rollback");
        if(argc!=2) { std::cout<<"Empty/invalid image guards passed; historical fixtures tested separately\n";return 0; }
        const std::filesystem::path root=argv[1];
        const std::filesystem::path paths[]{root/"history/ReSkate-1.1.5.dll",root/"history/ReSkate-1.1.6.dll",root/"ReSkate-1.1.7.dll",root/"release-1.1.8/ReSkate.dll",root/"ReSkate-2.0.0.dll",root/"ReSkate-2.0.1.dll",root/"ReSkate-2.0.2.dll",root/"ReSkate-2.0.3.dll",root/"ReSkate-2.0.4.dll",root/"ReSkate-2.0.5.dll",root/"ReSkate-2.0.6.dll",root/"ReSkate-2.0.7.dll",root/"ReSkate-2.0.8.dll"};
        for(unsigned i=0;i<std::size(paths);++i) {
            auto image=load(paths[i]);const auto& expected=stock_contract::contracts[i];
            const auto hash=hash_of(expected.sha256);const auto detected=resolve(image,hash,true);
            if(!detected) throw std::runtime_error(std::string(expected.version)+": "+detected.error);
            const auto& c=detected.contract;
            check(c.status_rva==expected.status_rva && c.state_pointer_rva==expected.state_pointer_rva && c.mutex_lock_rva==expected.mutex_lock_rva && c.mutex_unlock_rva==expected.mutex_unlock_rva && c.write_fov_rva==expected.write_fov_rva && c.source_initialized_rva==expected.source_initialized_rva && c.source_base_rva==expected.source_base_rva && c.source_transform_rva==expected.source_transform_rva,"detected profile differs");
            check(resolve(image,hash).known,"known profile path failed");
            auto unknown=hash;unknown[0]^=1;if(i<4) check(bool(resolve(image,unknown)) && !resolve(image,unknown).known,"unknown hash fallback failed");
            else check(!resolve(image,unknown),"unreviewed 2.0 DLL accepted");
            auto bad=image;bad.status=0;check(!resolve(bad,hash,true),"missing export accepted");
            bad=image;bad.bytes[image.status]=0x90;check(!resolve(bad,hash,true),"changed status form accepted");
            bad=image;const std::int32_t outside=static_cast<std::int32_t>(bad.bytes.size());
            std::memcpy(bad.bytes.data()+image.status+5,&outside,4);
            check(!resolve(bad,hash,true),"branch outside executable image accepted");
            // Duplicate a recognized FOV function into another executable position.
            bad=image;
            const auto section=std::find_if(bad.sections.begin(),bad.sections.end(),[](const auto& s) { return (s.flags&IMAGE_SCN_MEM_EXECUTE)!=0; });
            const auto duplicate=section->start+section->size-512;
            std::copy_n(image.bytes.begin()+c.write_fov_rva,160,bad.bytes.begin()+duplicate);
            // Keep the copied function's direct CRT call inside the image.
            std::int32_t call_offset{};std::memcpy(&call_offset,image.bytes.data()+c.write_fov_rva+0x15,4);
            call_offset=static_cast<std::int32_t>(static_cast<std::int64_t>(call_offset)+static_cast<std::int64_t>(c.write_fov_rva)-static_cast<std::int64_t>(duplicate));
            std::memcpy(bad.bytes.data()+duplicate+0x15,&call_offset,4);
            check(!resolve(bad,hash,true),"duplicate FOV candidate accepted");
            // The game setter's displacement is a required constant, not a relocation wildcard.
            bad=image;const unsigned char setter[]{0x48,0x8d,0x87,0x90,0xfa,0xf8,0x2};
            auto located=std::search(bad.bytes.begin(),bad.bytes.end(),std::begin(setter),std::end(setter));
            check(located!=bad.bytes.end(),"fixture initializer missing");located[3]^=0x10;
            check(!resolve(bad,hash,true),"changed native camera setter accepted");
            bad=image;const auto mdl=stock_contract::profile_model_offset(&expected);
            const unsigned char model[]{0x48,0x8d,0x8f,static_cast<unsigned char>(mdl),static_cast<unsigned char>(mdl>>8),0,0};
            for(auto it=bad.bytes.begin();it!=bad.bytes.end();) {
                it=std::search(it,bad.bytes.end(),std::begin(model),std::end(model));
                if(it==bad.bytes.end()) break;it[3]^=0x10;++it;
            }
            check(!resolve(bad,hash,true),"changed model offset accepted");
            bad=image;for(auto& s:bad.sections) if(c.write_fov_rva>=s.start && c.write_fov_rva<s.start+s.size) s.flags&=~IMAGE_SCN_MEM_EXECUTE;
            check(!resolve(bad,hash,true),"wrong code section accepted");
            check(!validate_callbacks(nullptr,detected.read_callback,detected.queue_callback),"foreign callbacks accepted before initialization");
            std::cout<<expected.version<<": compiled and forced automatic paths match all eight addresses\n";
        }
    } catch(const std::exception& e) { std::cerr<<e.what()<<"\n";return 1; }
}
