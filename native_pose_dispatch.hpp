#pragma once
#include "build_adapter.hpp"
#include <atomic>
// Shared owner for the verified E14567BC native pose/camera transaction.
// Subscribers observe one completed native pass, never independent timer ticks.
namespace native_pose_dispatch {
using Callback=void(*)();using Completed=void(*)(bool);using Frame=void(__cdecl*)();
struct Listener {Callback before=nullptr;Completed after=nullptr;};
inline Listener listeners[4];inline std::atomic<unsigned> count{0};
inline Frame original=nullptr;inline thread_local unsigned nesting=0;
inline void __cdecl publish(){
 const bool outer=++nesting==1;const auto n=count.load();bool completed=false;
 __try{
  if(outer)for(unsigned i=0;i<n;i++)if(listeners[i].before)listeners[i].before();
  original();completed=true;
 }__finally{
  __try{if(outer)for(unsigned i=0;i<n;i++)if(listeners[i].after)listeners[i].after(completed);}
  __finally{--nesting;}
 }
}
inline bool local(Callback before,Completed after){
 for(unsigned i=0;i<count;i++)if(listeners[i].before==before&&listeners[i].after==after)return true;
 if(count>=4)return false;
 if(!original){
  auto b=build_adapter::heldModule();if(!b)return false;
  const unsigned char entry[]={0x53,0x8B,0xDC,0x83,0xEC,0x08};
  const unsigned char flags[]={0x0F,0xB6,0x87,0x80,0x06,0x00,0x00};
  if(memcmp(b+0x41F900,entry,6)||memcmp(b+0x41FEC7,flags,7))return false;
  auto stub=static_cast<unsigned char*>(VirtualAlloc(nullptr,16,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!stub)return false;
  memcpy(stub,entry,6);stub[6]=0xE9;*reinterpret_cast<int*>(stub+7)=int((b+0x41F906)-(stub+11));
  DWORD old=0;if(!VirtualProtect(stub,16,PAGE_EXECUTE_READ,&old)){VirtualFree(stub,0,MEM_RELEASE);return false;}
  if(!VirtualProtect(b+0x41F900,6,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(stub,0,MEM_RELEASE);return false;}
  original=reinterpret_cast<Frame>(stub);
  b[0x41F900]=0xE9;*reinterpret_cast<int*>(b+0x41F901)=int(reinterpret_cast<unsigned char*>(&publish)-(b+0x41F905));b[0x41F905]=0x90;
  FlushInstructionCache(GetCurrentProcess(),b+0x41F900,6);FlushInstructionCache(GetCurrentProcess(),stub,16);
  DWORD ignored=0;VirtualProtect(b+0x41F900,6,old,&ignored);
 }
 const unsigned index=count.load();listeners[index]={before,after};count.store(index+1);return true;
}
inline bool subscribe(Callback before,Completed after){
 using Owner=bool(__cdecl*)();using Add=bool(__cdecl*)(Callback,Completed);
 for(auto name:{L"FilthyCasualPool.dll",L"FilthyCasualVehicles.dll",L"SeatedControls.dll"})if(auto m=GetModuleHandleW(name)){
  auto owner=reinterpret_cast<Owner>(GetProcAddress(m,"FC_NativePoseOwner"));
  auto add=reinterpret_cast<Add>(GetProcAddress(m,"FC_NativePoseSubscribe"));
  if(owner&&add&&owner())return add(before,after);
 }
 return local(before,after);
}
}
