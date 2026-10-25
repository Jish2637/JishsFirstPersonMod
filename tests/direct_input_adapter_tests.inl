struct FakeDirect {
    void** table{};
    bool sony=true,ansi=false,axes=false;
    DWORD l1=52,pov=32,cross=49,r1=53;bool rb{};
    std::vector<unsigned char> physical=std::vector<unsigned char>(sizeof(DIJOYSTATE2),0);
    std::vector<DIDEVICEOBJECTDATA> queued;
    HRESULT state_result=DI_OK,data_result=DI_OK;
    ULONG references=1;
};
HRESULT STDMETHODCALLTYPE fake_direct_property(void* pointer,REFGUID property,DIPROPHEADER* header) {
    if(&property==&DIPROP_RANGE && static_cast<FakeDirect*>(pointer)->axes) {
        auto& range=*reinterpret_cast<DIPROPRANGE*>(header);range.lMin=0;range.lMax=65535;return DI_OK;
    }
    if(&property!=&DIPROP_VIDPID) return DIERR_UNSUPPORTED;
    reinterpret_cast<DIPROPDWORD*>(header)->dwData=static_cast<FakeDirect*>(pointer)->sony ? 0x0ce6054c:0x028e045e;
    return DI_OK;
}
HRESULT STDMETHODCALLTYPE fake_direct_object(void* pointer,void* output,DWORD offset,DWORD how) {
    auto& device=*static_cast<FakeDirect*>(pointer);
    const DWORD size=*static_cast<DWORD*>(output);
    if(how!=DIPH_BYOFFSET || size!=(device.ansi?sizeof(DIDEVICEOBJECTINSTANCEA):sizeof(DIDEVICEOBJECTINSTANCEW))) return DIERR_INVALIDPARAM;
    DWORD type{};WORD page{},usage{};
    if(offset==device.pov) {type=DIDFT_POV;page=1;usage=0x39;}
    else if(offset==device.l1) {type=DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(4);page=9;usage=5;}
    else if(offset==device.r1) {type=DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(5);page=9;usage=6;}
    else if(offset==device.cross) {type=DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(1);page=9;usage=2;}
    else if(device.axes && (offset==offsetof(DIJOYSTATE,lZ) || offset==offsetof(DIJOYSTATE,lRz))) {
        type=DIDFT_ABSAXIS;page=1;usage=offset==offsetof(DIJOYSTATE,lZ) ? 0x32:0x35;
    }
    else return DIERR_OBJECTNOTFOUND;
    if(device.ansi) {auto& value=*static_cast<DIDEVICEOBJECTINSTANCEA*>(output);value.dwType=type;value.wUsagePage=page;value.wUsage=usage;}
    else {auto& value=*static_cast<DIDEVICEOBJECTINSTANCEW*>(output);value.dwType=type;value.wUsagePage=page;value.wUsage=usage;}
    return DI_OK;
}
HRESULT STDMETHODCALLTYPE fake_direct_state(void* pointer,DWORD size,void* output) {
    auto& device=*static_cast<FakeDirect*>(pointer);SetLastError(77);
    if(FAILED(device.state_result)) return device.state_result;
    if(size>device.physical.size() || !output) return DIERR_INVALIDPARAM;
    std::memcpy(output,device.physical.data(),size);return device.state_result;
}
HRESULT STDMETHODCALLTYPE fake_direct_data(void* pointer,DWORD stride,DIDEVICEOBJECTDATA* output,DWORD* count,DWORD flags) {
    auto& device=*static_cast<FakeDirect*>(pointer);SetLastError(77);
    if(FAILED(device.data_result)) return device.data_result;
    if(!count) return DIERR_INVALIDPARAM;
    const auto n=std::min<DWORD>(*count,static_cast<DWORD>(device.queued.size()));
    if(output) for(DWORD i=0;i<n;++i) std::memcpy(reinterpret_cast<unsigned char*>(output)+i*stride,&device.queued[i],stride);
    *count=n;
    if(!(flags&DIGDD_PEEK)) device.queued.erase(device.queued.begin(),device.queued.begin()+n);
    return device.data_result;
}
HRESULT STDMETHODCALLTYPE fake_direct_format(void*,const DIDATAFORMAT*) {SetLastError(77);return DI_OK;}
HRESULT STDMETHODCALLTYPE fake_direct_acquire(void*) {SetLastError(77);return DI_OK;}
ULONG STDMETHODCALLTYPE fake_direct_release(void* pointer) {return --static_cast<FakeDirect*>(pointer)->references;}

void test_direct_input_adapters(bool windows_drivers) {
    std::array<void*,32> table{};
    table[5]=reinterpret_cast<void*>(&fake_direct_property);
    table[9]=reinterpret_cast<void*>(&direct_state<0>);
    table[14]=reinterpret_cast<void*>(&fake_direct_object);
    direct_hooks[di_state][0].original=reinterpret_cast<void*>(&fake_direct_state);
    direct_hooks[di_data][0].original=reinterpret_cast<void*>(&fake_direct_data);
    direct_hooks[di_format][0].original=reinterpret_cast<void*>(&fake_direct_format);
    direct_hooks[di_acquire][0].original=reinterpret_cast<void*>(&fake_direct_acquire);
    direct_hooks[di_unacquire][0].original=reinterpret_cast<void*>(&fake_direct_acquire);
    direct_hooks[di_release][0].original=reinterpret_cast<void*>(&fake_direct_release);
    dingosdk::FirstPersonLookControls bindings;for(unsigned i=0;i<4;++i) bindings[i].combo=0x100|(1u<<i);
    DWORD toggle{},native_toggle{};std::uintptr_t context=900;
    auto publish=[&] {publish_glance_filter(bindings,qa_active,{1,2,3,4},0x100,toggle,toggle || native_toggle,context,native_toggle);};
    auto reset=[&] {clear_glance_filter();direct_devices.clear();qa_focus=qa_active=true;qa_menu=qa_spectator=false;toggle=native_toggle=0;qa_now+=1000;publish();};
    auto physical=[&](FakeDirect& device,DWORD hat,bool l1=true) {
        std::fill(device.physical.begin(),device.physical.end(),0x33);
        device.physical[device.l1]=l1?0x80:0;
        device.physical[device.r1]=device.rb?0x80:0;
        device.physical[device.cross]=0x80;
        std::memcpy(device.physical.data()+device.pov,&hat,4);
    };
    auto poll=[&](FakeDirect& device,DWORD hat,bool l1=true,DWORD size=sizeof(DIJOYSTATE2)) {
        physical(device,hat,l1);qa_now+=10;publish();std::vector<unsigned char> output(size);
        SetLastError(123);check(direct_state<0>(&device,size,output.data())==DI_OK && GetLastError()==77,"DirectInput state API result/error changed");
        DWORD edited{};std::memcpy(&edited,output.data()+device.pov,4);
        std::memcpy(output.data()+device.pov,&hat,4);
        check(std::equal(output.begin(),output.end(),device.physical.begin()),"DirectInput changed unrelated buttons/axes");
        return edited;
    };
    auto buffered=[&](FakeDirect& device,DWORD flags=0,DWORD stride=sizeof(DIDEVICEOBJECTDATA)) {
        std::vector<unsigned char> output(device.queued.size()*stride);
        DWORD count=static_cast<DWORD>(device.queued.size());publish();
        check(direct_data<0>(&device,stride,reinterpret_cast<DIDEVICEOBJECTDATA*>(output.data()),&count,flags)==device.data_result && GetLastError()==77,"DirectInput buffered API result/error changed");
        std::vector<DIDEVICEOBJECTDATA> events(count);
        for(DWORD i=0;i<count;++i) std::memcpy(&events[i],output.data()+i*stride,stride);
        return events;
    };
    auto event=[&](FakeDirect& device,DWORD offset,DWORD data,DWORD time,DWORD sequence) {
        device.queued.push_back({offset,data,time,sequence,0x1234});
    };
    for(bool ansi:{false,true}) for(DWORD size:{DWORD(sizeof(DIJOYSTATE)),DWORD(sizeof(DIJOYSTATE2))}) {
        reset();FakeDirect device;device.table=table.data();device.ansi=ansi;
        check(poll(device,0,true,size)==0xffffffffu,"Sony standard first glance leaked");
        check(poll(device,0,true,size)==0xffffffffu && !glance_emote_active(),"Sony standard first hold became emote");
        poll(device,0xffffffffu,true,size);check(poll(device,0,true,size)==0 && glance_emote_active(),"Sony standard double tap blocked");
        check(poll(device,0,true,size)==0,"Sony standard second hold blocked");
        poll(device,0xffffffffu,true,size);check(!glance_emote_active(),"Sony standard release stuck");
        qa_now+=260;check(poll(device,0,true,size)==0xffffffffu,"Late Sony standard tap bypassed");
    }
    reset();FakeDirect custom;custom.table=table.data();custom.ansi=true;custom.l1=3;custom.pov=12;custom.cross=5;
    std::array<DIOBJECTDATAFORMAT,3> objects{};
    objects[0].dwOfs=custom.l1;objects[1].dwOfs=custom.pov;objects[2].dwOfs=custom.cross;
    DIDATAFORMAT format{sizeof(DIDATAFORMAT),sizeof(DIOBJECTDATAFORMAT),DIDF_ABSAXIS,24,3,objects.data()};
    check(direct_format<0>(&custom,&format)==DI_OK && GetLastError()==77,"Custom format API changed");
    check(poll(custom,0,true,24)==0xffffffffu,"Custom Sony POV offset missed");
    poll(custom,0xffffffffu,true,24);check(poll(custom,0,true,24)==0,"Custom Sony double tap failed");
    // Overlapping toggle/glance hotkey has no double-tap escape, in either camera.
    for(bool active:{false,true}) {
        reset();qa_active=active;toggle=0x101;publish();FakeDirect device;device.table=table.data();
        check(poll(device,0)==0xffffffffu,"DirectInput enter/exit hotkey gesture leaked");
        poll(device,0xffffffffu);check(poll(device,0)==0xffffffffu && !glance_emote_active(),"DirectInput toggle double tap escaped");
        qa_active=!active;qa_spectator=true;qa_now+=1200;
        check(poll(device,0,false)==0xffffffffu,"Camera transition/L1-first release lost hotkey latch");
        poll(device,0xffffffffu,false);check(poll(device,0,false)==0,"Hotkey release did not clear latch");
        qa_spectator=false;
    }
    for(bool active:{false,true}) {
        reset();qa_active=active;native_toggle=0x302;FakeDirect native;native.table=table.data();native.rb=true;
        check(poll(native,18000)==0xffffffffu,"native-owned DirectInput enter/exit toggle leaked");
        qa_active=!active;native.rb=false;
        check(poll(native,18000,false)==0xffffffffu,"native DirectInput shoulder-first release leaked");
        poll(native,0xffffffffu,false);native.rb=true;
        check(poll(native,18000)==0xffffffffu && !glance_emote_active(),"native DirectInput double tap leaked");
        raw_controller_read=true;check(poll(native,18000)==18000,"native DirectInput binding sampler was blocked");raw_controller_read=false;
        reset();qa_active=active;native_toggle=0x302;FakeDirect batch;batch.table=table.data();batch.rb=true;physical(batch,18000);
        event(batch,batch.l1,0x80,static_cast<DWORD>(qa_now),1);event(batch,batch.r1,0x80,static_cast<DWORD>(qa_now),2);
        event(batch,batch.pov,18000,static_cast<DWORD>(qa_now),3);
        const auto peek=buffered(batch,DIGDD_PEEK);
        check(peek.size()==3 && peek.back().dwData==0xffffffffu,"native buffered DirectInput toggle leaked");
        check(buffered(batch).back().dwData==0xffffffffu,"native buffered peek replay leaked");
    }
    reset();toggle=0x1101;publish();FakeDirect extended;extended.table=table.data();
    check(poll(extended,0)==0xffffffffu,"Additional Cross hotkey button was not mapped");
    reset();FakeDirect device;device.table=table.data();poll(device,0xffffffffu);
    physical(device,0);qa_now+=50;event(device,device.pov,0,static_cast<DWORD>(qa_now),1);
    auto first=buffered(device,DIGDD_PEEK);check(first[0].dwData==0xffffffffu,"Buffered first glance leaked");
    const auto peek=buffered(device,DIGDD_PEEK);check(peek[0].dwData==first[0].dwData && !glance_emote_active(),"Repeated peek advanced tap");
    check(buffered(device)[0].dwData==0xffffffffu,"Peek then consume changed first tap");
    qa_now+=10;physical(device,0xffffffffu);event(device,device.pov,0xffffffffu,static_cast<DWORD>(qa_now),2);buffered(device);
    qa_now+=10;physical(device,0);event(device,device.pov,0,static_cast<DWORD>(qa_now),3);
    check(buffered(device,0,16)[0].dwData==0 && glance_emote_active(),"DX3 buffered double tap blocked");
    check(poll(device,0)==0,"Mixed buffered/immediate second hold lost");
    // Snapshot decisions must also cover later delivery of the same queued events.
    reset();FakeDirect mixed;mixed.table=table.data();poll(mixed,0xffffffffu);
    poll(mixed,0);const DWORD t1=static_cast<DWORD>(qa_now);
    poll(mixed,0xffffffffu);const DWORD t2=static_cast<DWORD>(qa_now);
    poll(mixed,0);const DWORD t3=static_cast<DWORD>(qa_now);
    event(mixed,mixed.pov,0,t1,10);event(mixed,mixed.pov,0xffffffffu,t2,11);event(mixed,mixed.pov,0,t3,12);
    const auto history=buffered(mixed);check(history[0].dwData==0xffffffffu && history[1].dwData==0xffffffffu && history[2].dwData==0,"Mixed historical buffered decisions changed");
    // One batch can contain both taps, and its timestamp spacing controls pairing.
    for(DWORD spacing:{100u,300u}) {
        reset();FakeDirect batch;batch.table=table.data();poll(batch,0xffffffffu);physical(batch,0);
        const auto start=static_cast<DWORD>(qa_now+10);qa_now+=spacing+20;
        event(batch,batch.pov,0,start,1);event(batch,batch.pov,0xffffffffu,start+10,2);event(batch,batch.pov,0,start+spacing,3);
        const auto result=buffered(batch);check(result[0].dwData==0xffffffffu && result[2].dwData==(spacing<=250?0:0xffffffffu),"Buffered timestamps did not control double tap");
    }
    reset();toggle=0x101;publish();FakeDirect batch;batch.table=table.data();poll(batch,0xffffffffu);
    physical(batch,0,false);qa_now+=40;
    event(batch,batch.pov,0,static_cast<DWORD>(qa_now-30),1);
    event(batch,batch.l1,0,static_cast<DWORD>(qa_now-20),2);
    event(batch,batch.pov,0,static_cast<DWORD>(qa_now-10),3);
    const auto toggle_batch=buffered(batch);check(toggle_batch[0].dwData==0xffffffffu && toggle_batch[1].dwData==0 && toggle_batch[2].dwData==0xffffffffu,"Buffered hotkey latch lost after L1 release");
    // Unknown identities/formats and private reads remain transparent.
    reset();FakeDirect xbox;xbox.table=table.data();xbox.sony=false;check(poll(xbox,0)==0,"DirectInput Xbox was changed");
    reset();FakeDirect unknown;unknown.table=table.data();unknown.l1=260;physical(unknown,0,false);
    std::vector<unsigned char> output(sizeof(DIJOYSTATE2));
    direct_state<0>(&unknown,static_cast<DWORD>(output.size()),output.data());DWORD hat{};std::memcpy(&hat,output.data()+unknown.pov,4);check(hat==0,"Unknown mapping was edited");
    reset();FakeDirect private_pad;private_pad.table=table.data();raw_controller_read=true;
    check(poll(private_pad,0)==0,"Private DirectInput reader filtered");raw_controller_read=false;
    check(poll(private_pad,0)==0xffffffffu,"Normal read not filtered after private read");
    qa_focus=false;check(poll(private_pad,0)==0,"Focus loss continued filtering");qa_focus=true;
    check(poll(private_pad,0)==0xffffffffu,"Focus restore retained a stale double tap");
    private_pad.state_result=DIERR_INPUTLOST;
    check(direct_state<0>(&private_pad,sizeof(DIJOYSTATE2),output.data())==DIERR_INPUTLOST,"Input loss HRESULT changed");
    private_pad.state_result=DI_OK;direct_acquire<0>(&private_pad);check(poll(private_pad,0)==0xffffffffu,"Acquire retained stale tap");
    poll(private_pad,0xffffffffu);direct_unacquire<0>(&private_pad);check(poll(private_pad,0)==0xffffffffu,"Unacquire retained stale tap");
    direct_release<0>(&private_pad);check(!direct_devices.contains(&private_pad),"Released COM device retained input state");
    reset();FakeDirect buffered_only;buffered_only.table=table.data();physical(buffered_only,0);
    qa_now+=10;event(buffered_only,buffered_only.pov,0,static_cast<DWORD>(qa_now),1);
    check(buffered(buffered_only)[0].dwData==0xffffffffu,"Buffered-only pre-existing Sony device was missed");
    reset();
    std::cout<<"DirectInput synthetic adapter checks passed\n";
    if(!windows_drivers) return;
    // Exercise discovery and shared A/W hook preparation against Windows' real
    // DirectInput objects. No acquisition or controller input is required.
    direct_hooks={};std::vector<void*> pending;
    check(prepare_direct_input(pending) && !pending.empty(),"Windows DirectInput adapter discovery failed");
    const auto count=pending.size();check(prepare_direct_input(pending) && pending.size()==count,"A/W shared hook addresses were not deduplicated");
    direct_enable(pending);
    wchar_t system[MAX_PATH]{};GetSystemDirectoryW(system,MAX_PATH);
    const auto library=GetModuleHandleW((std::wstring(system)+L"\\dinput8.dll").c_str());
    const auto factory=reinterpret_cast<DirectFactory>(GetProcAddress(library,"DirectInput8Create"));
    for(const auto& iid:{IID_IDirectInput8A,IID_IDirectInput8W}) {
        void* owner{};check(SUCCEEDED(factory(GetModuleHandleW(nullptr),DIRECTINPUT_VERSION,iid,&owner,nullptr)),"Hooked Windows DirectInput factory failed");
        auto** owner_table=*reinterpret_cast<void***>(owner);void* mouse{};
        check(SUCCEEDED(reinterpret_cast<DirectDeviceCreate>(owner_table[3])(owner,GUID_SysMouse,&mouse,nullptr)),"Hooked Windows DirectInput device creation failed");
        auto** mouse_table=*reinterpret_cast<void***>(mouse);
        std::array<unsigned char,sizeof(DIJOYSTATE2)> state{};
        const auto result=reinterpret_cast<DirectState>(mouse_table[9])(mouse,static_cast<DWORD>(state.size()),state.data());
        check(FAILED(result),"Unacquired Windows mouse unexpectedly acquired by adapters");
        reinterpret_cast<DirectRelease>(mouse_table[2])(mouse);reinterpret_cast<DirectRelease>(owner_table[2])(owner);
    }
    check(dingosdk::rollback_owned_hooks(),"DirectInput prepared hook rollback failed");direct_hooks={};
    std::cout<<"DirectInput A/W state, custom formats, buffered peeks/timestamps, mixed reads, enter/exit hotkeys and Windows hook discovery passed\n";
}
