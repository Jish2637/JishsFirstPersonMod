#include <Windows.h>
#define DIRECTINPUT_VERSION 0x800
#include <dinput.h>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {
    try {
        if(argc!=2) throw std::runtime_error("proxy path required");
        wchar_t system[MAX_PATH]{};GetSystemDirectoryW(system,MAX_PATH);
        const auto real=LoadLibraryW((std::wstring(system)+L"\\dinput8.dll").c_str());
        const auto proxy=LoadLibraryA(argv[1]);
        if(!real || !proxy) throw std::runtime_error("DLL load failed");
        const char* names[]{"DirectInput8Create","DllCanUnloadNow","DllGetClassObject","DllRegisterServer","DllUnregisterServer","GetdfDIJoystick"};
        for(unsigned i=0;i<6;++i)
            if(!GetProcAddress(proxy,names[i]) || GetProcAddress(proxy,names[i])!=GetProcAddress(proxy,MAKEINTRESOURCEA(i+1)))
                throw std::runtime_error("export name/ordinal mismatch");
        using Format=LPCDIDATAFORMAT(WINAPI*)();
        const auto a=reinterpret_cast<Format>(GetProcAddress(real,"GetdfDIJoystick"))();
        const auto b=reinterpret_cast<Format>(GetProcAddress(proxy,"GetdfDIJoystick"))();
        if(!a || a!=b) throw std::runtime_error("format did not forward to genuine DLL");
        using Create=HRESULT(WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
        const auto create=reinterpret_cast<Create>(GetProcAddress(proxy,"DirectInput8Create"));
        IDirectInput8W* input{};
        const auto hr=create(GetModuleHandleW(nullptr),DIRECTINPUT_VERSION,IID_IDirectInput8W,reinterpret_cast<void**>(&input),nullptr);
        if(FAILED(hr) || !input) throw std::runtime_error("genuine DirectInput object creation failed");
        input->Release();Sleep(250);
        if(GetModuleHandleW(L"JishsFirstPersonMod.dll")) throw std::runtime_error("unrelated host loaded companion");
        std::cout<<"All six named/ordinal exports present; native format and DirectInput creation forward; unrelated host refused.\n";
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
