// Diagnostics are flushed after guards declared later have released their locks.
struct NativeDiagnostics {
    std::vector<std::string> messages;
    ~NativeDiagnostics(){const auto error=GetLastError();for(const auto& message:messages) log(message);SetLastError(error);}
};
// Native Sony reads need a game-facing adapter, independently of whether stock
// recorded that handle or routes a completion through its neutral_hid helper.
struct NativeHidDevice {
    dingosdk::PlayStationPad kind{};DWORD report_length{};bool reported{},consumption_reported{};
    const void* last_buffer{};ULONGLONG epoch{};std::vector<unsigned char> edited;
};
struct NativeHidRequest { StockHidRead read;LPOVERLAPPED_COMPLETION_ROUTINE callback{};bool filtered{}; };
thread_local bool native_hid_internal{};
struct NativeHidInternal {
    bool previous=native_hid_internal;
    NativeHidInternal(){native_hid_internal=true;}
    ~NativeHidInternal(){native_hid_internal=previous;}
};
std::mutex native_hid_mutex;
std::map<HANDLE,NativeHidDevice> native_hid_devices;
std::map<OVERLAPPED*,NativeHidRequest> native_hid_requests;
decltype(&ReadFile) native_original_read{};
decltype(&ReadFileEx) native_original_read_ex{};
decltype(&GetOverlappedResult) native_original_overlapped{};
decltype(&GetOverlappedResultEx) native_original_overlapped_ex{};
decltype(&GetQueuedCompletionStatus) native_original_completion{};
decltype(&GetQueuedCompletionStatusEx) native_original_completion_ex{};
decltype(&CloseHandle) native_original_close{};
decltype(&HidD_GetInputReport) native_original_report{};
decltype(&HidD_GetAttributes) native_attributes=&HidD_GetAttributes;
decltype(&HidD_GetPreparsedData) native_preparsed=&HidD_GetPreparsedData;
decltype(&HidP_GetCaps) native_caps=&HidP_GetCaps;
decltype(&HidD_FreePreparsedData) native_free_preparsed=&HidD_FreePreparsedData;

bool native_private_reader(HANDLE file,const void* caller) {
    if(diagnostic_file_io || raw_controller_read || native_hid_internal) return true;
    {std::lock_guard lock(glance_mutex);if(private_handles.contains(file)) return true;}
    // ReSkate's binding reader and translation libraries own independent raw
    // streams. Filter their game-facing state elsewhere, not their source data.
    HMODULE module{};
    if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(caller),&module)) {
        if(module==GetModuleHandleW(L"reskate.dll")) return true;
        wchar_t path[MAX_PATH]{};
        if(GetModuleFileNameW(module,path,MAX_PATH)) {
            const auto* slash=wcsrchr(path,L'\\');const auto* name=slash?slash+1:path;
            if(!_wcsicmp(name,L"dinput8.dll") || !_wcsicmp(name,L"gameoverlayrenderer64.dll")) return true;
        }
    }
    return false;
}
StockHidRead native_read_info(HANDLE file,void* buffer,DWORD capacity,const void* caller) {
    StockHidRead read{file,buffer,capacity,dingosdk::PlayStationPad::none};
    if(!buffer || capacity<10 || capacity>65536 || native_private_reader(file,caller)) return read;
    NativeDiagnostics diagnostics;
    NativeHidInternal internal;
    std::lock_guard lock(native_hid_mutex);
    const auto found=native_hid_devices.find(file);
    if(found!=native_hid_devices.end() &&
        (found->second.kind==dingosdk::PlayStationPad::none || found->second.report_length)) {
        read.kind=found->second.kind;return read;
    }
    const auto error=GetLastError();HIDD_ATTRIBUTES attributes{sizeof(attributes)};
    DWORD report_length{};
    if(native_attributes(file,&attributes)) {
        read.kind=dingosdk::playstation_pad(attributes.VendorID,attributes.ProductID);
        if(read.kind!=dingosdk::PlayStationPad::none) {
            PHIDP_PREPARSED_DATA preparsed{};HIDP_CAPS caps{};
            const bool opened=native_preparsed(file,&preparsed)!=FALSE;
            const bool valid=opened && native_caps(preparsed,&caps)==HIDP_STATUS_SUCCESS;
            if(opened) native_free_preparsed(preparsed);
            if(valid && caps.UsagePage==1 && caps.Usage==5 && caps.InputReportByteLength>=10)
                report_length=caps.InputReportByteLength;
            else read.kind=dingosdk::PlayStationPad::none;
        }
        if(read.kind!=dingosdk::PlayStationPad::none)
            diagnostics.messages.push_back("Native HID game-read handle identified: Sony product="+std::to_string(attributes.ProductID)+
                "; report length="+std::to_string(report_length)+"; buffer capacity="+std::to_string(capacity)+". DirectInput is not required for this path.");
    }
    if(native_hid_devices.size()>=512)
        std::erase_if(native_hid_devices,[](const auto& pair){return pair.second.kind==dingosdk::PlayStationPad::none;});
    auto& device=native_hid_devices[file];device.kind=read.kind;device.report_length=report_length;
    SetLastError(error);return read;
}
void native_filter_read(const StockHidRead& read,DWORD bytes,NativeDiagnostics& diagnostics,StickInputPolicy policy,bool menu_capture) {
    if(read.kind==dingosdk::PlayStationPad::none || !read.buffer || bytes<10 || bytes>read.capacity) return;
    auto& device=native_hid_devices[read.file];
    device.kind=read.kind;
    const DWORD stride=device.report_length?device.report_length:bytes;
    if(!stride || bytes%stride) return;
    const auto* buffer=static_cast<const unsigned char*>(read.buffer);
    if(device.last_buffer==read.buffer && device.epoch==direct_epoch.load() && device.edited.size()==bytes &&
        std::equal(device.edited.begin(),device.edited.end(),buffer)) return;
    // Use the same source identity as the shared stock adapter. If stock already
    // edited this completion, its duplicate guard preserves the physical latch.
    bool observed{};
    for(DWORD offset=0;offset<bytes;offset+=stride) {
        auto* report=static_cast<unsigned char*>(read.buffer)+offset;
        const auto physical=dingosdk::parse_playstation_report(read.kind,stride>64,report,stride);
        if(!physical) continue;
        observed=true;filter_sony_report(2,reinterpret_cast<std::uintptr_t>(read.file),read.kind,{report,stride},true,policy,menu_capture);
        const auto edited=dingosdk::parse_playstation_report(read.kind,stride>64,report,stride);
        if(edited && (physical->buttons&15)!=(edited->buttons&15) && !device.consumption_reported) {
            device.consumption_reported=true;
            diagnostics.messages.push_back("Native HID gesture consumed: physical buttons="+std::to_string(physical->buttons)+
                "; forwarded buttons="+std::to_string(edited->buttons)+"; report length="+std::to_string(stride)+".");
        }
    }
    if(!observed) return;
    device.last_buffer=read.buffer;device.epoch=direct_epoch.load();device.edited.assign(buffer,buffer+bytes);
    // Called under native_hid_mutex; one observed report log per game handle.
    if(!device.reported) {
        device.reported=true;diagnostics.messages.push_back("Native HID game report observed: bytes="+std::to_string(bytes)+
            "; report id="+std::to_string(static_cast<unsigned char*>(read.buffer)[0])+
            "; glance and enter/exit policy applied at read completion.");
    }
}
void filter_native_sony_read(const StockHidRead& read,DWORD size) {
    if(diagnostic_file_io || raw_controller_read || native_hid_internal ||
       read.kind==dingosdk::PlayStationPad::none || !read.buffer || size<10 || size>read.capacity) return;
    // OverlayGetStatus takes stock's render mutex. Rendering polls independent
    // Sony completions through our API hooks, which take native_hid_mutex.
    // Query eligibility BEFORE taking that mutex, never from a locked report.
    const bool menu_capture=panel_gameplay_blocked();
    const auto policy=stick_input_policy();
    if(!menu_capture && !policy.mask && !live_glances()) {clear_glance_filter();return;}
    NativeDiagnostics diagnostics;NativeHidInternal internal;std::lock_guard lock(native_hid_mutex);native_filter_read(read,size,diagnostics,policy,menu_capture);
}
void capture_stock_sony_read(const StockHidRead& read,DWORD size) {
    // Stock neutral_hid infers Bluetooth from its size argument. A USB read
    // may contain several 64-byte reports: passing the total length writes
    // DS4/simple-Bluetooth button offsets into the first DualSense USB report.
    // Query/cache the actual descriptor length before stock edits the buffer.
    // Never hold our input lock while calling back into ReSkate.
    DWORD stride{};
    if(!diagnostic_file_io && !raw_controller_read && !native_hid_internal &&
       read.kind!=dingosdk::PlayStationPad::none && read.buffer && size>=10 && size<=read.capacity) {
        const auto identified=native_read_info(read.file,read.buffer,read.capacity,
            reinterpret_cast<const void*>(&filtered_stock_hid));
        if(identified.kind==read.kind) {
            std::lock_guard lock(native_hid_mutex);
            const auto found=native_hid_devices.find(read.file);
            if(found!=native_hid_devices.end()) stride=found->second.report_length;
        }
    }
    if(!stride || size%stride || stride>size) { original_hid(read,size);return; }
    if(size>stride) {
        static std::atomic<bool> reported{};
        if(!reported.exchange(true)) log("Sony HID menu capture: batched read split at descriptor report boundaries; USB/Bluetooth layout preserved.");
    }
    for(DWORD offset=0;offset<size;offset+=stride) {
        StockHidRead report=read;
        report.buffer=static_cast<unsigned char*>(read.buffer)+offset;
        report.capacity=stride;
        original_hid(report,stride);
    }
}
void native_finish(OVERLAPPED* request,DWORD bytes,bool success,bool finished) {
    if(!request || diagnostic_file_io || native_hid_internal) return;
    StockHidRead read;bool filtered{};
    {
        std::lock_guard lock(native_hid_mutex);
        const auto found=native_hid_requests.find(request);if(found==native_hid_requests.end()) return;
        read=found->second.read;filtered=found->second.filtered;
        if(finished) {
            if(found->second.callback) found->second.filtered=true; // APC still owes the caller its callback.
            else native_hid_requests.erase(found);
        }
    }
    if(success && !filtered) filter_native_sony_read(read,bytes);
}
BOOL WINAPI native_read_file(HANDLE file,void* buffer,DWORD capacity,DWORD* bytes,OVERLAPPED* request) {
    const auto incoming=GetLastError();const auto read=native_read_info(file,buffer,capacity,_ReturnAddress());
    if(read.kind!=dingosdk::PlayStationPad::none && request) {
        std::lock_guard lock(native_hid_mutex);native_hid_requests[request]={read,nullptr};
    }
    SetLastError(incoming);
    const auto result=native_original_read(file,buffer,capacity,bytes,request);const auto error=GetLastError();
    if(read.kind!=dingosdk::PlayStationPad::none) {
        if(result) {
            const auto size=bytes?*bytes:request?static_cast<DWORD>(request->InternalHigh):0;
            if(request) native_finish(request,size,true,true);
            else filter_native_sony_read(read,size);
        } else if(request && error!=ERROR_IO_PENDING) native_finish(request,0,false,true);
    }
    SetLastError(error);return result;
}
void CALLBACK native_read_callback(DWORD error,DWORD bytes,OVERLAPPED* request) {
    const auto incoming=GetLastError();LPOVERLAPPED_COMPLETION_ROUTINE callback{};StockHidRead read;bool filtered=true;
    {
        std::lock_guard lock(native_hid_mutex);const auto found=native_hid_requests.find(request);
        if(found!=native_hid_requests.end()) {
            read=found->second.read;filtered=found->second.filtered;
            callback=found->second.callback;native_hid_requests.erase(found);
        }
    }
    if(error==ERROR_SUCCESS && !filtered) filter_native_sony_read(read,bytes);
    SetLastError(incoming);if(callback) callback(error,bytes,request);
}
BOOL WINAPI native_read_file_ex(HANDLE file,void* buffer,DWORD capacity,OVERLAPPED* request,LPOVERLAPPED_COMPLETION_ROUTINE callback) {
    const auto incoming=GetLastError();const auto read=native_read_info(file,buffer,capacity,_ReturnAddress());
    const bool tracked=read.kind!=dingosdk::PlayStationPad::none && request && callback;
    if(tracked) {std::lock_guard lock(native_hid_mutex);native_hid_requests[request]={read,callback};}
    SetLastError(incoming);
    const auto result=native_original_read_ex(file,buffer,capacity,request,tracked?native_read_callback:callback);const auto error=GetLastError();
    if(tracked && !result) {std::lock_guard lock(native_hid_mutex);native_hid_requests.erase(request);}
    SetLastError(error);return result;
}
BOOL WINAPI native_overlapped(HANDLE file,OVERLAPPED* request,DWORD* bytes,BOOL wait) {
    const auto result=native_original_overlapped(file,request,bytes,wait);const auto error=GetLastError();
    native_finish(request,result&&bytes?*bytes:0,result!=FALSE,result || error!=ERROR_IO_INCOMPLETE);
    SetLastError(error);return result;
}
BOOL WINAPI native_overlapped_ex(HANDLE file,OVERLAPPED* request,DWORD* bytes,DWORD timeout,BOOL alertable) {
    const auto result=native_original_overlapped_ex(file,request,bytes,timeout,alertable);const auto error=GetLastError();
    native_finish(request,result&&bytes?*bytes:0,result!=FALSE,result ||
        (error!=ERROR_IO_INCOMPLETE && error!=WAIT_TIMEOUT && error!=WAIT_IO_COMPLETION));
    SetLastError(error);return result;
}
BOOL WINAPI native_completion(HANDLE port,DWORD* bytes,ULONG_PTR* key,OVERLAPPED** request,DWORD timeout) {
    const auto result=native_original_completion(port,bytes,key,request,timeout);const auto error=GetLastError();
    if(request && *request) native_finish(*request,result&&bytes?*bytes:0,result!=FALSE,true);
    SetLastError(error);return result;
}
BOOL WINAPI native_completion_ex(HANDLE port,OVERLAPPED_ENTRY* entries,ULONG capacity,ULONG* count,DWORD timeout,BOOL alertable) {
    const auto result=native_original_completion_ex(port,entries,capacity,count,timeout,alertable);const auto error=GetLastError();
    if(result && entries && count && *count<=capacity) for(ULONG i=0;i<*count;++i)
        native_finish(entries[i].lpOverlapped,entries[i].dwNumberOfBytesTransferred,static_cast<LONG>(entries[i].Internal)>=0,true);
    SetLastError(error);return result;
}
BOOL WINAPI native_close(HANDLE file) {
    const auto result=native_original_close(file);const auto error=GetLastError();
    if(result && !native_hid_internal && !diagnostic_file_io) {
        bool sony{};
        {std::lock_guard lock(native_hid_mutex);
            const auto device=native_hid_devices.find(file);
            sony=device!=native_hid_devices.end() && device->second.kind!=dingosdk::PlayStationPad::none;
            native_hid_devices.erase(file);
            for(const auto& [request,tracked]:native_hid_requests)
                if(tracked.read.file==file && tracked.read.kind!=dingosdk::PlayStationPad::none) sony=true;
            for(auto& [request,tracked]:native_hid_requests) if(tracked.read.file==file && tracked.callback) {
                tracked.read.kind=dingosdk::PlayStationPad::none;tracked.read.buffer=nullptr;
            }
            std::erase_if(native_hid_requests,[&](const auto& pair){return pair.second.read.file==file && !pair.second.callback;});}
        if(sony) {std::lock_guard lock(glance_mutex);glance_sources.erase({2,reinterpret_cast<std::uintptr_t>(file)});}
    }
    SetLastError(error);return result;
}
BOOLEAN __stdcall native_input_report(HANDLE file,void* buffer,ULONG capacity) {
    const auto incoming=GetLastError();const auto read=native_read_info(file,buffer,capacity,_ReturnAddress());SetLastError(incoming);
    const auto result=native_original_report(file,buffer,capacity);const auto error=GetLastError();
    if(result && read.kind!=dingosdk::PlayStationPad::none) {
        DWORD length{};
        {std::lock_guard lock(native_hid_mutex);const auto found=native_hid_devices.find(file);
            if(found!=native_hid_devices.end()) length=found->second.report_length;}
        if(length && length<=capacity) filter_native_sony_read(read,length);
    }
    SetLastError(error);return result;
}
bool prepare_native_hid(std::vector<void*>& pending) {
    using namespace dingosdk;
    const auto kernel=GetModuleHandleW(L"kernel32.dll");const auto hid=LoadLibraryExW(L"hid.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!kernel || !hid) return false;
    const auto add=[&](HMODULE module,const char* name,void* detour,auto& original) {
        const auto entry=reinterpret_cast<std::uintptr_t>(GetProcAddress(module,name));if(!entry) return false;
        const auto stock=resolve_stock_handler(entry);auto* target=reinterpret_cast<void*>(stock?stock:entry);
        void* forwarding{};if(owned_hook_prepare(target,detour,&forwarding)!=HookOk) return false;
        original=reinterpret_cast<std::remove_reference_t<decltype(original)>>(forwarding);pending.push_back(target);return true;
    };
    const bool ready=add(kernel,"ReadFile",reinterpret_cast<void*>(&native_read_file),native_original_read) &&
        add(kernel,"ReadFileEx",reinterpret_cast<void*>(&native_read_file_ex),native_original_read_ex) &&
        add(kernel,"GetOverlappedResult",reinterpret_cast<void*>(&native_overlapped),native_original_overlapped) &&
        add(kernel,"GetOverlappedResultEx",reinterpret_cast<void*>(&native_overlapped_ex),native_original_overlapped_ex) &&
        add(kernel,"GetQueuedCompletionStatus",reinterpret_cast<void*>(&native_completion),native_original_completion) &&
        add(kernel,"GetQueuedCompletionStatusEx",reinterpret_cast<void*>(&native_completion_ex),native_original_completion_ex) &&
        add(kernel,"CloseHandle",reinterpret_cast<void*>(&native_close),native_original_close) &&
        add(hid,"HidD_GetInputReport",reinterpret_cast<void*>(&native_input_report),native_original_report);
    log(ready ? "Native HID completion adapters prepared: game Sony ReadFile/ReadFileEx, overlapped results, completion ports and input reports; per-report stock menu capture enabled; overlay status queried outside HID locks (test 2); binding/translation readers excluded."
        : "ERROR: native HID completion adapter preparation failed.");
    return ready;
}
