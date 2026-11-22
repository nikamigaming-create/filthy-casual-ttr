#pragma once
#include "equipment_wheel_store.hpp"
namespace wheelEquipment {
enum class Screen { Closed, Wheels, Profiles, Slots, Inventory };
struct Editor {
    Screen screen = Screen::Closed;
    int hand = 0, profile = 0, slot = 0, inventoryPage = 0;
    int active[2]{}, saved[2]{};
    int selected = 0;
    Layout draft{}, original{};
    std::vector<Item> owned;
    std::string status;
    fc_settings::View view{};
} editor;
unsigned nextEpoch = 0;
constexpr int itemsPerPage = 6;
int inventoryPages() {
    return std::max(1, int(editor.owned.size() + itemsPerPage - 1) / itemsPerPage);
}
void show(Screen screen, int row = 0) {
    editor.screen = screen;
    editor.selected = row;
    editor.status.clear();
}
void refreshProfiles() {
    for (int hand = 0; hand <= 1; ++hand) {
        editor.active[hand] = activeProfile(hand);
        editor.saved[hand] = profileMask(hand);
    }
}
void render() {
    using namespace fc_settings;
    auto& state = editor;
    auto& view = state.view;
    view = {};
    view.epoch = ++nextEpoch;
    view.page = WristEquipment;
    const auto row = [&](const char* label, const char* value, int kind = Action) {
        auto& line = view.rows[view.count++];
        strncpy_s(line.label,label,_TRUNCATE);
        strncpy_s(line.value,value,_TRUNCATE);
        line.kind = kind;
    };
    const auto title = [&](const char* text) { strncpy_s(view.title,text,_TRUNCATE); };
    const auto note = [&](const char* text) { strncpy_s(view.note,text,_TRUNCATE); };
    const char* wheelNames[] = {"Consumable wheel","Weapon wheel"};
    switch (state.screen) {
    case Screen::Wheels:
        title("EQUIPMENT WHEELS");
        for (int h : {1,0}) if (available(h)) row(wheelNames[h],fc::wheels::profileName(h,state.active[h]));
        break;
    case Screen::Profiles:
        title(wheelNames[state.hand]);
        for (int p = 0; p <= fc::wheels::profileCount; ++p) {
            const char* value = state.active[state.hand] == p ? "Active" :
                p == 0 ? "All carried items" : (state.saved[state.hand] & (1 << p)) ? "Saved" : "Empty";
            row(fc::wheels::profileName(state.hand,p),value);
        }
        break;
    case Screen::Slots:
        title(fc::wheels::profileName(state.hand,state.profile));
        for (int i = 0; i < fc::wheels::slotCount; ++i) {
            const auto item = state.draft[i];
            std::string name = item ? string(getName,item) : "Empty";
            row(("Slot " + std::to_string(i+1)).c_str(),name.c_str());
        }
        row("Save profile",state.draft != state.original ? "Edited" : "");
        row("Use profile","",state.draft == state.original && (state.saved[state.hand] & (1 << state.profile)) ? Action : Unavailable);
        break;
    case Screen::Inventory: {
        title(("Select item for slot " + std::to_string(state.slot+1)).c_str());
        row("Clear slot","");
        int first = state.inventoryPage * itemsPerPage;
        for (int i = first; i < std::min(first+itemsPerPage,int(state.owned.size())); ++i)
            row(state.owned[i].name.c_str(),("x" + std::to_string(state.owned[i].count)).c_str());
        if (inventoryPages() > 1) row("Inventory page",(std::to_string(state.inventoryPage+1) + " / " + std::to_string(inventoryPages())).c_str(),Number);
        if (state.owned.empty()) note("No carried items in this category.");
        break;
    }
    case Screen::Closed: break;
    }
    if (!state.status.empty()) note(state.status.c_str());
    state.selected = std::clamp(state.selected,0,std::max(0,int(view.count)-1));
    view.selected = state.selected;
}
bool preview(int hand,int profile,fc_equipment::LayoutView& out) {
    Layout layout{};
    bool draft=false;
    int focus=-1;
    if(hand<0){
        if(editor.screen==Screen::Closed)return false;
        hand=editor.hand;
        if(editor.screen==Screen::Wheels){
            hand=editor.selected==0 && available(1)?1:0;
            profile=editor.active[hand];
        }else if(editor.screen==Screen::Profiles)profile=editor.selected;
        else{
            draft=true;
            layout=editor.draft;
            if(editor.screen==Screen::Slots && editor.selected<fc::wheels::slotCount)focus=editor.selected;
            if(editor.screen==Screen::Inventory){
                focus=editor.slot;
                const int index=editor.inventoryPage*itemsPerPage+editor.selected-1;
                if(!editor.selected)layout[focus]=nullptr;
                else if(editor.selected<=itemsPerPage && index<int(editor.owned.size()))
                    fc::wheels::assign(layout,focus,editor.owned[index].form);
            }
        }
    }
    if(!available(hand))return false;
    if(!draft){
        if(!fc::wheels::validProfile(profile))return false;
        if(!profile){
            std::vector<Item> inventory;
            if(!readWheelInventory(hand,inventory,false))return false;
            for(size_t i=0;i<std::min(layout.size(),inventory.size());++i)layout[i]=inventory[i].form;
        }else if((profileMask(hand)&(1<<profile)) && !readProfile(hand,profile,layout))return false;
    }
    out={};
    out.kind=hand?fc_equipment::Kind::WeaponWheel:fc_equipment::Kind::AidWheel;
    out.count=unsigned(layout.size());
    out.focus=focus;
    for(unsigned i=0;i<out.count;++i){
        const float angle=2*pi*i/out.count;
        out.slots[i]={std::sin(angle),std::cos(angle),layout[i]?layout[i]->id:0};
    }
    return true;
}
void activate(int row, int direction, bool select) {
    auto& state = editor;
    int hand = state.hand;
    switch (state.screen) {
    case Screen::Wheels:
        if (select) {
            state.hand = row == 0 && available(1) ? 1 : 0;
            refreshProfiles(); show(Screen::Profiles);
        }
        break;
    case Screen::Profiles:
        if (!select) break;
        if (row == 0) {
            bool ok = useProfile(hand,0); refreshProfiles();
            state.status = ok ? "Wheel shows all carried items." : "Could not apply Automatic.";
        } else {
            state.profile = row;
            if ((state.saved[hand] & (1 << row)) && !readProfile(hand,row,state.draft)) {
                state.status = "Could not read saved slots."; break;
            }
            if (!(state.saved[hand] & (1 << row))) state.draft = {};
            state.original = state.draft; show(Screen::Slots);
        }
        break;
    case Screen::Slots:
        if (!select) break;
        if (row < fc::wheels::slotCount) {
            if (!readWheelInventory(hand,state.owned,false)) { state.status = "Inventory unavailable."; break; }
            state.slot = row; state.inventoryPage = 0; show(Screen::Inventory);
        } else if (row == fc::wheels::slotCount) {
            if (writeProfile(hand,state.profile,state.draft)) {
                state.original = state.draft; refreshProfiles(); state.status = "Profile saved.";
            } else state.status = "Could not save profile.";
        } else if (state.draft == state.original && useProfile(hand,state.profile)) {
            refreshProfiles(); show(Screen::Profiles,state.profile); state.status = "Profile active.";
        } else state.status = "Save this profile before using it.";
        break;
    case Screen::Inventory: {
        const bool pager = inventoryPages() > 1 && row == int(state.view.count)-1;
        if (pager) {
            state.inventoryPage = (state.inventoryPage + (direction < 0 ? -1 : 1) + inventoryPages()) % inventoryPages();
            state.selected = 0;
        } else if (select) {
            Form* item = row ? state.owned[state.inventoryPage * itemsPerPage + row-1].form : nullptr;
            fc::wheels::assign(state.draft,state.slot,item);
            state.owned.clear(); show(Screen::Slots,state.slot);
        }
        break;
    }
    case Screen::Closed: break;
    }
}
bool handle(unsigned cmd, int row, int direction, fc_settings::View* out, unsigned size) {
    using namespace fc_settings;
    if (cmd == DeviceClose) { editor = {}; return true; }
    if (cmd == DevicePresent) return available(0) || available(1);
    if (!out || size != sizeof(*out) || (!available(0) && !available(1))) { editor = {}; return false; }
    if (cmd == DeviceOpen) {
        editor = {}; refreshProfiles(); show(Screen::Wheels); render();
    } else if (editor.screen == Screen::Closed) return false;
    else if (editor.screen != Screen::Wheels && !available(editor.hand)) {
        editor = {}; refreshProfiles(); show(Screen::Wheels); render();
    } else if (cmd == DeviceBack) {
        switch (editor.screen) {
        case Screen::Wheels: editor = {}; return false;
        case Screen::Profiles: refreshProfiles(); show(Screen::Wheels); break;
        case Screen::Slots: show(Screen::Profiles,editor.profile); break;
        case Screen::Inventory: editor.owned.clear(); show(Screen::Slots,editor.slot); break;
        default: return false;
        }
        render();
    } else if (cmd == DeviceMove && editor.view.count && direction) {
        const int count = int(editor.view.count);
        editor.selected = (editor.selected + (direction > 0 ? 1 : -1) + count) % count;
        editor.view.selected = editor.selected; editor.view.epoch = ++nextEpoch;
    } else if ((cmd == DeviceSelect || cmd == DeviceAdjust) && row >= 0 && row < int(editor.view.count)) {
        editor.selected = row; activate(row,direction,cmd == DeviceSelect); render();
    }
    *out = editor.view;
    return true;
}
}
