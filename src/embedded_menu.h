#pragma once
#include "menu.h"
namespace jishsfirstpersonmod {
// Only POD arguments and borrowed opaque host objects cross this boundary.
struct EmbeddedWidgets {
    void (*begin_card)(void*,const char*,const char*,const char*);
    void (*end_card)();
    void (*field)(void*,const char*,const char*);
    void (*note)(const char*);
    bool (*toggle)(void*,const char*,const char*,bool*,bool,const char*);
    bool (*request_native)(int,bool,float);
    bool (*slider)(const char*,float*,float,float,const char*,ImGuiSliderFlags);
};
void draw_embedded_page(void* menu,const EmbeddedWidgets&,dingosdk::ControllerInput controller,bool focused);
void embedded_recording_state(bool active);
}
