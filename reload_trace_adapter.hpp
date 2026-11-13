#pragma once
#include "reload_adapter.hpp"
#include <ostream>
// Private, opt-in observation of exact native state transitions. No inventory,
// physics, input or save mutations; preserve flags/registers and displaced MOV.
namespace reload_trace_adapter {
struct Event {DWORD thread=0;unsigned site=0;ULONGLONG at=0;int held=0,rounds=0,installed=0,flight=0;};
inline Event events[64]{};inline unsigned total=0;inline DWORD mainThread=0;
inline unsigned char* module=nullptr;
inline void* jumps[8]{};
inline void __cdecl capture(unsigned site){
 if(!module)return;
 auto& e=events[total++%64];e={GetCurrentThreadId(),site,GetTickCount64(),
 *reinterpret_cast<int*>(module+0x17EE1E8),*reinterpret_cast<int*>(module+0x17EE1EC),
 *reinterpret_cast<int*>(module+0x17EE0C4),*reinterpret_cast<int*>(module+0x17EDF9C)};
}
#define SC_TRACE(N) __declspec(naked) inline void entry##N(){__asm{pushfd} __asm{pushad} __asm{mov eax,esp} __asm{sub esp,528} __asm{and esp,0FFFFFFF0h} __asm{mov [esp+512],eax} __asm{fxsave [esp]} __asm{push N} __asm{call capture} __asm{add esp,4} __asm{fxrstor [esp]} __asm{mov esp,[esp+512]} __asm{popad} __asm{popfd} __asm{jmp dword ptr [jumps+N*4]}}
SC_TRACE(0) SC_TRACE(1) SC_TRACE(2) SC_TRACE(3) SC_TRACE(4) SC_TRACE(5) SC_TRACE(6) SC_TRACE(7)
#undef SC_TRACE
inline const char* install(){
 // Legacy observation sites are deliberately absent from the new host adapter.
 return "disabled: legacy reload trace sites are not ported to 7EB72860";
}
inline void diagnostics(std::ostream& out){
 out<<",\"reloadTrace\":{\"active\":"<<(module?"true":"false")<<",\"mainThread\":"<<mainThread<<",\"total\":"<<total<<",\"events\":[";
 const unsigned start=total>64?total-64:0;
 for(unsigned i=start;i<total;i++){if(i!=start)out<<",";auto e=events[i%64];out<<"{\"site\":"<<e.site<<",\"thread\":"<<e.thread<<",\"at\":"<<e.at<<",\"held\":"<<e.held<<",\"rounds\":"<<e.rounds<<",\"installed\":"<<e.installed<<",\"flight\":"<<e.flight<<"}";}
 out<<"]}";
}
}
