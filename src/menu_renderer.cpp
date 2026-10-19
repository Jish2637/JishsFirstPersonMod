#include "menu.h"
#include "stock_bridge.h"
#include "owned_hooks.h"
#include "bridge_resolver.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <imgui_impl_dx12.h>
#include <intrin.h>
#include <array>
#include <mutex>
#include <sstream>

namespace jishsfirstpersonmod {
using Microsoft::WRL::ComPtr;
namespace {
using Present=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*,UINT,UINT);
using Present1=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain1*,UINT,UINT,const DXGI_PRESENT_PARAMETERS*);
using Resize=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*,UINT,UINT,UINT,DXGI_FORMAT,UINT);
using Resize1=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain3*,UINT,UINT,UINT,DXGI_FORMAT,UINT,const UINT*,IUnknown* const*);
using Execute=void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*);
Present original_present{}; Present1 original_present1{};
Resize original_resize{}; Resize1 original_resize1{}; Execute original_execute{};
struct Frame { ComPtr<ID3D12Resource> buffer; ComPtr<ID3D12CommandAllocator> allocator; D3D12_CPU_DESCRIPTOR_HANDLE rtv{}; UINT64 fence{}; };
struct Renderer {
    std::mutex mutex;
    ComPtr<IDXGISwapChain3> chain;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12DescriptorHeap> rtvs,srvs;
    ComPtr<ID3D12GraphicsCommandList> commands;
    ComPtr<ID3D12Fence> fence;
    std::vector<Frame> frames;
    std::vector<UINT64> upload_fences;
    HANDLE fence_event{};
    ImGuiContext* context{};
    HWND window{};
    UINT64 next_fence{},draws{};
    bool backend{},failed{};
};
Renderer& state() { static auto* value=new Renderer; return *value; }
struct Presentation {
    IDXGISwapChain* chain{};
    bool visible{},drawing{},drawn{};
};
thread_local Presentation presentation;
struct StockSubmission { ComPtr<ID3D12CommandQueue> queue; ULONGLONG time{}; };
thread_local StockSubmission pending_stock;
bool same_device(IUnknown* first,IUnknown* second) {
    ComPtr<IUnknown> a,b;
    return first && second && SUCCEEDED(first->QueryInterface(IID_PPV_ARGS(&a))) &&
        SUCCEEDED(second->QueryInterface(IID_PPV_ARGS(&b))) && a==b;
}
bool focused_chain(IDXGISwapChain* chain) {
    DXGI_SWAP_CHAIN_DESC desc{}; DWORD pid{};
    if(FAILED(chain->GetDesc(&desc)) || !desc.OutputWindow) return false;
    GetWindowThreadProcessId(desc.OutputWindow,&pid);
    return pid==GetCurrentProcessId() && GetAncestor(desc.OutputWindow,GA_ROOT)==GetForegroundWindow();
}
void check(HRESULT hr,const char* step) {
    if(FAILED(hr)) { std::ostringstream error; error<<step<<" failed (0x"<<std::hex<<static_cast<unsigned long>(hr)<<")"; throw std::runtime_error(error.str()); }
}
bool wait(Renderer& s,UINT64 value) {
    if(!value || s.fence->GetCompletedValue()>=value) return true;
    return SUCCEEDED(s.fence->SetEventOnCompletion(value,s.fence_event)) && WaitForSingleObject(s.fence_event,1000)==WAIT_OBJECT_0;
}
bool release(Renderer& s) {
    cancel_panel_recording();
    if(s.fence && !wait(s,s.next_fence)) {
        // On device removal GPU work cannot finish; releasing removed-device
        // resources is safe. A live timeout retains them and disables drawing.
        if(s.device && s.device->GetDeviceRemovedReason()==S_OK) { s.failed=true; hide_panel(); return false; }
    }
    const auto previous=ImGui::GetCurrentContext();
    if(s.context) {
        ImGui::SetCurrentContext(s.context);
        if(s.backend) ImGui_ImplDX12_Shutdown();
        ImGui::DestroyContext(s.context);
        ImGui::SetCurrentContext(previous==s.context ? nullptr:previous);
    }
    s.backend=false; s.context=nullptr;
    s.frames.clear(); s.upload_fences.clear(); s.commands.Reset(); s.rtvs.Reset(); s.srvs.Reset();
    s.fence.Reset(); s.queue.Reset(); s.device.Reset(); s.chain.Reset();
    if(s.fence_event) CloseHandle(s.fence_event);
    s.fence_event=nullptr; s.next_fence=s.draws=0; s.window=nullptr;
    hide_panel(); return true;
}
void initialize(Renderer& s,IDXGISwapChain* candidate,ID3D12CommandQueue* queue) {
    if(!release(s)) throw std::runtime_error("Waiting for the previous UI GPU submission timed out.");
    ComPtr<IDXGISwapChain3> chain; check(candidate->QueryInterface(IID_PPV_ARGS(&chain)),"Swapchain interface");
    DXGI_SWAP_CHAIN_DESC desc{}; check(chain->GetDesc(&desc),"Swapchain description");
    if(desc.BufferCount<2 || desc.BufferCount>8 || queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)
        throw std::runtime_error("Unsupported UI swapchain/queue.");
    ComPtr<ID3D12Device> queue_device,chain_device;
    check(queue->GetDevice(IID_PPV_ARGS(&queue_device)),"Queue device");
    check(chain->GetDevice(IID_PPV_ARGS(&chain_device)),"Swapchain device");
    if(!same_device(queue_device.Get(),chain_device.Get())) throw std::runtime_error("The captured UI queue belongs to another device.");
    s.device=queue_device; s.chain=chain; s.queue=queue; s.window=desc.OutputWindow;
    D3D12_DESCRIPTOR_HEAP_DESC heap{}; heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV; heap.NumDescriptors=desc.BufferCount;
    check(s.device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&s.rtvs)),"RTV heap");
    heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; heap.NumDescriptors=1; heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    check(s.device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&s.srvs)),"SRV heap");
    const auto stride=s.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    auto handle=s.rtvs->GetCPUDescriptorHandleForHeapStart();
    s.frames.resize(desc.BufferCount); s.upload_fences.assign(desc.BufferCount,0);
    for(UINT i=0;i<desc.BufferCount;++i) {
        auto& frame=s.frames[i]; check(chain->GetBuffer(i,IID_PPV_ARGS(&frame.buffer)),"UI backbuffer");
        ComPtr<ID3D12Device> buffer_device; check(frame.buffer->GetDevice(IID_PPV_ARGS(&buffer_device)),"Backbuffer device");
        if(!same_device(buffer_device.Get(),s.device.Get()) || !(frame.buffer->GetDesc().Flags&D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET))
            throw std::runtime_error("Backbuffer ownership/usage rejected.");
        check(s.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&frame.allocator)),"UI allocator");
        frame.rtv=handle; s.device->CreateRenderTargetView(frame.buffer.Get(),nullptr,handle); handle.ptr+=stride;
    }
    check(s.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,s.frames[0].allocator.Get(),nullptr,IID_PPV_ARGS(&s.commands)),"UI command list");
    check(s.commands->Close(),"Close initial UI list"); s.commands->SetName(L"JishsFirstPersonMod overlay draw");
    check(s.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&s.fence)),"UI fence");
    s.fence_event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    if(!s.fence_event) throw std::runtime_error("UI fence event unavailable.");
    const auto previous=ImGui::GetCurrentContext();
    struct Restore { ImGuiContext* previous; ~Restore() { ImGui::SetCurrentContext(previous); } } restore{previous};
    s.context=ImGui::CreateContext(); ImGui::SetCurrentContext(s.context); ImGui::StyleColorsDark(); load_panel_fonts();
    ImGui_ImplDX12_InitInfo info{}; info.Device=s.device.Get(); info.CommandQueue=queue; info.NumFramesInFlight=static_cast<int>(desc.BufferCount);
    info.RTVFormat=s.frames[0].buffer->GetDesc().Format; info.SrvDescriptorHeap=s.srvs.Get();
    info.LegacySingleSrvCpuDescriptor=s.srvs->GetCPUDescriptorHandleForHeapStart();
    info.LegacySingleSrvGpuDescriptor=s.srvs->GetGPUDescriptorHandleForHeapStart();
    if(!ImGui_ImplDX12_Init(&info)) throw std::runtime_error("ImGui DX12 backend initialization failed.");
    s.backend=true; s.failed=false;
    route_panel_input(s.window);
    log("Companion controls panel bound to stock's verified swapchain/queue.");
}
void render_submission(IDXGISwapChain* chain,ID3D12CommandQueue* queue) noexcept {
    if(!presentation.visible || presentation.drawn || presentation.drawing || !focused_chain(chain)) return;
    presentation.drawing=true;
    struct Scope { ~Scope() { presentation.drawing=false; } } scope;
    auto& s=state(); std::lock_guard lock(s.mutex);
    try {
        if(s.failed) return;
        ComPtr<IDXGISwapChain3> extended; check(chain->QueryInterface(IID_PPV_ARGS(&extended)),"Present interface");
        if(!s.chain || !same_device(s.chain.Get(),extended.Get()) || s.queue.Get()!=queue) initialize(s,chain,queue);
        const auto index=s.chain->GetCurrentBackBufferIndex();
        if(index>=s.frames.size()) throw std::runtime_error("Invalid UI backbuffer index.");
        auto& frame=s.frames[index];
        const auto upload=static_cast<std::size_t>(s.draws%s.upload_fences.size());
        if(!wait(s,frame.fence) || !wait(s,s.upload_fences[upload])) throw std::runtime_error("UI GPU fence timed out.");
        const auto previous=ImGui::GetCurrentContext();
        struct Restore { ImGuiContext* previous; ~Restore() { ImGui::SetCurrentContext(previous); } } restore{previous};
        ImGui::SetCurrentContext(s.context);
        ImGui_ImplDX12_NewFrame(); draw_panel(s.window,true);
        const auto data=ImGui::GetDrawData();
        if(!data || data->TotalVtxCount==0) return;
        check(frame.allocator->Reset(),"UI allocator reset"); check(s.commands->Reset(frame.allocator.Get(),nullptr),"UI list reset");
        D3D12_RESOURCE_BARRIER barrier{}; barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource=frame.buffer.Get(); barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_PRESENT; barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_RENDER_TARGET;
        s.commands->ResourceBarrier(1,&barrier); s.commands->OMSetRenderTargets(1,&frame.rtv,FALSE,nullptr);
        ID3D12DescriptorHeap* heaps[]{s.srvs.Get()}; s.commands->SetDescriptorHeaps(1,heaps);
        ImGui_ImplDX12_RenderDrawData(data,s.commands.Get());
        std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter); s.commands->ResourceBarrier(1,&barrier);
        check(s.commands->Close(),"UI list close");
        ID3D12CommandList* lists[]{s.commands.Get()}; queue->ExecuteCommandLists(1,lists);
        const auto signal=++s.next_fence; check(queue->Signal(s.fence.Get(),signal),"UI fence signal");
        frame.fence=signal; s.upload_fences[upload]=signal; ++s.draws; presentation.drawn=true;
    } catch(const std::exception& e) { s.failed=true; hide_panel(); publish_menu_error(std::string("ERROR: companion menu: ")+e.what()); }
      catch(...) { s.failed=true; hide_panel(); log("ERROR: companion menu rendering failed."); }
}
bool stock_overlay_submission(UINT count,ID3D12CommandList* const* lists,void* caller) {
    // Streamline/driver wrappers forward the stock submission, changing the
    // immediate return address. Identify the stock list itself instead. The
    // renderer also verifies direct queue, focused swapchain and device identity.
    (void)caller;
    for(UINT i=0;i<count;++i) {
        std::array<wchar_t,128> name{}; UINT size=static_cast<UINT>(sizeof(name));
        if(lists[i] && SUCCEEDED(lists[i]->GetPrivateData(WKPDID_D3DDebugObjectNameW,&size,name.data())) &&
            size<sizeof(name) && wcscmp(name.data(),L"ReSkate overlay draw")==0) return true;
    }
    return false;
}
void STDMETHODCALLTYPE execute_hook(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* lists) {
    const auto caller=_ReturnAddress();
    original_execute(queue,count,lists);
    const auto error=GetLastError();
    static std::atomic<bool> reported{};
    if(!reported.exchange(true)) log("DX12 queue submission hook observed");
    if(compatibility::enabled() && !presentation.drawing && queue->GetDesc().Type==D3D12_COMMAND_LIST_TYPE_DIRECT && stock_overlay_submission(count,lists,caller)) {
        static std::atomic<bool> captured{};
        if(!captured.exchange(true)) log("Named ReSkate overlay submission captured through queue hook");
        pending_stock={queue,GetTickCount64()};
        if(presentation.chain) { render_submission(presentation.chain,queue); pending_stock.queue.Reset(); }
    }
    SetLastError(error);
}
template<class Invoke> HRESULT present_frame(IDXGISwapChain* chain,UINT flags,Invoke invoke) {
    if(!compatibility::enabled()) return invoke();
    if(presentation.chain || (flags&DXGI_PRESENT_TEST)) return invoke();
    poll_stock_camera();
    bool visible{}; const bool available=overlay_available(visible);
    if(focused_chain(chain)) {
        static bool reported{};
        if(!reported) { reported=true; log("Game Present observed; waiting for named overlay queue pairing"); }
    }
    presentation={chain,available&&visible&&panel_user_visible()&&focused_chain(chain),false,false};
    struct Scope { ~Scope() { presentation={}; } } scope;
    if(!presentation.visible) {
        hide_panel();
        auto& s=state(); std::lock_guard lock(s.mutex);
        if(s.context) { const auto previous=ImGui::GetCurrentContext(); ImGui::SetCurrentContext(s.context); draw_panel(s.window,false); ImGui::SetCurrentContext(previous); }
    }
    // A proxy may have executed stock's draw before reaching this underlying
    // Present entry. Consume only that thread's fresh, named stock submission.
    if(pending_stock.queue && GetTickCount64()-pending_stock.time<100 && focused_chain(chain)) {
        auto queue=pending_stock.queue; pending_stock.queue.Reset();
        render_submission(chain,queue.Get());
    }
    const auto result=invoke();
    if(result==DXGI_ERROR_DEVICE_REMOVED || result==DXGI_ERROR_DEVICE_RESET) {
        auto& s=state(); std::lock_guard lock(s.mutex); s.failed=false; release(s); log("UI device removed/reset; waiting for recreated swapchain.");
    }
    return result;
}
HRESULT STDMETHODCALLTYPE present_hook(IDXGISwapChain* chain,UINT interval,UINT flags) {
    return present_frame(chain,flags,[&] { return original_present(chain,interval,flags); });
}
HRESULT STDMETHODCALLTYPE present1_hook(IDXGISwapChain1* chain,UINT interval,UINT flags,const DXGI_PRESENT_PARAMETERS* parameters) {
    return present_frame(chain,flags,[&] { return original_present1(chain,interval,flags,parameters); });
}
void before_resize(IDXGISwapChain* chain) {
    auto& s=state(); std::lock_guard lock(s.mutex);
    if(s.chain && same_device(s.chain.Get(),chain)) {
        if(release(s)) s.failed=false;
    }
}
HRESULT STDMETHODCALLTYPE resize_hook(IDXGISwapChain* chain,UINT count,UINT width,UINT height,DXGI_FORMAT format,UINT flags) {
    before_resize(chain); return original_resize(chain,count,width,height,format,flags);
}
HRESULT STDMETHODCALLTYPE resize1_hook(IDXGISwapChain3* chain,UINT count,UINT width,UINT height,DXGI_FORMAT format,UINT flags,const UINT* nodes,IUnknown* const* queues) {
    before_resize(chain); return original_resize1(chain,count,width,height,format,flags,nodes,queues);
}
bool attach(void* target,void* replacement,void** original,const char* name) {
    using namespace dingosdk;
    const auto status=owned_hook_prepare(target,replacement,original);
    if(status!=HookOk || hook_queue_enable(target)!=HookOk) { log(std::string("ERROR: menu hook preparation failed: ")+name+" status="+std::to_string(status)); return false; }
    return true;
}
}
bool start_menu() {
    using namespace dingosdk;
    try {
        const auto initialized=hook_initialize();
        if(initialized!=HookOk && initialized!=HookAlreadyInitialized)
            throw std::runtime_error("Menu hook registry initialization failed.");
        ComPtr<ID3D12Device> device;
        check(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"DX12 method discovery device");
        D3D12_COMMAND_QUEUE_DESC qdesc{}; qdesc.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
        ComPtr<ID3D12CommandQueue> queue; check(device->CreateCommandQueue(&qdesc,IID_PPV_ARGS(&queue)),"Discovery queue");
        ComPtr<IDXGIFactory2> factory;
        using CreateFactory=HRESULT(WINAPI*)(REFIID,void**);
        const auto interposer=GetModuleHandleW(L"sl.interposer.dll");
        const auto create=interposer ? reinterpret_cast<CreateFactory>(GetProcAddress(interposer,"CreateDXGIFactory1")):nullptr;
        const auto hr=create ? create(IID_PPV_ARGS(&factory)):CreateDXGIFactory1(IID_PPV_ARGS(&factory));
        check(hr,"DXGI method discovery factory");
        WNDCLASSW klass{}; klass.lpfnWndProc=DefWindowProcW; klass.hInstance=GetModuleHandleW(nullptr); klass.lpszClassName=L"JishsFirstPersonMod.MethodDiscovery";
        RegisterClassW(&klass);
        const auto window=CreateWindowExW(0,klass.lpszClassName,L"",WS_POPUP,0,0,32,32,nullptr,nullptr,klass.hInstance,nullptr);
        if(!window) throw std::runtime_error("Hidden DXGI discovery window unavailable.");
        struct Window { HWND value; ~Window() { DestroyWindow(value); } } owned{window};
        DXGI_SWAP_CHAIN_DESC1 desc{}; desc.Width=32; desc.Height=32; desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count=1; desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT; desc.BufferCount=2; desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
        ComPtr<IDXGISwapChain1> chain; check(factory->CreateSwapChainForHwnd(queue.Get(),window,&desc,nullptr,nullptr,&chain),"Discovery swapchain");
        ComPtr<IDXGISwapChain3> extended; check(chain.As(&extended),"Discovery swapchain3");
        auto** table=*reinterpret_cast<void***>(extended.Get()); auto** queue_table=*reinterpret_cast<void***>(queue.Get());
        if(!attach(table[8],reinterpret_cast<void*>(&present_hook),reinterpret_cast<void**>(&original_present),"Present") ||
            !attach(table[22],reinterpret_cast<void*>(&present1_hook),reinterpret_cast<void**>(&original_present1),"Present1") ||
            !attach(table[13],reinterpret_cast<void*>(&resize_hook),reinterpret_cast<void**>(&original_resize),"ResizeBuffers") ||
            !attach(table[39],reinterpret_cast<void*>(&resize1_hook),reinterpret_cast<void**>(&original_resize1),"ResizeBuffers1") ||
            !attach(queue_table[10],reinterpret_cast<void*>(&execute_hook),reinterpret_cast<void**>(&original_execute),"ExecuteCommandLists")) return false;
        if(!install_panel_input_hooks()) throw std::runtime_error("Companion mouse polling isolation hook failed.");
        if(!initialize_panel_recording()) throw std::runtime_error("Verified controller recording readers unavailable.");
        const auto status=apply_owned_hooks();
        if(status!=HookOk) { log("ERROR: menu hook transaction failed: "+std::to_string(status)); return false; }
        log("Companion menu presentation hooks installed; waiting for stock's overlay draw.");
        return true;
    } catch(const std::exception& e) { log(std::string("ERROR: menu bootstrap: ")+e.what()); return false; }
}
}
