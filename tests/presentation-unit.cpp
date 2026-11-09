#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
#include "../floating_hud_adapter.hpp"
#include "../dialogue_layout.hpp"
int main(){
 const auto rightPanel=dialogue_layout::sideAngle(2,1.5f,2,-.4f);
 const auto leftPanel=dialogue_layout::sideAngle(2,1.5f,2,.4f);
 assert(rightPanel>40&&rightPanel<50&&leftPanel==-rightPanel);
 assert(dialogue_layout::sideAngle(2,1.5f,.1f,0)<=60);
 using namespace floating_hud_adapter;
 HorizontalGate g;
 assert(!g.update(false,.9f,true));
 assert(!g.update(true,.9f,true)); // Popup cannot steal an ongoing turn.
 assert(!g.update(true,.7f,true));
 assert(g.update(true,0,true)); // Neutral arms paging.
 assert(g.update(true,.9f,true));
 assert(g.update(false,.9f,true)); // Paging hold cannot leak into snap-turn.
 assert(!g.update(false,0,true));
 assert(!g.update(false,.9f,true));
 assert(!g.update(true,.9f,false)); // Lost XR action resets ownership.
 assert(!g.update(true,.9f,true));
 Anchor a;
 assert(a.sample(1,7,{0,50,0},{0,0,0},{0,60,0},70,100));
 assert(std::abs(scene::distance(a.position,{0,0,0})-59.5f)<.001f);
 const auto first=a.position;
 assert(!a.sample(1,7,{4,50,0},{3,0,0},{0,60,0},70,200));
 assert(scene::distance(a.position,first)==0);
 assert(a.sample(1,7,{4,50,0},{90,0,0},{0,60,0},70,300));
 assert(a.sample(2,7,{4,50,0},{90,0,0},{0,60,0},70,400));
 assert(a.sample(2,8,{4,50,0},{90,0,0},{0,60,0},70,500));
 assert(a.sample(2,8,{4,50,0},{90,0,0},{0,120,0},70,600));
 assert(a.sample(2,8,{4,50,0},{90,0,0},{0,120,0},70,1301));
 // Regression for shared-page installation: no write may happen after an
 // earlier site restores the same page; final permissions must be original.
 auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x130000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
 assert(memory);
 unsigned sites[]={0x10846F,0x1133ED,0x108925,0x113854,0x10894C,0x11387B,0x112273};
 unsigned callTargets[]={0x25D1,0x25D1,0x218A5,0x218A5,0x16FE,0x16FE,0x123460};
 for(unsigned i=0;i<7;i++){memory[sites[i]]=0xE8;int rel=int(callTargets[i]-sites[i]-5);memcpy(memory+sites[i]+1,&rel,4);}
 memory[sites[6]]=0x90;assert(!install(memory));assert(!base);
 memory[sites[6]]=0xE8;DWORD old{};assert(VirtualProtect(memory,0x130000,PAGE_EXECUTE_READ,&old));
 assert(install(memory));
 for(auto site:sites){MEMORY_BASIC_INFORMATION info{};assert(VirtualQuery(memory+site,&info,sizeof(info)));assert(info.Protect==PAGE_EXECUTE_READ);}
 VirtualFree(memory,0,MEM_RELEASE);base=nullptr;
 puts("PASS: anchor lifetime, zone/target transitions, minimum distance, call guards, shared-page installation and protection restoration");
}
