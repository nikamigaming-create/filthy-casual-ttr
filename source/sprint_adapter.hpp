#pragma once
#include "build_adapter.hpp"
// Exact TTR 1.1.6 movement-extraction epilogue. The original routine and its
// caller-sensitive paths run first. Only the player's XY displacement is
// scaled before retail collision consumes it. No actor values/save data change.
namespace sprint_adapter {
inline float factor=1.f;
inline unsigned applied=0;
inline void* resume=nullptr;
inline bool installed=false;
__declspec(naked) inline void finish(){
 __asm {
  pushfd
  cmp bh, 0
  je done
  cmp esi, dword ptr ds:[011DEA3Ch]
  jne done
  cmp dword ptr [ebp+4], 009E9F1Bh
  je scaleXY
  cmp dword ptr [ebp+4], 009E9F34h
  jne done
 scaleXY:
  sub esp,16
  movups [esp],xmm0
  movss xmm0,[edi]
  mulss xmm0,factor
  movss [edi],xmm0
  movss xmm0,[edi+4]
  mulss xmm0,factor
  movss [edi+4],xmm0
  movups xmm0,[esp]
  add esp,16
  inc applied
 done:
  popfd
  mov al,bh
  mov ecx,[ebp-10h]
  jmp resume
 }
}
inline bool install(){
 if(installed)return true;
 auto b=build_adapter::heldModule();if(!b)return false;
 auto p=b+0x249D69;
 const unsigned char expect[]={0x8A,0xC7,0x8B,0x4D,0xF0,0x64,0x89,0x0D,0,0,0,0};
 if(memcmp(p,expect,sizeof expect))return false;
 DWORD old;if(!VirtualProtect(p,5,PAGE_EXECUTE_READWRITE,&old))return false;
 resume=p+5;p[0]=0xE9;*reinterpret_cast<int*>(p+1)=int(reinterpret_cast<unsigned char*>(&finish)-(p+5));
 FlushInstructionCache(GetCurrentProcess(),p,5);DWORD ignored;VirtualProtect(p,5,old,&ignored);
 installed=true;return true;
}
}
