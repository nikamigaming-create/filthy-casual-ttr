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
 char flag[8]{};if(!GetEnvironmentVariableA("SC_TEST_RELOAD_EVENTS",flag,sizeof(flag))||strcmp(flag,"1"))return "not requested";
 if(module)return "already installed";
 auto b=build_adapter::heldModule();if(!b)return "refused hash";
 const unsigned sites[]={0x2E4BC8,0x31278D,0x312A92,0x312DCD,0x313039,0x313470,0x2E0BC7,0x2E15AB};
 void* hooks[]={entry0,entry1,entry2,entry3,entry4,entry5,entry6,entry7};
 // Preflight every exact displaced instruction before installing any hook.
 for(unsigned i=0;i<8;i++){auto p=b+sites[i];if(p[0]!=0xC7||p[1]!=0x05||*reinterpret_cast<void**>(p+2)!=b+0x17EE1E8||*reinterpret_cast<unsigned*>(p+6)!=(i==7?1u:0u))return "refused instruction";}
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,8*16,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return "allocation failed";
 for(unsigned i=0;i<8;i++){auto t=code+i*16;memcpy(t,b+sites[i],10);t[10]=0xE9;*reinterpret_cast<int*>(t+11)=int((b+sites[i]+10)-(t+15));jumps[i]=t;}
 DWORD old=0;if(!VirtualProtect(code,8*16,PAGE_EXECUTE_READ,&old)){VirtualFree(code,0,MEM_RELEASE);return "trampoline protection failed";}
 FlushInstructionCache(GetCurrentProcess(),code,8*16);
 module=b;mainThread=GetCurrentThreadId();
 for(unsigned i=0;i<8;i++){auto p=b+sites[i];DWORD prev=0;if(!VirtualProtect(p,10,PAGE_EXECUTE_READWRITE,&prev))return "site protection failed";
 p[0]=0xE9;*reinterpret_cast<int*>(p+1)=int(static_cast<unsigned char*>(hooks[i])-(p+5));memset(p+5,0x90,5);FlushInstructionCache(GetCurrentProcess(),p,10);DWORD unused;VirtualProtect(p,10,prev,&unused);}
 return "eight observation sites active";
}
inline void diagnostics(std::ostream& out){
 out<<",\"reloadTrace\":{\"active\":"<<(module?"true":"false")<<",\"mainThread\":"<<mainThread<<",\"total\":"<<total<<",\"events\":[";
 const unsigned start=total>64?total-64:0;
 for(unsigned i=start;i<total;i++){if(i!=start)out<<",";auto e=events[i%64];out<<"{\"site\":"<<e.site<<",\"thread\":"<<e.thread<<",\"at\":"<<e.at<<",\"held\":"<<e.held<<",\"rounds\":"<<e.rounds<<",\"installed\":"<<e.installed<<",\"flight\":"<<e.flight<<"}";}
 out<<"]}";
}
}
