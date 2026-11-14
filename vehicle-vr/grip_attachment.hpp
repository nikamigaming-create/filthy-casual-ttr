#pragma once
#include "mounted_frame.hpp"
// Constrain native grip targets before TTR's arm solve. Interaction always reads
// the separate raw-pose query, so rendered contact can never steer itself.
namespace vehicle_grip {
using Mat=mounted_frame::Mat;using Vec=scene::Vec;
struct Target {bool active=false;void* manager=nullptr;scene::Transform pose{};};
inline Target targets[2];inline DWORD owner=0;inline unsigned applied[2]{};
using Query=bool(__thiscall*)(void*,bool,bool,Vec*,float*);
inline Query original=nullptr;
inline void reset(){targets[0]={};targets[1]={};}
inline bool __fastcall query(void* manager,void*,bool right,bool aim,Vec* p,float* rotation){
 bool ok=original(manager,right,aim,p,rotation);int h=right?1:0;auto& t=targets[h];
 if(ok&&!aim&&GetCurrentThreadId()==owner&&t.active&&t.manager==manager){*p=t.pose.p;memcpy(rotation,t.pose.r,sizeof t.pose.r);++applied[h];}return ok;
}
inline bool install(){
 if(original)return true;auto b=build_adapter::heldModule();if(!b)return false;
 const unsigned char prefix[]={0x83,0xEC,0x70,0xA1};
 if(memcmp(b+0x6949C0,prefix,4)||scene::field<unsigned char*>(b,0x6949C4)!=b+0x1016840)return false;
 // The raw query's first bool selects hand, second selects aim/grip. It remains
 // unhooked. Both prologues and its native tail are exact-build guarded.
 const unsigned char raw[]={0x0F,0xB6,0x44,0x24,0x08,0x83,0xEC,0x30};
 if(memcmp(b+0x694CD0,raw,8)||b[0x694D4D]!=0xC2||b[0x694D4E]!=0x10)return false;
 auto t=static_cast<unsigned char*>(VirtualAlloc(nullptr,16,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!t)return false;
 memcpy(t,b+0x6949C0,8);t[8]=0xE9;scene::field<int>(t,9)=int((b+0x6949C8)-(t+13));DWORD old;
 if(!VirtualProtect(t,16,PAGE_EXECUTE_READ,&old)){VirtualFree(t,0,MEM_RELEASE);return false;}
 if(!VirtualProtect(b+0x6949C0,8,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(t,0,MEM_RELEASE);return false;}
 original=reinterpret_cast<Query>(t);auto p=b+0x6949C0;p[0]=0xE9;scene::field<int>(p,1)=int(reinterpret_cast<unsigned char*>(&query)-(p+5));memset(p+5,0x90,3);
 DWORD unused;VirtualProtect(p,8,old,&unused);FlushInstructionCache(GetCurrentProcess(),p,8);FlushInstructionCache(GetCurrentProcess(),t,16);return true;
}
inline bool raw(void* manager,int h,scene::Transform& t){auto b=build_adapter::heldModule();t.scale=1;return b&&manager&&reinterpret_cast<Query>(b+0x694CD0)(manager,h!=0,false,&t.p,&t.r[0][0]);}
inline scene::Transform relative(const scene::Transform& frame,const scene::Transform& pose){
 scene::Transform t{};t.scale=1;t.p=scene::inverseRotate(frame,pose.p-frame.p)*(1/frame.scale);
 for(int i=0;i<3;i++)for(int j=0;j<3;j++)for(int k=0;k<3;k++)t.r[i][j]+=frame.r[k][i]*pose.r[k][j];return t;
}
inline scene::Transform world(const scene::Transform& frame,const scene::Transform& pose){
 scene::Transform t{};t.scale=1;t.p=frame.p+scene::rotate(frame,pose.p)*frame.scale;
 for(int i=0;i<3;i++)for(int j=0;j<3;j++)for(int k=0;k<3;k++)t.r[i][j]+=frame.r[i][k]*pose.r[k][j];return t;
}
}
