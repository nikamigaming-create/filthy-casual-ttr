#pragma once
#include <cstdint>
#include "../settings_ui.hpp"

namespace eyebot::wrist_frame {
constexpr uint32_t Width = 256, Height = 256;
constexpr unsigned Visible = 1, Focused = 2;
enum FilthyPage : uint8_t { NoPage, General, Eyebot, Voice, Pool, Workshop, Equipment };
enum PointerTarget : uint8_t { PointerMiss, CategoryTab, GeneralTab, EyebotTab, VoiceTab, PoolTab, WorkshopTab, EquipmentTab,
    Row0,Row1,Row2,Row3,Row4,BackButton,LessButton,MoreButton,StockStatsTab,StockItemsTab,StockDataTab };
constexpr unsigned StockTabTargets = 0x200;
// Logical centers match FilthyPages in the native 960 x 768 menu.
constexpr PointerTarget fullMenuPointer(float u,float v,uint8_t tabs=0x7e,bool compact=false,bool stockTabs=false) {
    if (v >= .87f && v <= .94f) {
        if (u >= .805f && u <= .94f) return CategoryTab;
        if (stockTabs) {
            if (u >= .25f && u <= .375f) return StockStatsTab;
            if (u >= .435f && u <= .56f) return StockItemsTab;
            if (u >= .62f && u <= .745f) return StockDataTab;
        }
    }
    const float x=u*960.f,y=v*768.f;
    if(x>=60 && x<=(compact?590:900) && y>=145 && y<452){
        const int offset=int(y-145);
        if(offset%63<55)return PointerTarget(Row0+offset/63);
    }
    if(y>=578 && y<=618){
        if(x>=85 && x<=215)return BackButton;
        if(x>=332 && x<=418)return LessButton;
        if(x>=448 && x<=533)return MoreButton;
    }
    if (!(v >= .81f && v <= .86f)) return PointerMiss;
    constexpr PointerTarget targets[]{GeneralTab, EyebotTab, VoiceTab, PoolTab, WorkshopTab, EquipmentTab};
    unsigned count=0,visible=0;
    for(unsigned i=0;i<6;++i)if(tabs&(1u<<(1+i)))++count;
    for(unsigned i=0;i<6;++i)if(tabs&(1u<<(1+i))){
        const float center=(.5f+visible++)/count;
        if(u>center-.075f && u<center+.075f)return targets[i];
    }
    return PointerMiss;
}
constexpr uint8_t bit(FilthyPage page){return uint8_t(1u<<page);}
constexpr FilthyPage nextPage(FilthyPage current, uint8_t available, int direction = 1) {
    constexpr FilthyPage order[]{General, Eyebot, Voice, Pool, Workshop, Equipment};
    constexpr unsigned count = sizeof(order) / sizeof(order[0]);
    for (unsigned i = 0; i < count; ++i) {
        if (order[i] != current) continue;
        for (unsigned offset = 1; offset < count; ++offset) {
            const auto next = order[(i + (direction < 0 ? count - offset : offset)) % count];
            if (available & bit(next)) return next;
        }
    }
    return NoPage;
}
struct View {
    uint32_t size = sizeof(View), generation{};
    int selection{};
    uint32_t hasStill{}, count{}, confirm{}, library{}, piloting{};
    uint32_t holsters{}, profile{}, holsterRow{}, holsterState{}, holsterMarkers{};
    bool settings=false;
    FilthyPage nextFilthyPage=NoPage;
    FilthyPage activeFilthyPage=NoPage;
    uint8_t availableFilthyPages=0;
    fc_settings::View settingsView{};
    char status[44]{};
    char hint[56]{};
    char clip[32]{};
};
// MainLoop publishes value copies. The XR thread reports only screen focus
// and copies the latest view on a content change.
using Exchange = unsigned (__cdecl*)(unsigned, View*, unsigned char*, unsigned);
}
