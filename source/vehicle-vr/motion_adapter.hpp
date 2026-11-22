#pragma once
#include "../build_adapter.hpp"
#include "../scene_adapter.hpp"
#include <atomic>
// Retail 1.4.0.525 AnimData movement extraction, already intercepted by exact
// 119A085A TTR. Keep its existing hook and its caller-sensitive player path.
// Only our driven actor receives a commanded displacement; native Havok still
// performs collision, support, gravity and reference publication afterward.
namespace vehicle_motion {
using Extract=bool(__thiscall*)(void*,scene::Vec*,void*,bool,bool);
inline Extract original=nullptr;
inline void* owner=nullptr;
inline std::atomic<float> speed{0};
inline std::atomic<unsigned> calls{0};
inline std::atomic<float> lastDelta{0},lastBefore{0},lastAfter{0};
inline bool installed=false;
inline void (*beforeOwnedMotion)(Form*)=nullptr;
inline bool __fastcall extract(void* data,void*,scene::Vec* out,void* actor,bool world,bool flatten){
 // Presentation suspension must be removed before native movement/collision,
 // not only in the later NVSE MainLoop callback. Otherwise last frame's tilted
 // render root can become the next physics frame's starting transform.
 if(actor==owner&&beforeOwnedMotion)beforeOwnedMotion(static_cast<Form*>(actor));
 bool ok=original(data,out,actor,world,flatten);
 if(!ok||!out||actor!=owner||scene::field<void*>(data,4)!=actor)return ok;
 auto root=scene::root(actor);if(!root)return ok;
 float dt=scene::field<float>(reinterpret_cast<void*>(0x11F6394),0xC);
 float scale=scene::field<float>(data,0x114);
 if(!std::isfinite(dt)||dt<=0||dt>.25f||!std::isfinite(scale)||scale<0||scale>4)return ok;
 float distance=speed.load(std::memory_order_relaxed)*69.99125f*std::min(dt,.05f)*scale;
 scene::Vec delta{0,distance,0};if(world)delta=scene::rotate(scene::world(root),delta);
 if(flatten)delta.z=0;
 lastBefore=std::sqrt(out->x*out->x+out->y*out->y);
 // Replace the complete authored travel vector. Retaining the source clip's
 // world-Z travel injects its old speed into slope collision even when braking
 // has brought XY to zero. Havok still supplies gravity after this extraction.
 *out=delta;
 lastAfter=std::sqrt(delta.x*delta.x+delta.y*delta.y);lastDelta=dt;++calls;
 return ok;
}
// A plain C++ forwarding call would change the return address inspected by
// TTR for player locomotion. Tail-jump untouched actors with the original stack.
__declspec(naked) inline void dispatch(){
 __asm {
  mov eax, [ecx+4]
  cmp eax, owner
  jne nativePath
  jmp extract
 nativePath:
  jmp original
 }
}
inline bool install(){
 if(installed)return true;auto b=build_adapter::heldModule();if(!b)return false;
 auto entry=reinterpret_cast<unsigned char*>(0x494390);
 auto target=b+0x249BC0;
 const unsigned char head[]={0x55,0x8B,0xEC,0x6A,0xFE,0x68};
 const unsigned char caller[]={0x8B,0x45,0x04,0x3D,0x1B,0x9F,0x9E,0x00};
 const unsigned char clock[]={0x55,0x8B,0xEC,0x51,0x89,0x4D,0xFC,0x8B,0x45,0xFC,0xD9,0x40,0x0C};
 if(entry[0]!=0xE9||entry+5+scene::field<int>(entry,1)!=target||memcmp(target,head,sizeof head)||memcmp(target+0x50,caller,sizeof caller)||memcmp(reinterpret_cast<void*>(0x84D030),clock,sizeof clock))return false;
 DWORD previous;if(!VirtualProtect(entry,5,PAGE_EXECUTE_READWRITE,&previous))return false;
 original=reinterpret_cast<Extract>(target);
 scene::field<int>(entry,1)=int(reinterpret_cast<unsigned char*>(&dispatch)-(entry+5));
 FlushInstructionCache(GetCurrentProcess(),entry,5);DWORD ignored;VirtualProtect(entry,5,previous,&ignored);
 installed=true;return true;
}
inline void command(void* actor,float metresPerSecond){speed=metresPerSecond;owner=actor;}
inline void clear(){owner=nullptr;speed=0;}
}
