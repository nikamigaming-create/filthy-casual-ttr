#pragma once

namespace workshop_profiles {
using workshop_native_holsters::Layout;
struct Profile {
    Layout layout{};
    u32 favorite[8]{};
    u32 saved = 0;
    u32 options = 1; // Bit 0 is the legacy previews flag. Bit 1 locks movement.
};
struct File {
    u32 magic = 0x314C4346;
    u32 selected = 0;
    Profile profile[3]{};
};
File store;
bool ready = false;
bool preview = true, movementLocked = false;
std::wstring path;
Script *getFavorite, *setFavorite, *clearFavorites, *getCount;

bool readFile() {
    wchar_t folder[MAX_PATH]{};
    if (FAILED(SHGetFolderPathW(nullptr,CSIDL_PERSONAL,nullptr,SHGFP_TYPE_CURRENT,folder))) return false;
    path = std::wstring(folder)+L"\\My Games\\FalloutNV\\FilthyCasualHolsterLayouts.dat";
    wchar_t fixture[MAX_PATH]{};
    const auto testLength = GetEnvironmentVariableW(L"SEATED_TEST_DIR",fixture,MAX_PATH);
    if (testLength && testLength < MAX_PATH)
        path = std::wstring(fixture)+L"\\physical-holster-layouts.dat";
    FILE* f = nullptr;
    if (!_wfopen_s(&f,path.c_str(),L"rb")) {
        File input;
        if (fread(&input,sizeof input,1,f)==1 && input.magic==store.magic && input.selected<3)
            store=input;
        fclose(f);
    }
    getFavorite=scripts->compile("int slot\nbegin function {slot}\nSetFunctionValue (GetHotkeyItem slot)\nend");
    setFavorite=scripts->compile("int slot\nref item\nbegin function {slot,item}\nSetHotkeyItem slot item\nSetFunctionValue 1\nend");
    clearFavorites=scripts->compile("begin function {}\nClearAllHotkeys\nSetFunctionValue 1\nend");
    getCount=scripts->compile("ref item\nbegin function {item}\nSetFunctionValue (player.GetItemCount item)\nend");
    const auto options=store.profile[store.selected].options;
    preview=(options&1) != 0;
    movementLocked=(options&2) != 0;
    return getFavorite && setFavorite && clearFavorites && getCount && workshop_native_holsters::lockMovement(movementLocked);
}

bool saveFile() {
    const auto temporary = path+L".tmp";
    FILE* f = nullptr;
    if (_wfopen_s(&f,temporary.c_str(),L"wb")) return false;
    const bool complete=fwrite(&store,sizeof store,1,f)==1;
    const bool closed=fclose(f)==0;
    const bool moved=complete && closed && MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    if (!moved) DeleteFileW(temporary.c_str());
    return moved;
}

void captureFavorites(Profile& profile) {
    for (unsigned i=0;i<8;++i) {
        const auto item=form(call(getFavorite,nullptr,i+1));
        profile.favorite[i]=item ? item->id : 0;
    }
}

bool restoreFavorites(const Profile& profile) {
    if (number(call(clearFavorites,nullptr))!=1) return false;
    for (unsigned i=0;i<8;++i) {
        const auto item=id(profile.favorite[i]);
        if (item && number(call(getCount,nullptr,item))>0 &&
            number(call(setFavorite,nullptr,i+1,item))!=1) return false;
    }
    return true;
}

bool seed(unsigned shape, Layout& layout) {
    if (!workshop_native_holsters::read(layout)) return false;
    // Body-relative native offsets, in metres. Every starter shape uses
    // the chest anchor so grips and arm movements leave the layout stable.
    constexpr float shapes[3][8][2]{
        {{-.25f,-.245f},{-.155f,-.147f},{-.155f,-.343f},{0,-.201f},{.155f,-.147f},{.25f,-.245f},{.155f,-.343f},{0,-.289f}},
        {{0,-.10f},{.108f,-.17f},{.215f,-.24f},{.108f,-.31f},{0,-.38f},{-.108f,-.31f},{-.215f,-.24f},{-.108f,-.17f}},
        {{-.32f,-.32f},{-.24f,-.25f},{-.148f,-.20f},{-.05f,-.17f},{.05f,-.17f},{.148f,-.20f},{.24f,-.25f},{.32f,-.32f}}
    };
    // Read the native game-to-tracking basis. This includes the player's
    // configured world scale and matches native anchor values in FNU.
    const auto p = pooladapter::tracking({0,0,0});
    const float step = scene::distance(p,pooladapter::tracking({0,0,10}));
    if (!std::isfinite(step) || step < .03f || step > 1.f) return false;
    const float scale = 10.f/step;
    for (unsigned i=0;i<8;++i) {
        const auto right=shapes[shape][i][0], up=shapes[shape][i][1];
        layout.point[i]={.46f,right,up,.048f};
        layout.anchor[i]={1,{(up+.4114f)*scale,.5243f*scale,-right*scale},0};
        // Slot activation belongs to TTR's hotkey wheel. Choosing a geometric
        // starter layout only changes where its eight slots are positioned.
    }
    return true;
}

// A legacy forearm slot can be centered inside the elbow. Move its native
// grip volume and preview together when the player applies that layout.
void clearForearmSleeves(Layout& layout) {
    auto rig=scene::root(id(0x14));
    auto head=scene::find(rig,"Bip01 Head");
    if(!head)return;
    auto dot=[](scene::Vec a,scene::Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;};
    const float step=scene::distance(pooladapter::tracking({}),pooladapter::tracking({0,0,10}));
    if(!(step>.03f && step<1.f))return;
    for(unsigned i=0;i<8;++i){
        auto& a=layout.anchor[i];
        if(!layout.enabled[i] || a.rebind || (a.bone!=8 && a.bone!=9))continue;
        auto hand=scene::find(rig,a.bone==8?"Bip01 L Hand":"Bip01 R Hand");
        scene::Transform bone{};
        if(!hand || !workshop_native_holsters::bonePose(a.bone,bone))continue;
        auto arm=scene::world(hand).p-bone.p;
        const float length=scene::distance(arm,{});
        if(!(length>1.f))continue;
        arm=arm*(1.f/length);
        const auto offset=scene::rotate(bone,a.offset);
        const float along=dot(offset,arm);
        if(along<-.15f*length || along>length)continue;
        auto radial=offset-arm*along;
        const float distance=scene::distance(radial,{});
        const float radius=10.f/step*(.073f+.65f*std::clamp(layout.point[i].radius,.02f,.12f));
        if(!(distance<radius-.01f))continue;
        if(distance<1.f){
            const auto eye=scene::world(head).p-bone.p;
            radial=eye-arm*dot(eye,arm);
            if(scene::distance(radial,{})<.01f){
                const scene::Vec up=std::abs(arm.z)<.9f?scene::Vec{0,0,1}:scene::Vec{0,1,0};
                radial=up-arm*dot(up,arm);
            }
        }
        const float norm=scene::distance(radial,{});
        if(!(norm>.01f))continue;
        const auto exterior=arm*std::max(along,.30f*length)+radial*(radius/norm);
        a.offset=scene::inverseRotate(bone,exterior);
    }
}

unsigned operate(unsigned command,unsigned selected) {
    if (!ready) ready=readFile();
    if (!ready || selected >= 3) return 0;
    if (command==0) {
        unsigned state = 0x100u | (store.selected << 12) | (preview ? 0x10u : 0) | (movementLocked ? 0x20u : 0)
            | (workshop_native_holsters::markersVisible() ? 0x40u : 0);
        for (unsigned i=0;i<3;++i) if (store.profile[i].saved) state |= 1u << i;
        return state;
    }
    auto updated=store.profile[selected];
    if (command==1) { // Save current native edits and favorites in this profile.
        if (!workshop_native_holsters::read(updated.layout)) return 0;
        captureFavorites(updated);
        updated.saved=1;
        updated.options=(preview?1u:0u)|(movementLocked?2u:0u);
    } else if (command==2) { // Apply or start this shape live.
        if (!updated.saved && !seed(selected,updated.layout)) return 0;
        if (!updated.saved) captureFavorites(updated);
        if (grips.load() || number(call(drawn,nullptr))) return 0;
        Profile previous;
        if (!workshop_native_holsters::read(previous.layout)) return 0;
        captureFavorites(previous);
        clearForearmSleeves(updated.layout);
        if (!workshop_native_holsters::apply(updated.layout)) return 0;
        updated.saved=1;
        const bool nextLock=(updated.options&2)!=0;
        auto restoreLive=[&] {
            workshop_native_holsters::apply(previous.layout);
            restoreFavorites(previous);
            workshop_native_holsters::lockMovement(movementLocked);
        };
        if (!restoreFavorites(updated) || !workshop_native_holsters::lockMovement(nextLock)) {
            restoreLive();
            return 0;
        }
        const auto priorStore=store;
        store.selected=selected;
        store.profile[selected]=updated;
        if (!saveFile()) {
            store=priorStore;
            restoreLive();
            return 0;
        }
        preview=(updated.options&1)!=0;
        movementLocked=nextLock;
        return 1;
    } else if (command==3) {
        auto& options=store.profile[store.selected].options;
        options^=1u;
        if (!saveFile()) { options^=1u; return 0; }
        preview=(options&1)!=0;
        return preview ? 2u : 1u;
    } else if (command==4 || command==5) {
        return workshop_native_holsters::showMarkers(command==4) ? 1u : 0u;
    } else if (command==6) {
        if (grips.load() || !workshop_native_holsters::lockMovement(!movementLocked)) return 0;
        auto& options=store.profile[store.selected].options;
        options^=2u;
        if (!saveFile()) {
            options^=2u;
            workshop_native_holsters::lockMovement(movementLocked);
            return 0;
        }
        movementLocked=!movementLocked;
        return movementLocked ? 2u : 1u;
    } else return 0;
    const auto priorProfile=store.profile[selected];
    store.profile[selected]=updated;
    if (saveFile()) return 1;
    store.profile[selected]=priorProfile;
    return 0;
}
} // namespace workshop_profiles
