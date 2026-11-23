#pragma once
#include <algorithm>
#include <array>
#include <cstring>
#include <string_view>

namespace eyebot::full_page {
// An unused slot inside the retail menu registry. Refuse entry if another
// plugin has registered it. The factory is scoped to our own XML load.
constexpr int id=1005;
constexpr int dataCategory=1023;
constexpr char xml[]="Data\\Menus\\prefabs\\RobCoEyebot\\full_filthy.xml";
inline void* instance=nullptr;
inline void* root=nullptr;
inline bool loading=false;
inline bool prepared=false;
inline bool dataHidden=false;
inline void (*click)(int)=nullptr;
inline void (*stockTab)(int)=nullptr;
inline std::array<void*,18> methods{};

template<class T> T& at(void* base,unsigned offset){
    return *reinterpret_cast<T*>(static_cast<char*>(base)+offset);
}
inline void* gameRoot(){return *reinterpret_cast<void**>(0x11D8A80);}
inline void* registered(int category){
    return reinterpret_cast<void*(__cdecl*)(int)>(0xA09030)(category);
}
inline int __fastcall category(void*,void*){return id;}
inline void __fastcall onClick(void*,void*,int number,void*){
    if(number>=7140 && number<7143){
        if(stockTab)stockTab(number-7140);
        return;
    }
    if(click && number>=7101 && number<=7132)click(number);
}
inline void* __fastcall destroy(void* self,void*,bool free){
    if(self==instance){root=nullptr;instance=nullptr;prepared=false;}
    return reinterpret_cast<void*(__thiscall*)(void*,bool)>(0xA1E170)(self,free);
}
inline void* __cdecl create(int requested){
    if(requested!=id || !loading)
        return reinterpret_cast<void*(__cdecl*)(int)>(0x707990)(requested);
    if(instance)return nullptr;
    auto memory=reinterpret_cast<void*(__cdecl*)(unsigned)>(0x401000)(0x28);
    if(!memory)return nullptr;
    reinterpret_cast<void(__thiscall*)(void*)>(0xA1C4A0)(memory);
    std::copy_n(at<void**>(memory,0),methods.size(),methods.begin());
    methods[0]=reinterpret_cast<void*>(&destroy);
    methods[3]=reinterpret_cast<void*>(&onClick);
    methods[13]=reinterpret_cast<void*>(&category);
    at<void**>(memory,0)=methods.data();
    instance=memory;
    return memory;
}

struct Child{Child* next;Child* previous;void* tile;};
inline void* find(void* tile,std::string_view path){
    while(tile && !path.empty()){
        const auto end=path.find('/');
        const auto part=path.substr(0,end);
        auto child=at<Child*>(tile,4);
        for(;child;child=child->next){
            auto name=child->tile?at<const char*>(child->tile,0x20):nullptr;
            if(name && part==name)break;
        }
        tile=child?child->tile:nullptr;
        if(end==path.npos)break;
        path.remove_prefix(end+1);
    }
    return tile;
}
inline unsigned trait(const char* name){
    return reinterpret_cast<unsigned(__cdecl*)(const char*)>(0xA01860)(name);
}
inline void number(void* node,const char* name,float value){
    if(node)reinterpret_cast<void(__thiscall*)(void*,unsigned,float,bool)>(0xA012D0)(node,trait(name),value,true);
}
inline void restoreStockData(){
    if(!dataHidden)return;
    if(auto tile=registered(dataCategory))number(tile,"visible",1.f);
    dataHidden=false;
}
inline void text(void* node,const char* name,const char* value){
    if(node)reinterpret_cast<void(__thiscall*)(void*,unsigned,const char*,bool)>(0xA01350)(node,trait(name),value,true);
}
inline void value(std::string_view path,bool flag){
    const auto pos=path.rfind('/');
    if(pos!=path.npos)number(find(root,path.substr(0,pos)),path.data()+pos+1,flag?1.f:0.f);
}
inline bool prepare(){
    if(prepared)return true;
    auto manager=gameRoot();
    if(!manager || !at<void*>(manager,0x9c) || (!instance && registered(id)))return false;
    if(!root){
        loading=true;
        root=reinterpret_cast<void*(__thiscall*)(void*,const char*)>(0xA01B00)(at<void*>(manager,0x9c),xml);
        loading=false;
    }
    if(!root || !instance || at<void*>(root,0x3c)!=instance || registered(id)!=root)return false;
    // Build the fourth button in the very same row as the stock three.
    auto row=find(root,"GLOW_BRANCH/NVVR_PipboyTabs");
    if(!row || !find(root,"GLOW_BRANCH/RobCoEyebotWrist/Settings"))return false;
    if(!find(row,"FC_TabFilthy")){
        reinterpret_cast<void*(__thiscall*)(void*,const char*)>(0xA01B00)(
            row,"Data\\Menus\\prefabs\\RobCoEyebot\\filthy_tab.xml");
    }
    if(!find(row,"FC_TabFilthy"))return false;
    constexpr const char* names[]{"NVVR_TabStats","NVVR_TabItems","NVVR_TabData"};
    for(int index=0;index<3;++index){
        auto tab=find(row,names[index]);
        number(tab,"id",7140.f+index);
        number(tab,"target",1.f);
        number(tab,"_selected",0.f);
    }
    // The prepared tree stays out of the menu stack until a category gesture.
    number(root,"visible",0.f);
    prepared=true;
    return true;
}
inline bool open(int previous,bool (*removePrior)(int)){
    if(previous!=1002 && previous!=1003 && previous!=1023)return false;
    if(!prepare() || !instance || registered(id)!=root)return false;
    auto manager=gameRoot();
    if(!manager)return false;
    auto old=registered(previous);
    if(!old || !at<void*>(old,0x3c))return false;
    if(!removePrior(previous))return false;
    if(registered(previous)==old){
        if(previous==dataCategory){
            // Engine and NVSE fast-travel checks still need the native map.
            number(old,"visible",0.f);
            dataHidden=true;
        }else{
            auto destroyTile=reinterpret_cast<void(__thiscall*)(void*,bool)>(at<void**>(old,0)[0]);
            destroyTile(old,true);
        }
    }
    number(root,"visible",1.f);
    // Register in the engine's live menu stack, just as native XML menus do.
    reinterpret_cast<void(__thiscall*)(void*,int)>(0x714D90)(manager,id);
    reinterpret_cast<void(__thiscall*)(void*)>(0xA1DB20)(instance);
    return true;
}
inline void close(){
    if(instance && registered(id)==root)
        reinterpret_cast<void(__thiscall*)(void*)>(0xA1D910)(instance);
}
}
