#pragma once
#include <windows.h>
#include <atomic>
#include "../scene_adapter.hpp"
#include "../build_adapter.hpp"
namespace mounted_frame {
using Vec=scene::Vec;
struct Mat {float a[3][3]{};};
inline Mat identity(){Mat a;for(int i=0;i<3;i++)a.a[i][i]=1;return a;}
inline Mat transpose(Mat a){Mat b;for(int i=0;i<3;i++)for(int j=0;j<3;j++)b.a[i][j]=a.a[j][i];return b;}
inline Mat mul(Mat a,Mat b){Mat c;for(int i=0;i<3;i++)for(int j=0;j<3;j++)for(int k=0;k<3;k++)c.a[i][j]+=a.a[i][k]*b.a[k][j];return c;}
inline Vec apply(Mat a,Vec v){return {a.a[0][0]*v.x+a.a[0][1]*v.y+a.a[0][2]*v.z,a.a[1][0]*v.x+a.a[1][1]*v.y+a.a[1][2]*v.z,a.a[2][0]*v.x+a.a[2][1]*v.y+a.a[2][2]*v.z};}
// Flatten only boarding yaw. Physical pitch/roll remain part of the HMD pose.
inline bool headingFromForward(Vec forward,Mat& heading){
 float n=std::sqrt(forward.x*forward.x+forward.z*forward.z);
 if(!std::isfinite(n)||n<.1f)return false;
 forward=forward*(1/n);
 heading={{{-forward.z,0,-forward.x},{0,1,0},{forward.x,0,-forward.z}}};return true;
}
struct Frame {void* manager=nullptr;Mat correction=identity();Vec origin{};};
inline thread_local bool nativeSetup=false;
inline SRWLOCK frameLock=SRWLOCK_INIT;inline Frame current;inline std::atomic<bool> enabled{false};
inline Frame read(){if(nativeSetup||!enabled.load(std::memory_order_acquire))return {};AcquireSRWLockShared(&frameLock);auto f=current;ReleaseSRWLockShared(&frameLock);return f;}
inline void write(Frame f){AcquireSRWLockExclusive(&frameLock);current=f;enabled.store(f.manager!=nullptr,std::memory_order_release);ReleaseSRWLockExclusive(&frameLock);}
inline void reset(){write({});}
using Point=Vec*(__thiscall*)(void*,Vec*,const Vec*);
using Rotation=void*(__thiscall*)(void*,void*,const void*);
inline Point originalToWorld=nullptr,originalToTracking=nullptr;
inline Rotation originalWorldRotation=nullptr,originalTrackingRotation=nullptr;
inline Vec* __fastcall toWorld(void* manager,void*,Vec* out,const Vec* in){
 auto f=read();auto result=originalToWorld(manager,out,in);if(f.manager==manager)*result=f.origin+apply(f.correction,*result-f.origin);return result;
}
inline Vec* __fastcall toTracking(void* manager,void*,Vec* out,const Vec* in){
 auto f=read();if(f.manager!=manager)return originalToTracking(manager,out,in);
 auto p=f.origin+apply(transpose(f.correction),*in-f.origin);return originalToTracking(manager,out,&p);
}
inline void* __fastcall worldRotation(void* manager,void*,void* out,const void* in){
 auto f=read();auto result=originalWorldRotation(manager,out,in);if(f.manager==manager){Mat native;memcpy(&native,result,sizeof native);auto corrected=mul(f.correction,native);memcpy(result,&corrected,sizeof corrected);}return result;
}
inline void* __fastcall trackingRotation(void* manager,void*,void* out,const void* in){
 auto f=read();if(f.manager!=manager)return originalTrackingRotation(manager,out,in);
 Mat native;memcpy(&native,in,sizeof native);auto corrected=mul(transpose(f.correction),native);return originalTrackingRotation(manager,out,&corrected);
}
// Obtain the native stage basis from the exact conversion itself: no guessed
// handedness, matrix layout, world-unit scale or seated-height convention.
inline Mat nativeBasis(void* manager){
 Vec origin{},axes[3]={{1,0,0},{0,1,0},{0,0,1}},zero{};originalToWorld(manager,&origin,&zero);Mat b;
 for(int j=0;j<3;j++){Vec p;originalToWorld(manager,&p,&axes[j]);p=p-origin;float n=scene::distance(p,{});if(n<.001f)return identity();p=p*(1/n);b.a[0][j]=p.x;b.a[1][j]=p.y;b.a[2][j]=p.z;}return b;
}
inline bool install(){
 if(originalToWorld)return true;auto base=build_adapter::heldModule();if(!base)return false;
 struct Hook{unsigned rva,size;unsigned char bytes[6];void* target;void* trampoline;DWORD old;};
 Hook hooks[]={
 {0x698960,5,{0x83,0xEC,0x5C,0x56,0x57},reinterpret_cast<void*>(&toWorld)},
 {0x69AAC0,6,{0x83,0xEC,0x60,0x56,0x8B,0xF1},reinterpret_cast<void*>(&toTracking)},
 {0x698C20,6,{0x55,0x8B,0xEC,0x83,0xE4,0xF0},reinterpret_cast<void*>(&worldRotation)},
 {0x69A7B0,6,{0x55,0x8B,0xEC,0x83,0xE4,0xF0},reinterpret_cast<void*>(&trackingRotation)}};
 for(auto& h:hooks)if(memcmp(base+h.rva,h.bytes,h.size))return false;
 unsigned prepared=0;
 for(auto& h:hooks){h.trampoline=VirtualAlloc(nullptr,16,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);if(!h.trampoline)break;auto t=static_cast<unsigned char*>(h.trampoline);memcpy(t,h.bytes,h.size);t[h.size]=0xE9;*reinterpret_cast<int*>(t+h.size+1)=int((base+h.rva+h.size)-(t+h.size+5));DWORD old;if(!VirtualProtect(t,16,PAGE_EXECUTE_READ,&old))break;++prepared;}
 if(prepared!=4){for(auto& h:hooks)if(h.trampoline)VirtualFree(h.trampoline,0,MEM_RELEASE);return false;}
 prepared=0;for(auto& h:hooks){if(!VirtualProtect(base+h.rva,h.size,PAGE_EXECUTE_READWRITE,&h.old))break;++prepared;}
 if(prepared!=4){for(int i=int(prepared)-1;i>=0;i--){DWORD unused;VirtualProtect(base+hooks[i].rva,hooks[i].size,hooks[i].old,&unused);}for(auto& h:hooks)VirtualFree(h.trampoline,0,MEM_RELEASE);return false;}
 originalToWorld=reinterpret_cast<Point>(hooks[0].trampoline);originalToTracking=reinterpret_cast<Point>(hooks[1].trampoline);
 originalWorldRotation=reinterpret_cast<Rotation>(hooks[2].trampoline);originalTrackingRotation=reinterpret_cast<Rotation>(hooks[3].trampoline);
 for(auto& h:hooks){auto p=base+h.rva;p[0]=0xE9;*reinterpret_cast<int*>(p+1)=int(static_cast<unsigned char*>(h.target)-(p+5));if(h.size==6)p[5]=0x90;FlushInstructionCache(GetCurrentProcess(),p,h.size);FlushInstructionCache(GetCurrentProcess(),h.trampoline,16);}
 // Two functions share one code page: finish every write before restoring any
 // protection, then unwind in reverse so the original RX protection wins.
 for(int i=3;i>=0;i--){DWORD unused;VirtualProtect(base+hooks[i].rva,hooks[i].size,hooks[i].old,&unused);}return true;
}
}
