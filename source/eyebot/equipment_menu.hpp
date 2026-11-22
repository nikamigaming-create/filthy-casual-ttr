#pragma once
#include "../settings_ui.hpp"
#include "equipment_kits.hpp"
#include "../equipment_layout_view.hpp"

namespace eyebot::equipment {
using Holster = unsigned (__cdecl*)(unsigned,unsigned);
enum class Screen { Home, Holsters, Wheels, Kits };

inline auto holsters() {
    const auto module=GetModuleHandleW(L"FilthyCasualPhysicalWorkshop.dll");
    return module?reinterpret_cast<Holster>(GetProcAddress(module,"FC_HolsterLayout")):nullptr;
}
inline auto wheels() {
    const auto module=GetModuleHandleW(L"SeatedControls.dll");
    return module?reinterpret_cast<fc_settings::DeviceSettings>(GetProcAddress(module,"FC_EquipmentWheels")):nullptr;
}
class Menu {
    Screen screen=Screen::Home;
    unsigned selected=0,profile=0,homeRow=0;
    const char* status="";
    struct Providers {
        Holster holster=holsters();
        fc_settings::DeviceSettings wheel=wheels();
        unsigned layout=holster?holster(0,0):0;
        bool wheelReady=wheel&&wheel(fc_settings::DevicePresent,0,0,nullptr,0);
        bool kitsReady() const {
            return layout&&wheelReady&&fc_equipment_kits::command(fc_settings::DevicePresent,0,0,nullptr,0);
        }
    };
    static unsigned home(const Providers& p,Screen (&screens)[3]){
        unsigned count=0;
        if(p.layout)screens[count++]=Screen::Holsters;
        if(p.wheelReady)screens[count++]=Screen::Wheels;
        if(p.kitsReady())screens[count++]=Screen::Kits;
        return count;
    }
    void editHolster(const Providers& p,unsigned row,int direction){
        const auto apply=p.holster;
        status="";
        switch(row){
        case 0: profile=(profile+(direction<0?2:1))%3;break;
        case 1: status=apply(2,profile)?"Layout loaded.":"Release both grips and holster your weapon to load.";break;
        case 2: status=apply(1,profile)?"Layout saved.":"Could not save this layout.";break;
        case 3: if(!apply(3,profile))status="Could not change previews.";break;
        case 4: if(!apply((p.layout&0x40u)?5:4,profile))status="Could not change marker visibility.";break;
        case 5: if(!apply(6,profile))status="Release both grips to change movement.";break;
        }
    }
public:
    bool present() const {const Providers p;return p.layout||p.wheelReady;}
    bool layoutView(const fc_settings::View& view,fc_equipment::LayoutView& layout) const {
        int hand=-1,wheelProfile=-1;
        unsigned holsterProfile=profile;
        bool showHolsters=screen==Screen::Holsters;
        if(screen==Screen::Kits){
            const auto& kit=fc_equipment_kits::currentEditor().draft;
            if(view.selected==1){hand=1;wheelProfile=kit.weapons;}
            else if(view.selected==2){hand=0;wheelProfile=kit.aid;}
            else if(view.selected==3){showHolsters=true;holsterProfile=kit.holsters;}
            else return false;
        }else if(!showHolsters && screen!=Screen::Wheels)return false;
        const auto module=GetModuleHandleW(showHolsters?L"FilthyCasualPhysicalWorkshop.dll":L"SeatedControls.dll");
        if(!module)return false;
        if(showHolsters){
            auto read=reinterpret_cast<fc_equipment::ReadHolsters>(GetProcAddress(module,"FC_HolsterLayoutView"));
            return read && read(holsterProfile,&layout,sizeof(layout));
        }
        auto read=reinterpret_cast<fc_equipment::ReadWheel>(GetProcAddress(module,"FC_WheelLayoutView"));
        return read && read(hand,wheelProfile,&layout,sizeof(layout));
    }
    bool command(unsigned op,int row,int direction,fc_settings::View* out,unsigned size){
        using namespace fc_settings;
        if(op==DevicePresent)return present();
        Providers p;
        Screen screens[3]{};
        const unsigned homes=home(p,screens);
        const bool delegated=screen==Screen::Kits||screen==Screen::Wheels;
        DeviceSettings delegate=screen==Screen::Kits?fc_equipment_kits::command:p.wheel;
        if(op==DeviceClose){
            if(delegated&&delegate)delegate(op,0,0,nullptr,0);
            screen=Screen::Home;homeRow=selected=0;return true;
        }
        if(!out||size!=sizeof(*out)||!homes)return false;
        if(op==DeviceOpen){screen=Screen::Home;homeRow=selected=0;status="";}
        else if(delegated){
            if(delegate&&delegate(op,row,direction,out,size)){out->page=WristEquipment;return true;}
            if(delegate)delegate(DeviceClose,0,0,nullptr,0);
            screen=Screen::Home;selected=homeRow;
        }else if(screen==Screen::Holsters&&!p.layout){screen=Screen::Home;selected=homeRow;}
        else if(op==DeviceBack){
            if(screen==Screen::Home)return false;
            screen=Screen::Home;selected=homeRow;
        }
        else if(op==DeviceMove){
            const int count=screen==Screen::Home?int(homes):6;
            selected=(int(selected)+direction%count+count)%count;status="";
        }else if(op==DeviceSelect||op==DeviceAdjust){
            if(screen==Screen::Home&&op==DeviceSelect&&row>=0&&unsigned(row)<homes){
                homeRow=row;screen=screens[row];selected=0;status="";
                if(screen==Screen::Holsters)profile=std::min((p.layout>>12)&3u,2u);
                else{
                    const auto open=screen==Screen::Kits?fc_equipment_kits::command:p.wheel;
                    if(open&&open(DeviceOpen,0,0,out,size)){out->page=WristEquipment;return true;}
                    screen=Screen::Home;selected=homeRow;
                }
            }else if(screen==Screen::Holsters&&row>=0&&row<6&&(op==DeviceSelect||row==0)){
                selected=row;editHolster(p,selected,direction);
            }
        }
        *out={};out->page=WristEquipment;
        auto add=[&](const char* label,const char* value,int kind=Action){
            auto& r=out->rows[out->count++];strcpy_s(r.label,label);strcpy_s(r.value,value);r.kind=kind;
        };
        if(screen==Screen::Home){
            strcpy_s(out->title,"EQUIPMENT");
            for(unsigned i=0;i<homes;i++)add(screens[i]==Screen::Holsters?"Holsters":screens[i]==Screen::Wheels?"Wheels":"Kits","OPEN");
        }else{
            const auto state=p.holster(0,0);
            static constexpr const char* names[]{"INFINITY","DIAMOND","ARC"};
            strcpy_s(out->title,"HOLSTER PROFILES");strcpy_s(out->note,status);
            add("Layout",names[profile],Number);
            add("Load layout","LOAD");add("Save current","SAVE");add("Previews",state&0x10?"ON":"OFF",Toggle);
            add("Edit markers",state&0x40?"ON":"OFF",Toggle);add("Grip movement",state&0x20?"LOCKED":"UNLOCKED",Toggle);
        }
        out->selected=selected=std::min(selected,out->count-1);
        return true;
    }
};
}
