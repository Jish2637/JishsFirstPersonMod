// Included inside stick_input_filter.cpp's private namespace. Reuse its policy
// and publication gates; DirectInput never invokes the camera toggle itself.
struct DirectField { DWORD offset{}, mask{}; bool pov{},axis{},vertical{},right_stick{},trigger{};LONG center{},low{},high{}; };
struct DirectEvent { DWORD offset{}, data{}, time{}, sequence{}, edited{}; };
struct DirectDecision { ULONGLONG time{}; DWORD buttons{}, consume{}; };
struct DirectDevice {
    bool identified{}, sony{}, mapped{}, reported{}, buffered_known{};
    DWORD size{}, buffered_buttons{};
    bool stick_captured{};
    std::deque<DIDEVICEOBJECTDATA> restore_axes;
    ULONGLONG epoch{};
    std::vector<DirectField> fields;
    std::deque<DirectEvent> events;
    std::deque<DirectDecision> decisions;
};
std::mutex direct_mutex;
std::map<void*,DirectDevice> direct_devices;
thread_local bool direct_internal{};
struct DirectInternal {
    bool previous=direct_internal;
    DirectInternal(){direct_internal=true;}
    ~DirectInternal(){direct_internal=previous;}
};
enum DirectMethod { di_state,di_data,di_format,di_acquire,di_unacquire,di_release,di_device,di_factory,di_methods };
struct DirectHook { void* target{}; void* original{}; };
constexpr unsigned direct_slots=16;
std::array<std::array<DirectHook,direct_slots>,di_methods> direct_hooks;
std::recursive_mutex direct_install_mutex;
using DirectState=HRESULT(STDMETHODCALLTYPE*)(void*,DWORD,void*);
using DirectData=HRESULT(STDMETHODCALLTYPE*)(void*,DWORD,DIDEVICEOBJECTDATA*,DWORD*,DWORD);
using DirectFormat=HRESULT(STDMETHODCALLTYPE*)(void*,const DIDATAFORMAT*);
using DirectSimple=HRESULT(STDMETHODCALLTYPE*)(void*);
using DirectRelease=ULONG(STDMETHODCALLTYPE*)(void*);
using DirectDeviceCreate=HRESULT(STDMETHODCALLTYPE*)(void*,REFGUID,void**,IUnknown*);
using DirectFactory=HRESULT(WINAPI*)(HINSTANCE,DWORD,REFIID,void**,IUnknown*);

void direct_sync(DirectDevice& state) {
    const auto epoch=direct_epoch.load();
    if(state.epoch!=epoch) {
        state.epoch=epoch;state.buffered_known=false;state.buffered_buttons=0;state.stick_captured=false;state.restore_axes.clear();
        state.events.clear();state.decisions.clear();
    }
}

DWORD direct_dpad(DWORD pov) {
    if(LOWORD(pov)==0xffff || pov>=36000) return 0;
    constexpr DWORD directions[]{1,9,8,10,2,6,4,5};
    return directions[((pov+2250)/4500)%8];
}
DWORD direct_pov(DWORD buttons) {
    constexpr DWORD angles[]{0xffffffffu,0,18000,0xffffffffu,27000,31500,22500,0xffffffffu,
        9000,4500,13500,0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu};
    return angles[buttons&15];
}
void direct_reset(void* device) {
    std::lock_guard lock(direct_mutex);
    if(auto found=direct_devices.find(device);found!=direct_devices.end()) {
        found->second.buffered_known=false;found->second.buffered_buttons=0;found->second.stick_captured=false;found->second.restore_axes.clear();
        found->second.events.clear();found->second.decisions.clear();
    }
    std::lock_guard policy_lock(glance_mutex);
    glance_sources.erase({4,reinterpret_cast<std::uintptr_t>(device)});
}
void direct_identify(void* device,DirectDevice& state) {
    if(state.identified) return;
    state.identified=true;
    DIPROPDWORD identity{};identity.diph={sizeof(identity),sizeof(identity.diph),0,DIPH_DEVICE};
    auto** table=*reinterpret_cast<void***>(device);
    using Property=HRESULT(STDMETHODCALLTYPE*)(void*,REFGUID,DIPROPHEADER*);
    if(SUCCEEDED(reinterpret_cast<Property>(table[5])(device,DIPROP_VIDPID,&identity.diph)))
        state.sony=dingosdk::playstation_pad(LOWORD(identity.dwData),HIWORD(identity.dwData))!=dingosdk::PlayStationPad::none;
}
// A and W object records have identical semantic metadata but different names.
// Try the W-sized record first and the A-sized record for an A interface.
bool direct_object(void* device,DWORD object,DWORD how,WORD& usage,WORD& page,DWORD& type) {
    auto** table=*reinterpret_cast<void***>(device);
    using Object=HRESULT(STDMETHODCALLTYPE*)(void*,void*,DWORD,DWORD);
    const auto get=reinterpret_cast<Object>(table[14]);
    DIDEVICEOBJECTINSTANCEW wide{};wide.dwSize=sizeof(wide);
    if(SUCCEEDED(get(device,&wide,object,how))) {usage=wide.wUsage;page=wide.wUsagePage;type=wide.dwType;return true;}
    DIDEVICEOBJECTINSTANCEA narrow{};narrow.dwSize=sizeof(narrow);
    if(SUCCEEDED(get(device,&narrow,object,how))) {usage=narrow.wUsage;page=narrow.wUsagePage;type=narrow.dwType;return true;}
    return false;
}
void direct_map(void* device,DirectDevice& state,const DIDATAFORMAT& format) {
    state.fields.clear();state.size=format.dwDataSize;state.mapped=false;
    if(!state.sony || format.dwSize!=sizeof(DIDATAFORMAT) || format.dwObjSize!=sizeof(DIOBJECTDATAFORMAT) ||
        format.dwNumObjs>1024 || (format.dwNumObjs && !format.rgodf)) return;
    unsigned shoulders{},povs{};
    for(DWORD i=0;i<format.dwNumObjs;++i) {
        const auto& object=format.rgodf[i];WORD usage{},page{};DWORD type{};
        if(!direct_object(device,object.dwOfs,DIPH_BYOFFSET,usage,page,type)) continue;
        const bool pov=(type&DIDFT_POV)!=0;
        constexpr DWORD sony_buttons[]{0,0x4000,0x1000,0x2000,0x8000,0x100,0x200,0x10000,0x20000,0x20,0x10,0x40,0x80};
        const DWORD mask=(type&DIDFT_BUTTON)!=0 && page==9 && usage<std::size(sony_buttons) ? sony_buttons[usage]:0;
        const bool axis=(type&DIDFT_ABSAXIS)!=0 && page==1 && (usage>=0x30 && usage<=0x35);
        LONG center{},low{},high{};
        if(axis) {
            DIPROPRANGE range{};range.diph={sizeof(range),sizeof(range.diph),object.dwOfs,DIPH_BYOFFSET};
            auto** table=*reinterpret_cast<void***>(device);
            using Property=HRESULT(STDMETHODCALLTYPE*)(void*,REFGUID,DIPROPHEADER*);
            if(FAILED(reinterpret_cast<Property>(table[5])(device,DIPROP_RANGE,&range.diph)) || range.lMin>=range.lMax) continue;
            low=range.lMin;high=range.lMax;
            center=static_cast<LONG>((static_cast<long long>(range.lMin)+range.lMax)/2);
        }
        const bool l1=mask==0x100;
        const DWORD width=pov || axis ? 4:1;
        if((!pov && !mask && !axis) || object.dwOfs>state.size || width>state.size-object.dwOfs) continue;
        if(pov && (page!=1 || usage!=0x39)) continue;
        state.fields.push_back({object.dwOfs,mask,pov,axis,usage==0x35,usage==0x32 || usage==0x35,usage==0x33 || usage==0x34,center,low,high});
        shoulders+=l1;povs+=pov;
    }
    // Multiple hats/shoulder aliases require an explicit mapping, not guesses.
    state.mapped=shoulders==1 && povs==1;
    if(!state.mapped) state.fields.clear();
    log(state.mapped ? "DirectInput Sony gesture adapter mapped L1 and POV; enter/exit and glance filtering available."
        : "DirectInput Sony gesture coverage incomplete: L1/POV metadata or data format is unsupported.");
}
void direct_standard(void* device,DirectDevice& state,DWORD size) {
    if(state.size || (size!=sizeof(DIJOYSTATE) && size!=sizeof(DIJOYSTATE2))) return;
    // Validate standard offsets against the device's active object metadata.
    if(!state.sony) return;
    std::array<DIOBJECTDATAFORMAT,135> objects{};
    objects[0].dwOfs=offsetof(DIJOYSTATE,rgdwPOV);
    for(unsigned i=0;i<128;++i) objects[i+1].dwOfs=offsetof(DIJOYSTATE,rgbButtons)+i;
    objects[129].dwOfs=offsetof(DIJOYSTATE,lZ);objects[130].dwOfs=offsetof(DIJOYSTATE,lRz);
    objects[131].dwOfs=offsetof(DIJOYSTATE,lX);objects[132].dwOfs=offsetof(DIJOYSTATE,lY);
    objects[133].dwOfs=offsetof(DIJOYSTATE,lRx);objects[134].dwOfs=offsetof(DIJOYSTATE,lRy);
    DIDATAFORMAT format{sizeof(DIDATAFORMAT),sizeof(DIOBJECTDATAFORMAT),DIDF_ABSAXIS,size,
        static_cast<DWORD>(objects.size()),objects.data()};
    direct_map(device,state,format);
}
LONG direct_axis(const DirectField& field,StickAxes axes) {
    const float value=field.vertical ? -axes.y:axes.x;
    const double scale=value<0 ? static_cast<double>(field.center)-field.low:static_cast<double>(field.high)-field.center;
    return static_cast<LONG>(std::clamp(std::llround(field.center+value*scale),static_cast<long long>(field.low),static_cast<long long>(field.high)));
}
DWORD direct_buttons(const DirectDevice& state,const void* data) {
    DWORD buttons{};const auto* bytes=static_cast<const unsigned char*>(data);
    for(const auto& field:state.fields) {
        if(field.pov) {DWORD value{};std::memcpy(&value,bytes+field.offset,4);buttons|=direct_dpad(value);}
        else if(!field.axis && (bytes[field.offset]&0x80)) buttons|=field.mask;
    }
    return buttons;
}
ULONGLONG direct_time(DWORD timestamp) {
    const auto now=GetTickCount64();const auto age=static_cast<DWORD>(static_cast<DWORD>(now)-timestamp);
    return age<=now ? now-age:now;
}
DWORD direct_classify(void* device,DirectDevice& state,DWORD buttons,ULONGLONG time) {
    // An immediate read can precede delivery of the same buffered event. Reuse
    // its decision rather than replay a historical release into the live policy.
    if(!state.decisions.empty() && time<state.decisions.back().time) {
        for(const auto& decision:state.decisions)
            if(decision.time>=time && decision.buttons==buttons) return decision.consume;
        // An old transition absent from snapshots is conservatively a first tap.
        // Do not let it alter the current held press or create an extra emote.
        GlanceEmotePolicy first;ToggleEmoteBlock toggle;
        std::lock_guard lock(glance_mutex);
        const auto consumed=toggle.update(toggle_eligible ? toggle_binding:0,buttons,toggle_eligible ? native_toggle_binding:0);
        return consumed ? consumed:first.update(glance_bindings,buttons,time).consume;
    }
    std::lock_guard lock(glance_mutex);
    const auto result=classify(4,reinterpret_cast<std::uintptr_t>(device),buttons,time);
    state.decisions.push_back({time,buttons,result.consume});
    while(state.decisions.size()>512) state.decisions.pop_front();
    if(result.consume && !state.reported) {
        state.reported=true;log("DirectInput Sony gesture filtering observed: L1 + POV consumed; existing enter/exit action retains its private input stream.");
    }
    return result.consume;
}
template<unsigned Slot> HRESULT STDMETHODCALLTYPE direct_state(void* device,DWORD size,void* data) {
    const auto result=reinterpret_cast<DirectState>(direct_hooks[di_state][Slot].original)(device,size,data);
    const auto error=GetLastError();
    if(direct_internal || raw_controller_read) {SetLastError(error);return result;}
    if(FAILED(result)) {if(result==DIERR_INPUTLOST || result==DIERR_NOTACQUIRED) direct_reset(device);SetLastError(error);return result;}
    if(data) {
        const auto policy=stick_input_policy();const bool menu_capture=panel_gameplay_blocked();
        const bool live=menu_capture || policy.mask || live_glances();
        if(!live) direct_reset(device);
        else {
            std::lock_guard lock(direct_mutex);auto& state=direct_devices[device];direct_sync(state);direct_identify(device,state);
            direct_standard(device,state,size);
            if(state.mapped && size==state.size) {
                const auto buttons=direct_buttons(state,data);
                if(menu_capture) for(const auto& field:state.fields) {
                    if(field.axis) {const LONG neutral=field.trigger ? field.low:field.center;std::memcpy(static_cast<unsigned char*>(data)+field.offset,&neutral,4);}
                    else if(field.pov) {const DWORD neutral=0xffffffffu;std::memcpy(static_cast<unsigned char*>(data)+field.offset,&neutral,4);}
                    else static_cast<unsigned char*>(data)[field.offset]=0;
                }
                if(!menu_capture && policy.captures(buttons)) for(const auto& field:state.fields) if(field.axis && field.right_stick) {
                    const auto value=direct_axis(field,policy.axes);
                    std::memcpy(static_cast<unsigned char*>(data)+field.offset,&value,4);
                }
                const auto consumed=direct_classify(device,state,buttons,GetTickCount64());
                for(const auto& field:state.fields) if(field.pov && (buttons&consumed&15)) {
                    const auto neutral=direct_pov((buttons&15)&~consumed);
                    std::memcpy(static_cast<unsigned char*>(data)+field.offset,&neutral,4);
                }
            }
        }
    }
    SetLastError(error);return result;
}
template<unsigned Slot> HRESULT STDMETHODCALLTYPE direct_data(void* device,DWORD stride,DIDEVICEOBJECTDATA* data,DWORD* count,DWORD flags) {
    const auto capacity=count ? *count:0;
    const auto result=reinterpret_cast<DirectData>(direct_hooks[di_data][Slot].original)(device,stride,data,count,flags);
    const auto error=GetLastError();
    if(direct_internal || raw_controller_read) {SetLastError(error);return result;}
    if(FAILED(result)) {if(result==DIERR_INPUTLOST || result==DIERR_NOTACQUIRED) direct_reset(device);SetLastError(error);return result;}
    const auto policy=stick_input_policy();const bool menu_capture=panel_gameplay_blocked();
    if(result==DI_BUFFEROVERFLOW) direct_reset(device);
    if(!menu_capture && !policy.mask && !live_glances()) direct_reset(device);
    else if(data && count && *count<=capacity && stride>=4*sizeof(DWORD) && stride<=sizeof(DIDEVICEOBJECTDATA)) {
        std::lock_guard lock(direct_mutex);auto& state=direct_devices[device];direct_sync(state);direct_identify(device,state);
        if(state.sony && !state.size) {
            // A device opened before our adapters may have set its format
            // already, then use only buffered reads. Recover a standard format
            // only when both the state read and semantic offsets verify.
            auto** table=*reinterpret_cast<void***>(device);DirectInternal internal;
            std::array<unsigned char,sizeof(DIJOYSTATE2)> snapshot{};
            for(const DWORD size:{DWORD(sizeof(DIJOYSTATE2)),DWORD(sizeof(DIJOYSTATE))})
                if(SUCCEEDED(reinterpret_cast<DirectState>(table[9])(device,size,snapshot.data()))) {
                    direct_standard(device,state,size);break;
                }
        }
        if(state.mapped) {
            if(!state.buffered_known) {
                // Seed buttons from an independent snapshot. Replay queued button
                // transitions from their pre-batch state, even on the first read.
                std::vector<unsigned char> snapshot(state.size);
                auto** table=*reinterpret_cast<void***>(device);DirectInternal internal;
                if(SUCCEEDED(reinterpret_cast<DirectState>(table[9])(device,state.size,snapshot.data()))) {
                    state.buffered_buttons=direct_buttons(state,snapshot.data())&~15u;
                    for(DWORD i=*count;i>0;--i) {
                        DIDEVICEOBJECTDATA event{};std::memcpy(&event,reinterpret_cast<unsigned char*>(data)+(i-1)*stride,stride);
                        for(const auto& field:state.fields) if(!field.pov && !field.axis && event.dwOfs==field.offset)
                            state.buffered_buttons=(state.buffered_buttons&~field.mask)|((event.dwData&0x80) ? 0:field.mask);
                    }
                    state.buffered_known=true;
                }
            }
            std::vector<unsigned char> physical(state.size);
            bool restore=false,physical_valid=false;
            if(std::any_of(state.fields.begin(),state.fields.end(),[](const auto& field){return field.axis;})) {
                auto** table=*reinterpret_cast<void***>(device);DirectInternal internal;
                physical_valid=SUCCEEDED(reinterpret_cast<DirectState>(table[9])(device,state.size,physical.data()));
                if(physical_valid) {
                    const bool captured=menu_capture || policy.captures(direct_buttons(state,physical.data()));
                    restore=state.stick_captured && !captured;
                    state.stick_captured=captured;
                }
            }
            std::set<DWORD> restored;
            if(state.buffered_known) for(DWORD i=0;i<*count;++i) {
                auto* bytes=reinterpret_cast<unsigned char*>(data)+i*stride;
                DIDEVICEOBJECTDATA event{};std::memcpy(&event,bytes,stride);
                const auto field=std::find_if(state.fields.begin(),state.fields.end(),[&](const auto& value){return value.offset==event.dwOfs;});
                if(field!=state.fields.end() && field->axis) {
                    if(menu_capture) {
                        event.dwData=static_cast<DWORD>(field->trigger ? field->low:field->center);std::memcpy(bytes,&event,stride);
                    } else if(restore) {
                        std::memcpy(&event.dwData,physical.data()+field->offset,4);restored.insert(field->offset);
                        std::memcpy(bytes,&event,stride);
                    } else if(field->right_stick && policy.captures(state.buffered_buttons)) {
                        event.dwData=static_cast<DWORD>(direct_axis(*field,policy.axes));std::memcpy(bytes,&event,stride);
                    }
                    continue;
                }
                if(field==state.fields.end()) continue;
                const auto duplicate=std::find_if(state.events.begin(),state.events.end(),[&](const auto& old) {
                    return old.offset==event.dwOfs && old.data==event.dwData && old.time==event.dwTimeStamp && old.sequence==event.dwSequence;
                });
                if(duplicate!=state.events.end()) {std::memcpy(bytes+offsetof(DIDEVICEOBJECTDATA,dwData),&duplicate->edited,4);continue;}
                if(field->pov) state.buffered_buttons=(state.buffered_buttons&~15u)|direct_dpad(event.dwData);
                else state.buffered_buttons=(state.buffered_buttons&~field->mask)|((event.dwData&0x80)?field->mask:0);
                const auto consumed=direct_classify(device,state,state.buffered_buttons,direct_time(event.dwTimeStamp));
                DWORD edited=menu_capture ? (field->pov ? 0xffffffffu:0u):event.dwData;
                if(field->pov && (state.buffered_buttons&consumed&15)) edited=direct_pov((state.buffered_buttons&15)&~consumed);
                std::memcpy(bytes+offsetof(DIDEVICEOBJECTDATA,dwData),&edited,4);
                state.events.push_back({event.dwOfs,event.dwData,event.dwTimeStamp,event.dwSequence,edited});
                // Retain an entire peek batch, even when its configured buffer
                // exceeds the normal history budget; otherwise a repeated peek
                // would replay the beginning of that batch as fresh input.
                const auto history=std::max<std::size_t>(512,static_cast<std::size_t>(*count)*2);
                while(state.events.size()>history) state.events.pop_front();
            }
            // A modifier release need not generate a new axis event when the
            // physical stick is stationary. Send its complete current position.
            if(restore) for(const auto& field:state.fields) if(field.axis && !restored.contains(field.offset)) {
                DWORD value{};std::memcpy(&value,physical.data()+field.offset,4);
                state.restore_axes.push_back({field.offset,value,static_cast<DWORD>(GetTickCount64()),0,0});
            }
            const auto available=capacity-*count;
            const auto added=std::min<std::size_t>(available,state.restore_axes.size());
            for(std::size_t i=0;i<added;++i) std::memcpy(reinterpret_cast<unsigned char*>(data)+(*count+i)*stride,&state.restore_axes[i],stride);
            *count+=static_cast<DWORD>(added);
            if(!(flags&DIGDD_PEEK)) for(std::size_t i=0;i<added;++i) state.restore_axes.pop_front();
        }
    }
    SetLastError(error);return result;
}
template<unsigned Slot> HRESULT STDMETHODCALLTYPE direct_format(void* device,const DIDATAFORMAT* format) {
    const auto result=reinterpret_cast<DirectFormat>(direct_hooks[di_format][Slot].original)(device,format);const auto error=GetLastError();
    if(SUCCEEDED(result) && format && !direct_internal) {
        direct_reset(device);std::lock_guard lock(direct_mutex);auto& state=direct_devices[device];
        direct_identify(device,state);direct_map(device,state,*format);
    }
    SetLastError(error);return result;
}
template<unsigned Slot> HRESULT STDMETHODCALLTYPE direct_acquire(void* device) {
    const auto result=reinterpret_cast<DirectSimple>(direct_hooks[di_acquire][Slot].original)(device);const auto error=GetLastError();
    if(result==DI_OK && !direct_internal) direct_reset(device);SetLastError(error);return result;
}
template<unsigned Slot> HRESULT STDMETHODCALLTYPE direct_unacquire(void* device) {
    const auto result=reinterpret_cast<DirectSimple>(direct_hooks[di_unacquire][Slot].original)(device);const auto error=GetLastError();
    if(SUCCEEDED(result) && !direct_internal) direct_reset(device);SetLastError(error);return result;
}
template<unsigned Slot> ULONG STDMETHODCALLTYPE direct_release(void* device) {
    const auto result=reinterpret_cast<DirectRelease>(direct_hooks[di_release][Slot].original)(device);const auto error=GetLastError();
    if(!result && !direct_internal) {direct_reset(device);std::lock_guard lock(direct_mutex);direct_devices.erase(device);}
    SetLastError(error);return result;
}
bool direct_install_device(void* device,std::vector<void*>& pending);
bool direct_install_owner(void* owner,std::vector<void*>& pending);
void direct_enable(std::vector<void*>& pending) {
    using namespace dingosdk;
    bool ready=true;for(auto* target:pending) ready=(hook_queue_enable(target)==HookOk)&&ready;
    if(!pending.empty()) ready=(apply_owned_hooks()==HookOk)&&ready;
    if(!ready) log("DirectInput gesture coverage incomplete: dynamic hook installation failed.");
}
template<unsigned Slot> HRESULT STDMETHODCALLTYPE direct_device(void* owner,REFGUID guid,void** output,IUnknown* outer) {
    const auto result=reinterpret_cast<DirectDeviceCreate>(direct_hooks[di_device][Slot].original)(owner,guid,output,outer);const auto error=GetLastError();
    if(SUCCEEDED(result) && output && *output && !direct_internal) {
        std::lock_guard lock(direct_install_mutex);std::vector<void*> pending;
        if(!direct_install_device(*output,pending)) log("DirectInput gesture coverage incomplete: device hook slots unavailable.");
        direct_enable(pending);
    }
    SetLastError(error);return result;
}
template<unsigned Slot> HRESULT WINAPI direct_factory(HINSTANCE instance,DWORD version,REFIID iid,void** output,IUnknown* outer) {
    const auto result=reinterpret_cast<DirectFactory>(direct_hooks[di_factory][Slot].original)(instance,version,iid,output,outer);const auto error=GetLastError();
    if(SUCCEEDED(result) && output && *output && !direct_internal && (iid==IID_IDirectInput8A || iid==IID_IDirectInput8W)) {
        std::lock_guard lock(direct_install_mutex);std::vector<void*> pending;
        if(!direct_install_owner(*output,pending)) log("DirectInput gesture coverage incomplete: creation hook unavailable.");
        direct_enable(pending);
    }
    SetLastError(error);return result;
}
template<DirectMethod Method> bool direct_hook(void* entry,std::vector<void*>& pending) {
    using namespace dingosdk;
    const auto stock=resolve_stock_handler(reinterpret_cast<std::uintptr_t>(entry));
    auto* target=stock ? reinterpret_cast<void*>(stock):entry;
    auto& family=direct_hooks[Method];
    for(const auto& hook:family) if(hook.target==target) return true;
    static const auto detours=[]<std::size_t... I>(std::index_sequence<I...>) {
        if constexpr(Method==di_state) return std::array{reinterpret_cast<void*>(&direct_state<I>)...};
        else if constexpr(Method==di_data) return std::array{reinterpret_cast<void*>(&direct_data<I>)...};
        else if constexpr(Method==di_format) return std::array{reinterpret_cast<void*>(&direct_format<I>)...};
        else if constexpr(Method==di_acquire) return std::array{reinterpret_cast<void*>(&direct_acquire<I>)...};
        else if constexpr(Method==di_unacquire) return std::array{reinterpret_cast<void*>(&direct_unacquire<I>)...};
        else if constexpr(Method==di_release) return std::array{reinterpret_cast<void*>(&direct_release<I>)...};
        else if constexpr(Method==di_device) return std::array{reinterpret_cast<void*>(&direct_device<I>)...};
        else return std::array{reinterpret_cast<void*>(&direct_factory<I>)...};
    }(std::make_index_sequence<direct_slots>{});
    for(unsigned i=0;i<direct_slots;++i) if(!family[i].target) {
        if(owned_hook_prepare(target,detours[i],&family[i].original)!=HookOk) return false;
        family[i].target=target;pending.push_back(target);return true;
    }
    return false;
}
bool direct_install_device(void* device,std::vector<void*>& pending) {
    auto** table=*reinterpret_cast<void***>(device);
    // Prepare each method independently; a shared implementation is installed once.
    bool ready=direct_hook<di_state>(table[9],pending);
    ready=direct_hook<di_data>(table[10],pending)&&ready;
    ready=direct_hook<di_format>(table[11],pending)&&ready;
    ready=direct_hook<di_acquire>(table[7],pending)&&ready;
    ready=direct_hook<di_unacquire>(table[8],pending)&&ready;
    ready=direct_hook<di_release>(table[2],pending)&&ready;
    return ready;
}
bool direct_install_owner(void* owner,std::vector<void*>& pending) {
    return direct_hook<di_device>((*reinterpret_cast<void***>(owner))[3],pending);
}
struct DirectProbe { void* owner{};std::vector<void*>* pending{};bool ready=true; };
BOOL CALLBACK direct_probe(const DIDEVICEINSTANCEW* instance,void* context) {
    auto& probe=*static_cast<DirectProbe*>(context);void* device{};
    auto** table=*reinterpret_cast<void***>(probe.owner);
    if(SUCCEEDED(reinterpret_cast<DirectDeviceCreate>(table[3])(probe.owner,instance->guidInstance,&device,nullptr))) {
        probe.ready=direct_install_device(device,*probe.pending)&&probe.ready;
        reinterpret_cast<DirectRelease>((*reinterpret_cast<void***>(device))[2])(device);
    } else probe.ready=false;
    return DIENUM_CONTINUE;
}
bool prepare_direct_input(std::vector<void*>& pending) {
    std::lock_guard lock(direct_install_mutex);DirectInternal internal;
    // The proxy forwards this export to the system DLL. Keep a reference for
    // process lifetime because the method hooks retain its callable addresses.
    wchar_t directory[MAX_PATH]{};
    const auto length=GetSystemDirectoryW(directory,MAX_PATH);
    if(!length || length>=MAX_PATH) return false;
    const std::wstring path=std::wstring(directory)+L"\\dinput8.dll";
    const auto module=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!module) return false;
    const auto create=reinterpret_cast<DirectFactory>(GetProcAddress(module,"DirectInput8Create"));
    if(!create) return false;
    bool ready=direct_hook<di_factory>(reinterpret_cast<void*>(create),pending);
    for(const auto& iid:{IID_IDirectInput8A,IID_IDirectInput8W}) {
        void* owner{};
        if(FAILED(create(GetModuleHandleW(nullptr),DIRECTINPUT_VERSION,iid,&owner,nullptr))) {ready=false;continue;}
        ready=direct_install_owner(owner,pending)&&ready;
        DirectProbe probe{owner,&pending};
        auto** table=*reinterpret_cast<void***>(owner);
        using Enumerate=HRESULT(STDMETHODCALLTYPE*)(void*,DWORD,LPDIENUMDEVICESCALLBACKW,void*,DWORD);
        // The A/W instance records share GUID offsets. The callback reads only
        // guidInstance, so one callback safely probes both interface versions.
        if(FAILED(reinterpret_cast<Enumerate>(table[4])(owner,DI8DEVCLASS_GAMECTRL,direct_probe,&probe,DIEDFL_ATTACHEDONLY))) ready=false;
        ready=probe.ready&&ready;
        for(const auto& guid:{GUID_SysMouse,GUID_SysKeyboard}) {
            DIDEVICEINSTANCEW instance{};instance.guidInstance=guid;direct_probe(&instance,&probe);
        }
        ready=probe.ready&&ready;
        reinterpret_cast<DirectRelease>(table[2])(owner);
    }
    log(ready ? "DirectInput gesture adapters prepared; live Sony mapping/filtering is reported on device reads."
        : "DirectInput gesture adapters partially prepared; some device paths remain uncovered.");
    return ready;
}
