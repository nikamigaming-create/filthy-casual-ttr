#pragma once

namespace eyebot::native_tabs {

// TTR 1.1.7 has separate selectors for the world Pip-Boy and the worn screen.
// Attach to the already-qualified tab gesture in each selector. TTR retains
// its next/previous and page gestures before and after the fourth category.
inline unsigned char* ttr{};
inline std::atomic<int> fullMenu{}, request{};
constexpr int enterWorn=1,changeWornPage=2,enterFull=3,previousWornPage=4,exitWornBase=10,exitFullBase=20;
enum class Selection { Stock, WornFilthy, FullFilthy };
inline std::atomic<Selection> selection{Selection::Stock};
inline std::atomic_bool wornAvailable{};
// Keep the request published while the main loop opens or closes the page.
struct PendingTransition {
    const int value=request.load();
    ~PendingTransition(){
        int expected=value;
        if(value)request.compare_exchange_strong(expected,0);
    }
};
inline bool ownsWornInput(){
    const int pending=request.load();
    return selection==Selection::WornFilthy||pending==enterWorn||pending==changeWornPage||pending==previousWornPage||
        (pending>=exitWornBase&&pending<exitWornBase+3);
}
inline bool ownsFullInput(){
    const int pending=request.load();
    return selection==Selection::FullFilthy||pending==enterFull||(pending>=exitFullBase&&pending<exitFullBase+3);
}
inline std::atomic_bool installed{false};
constexpr int fullCategories[]{1003,1002,1023};
constexpr int wornCategories[]{1,0,2};
constexpr int filthyWornCategory = 3;
inline int previousWornCategory = 2;

inline int wornCategory() {
    return reinterpret_cast<int(__cdecl*)()>(ttr+0x11E640)();
}

inline void setWornCategory(int target) {
    reinterpret_cast<void(__cdecl*)(int)>(ttr+0x13AF90)(target);
}

inline void leaveWorn() {
    if(installed && wornCategory()==filthyWornCategory) setWornCategory(previousWornCategory);
}

using PagePainter = void (*)(uint32_t*, int, int);
inline PagePainter paintFilthy = nullptr;

inline void __cdecl drawWornPage(void* owner,uint32_t* pixels,int width,int height,void* theme) {
    if(wornCategory()==filthyWornCategory && paintFilthy && pixels && width==512 && height==384) {
        paintFilthy(pixels,width,height);
        return;
    }
    reinterpret_cast<void(__cdecl*)(void*,uint32_t*,int,int,void*)>(ttr+0xDA99)(owner,pixels,width,height,theme);
}

inline int indexOf(const int (&categories)[3],int value){
    for(int i=0;i<3;++i)if(categories[i]==value)return i;
    return -1;
}

inline bool route(const int (&categories)[3],int current,int target,bool selected,int exitBase){
    const int from=indexOf(categories,current),to=indexOf(categories,target);
    if(from<0||to<0||from==to)return false;
    const bool next=(from+1)%3==to;
    if(request.load()!=0)return true;
    if(selected){
        request=exitBase+(next?0:2);
        return true;
    }
    if((next&&from==2)||(!next&&from==0)){
        request=exitBase==exitWornBase?enterWorn:enterFull;
        return true;
    }
    return false;
}

// TTR checks a closed list to decide whether to submit its full-size surface.
inline bool __cdecl categoryVisible(int category){
    if(category!=full_page::id)
        return reinterpret_cast<bool(__cdecl*)(int)>(ttr+0x35AF90)(category);
    auto manager=full_page::gameRoot();
    if(!manager || !full_page::root || full_page::registered(category)!=full_page::root)return false;
    auto stack=&scene::field<int>(manager,0x114);
    return std::find(stack,stack+10,category)!=stack+10;
}
inline bool __cdecl lastVisiblePipboyCategory(int original){
    return categoryVisible(original) || categoryVisible(full_page::id);
}
inline bool removePriorCategory(int previous){
    auto tracked=scene::field<int>(ttr,0x19F5DE0);
    if(tracked==previous)return reinterpret_cast<bool(__cdecl*)(const char*,bool)>(ttr+0x35C5E0)("FILTHY",false);
    if(tracked)return false;
    return reinterpret_cast<int(__thiscall*)(void*,int,bool)>(0x714FD0)(full_page::gameRoot(),previous,false)>=0;
}
inline void publishFullCategory(){
    scene::field<int>(ttr,0x19F5DE8)=0;
    scene::field<int>(ttr,0x19F5DE0)=full_page::id;
    reinterpret_cast<void(__cdecl*)(int,const char*)>(ttr+0x35D600)(full_page::id,"FILTHY");
}
inline void __cdecl switchFull(int target,bool byTab){
    const int current=fullMenu.load();
    if(byTab && current==full_page::id){
        int idle=0;
        if(target==fullCategories[2] || target==fullCategories[0])
            request.compare_exchange_strong(idle,exitFullBase+(target==fullCategories[2]?0:2));
        return;
    }
    if(byTab&&current&&route(fullCategories,current,target,selection==Selection::FullFilthy,exitFullBase))return;
    reinterpret_cast<void(__cdecl*)(int,bool)>(ttr+0x35CAD0)(target,byTab);
}

inline void __cdecl switchFullFromStockTab(int target,bool byTab){
    // The stock tab click can arrive after FILTHY has replaced the visible
    // tabs. The separately qualified category grip still uses switchFull.
    if(ownsFullInput())return;
    reinterpret_cast<void(__cdecl*)(int,bool)>(ttr+0x35CAD0)(target,byTab);
}

// TTR chooses between its worn Pip-Boy and the wrist HUD here. A qualified
// tab gesture comes from TTR directly, independent of compositor layers.
inline bool wornPipboyActive(){
    return installed&&scene::field<int>(ttr,0x12484A8)==1;
}

// TTR 1.1.7 samples its navigation predicate at 0x13C9D9. This is the
// same result tested before the stock Stats, Items and Data input dispatch.
inline bool wornNavigationActive(){
    return installed && scene::field<unsigned char>(ttr,0x124A13A)!=0;
}

inline void __cdecl switchWorn(int target){
    if(wornAvailable&&wornPipboyActive()){
        const int current=wornCategory();
        // For an extended category the three-way stock iterator computes
        // its target as if starting at STATS. Decode the two grip directions.
        if(current==filthyWornCategory){
            if(!request && target==wornCategories[1])request=exitWornBase;
            else if(!request && target==wornCategories[2])request=exitWornBase+2;
            return;
        }
        if(route(wornCategories,current,target,false,exitWornBase)){
            if(request==enterWorn){
                previousWornCategory=current;
                setWornCategory(filthyWornCategory);
            }
            return;
        }
    }
    reinterpret_cast<void(__cdecl*)(int)>(ttr+0x13AF90)(target);
}

// TTR calls this branch for a grip edge without its category modifier.
// Its Data page handler can open the map, so a selected FILTHY owns the
// entire page event, even when the optional Eyebot page is unavailable.
inline bool __cdecl switchWornPage(int direction){
    if(wornPipboyActive() && ownsWornInput()){
        int expected=0;
        return direction!=0&&selection==Selection::WornFilthy&&wornPipboyActive()&&
            request.compare_exchange_strong(expected,direction>0?changeWornPage:previousWornPage);
    }
    return reinterpret_cast<bool(__cdecl*)(int)>(ttr+0x13C270)(direction);
}

inline void __cdecl dispatchWornData(void* screen,void* input,bool activate,bool secondary,bool left,bool right){
    // The two categories share a dispatch site. The stock DATA handler
    // can promote a long activation on a quest to the Map independently of
    // the grip-page handler. The page owner handles the events in FILTHY.
    if(wornPipboyActive() && (wornCategory()==filthyWornCategory || ownsWornInput()))return;
    reinterpret_cast<void(__cdecl*)(void*,void*,bool,bool,bool,bool)>(ttr+0x13D8C0)
        (screen,input,activate,secondary,left,right);
}

inline void finishExit(int transition,bool onArm,int full){
    if(transition>=exitWornBase&&transition<exitWornBase+3&&onArm){
        const int target=wornCategories[transition-exitWornBase];
        if(target!=reinterpret_cast<int(__cdecl*)()>(ttr+0x11E640)())
            reinterpret_cast<void(__cdecl*)(int)>(ttr+0x13AF90)(target);
    }else if(transition>=exitFullBase&&transition<exitFullBase+3&&full){
        const int target=fullCategories[transition-exitFullBase];
        if(target!=full)reinterpret_cast<void(__cdecl*)(int,bool)>(ttr+0x35CAD0)(target,true);
    }
}

inline void __cdecl drawWornLights(void* model){
    reinterpret_cast<void(__cdecl*)(void*)>(ttr+0xFFC30)(model);
    if(selection!=Selection::WornFilthy||!wornPipboyActive()||!model)return;
    // These are the same three scene nodes TTR just published. Use the
    // native recursive cull setter so render flags follow the active tab.
    auto stock=reinterpret_cast<void**>(ttr+0x12485FC);
    const auto cull=reinterpret_cast<void(__fastcall*)(void*,void*,bool)>(0x450F90);
    for(int i=0;i<3;++i)if(stock[i]&&!(scene::field<unsigned>(stock[i],0x30)&1u))
        cull(stock[i],nullptr,true);
}

inline void install(PagePainter painter){
    if(installed)return;
    ttr=build_adapter::heldModule();
    if(!ttr)return;
    struct Patch{unsigned offset,original;uintptr_t replacement;DWORD protection;unsigned char* base=ttr;};
    // Stock routes 0 to inventory, 1 to stats and 2 to DATA. Route 3 to
    // our category-aware dispatcher as well. Verify the entire comparison.
    const unsigned char inputBranch[]{0x83,0xf8,0x02,0x75,0x15};
    if(std::memcmp(ttr+0x13CB75,inputBranch,sizeof(inputBranch)))return;
    Patch patches[]{
        {0xA1F188,0x707990,reinterpret_cast<uintptr_t>(&full_page::create),0,nullptr},
        {0x35EADD,0x35AF90,reinterpret_cast<uintptr_t>(&categoryVisible),0},
        {0x35D606,0x35AF90,reinterpret_cast<uintptr_t>(&categoryVisible),0},
        {0x35AC82,0x35AF90,reinterpret_cast<uintptr_t>(&lastVisiblePipboyCategory),0},
        {0x101C39,0xDA99,reinterpret_cast<uintptr_t>(&drawWornPage),0},
        {0x35B058,0x35CAD0,reinterpret_cast<uintptr_t>(&switchFull),0},
        {0x35A699,0x35CAD0,reinterpret_cast<uintptr_t>(&switchFullFromStockTab),0},
        {0x13CA61,0x13AF90,reinterpret_cast<uintptr_t>(&switchWorn),0},
        {0x13CA91,0x13C270,reinterpret_cast<uintptr_t>(&switchWornPage),0},
        {0x13CB88,0x13D8C0,reinterpret_cast<uintptr_t>(&dispatchWornData),0},
        {0x10263F,0xFFC30,reinterpret_cast<uintptr_t>(&drawWornLights),0}
    };
    for(const auto& patch:patches){
        const auto site=reinterpret_cast<unsigned char*>(reinterpret_cast<uintptr_t>(patch.base)+patch.offset);
        uint32_t displacement;
        std::memcpy(&displacement,site+1,sizeof(displacement));
        if(*site!=0xE8||patch.offset+5+displacement!=patch.original)return;
    }
    size_t protectedCount=0;
    for(auto& patch:patches){
        if(!VirtualProtect(reinterpret_cast<unsigned char*>(reinterpret_cast<uintptr_t>(patch.base)+patch.offset),5,PAGE_EXECUTE_READWRITE,&patch.protection))break;
        ++protectedCount;
    }
    DWORD branchProtection;
    if(protectedCount==std::size(patches) && VirtualProtect(ttr+0x13CB78,1,PAGE_EXECUTE_READWRITE,&branchProtection)){
        paintFilthy=painter;
        ttr[0x13CB78]=0x72; // jb: inventory alone takes the lower branch.
        FlushInstructionCache(GetCurrentProcess(),ttr+0x13CB78,1);
        DWORD ignored;
        VirtualProtect(ttr+0x13CB78,1,branchProtection,&ignored);
        for(const auto& patch:patches){
            const auto site=reinterpret_cast<unsigned char*>(reinterpret_cast<uintptr_t>(patch.base)+patch.offset);
            const auto displacement=uint32_t(patch.replacement-reinterpret_cast<uintptr_t>(site+5));
            std::memcpy(site+1,&displacement,sizeof(displacement));
            FlushInstructionCache(GetCurrentProcess(),site,5);
        }
        installed=true;
    }
    for(size_t n=protectedCount;n>0;--n){
        const size_t i=n-1;
        DWORD ignored;
        VirtualProtect(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(patches[i].base)+patches[i].offset),5,patches[i].protection,&ignored);
    }
}

} // namespace eyebot::native_tabs
