#include "bridge_resolver.h"
#include "bridge_patterns.h"
#include "native_210.h"
#include "Engine/Game/Build/image_identity.h"
#include "Engine/Core/Platform/memory.h"
#include "detours.h"
#include <atomic>
#include <cstring>
#include <set>
#include <stdexcept>
namespace jishsfirstpersonmod::compatibility {
namespace {
std::atomic<bool> activated{};
Resolution selected;
std::string digest_text;
HMODULE selected_module{};
constexpr unsigned executable=IMAGE_SCN_MEM_EXECUTE, writable=IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE;
template<class T> T read(const Image& image,std::uintptr_t at) {
    if(at>image.bytes.size() || sizeof(T)>image.bytes.size()-at) throw std::runtime_error("reference outside image");
    T value{};std::memcpy(&value,image.bytes.data()+at,sizeof(T));return value;
}
unsigned instruction_size(const unsigned char* source) {
    unsigned char data[32]{};std::memcpy(data,source,16);
    // Do not let Detours dereference an untrusted indirect branch's address slot.
    if(data[0]==0xff && (data[1]==0x15 || data[1]==0x25)) return 6;
    __try {
        auto* next=static_cast<unsigned char*>(DetourCopyInstruction(nullptr,nullptr,data,nullptr,nullptr));
        const auto size=next-data;return size>0 && size<=15 ? static_cast<unsigned>(size):0;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return 0; }
}
std::vector<std::int64_t> branches(const Image& image,const Pattern& p,std::uintptr_t at) {
    std::vector<std::int64_t> result;
    for(unsigned j=0;j<p.count;++j) {
        const auto pos=at+p.boundaries[j];const auto opcode=image.bytes[pos];
        const auto length=p.boundaries[j+1]-p.boundaries[j];
        if((opcode==0xe8 || opcode==0xe9) && length==5) result.push_back(static_cast<std::int64_t>(pos+5)+read<std::int32_t>(image,pos+1));
        else if((opcode==0xeb || (opcode>=0x70 && opcode<=0x7f)) && length==2) result.push_back(static_cast<std::int64_t>(pos+2)+read<std::int8_t>(image,pos+1));
        else if(opcode==0x0f && image.bytes[pos+1]>=0x80 && image.bytes[pos+1]<=0x8f && length==6) result.push_back(static_cast<std::int64_t>(pos+6)+read<std::int32_t>(image,pos+2));
    }
    return result;
}
const Pattern* match(const Image& image,const char* role,std::uintptr_t at) {
    for(const auto& p:patterns) {
        if(std::strcmp(role,p.role) || !image.contains(at,p.size+16,executable)) continue;
        bool same=true;
        for(unsigned j=0;j<p.size;++j) if(p.mask[j] && image.bytes[at+j]!=p.bytes[j]) { same=false;break; }
        if(!same) continue;
        for(unsigned j=0;j<p.count;++j) if(instruction_size(image.bytes.data()+at+p.boundaries[j])!=static_cast<unsigned>(p.boundaries[j+1]-p.boundaries[j])) { same=false;break; }
        if(same && !image.local_prefix) for(const auto target:branches(image,p,at)) if(target<0 || !image.contains(static_cast<std::uintptr_t>(target),1,executable)) { same=false;break; }
        if(same) return &p;
    }
    return nullptr;
}
std::uintptr_t one(const std::set<std::uintptr_t>& found,const char* name) {
    if(found.size()!=1) throw std::runtime_error(std::string(name)+": expected one candidate, got "+std::to_string(found.size()));
    return *found.begin();
}
std::uintptr_t scan(const Image& image,const char* role) {
    std::set<std::uintptr_t> found;
    for(const auto& p:patterns) if(!std::strcmp(role,p.role)) for(const auto& s:image.sections) {
        if(!(s.flags&executable) || s.size<p.size+16) continue;
        for(std::uintptr_t at=s.start;at<=s.start+s.size-p.size-16;++at) {
            if(image.bytes[at]!=p.bytes[0]) continue;
            bool same=true;
            for(unsigned j=1;j<p.size;++j) if(p.mask[j] && image.bytes[at+j]!=p.bytes[j]) { same=false;break; }
            if(same && match(image,role,at)) found.insert(at);
        }
    }
    return one(found,role);
}
std::uintptr_t relative(const Image& image,std::uintptr_t instruction,unsigned displacement,unsigned size) {
    const auto target=static_cast<std::int64_t>(instruction+size)+read<std::int32_t>(image,instruction+displacement);
    if(target<0 || static_cast<std::uint64_t>(target)>=image.bytes.size()) throw std::runtime_error("relative reference outside image");
    return static_cast<std::uintptr_t>(target);
}
std::vector<std::uintptr_t> direct_calls(const Image& image,std::uintptr_t at,const Pattern& p) {
    std::vector<std::uintptr_t> values;
    for(unsigned j=0;j<p.count;++j) {
        const auto offset=p.boundaries[j];const auto opcode=image.bytes[at+offset];
        if(opcode==0xc3) break;
        if(opcode==0xe8) values.push_back(relative(image,at+offset,1,5));
    }
    return values;
}
}
bool Image::contains(std::uintptr_t rva,std::size_t size,unsigned flags) const {
    if(rva>bytes.size() || size>bytes.size()-rva) return false;
    for(const auto& s:sections) if(rva>=s.start && rva-s.start<=s.size && size<=s.size-(rva-s.start) && (s.flags&flags)==flags) return true;
    return false;
}
bool Image::parse(std::string& error) {
    try {
        sections.clear();status=0;
        if(read<WORD>(*this,0)!=IMAGE_DOS_SIGNATURE) throw std::runtime_error("invalid DOS header");
        const auto offset=read<LONG>(*this,0x3c);
        if(offset<0) throw std::runtime_error("invalid PE offset");
        const auto nt=read<IMAGE_NT_HEADERS64>(*this,offset);
        if(nt.Signature!=IMAGE_NT_SIGNATURE || nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 || nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC || nt.OptionalHeader.SizeOfImage!=bytes.size() || nt.FileHeader.NumberOfSections>96) throw std::runtime_error("unsupported PE image");
        const auto first=offset+sizeof(DWORD)+sizeof(IMAGE_FILE_HEADER)+nt.FileHeader.SizeOfOptionalHeader;
        for(unsigned i=0;i<nt.FileHeader.NumberOfSections;++i) {
            const auto s=read<IMAGE_SECTION_HEADER>(*this,first+i*sizeof(IMAGE_SECTION_HEADER));
            if(s.VirtualAddress>bytes.size() || s.Misc.VirtualSize>bytes.size()-s.VirtualAddress) throw std::runtime_error("invalid PE section");
            for(const auto& other:sections) if(s.VirtualAddress<other.start+other.size && other.start<s.VirtualAddress+s.Misc.VirtualSize) throw std::runtime_error("overlapping PE sections");
            sections.push_back({s.VirtualAddress,s.Misc.VirtualSize,s.Characteristics});
        }
        const auto dir=nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        if(!dir.VirtualAddress || !contains(dir.VirtualAddress,dir.Size,IMAGE_SCN_MEM_READ)) throw std::runtime_error("missing export directory");
        const auto e=read<IMAGE_EXPORT_DIRECTORY>(*this,dir.VirtualAddress);
        if(e.NumberOfNames>65536 || e.NumberOfFunctions>65536) throw std::runtime_error("invalid export count");
        for(unsigned i=0;i<e.NumberOfNames;++i) {
            const auto name=read<DWORD>(*this,e.AddressOfNames+i*4);
            constexpr char expected[]="DingoSDKOverlayGetStatus";
            if(name>bytes.size() || sizeof(expected)>bytes.size()-name || std::memcmp(bytes.data()+name,expected,sizeof(expected))) continue;
            const auto ordinal=read<WORD>(*this,e.AddressOfNameOrdinals+i*2);
            if(ordinal>=e.NumberOfFunctions) throw std::runtime_error("invalid export ordinal");
            if(status) throw std::runtime_error("duplicate status export");
            status=read<DWORD>(*this,e.AddressOfFunctions+ordinal*4);
        }
        if(!contains(status,16,executable)) throw std::runtime_error("missing or forwarded status export");
        return true;
    } catch(const std::exception& e) { error=e.what();return false; }
}
Resolution resolve(const Image& image,const std::array<unsigned char,32>& hash,bool force) {
    Resolution result;
    try {
        for(const auto& s:image.sections) if(s.start>image.bytes.size() || s.size>image.bytes.size()-s.start) throw std::runtime_error("invalid section bounds");
        const auto* reviewed=stock_contract::find_contract(hash,image.status);

        if(stock_contract::uses_v210_layout(reviewed)) {
            for(const auto& entry:native210::entries) {
                const auto count=entry.prefix.size()/2;
                if(!image.contains(entry.rva,count,executable)) throw std::runtime_error("2.1.0 entry outside code");
                const auto nibble=[](char c){return c<='9' ? c-'0':c-'a'+10;};
                for(std::size_t i=0;i<count;++i) if(image.bytes[entry.rva+i]!=(nibble(entry.prefix[2*i])*16+nibble(entry.prefix[2*i+1])))
                    throw std::runtime_error(std::string("2.1.0 native signature differs: ")+entry.role);
            }
            const std::uintptr_t slots[]{reviewed->state_pointer_rva,reviewed->source_initialized_rva,reviewed->source_base_rva,reviewed->source_transform_rva};
            for(auto slot:slots) if(!image.contains(slot,8,writable)) throw std::runtime_error("2.1.0 data slot outside writable section");
            if(relative(image,0xde4a07,3,7)!=reviewed->state_pointer_rva) throw std::runtime_error("2.1.0 state accessor reference differs");
            result.contract=*reviewed;result.known=true;result.read_callback=native210::read.rva;result.queue_callback=native210::queue.rva;
            return result;
        }
        const bool v2=stock_contract::uses_v2_layout(reviewed);
        auto& c=result.contract;
        c.version="automatically detected ABI 1";c.sha256="";c.status_rva=image.status;
        const auto* status=match(image,"status",c.status_rva);
        if(!status) throw std::runtime_error("status implementation/ABI differs");
        const auto calls=direct_calls(image,c.status_rva,*status);
        if(calls.size()!=3) throw std::runtime_error("status accessor/mutex relationship differs");
        const auto getter=calls[0];
        const auto* g=match(image,"getter",getter);
        if(!g) throw std::runtime_error("state accessor differs");
        if(read<DWORD>(image,getter+0x68)<0x720) throw std::runtime_error("overlay allocation too small for required model fields");
        c.state_pointer_rva=relative(image,getter+0x27,3,7);
        c.mutex_lock_rva=calls[1];c.mutex_unlock_rva=calls[2];
        if(read<WORD>(image,c.mutex_lock_rva)!=0xd233 || read<unsigned char>(image,c.mutex_lock_rva+2)!=0xe9 || !match(image,"mutex",relative(image,c.mutex_lock_rva+2,1,5)) || !match(image,"unlock",c.mutex_unlock_rva)) throw std::runtime_error("mutex ABI differs");
        std::set<std::uintptr_t> constructors;
        for(unsigned j=0;j<g->count;++j) {
            const auto at=getter+g->boundaries[j];
            if(image.bytes[at]==0xe8) { const auto target=relative(image,at,1,5);if(match(image,"ctor",target)) constructors.insert(target); }
        }
        const auto ctor=one(constructors,"overlay constructor");
        // Require unmasked fields used by this bridge, independently of total allocation size.
        const auto cb_offset=stock_contract::profile_callbacks_offset(reviewed), mdl_offset=stock_contract::profile_model_offset(reviewed);
        const unsigned char cb[]{0x48,0x89,0xb7,static_cast<unsigned char>(cb_offset),static_cast<unsigned char>(cb_offset>>8),0,0},model[]{0x48,0x8d,0x8f,static_cast<unsigned char>(mdl_offset),static_cast<unsigned char>(mdl_offset>>8),0,0};
        if(!image.contains(ctor,2048,executable)) throw std::runtime_error("constructor outside executable section");
        const auto begin=image.bytes.begin()+ctor,end=begin+2048;
        if(std::search(begin,end,std::begin(cb),std::end(cb))==end || std::search(begin,end,std::begin(model),std::end(model))==end) throw std::runtime_error("callback/model offsets changed");
        c.write_fov_rva=scan(image,"fov");
        const auto init=scan(image,"init");
        // Observed initializer writes remain at these instruction offsets within this ABI family.
        c.source_initialized_rva=relative(image,init+0x60,3,7);
        c.source_transform_rva=relative(image,init+0x81,3,7);
        c.source_base_rva=relative(image,init+0x88,3,7);
        result.read_callback=scan(image,"read");result.queue_callback=scan(image,"queue");
        const std::uintptr_t slots[]{c.state_pointer_rva,c.source_initialized_rva,c.source_base_rva,c.source_transform_rva};
        for(unsigned i=0;i<4;++i) if(!image.contains(slots[i],i==1 ? 1:8,writable)) throw std::runtime_error("bridge data slot section differs");
        if(c.state_pointer_rva%8 || c.source_base_rva%8 || c.source_transform_rva%8) throw std::runtime_error("unaligned data slot");
        if(!force || v2) if(const auto* known=reviewed) {
            const std::uintptr_t a[]{known->state_pointer_rva,known->mutex_lock_rva,known->mutex_unlock_rva,known->write_fov_rva,known->source_initialized_rva,known->source_base_rva,known->source_transform_rva};
            const std::uintptr_t b[]{c.state_pointer_rva,c.mutex_lock_rva,c.mutex_unlock_rva,c.write_fov_rva,c.source_initialized_rva,c.source_base_rva,c.source_transform_rva};
            if(!std::equal(std::begin(a),std::end(a),std::begin(b))) throw std::runtime_error("known profile disagrees with validated structure");
            c=*known;result.known=true;
        }
    } catch(const std::exception& e) { result.error=e.what(); }
    return result;
}
bool initialize(HMODULE module) {
    try {
        wchar_t path[32768]{};if(!GetModuleFileNameW(module,path,32768)) throw std::runtime_error("module path unavailable");
        auto file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        std::array<unsigned char,32> hash{};const bool hashed=dingosdk::supported_build::file_sha256(file,hash);
        if(file!=INVALID_HANDLE_VALUE) CloseHandle(file);if(!hashed) throw std::runtime_error("DLL hash unavailable");
        digest_text.clear();constexpr char hex[]="0123456789abcdef";for(auto b:hash) { digest_text+=hex[b>>4];digest_text+=hex[b&15]; }
        log("ReSkate SHA-256: "+digest_text);
        const auto base=reinterpret_cast<std::uintptr_t>(module);
        IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
        if(!dingosdk::memory::read(base,dos) || dos.e_lfanew<0 || dos.e_lfanew>0x100000 || !dingosdk::memory::read(base+dos.e_lfanew,nt) || nt.OptionalHeader.SizeOfImage>128*1024*1024 || !nt.OptionalHeader.SizeOfImage) throw std::runtime_error("invalid module headers");
        Image image;image.bytes.resize(nt.OptionalHeader.SizeOfImage);
        for(std::size_t offset=0;offset<image.bytes.size();) {
            MEMORY_BASIC_INFORMATION page{};if(!VirtualQuery(reinterpret_cast<void*>(base+offset),&page,sizeof(page))) throw std::runtime_error("module page query failed");
            const auto count=std::min(image.bytes.size()-offset,static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(page.BaseAddress)+page.RegionSize-base-offset));
            if(!count) throw std::runtime_error("invalid module region");
            if(page.State==MEM_COMMIT && !(page.Protect&(PAGE_NOACCESS|PAGE_GUARD))) if(!dingosdk::memory::read_bytes(base+offset,image.bytes.data()+offset,count)) throw std::runtime_error("module snapshot unreadable");
            offset+=count;
        }
        std::string error;if(!image.parse(error)) throw std::runtime_error(error);
        if(!stock_contract::uses_v210_layout(stock_contract::find_contract(hash,image.status))) throw std::runtime_error("This compatibility build requires the exact official ReSkate 2.1.0 DLL; private hooks remain inactive.");
        wchar_t force[2]{};
        const bool forced=GetEnvironmentVariableW(L"JISHS_FP_FORCE_DETECTION",force,2)==1 && force[0]==L'1';
        selected=resolve(image,hash,forced);if(!selected) throw std::runtime_error(selected.error);
        selected.contract.sha256=digest_text.c_str();selected_module=module;
        stock_contract::selected=&selected.contract;
        log(selected.known ? "Validated compiled ReSkate profile: "+std::string(selected.contract.version):"Validated automatic ReSkate detection: ABI family 1.1.5-1.1.8");
        return true;
    } catch(const std::exception& e) { log(std::string("ERROR: ReSkate compatibility: ")+e.what());return false; }
}
bool validate_callbacks(HMODULE module,std::uintptr_t read_address,std::uintptr_t queue_address) {
    const auto base=reinterpret_cast<std::uintptr_t>(module);
    if(module!=selected_module || !selected || read_address!=base+selected.read_callback || queue_address!=base+selected.queue_callback) return false;
    if(stock_contract::uses_v210_layout(&selected.contract)) {
        for(const auto& entry:{native210::read,native210::queue}) {
            std::array<unsigned char,32> actual{};
            if(!dingosdk::memory::read_bytes(base+entry.rva,actual.data(),actual.size())) return false;
            const auto nibble=[](char c){return c<='9' ? c-'0':c-'a'+10;};
            for(unsigned i=0;i<32;++i) if(actual[i]!=(nibble(entry.prefix[2*i])*16+nibble(entry.prefix[2*i+1]))) return false;
        }
        return true;
    }
    Image live;live.bytes.resize(512);live.sections.push_back({0,512,executable});live.local_prefix=true;
    const auto validate=[&](std::uintptr_t address,const char* role) {
        if(!dingosdk::memory::read_bytes(address,live.bytes.data(),512)) return false;
        const auto* pattern=match(live,role,0);if(!pattern) return false;
        for(const auto target:branches(live,*pattern,0)) {
            const auto absolute=static_cast<std::int64_t>(address)+target;if(absolute<0) return false;
            MEMORY_BASIC_INFORMATION page{};
            if(!VirtualQuery(reinterpret_cast<void*>(static_cast<std::uintptr_t>(absolute)),&page,sizeof(page)) || page.AllocationBase!=module || page.State!=MEM_COMMIT ||
               (page.Protect&(PAGE_GUARD|PAGE_NOACCESS)) || !(page.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))) return false;
        }
        return true;
    };
    return validate(read_address,"read") && validate(queue_address,"queue");
}
bool enabled() { return activated.load(std::memory_order_acquire); }
void enable() { activated.store(true,std::memory_order_release); }
void disable() { activated.store(false,std::memory_order_release); }
}
