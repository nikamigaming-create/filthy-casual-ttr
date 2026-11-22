#pragma once
#include "build_adapter.hpp"
#include "projected_lighting.hpp"
#include "lamp_shadow_renderer.hpp"
namespace projected_render_adapter {
inline bool installed=false,attempted=false;
// Verified native FNV world-end call: pushes two integers; cdecl callee,
// caller adds 8. The native state setter is always called exactly once.
inline IDirect3DDevice9* device(){auto renderer=*reinterpret_cast<unsigned char**>(0x11F4748);return renderer?*reinterpret_cast<IDirect3DDevice9**>(renderer+0x288):nullptr;}
inline void __cdecl worldEnd(int a,int b){
 if(auto d=device()){
  if(projected_depth_adapter::requested.load(std::memory_order_relaxed))
   projected_depth_adapter::selectWorldDepth(d);
  lamp_shadow_renderer::draw(d);
  projected_lighting::draw(d);
 }
 reinterpret_cast<void(__cdecl*)(int,int)>(0x872DD0)(a,b);
}
inline void install(bool requested){
 projected_lighting::enabled=requested&&installed;if(!requested||attempted)return;
 if(!build_adapter::heldModule())return;auto d=device();if(!d)return;attempted=true;
 const unsigned char call[]={0x6a,0,0x6a,0,0xe8,0xb3,0xe9,0xff,0xff,0x83,0xc4,8};
 const unsigned char target[]={0x55,0x8b,0xec,0x8a,0x45,8,0xa2};
 if(memcmp((void*)0x874414,call,sizeof(call))||memcmp((void*)0x872dd0,target,sizeof(target)))return;
 DWORD protection=0;if(!VirtualProtect((void*)0x874418,5,PAGE_EXECUTE_READWRITE,&protection))return;
 bool depth=projected_depth_adapter::install(d);
 if(depth){
  projected_depth_adapter::beforeReset=[](){lamp_shadow_renderer::release();projected_lighting::release();};
  projected_lighting::lampShadow=lamp_shadow_renderer::bindLamp;
  int rel=int(reinterpret_cast<unsigned char*>(&worldEnd)-reinterpret_cast<unsigned char*>(0x87441d));memcpy((void*)0x874419,&rel,4);FlushInstructionCache(GetCurrentProcess(),(void*)0x874418,5);
  installed=true;projected_lighting::enabled=requested;
  char privateDirectory[MAX_PATH]{};projected_lighting::measureGpu=GetEnvironmentVariableA("SEATED_TEST_DIR",privateDirectory,MAX_PATH)>0;
 }
 DWORD ignored;VirtualProtect((void*)0x874418,5,protection,&ignored);
}
}
