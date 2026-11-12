#pragma once
#include "scene_adapter.hpp"
#include <atomic>
#include <cmath>

// TTWVR C9D047D4 only. Both custom target-info and container-loot paths
// publish manager+3040. They project an actor centre onto the hand ray every
// update, then convert world position and world orientation to tracking space.
// Latch BEFORE those conversions, so snap-turn/locomotion still use the native
// conversion and the result remains in the world instead of following a hand.
namespace floating_hud_adapter {
using Vec=scene::Vec;
struct Matrix {float v[9];};
struct Anchor {
 uintptr_t key=0; unsigned form=0; ULONGLONG seen=0;
 Vec position{},head{},target{}; Matrix rotation{};
 bool valid=false,rotationValid=false;
 bool sample(uintptr_t identity,unsigned id,Vec wanted,Vec eye,Vec actor,
             float units,ULONGLONG now){
  const bool fresh=!valid||key!=identity||form!=id||now-seen>600||
    scene::distance(head,eye)>1.25f*units||scene::distance(target,actor)>.75f*units;
  seen=now;
  if(fresh){
   key=identity;form=id;head=eye;target=actor;position=wanted;
   auto delta=wanted-eye;const float distance=scene::distance(wanted,eye);
   if(distance>.001f&&distance<.85f*units)position=eye+delta*(.85f*units/distance);
   valid=true;rotationValid=false;
  }
  return fresh;
 }
};
inline unsigned char* base=nullptr;
inline thread_local Anchor anchors[2];
struct Target {uintptr_t key=0;unsigned form=0;Vec centre{};};
inline thread_local Target targets[2];
inline std::atomic<unsigned> samples[2]{},relocations[2]{},pages{0};
inline std::atomic<ULONGLONG> lootInputAt{0},rawInputAt{0};
inline std::atomic<float> rawHorizontal{0};
struct HorizontalGate {
 bool inPanel=false,armed=false,draining=false;
 bool update(bool panel,float x,bool active){
  if(!active||!std::isfinite(x)){*this={};return false;}
  const bool neutral=std::abs(x)<.25f;
  if(!panel){
   if(inPanel&&armed&&!neutral)draining=true;
   inPanel=false;armed=false;
   if(neutral)draining=false;
   return draining;
  }
  if(!inPanel){inPanel=true;armed=neutral;}
  if(neutral){armed=true;draining=false;}
  return armed||draining;
 }
};
inline std::atomic<bool> pagingOwned{false};
inline bool horizontalInput(float x,bool active){
 static HorizontalGate gate;
 rawHorizontal=active?x:0;rawInputAt=GetTickCount64();
 const bool panel=base&&GetTickCount64()-lootInputAt.load()<100;
 const bool owned=gate.update(panel,x,active);
 pagingOwned=panel&&owned;
 return owned;
}
inline Vec lastWorld[2]{};
template<int Kind> bool __cdecl target(void* ref,Vec* centre,float* extent){
 bool ok=reinterpret_cast<bool(__cdecl*)(void*,Vec*,float*)>(base+0x1966C0)(ref,centre,extent);
 targets[Kind]={};
 if(ok&&ref){targets[Kind]={reinterpret_cast<uintptr_t>(ref),*reinterpret_cast<unsigned*>(static_cast<char*>(ref)+0xC),*centre};}
 return ok;
}
template<int Kind> Vec* __fastcall position(void* manager,void*,Vec* out,const Vec* world){
 auto& a=anchors[Kind];const auto& t=targets[Kind];
 if(t.key){
  Vec eye{};
  reinterpret_cast<Vec*(__thiscall*)(void*,Vec*,const Vec*)>(base+0x649010)(manager,&eye,
   reinterpret_cast<const Vec*>(static_cast<unsigned char*>(manager)+0x1C));
  auto nativeSettings=reinterpret_cast<unsigned char*(__cdecl*)()>(base+0xA641)();
  float units=*reinterpret_cast<float*>(nativeSettings+0x28);
  if(std::isfinite(units)&&units>1.f&&units<1000.f){
   if(a.sample(t.key,t.form,*world,eye,t.centre,units,GetTickCount64()))++relocations[Kind];
   ++samples[Kind];lastWorld[Kind]=a.position;world=&a.position;
  }else a.valid=false;
 }else a.valid=false;
 return reinterpret_cast<Vec*(__thiscall*)(void*,Vec*,const Vec*)>(base+0x64B0B0)(manager,out,world);
}
template<int Kind> float* __fastcall rotation(void* manager,void*,float* out,const Matrix* world){
 auto& a=anchors[Kind];
 if(a.valid&&a.key==targets[Kind].key){
  if(!a.rotationValid){a.rotation=*world;a.rotationValid=true;}
  world=&a.rotation;
 }
 return reinterpret_cast<float*(__thiscall*)(void*,float*,const Matrix*)>(base+0x64ADA0)(manager,out,world);
}
struct List {unsigned char* first;unsigned char* end;unsigned char* capacity;};
inline int __cdecl selection(List* list,unsigned selected,int* remembered){
 int index=reinterpret_cast<int(__cdecl*)(List*,unsigned,int*)>(base+0x12DDB0)(list,selected,remembered);
 static uintptr_t lastTarget=0;static int previousDirection=0;static ULONGLONG repeatAt=0;
 auto manager=static_cast<unsigned char*>(reinterpret_cast<void*(__cdecl*)()>(base+0x12BC5)());
 const uintptr_t current=*reinterpret_cast<uintptr_t*>(base+0x10D6F38);
 if(current!=lastTarget){previousDirection=0;repeatAt=0;lastTarget=current;}
 // Native item-detail mode owns horizontal quantity/option adjustment.
 // Only the top-level loot list gains paging; vertical selection stays native.
 if(!current||base[0x10D657B]||manager[0xC6E]||manager[0x18D4]||!list||!list->first||index<0){lootInputAt=0;previousDirection=0;return index;}
 lootInputAt=GetTickCount64();
 if(!pagingOwned){previousDirection=0;return index;}
 const float x=GetTickCount64()-rawInputAt.load()<250?rawHorizontal.load():*reinterpret_cast<float*>(manager+0x124),y=*reinterpret_cast<float*>(manager+0x128);
 if(!std::isfinite(x)||!std::isfinite(y)){previousDirection=0;return index;}
 int direction=(std::abs(x)>.65f&&std::abs(x)>std::abs(y))?(x>0?1:-1):0;
 if(!direction){if(std::abs(x)<.35f)previousDirection=0;return index;}
 auto now=GetTickCount64();if(direction==previousDirection&&now<repeatAt)return index;
 previousDirection=direction;repeatAt=now+400;
 const auto bytes=reinterpret_cast<uintptr_t>(list->end)-reinterpret_cast<uintptr_t>(list->first);
 if(bytes%160||bytes/160>100000)return index;
 const int count=int(bytes/160);if(count<=0)return index;
 auto& top=*reinterpret_cast<int*>(base+0xE8AD30);
 const int newTop=std::clamp(top+direction*7,0,std::max(0,count-7));
 const int newIndex=std::clamp(index+direction*7,0,count-1);
 if(newIndex!=index){top=newTop;++pages;}
 return newIndex;
}
inline bool install(unsigned char* module){
 if(base)return true;
 struct Site {unsigned rva,target;void* hook;};
 Site sites[]={
  {0x112CDF,0x29B9,reinterpret_cast<void*>(&target<0>)},
  {0x11DC5D,0x29B9,reinterpret_cast<void*>(&target<1>)},
  {0x113195,0x273FE,reinterpret_cast<void*>(&position<0>)},
  {0x11E0C4,0x273FE,reinterpret_cast<void*>(&position<1>)},
  {0x1131BC,0x185C,reinterpret_cast<void*>(&rotation<0>)},
  {0x11E0EB,0x185C,reinterpret_cast<void*>(&rotation<1>)},
  {0x11CAE3,0x12DDB0,reinterpret_cast<void*>(&selection)}};
 DWORD protections[7]{};unsigned reserved=0;
 for(const auto& s:sites){int rel{};memcpy(&rel,module+s.rva+1,4);
  if(module[s.rva]!=0xE8||module+s.rva+5+rel!=module+s.target)return false;}
 for(const auto& s:sites){
  if(!VirtualProtect(module+s.rva,5,PAGE_EXECUTE_READWRITE,&protections[reserved])){
   DWORD ignored;while(reserved){--reserved;VirtualProtect(module+sites[reserved].rva,5,protections[reserved],&ignored);}return false;
  }++reserved;
 }
 base=module;
 for(unsigned i=0;i<7;++i){const auto& s=sites[i];int rel=int(static_cast<unsigned char*>(s.hook)-(module+s.rva+5));
  memcpy(module+s.rva+1,&rel,4);FlushInstructionCache(GetCurrentProcess(),module+s.rva,5);
 }
 // Sites share memory pages. Write the complete set before restoring, then
 // unwind in reverse so the first protection saved for each page wins.
 DWORD ignored;while(reserved){--reserved;VirtualProtect(module+sites[reserved].rva,5,protections[reserved],&ignored);}
 return true;
}
}
