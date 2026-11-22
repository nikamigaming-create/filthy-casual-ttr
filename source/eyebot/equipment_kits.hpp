#pragma once

#include "../settings_ui.hpp"
#include "../equipment_wheel_profiles.hpp"
#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <optional>
#include <string>

namespace fc_equipment_kits {
// Kits contain links. Item lists and physical positions stay with their owners.
struct Links {
    int weapons = 0, aid = 0, holsters = 0;
    bool operator==(const Links& rhs) const {
        return weapons == rhs.weapons && aid == rhs.aid && holsters == rhs.holsters;
    }
    bool valid() const {
        return weapons >= 0 && weapons < 4 && aid >= 0 && aid < 4 && holsters >= 0 && holsters < 3;
    }
};

struct Providers {
    int (__cdecl* wheel)(int);
    bool (__cdecl* useWheel)(int,int);
    unsigned (__cdecl* holster)(unsigned,unsigned);
    int (__cdecl* savedWheels)(int);
    static Providers find() {
        const auto core = GetModuleHandleW(L"SeatedControls.dll");
        const auto workshop = GetModuleHandleW(L"FilthyCasualPhysicalWorkshop.dll");
        return {
            core ? reinterpret_cast<decltype(wheel)>(GetProcAddress(core,"FC_WheelProfile")) : nullptr,
            core ? reinterpret_cast<decltype(useWheel)>(GetProcAddress(core,"FC_UseWheelProfile")) : nullptr,
            workshop ? reinterpret_cast<decltype(holster)>(GetProcAddress(workshop,"FC_HolsterLayout")) : nullptr,
            core ? reinterpret_cast<decltype(savedWheels)>(GetProcAddress(core,"FC_WheelProfiles")) : nullptr
        };
    }
    std::optional<Links> current() const {
        if (!wheel || !useWheel || !holster || !savedWheels) return {};
        const unsigned state = holster(0,0);
        if (!(state & 0x100)) return {};
        const Links links{wheel(1),wheel(0),int((state >> 12) & 3)};
        return links.valid() ? std::optional<Links>(links) : std::nullopt;
    }
    bool configured(const Links& kit) const {
        return kit.valid() && (savedWheels(1) & (1 << kit.weapons)) &&
               (savedWheels(0) & (1 << kit.aid));
    }
};

enum class Apply { Done, Unavailable, MissingWheel, HolsterBusy, RestoreFailed };
inline Apply apply(const Links& kit,const Providers& host) {
    const auto previous = host.current();
    if (!previous || !kit.valid()) return Apply::Unavailable;
    if (!host.configured(kit)) return Apply::MissingWheel;
    const bool weaponsChanged = kit.weapons != previous->weapons;
    const bool aidChanged = kit.aid != previous->aid;
    if (weaponsChanged && !host.useWheel(1,kit.weapons)) return Apply::MissingWheel;
    if (aidChanged && !host.useWheel(0,kit.aid)) {
        if (weaponsChanged && !host.useWheel(1,previous->weapons)) return Apply::RestoreFailed;
        return Apply::MissingWheel;
    }
    // Always reload the stored positions and favorites, even for the current layout.
    if (host.holster(2,kit.holsters)) return Apply::Done;
    const bool aidRestored = !aidChanged || host.useWheel(0,previous->aid);
    const bool weaponsRestored = !weaponsChanged || host.useWheel(1,previous->weapons);
    return aidRestored && weaponsRestored ? Apply::HolsterBusy : Apply::RestoreFailed;
}

inline std::filesystem::path storePath() {
    wchar_t dir[MAX_PATH]{};
    const auto count = GetEnvironmentVariableW(L"SEATED_TEST_DIR",dir,MAX_PATH);
    if (count && count < MAX_PATH) return std::filesystem::path(dir)/L"EquipmentKits.ini";
    if (FAILED(SHGetFolderPathW(nullptr,CSIDL_PERSONAL,nullptr,SHGFP_TYPE_CURRENT,dir))) return {};
    return std::filesystem::path(dir)/L"My Games"/L"FalloutNV"/L"FilthyCasualEquipmentKits.ini";
}
inline std::optional<Links> load(const std::filesystem::path& path,int slot) {
    if (slot < 0 || slot >= 3 || path.empty()) return {};
    wchar_t value[16]{};
    const auto key = L"Kit"+std::to_wstring(slot+1);
    if (GetPrivateProfileStringW(L"Equipment",key.c_str(),L"",value,16,path.c_str()) != 5 ||
        value[1] != L',' || value[3] != L',') return {};
    Links links{int(value[0]-L'0'),int(value[2]-L'0'),int(value[4]-L'0')};
    return links.valid() ? std::optional<Links>(links) : std::nullopt;
}
inline bool save(const std::filesystem::path& path,int slot,const Links& links) {
    if (slot < 0 || slot >= 3 || path.empty() || !links.valid()) return false;
    const auto key = L"Kit"+std::to_wstring(slot+1);
    const auto value = std::to_wstring(links.weapons)+L","+std::to_wstring(links.aid)+L","+std::to_wstring(links.holsters);
    return WritePrivateProfileStringW(L"Equipment",key.c_str(),value.c_str(),path.c_str()) != FALSE;
}
struct Editor {
    int slot=0, selected=0;
    Links draft{};
    std::optional<Links> saved;
    const char* note="";
    void browse(const Links& active) {
        saved=load(storePath(),slot);
        draft=saved.value_or(active);
        note=saved ? "" : "Choose profiles, then save this kit.";
    }
    void select(int row,int step,const Links& active,const Providers& host) {
        selected=row;
        note="";
        switch (row) {
        case 0: slot=(slot+step+3)%3; browse(active); break;
        case 1: draft.weapons=(draft.weapons+step+4)%4; break;
        case 2: draft.aid=(draft.aid+step+4)%4; break;
        case 3: draft.holsters=(draft.holsters+step+3)%3; break;
        case 4:
            if (!host.configured(draft)) note="Save the selected wheel profile first.";
            else if (save(storePath(),slot,draft)) { saved=draft; note="Kit saved."; }
            else note="Could not save kit.";
            break;
        case 5:
            if (!saved || !(*saved == draft)) { note="Save this selection first."; break; }
            switch (apply(*saved,host)) {
            case Apply::Done: note="Kit applied."; break;
            case Apply::MissingWheel: note="Save the selected wheel profile first."; break;
            case Apply::HolsterBusy: note="Holster unavailable. Stow your weapon and release grips."; break;
            case Apply::RestoreFailed: note="Could not restore previous profiles. Check Equipment."; break;
            default: note="Equipment unavailable."; break;
            }
            break;
        }
    }
    void view(fc_settings::View& out) const {
        out={};
        out.page=fc_settings::WristEquipment;
        out.count=6;
        out.selected=selected;
        strcpy_s(out.title,"EQUIPMENT KITS");
        strcpy_s(out.note,note);
        constexpr const char* labels[]{"Kit","Weapon wheel","Consumables","Holsters","Save kit","Apply kit"};
        constexpr const char* layouts[]{"Infinity","Diamond","Arc"};
        for (int i=0;i<6;++i) strcpy_s(out.rows[i].label,labels[i]);
        sprintf_s(out.rows[0].value,"Kit %d",slot+1);
        strcpy_s(out.rows[1].value,fc::wheels::profileName(1,draft.weapons));
        strcpy_s(out.rows[2].value,fc::wheels::profileName(0,draft.aid));
        strcpy_s(out.rows[3].value,layouts[draft.holsters]);
        const bool matches=saved && *saved == draft;
        strcpy_s(out.rows[4].value,matches?"Saved":saved?"Edited":"New");
        strcpy_s(out.rows[5].value,matches?"Ready":"Save first");
        if (!matches) out.rows[5].kind=fc_settings::Unavailable;
    }
};
inline Editor& currentEditor(){static Editor editor;return editor;}
inline bool command(unsigned action,int row,int direction,fc_settings::View* out,unsigned size) {
    using namespace fc_settings;
    if (action==DeviceClose || action==DeviceBack) return false;
    const auto host=Providers::find();
    const auto active=host.current();
    if (!active) return false;
    if (action==DevicePresent) return true;
    if (!out || size < sizeof(View)) return false;
    auto& editor=currentEditor();
    const int step=direction<0 ? -1 : 1;
    switch(action) {
    case DeviceOpen: editor.selected=0; editor.browse(*active); break;
    case DeviceRead: break;
    case DeviceMove: editor.selected=(editor.selected+step+6)%6; break;
    case DeviceSelect: case DeviceAdjust:
        if (row>=0 && row<6 && (action==DeviceSelect || row<4))
            editor.select(row,step,*active,host);
        break;
    default: return false;
    }
    editor.view(*out);
    return true;
}
} // namespace fc_equipment_kits
