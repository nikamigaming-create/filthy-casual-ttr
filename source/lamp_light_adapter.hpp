#pragma once
#include "scene_adapter.hpp"
#include "build_adapter.hpp"
// FalloutNV 1.4.0.525 + exact TTR 1.1.6. Read-only live disassembly:
// work/lamp-native-inspect.txt, PID19772. JIP allocates 0x110-byte point lights.
// No proprietary code/assets included; fixed three-object runtime pool.
namespace lamp_lights {
inline void* points[3]{};inline void* records[3]{};
inline bool checked=false,supported=false;inline unsigned allocations=0,activeCount=0;
inline bool verify(){
 if(checked)return supported;checked=true;
 const unsigned char factory[]={0x56,0x68,0x10,1,0,0,0xE8,0xF5,0x3C,2,0};
 const unsigned char queue[]={0x6A,0xFF,0x68,0x68,0x79,0xF2,0,0x64,0xA1,0,0,0};
 const unsigned char ctor[]={0x6A,0xFF,0x68,0x4E,0xA3,0xF2,0,0x64,0xA1,0,0,0};
 const unsigned char alloc[]={0x51,0x8B,0x44,0x24,8,0x85,0xC0,0x75,5,0xB8,1,0};
 const unsigned char refresh[]={0x83,0xEC,0x2C,0x57,0x8B,0xF9,0x8B,0x4C,0x24,0x34,0x8B,0x81};
 supported=build_adapter::heldModule()&&!memcmp((void*)0xA7D6E0,factory,sizeof factory)&&!memcmp((void*)0xB5ECA0,queue,sizeof queue)&&!memcmp((void*)0xB9FDA0,ctor,sizeof ctor)&&!memcmp((void*)0xAA1070,alloc,sizeof alloc)&&!memcmp((void*)0xB5D300,refresh,sizeof refresh);
 return supported;
}
inline void* root(){return *reinterpret_cast<void**>(0x11F91C8);}
inline void* registered(void* sceneRoot,void* light){
 struct Entry{Entry* next;Entry* previous;void* value;};
 auto e=scene::field<Entry*>(sceneRoot,0xB4);
 for(unsigned n=0;e&&n<1024;e=e->next,++n)if(e->value&&scene::field<void*>(e->value,0xF8)==light)return e->value;
 return nullptr;
}
inline void disable(){
 activeCount=0;if(!supported)return;auto s=root();
 for(auto p:points)if(p){scene::field<float>(p,0xC4)=0;scene::field<bool>(p,0x9C)=false;scene::field<unsigned>(p,0x30)|=1;
  if(s)if(auto data=registered(s,p))reinterpret_cast<void(__thiscall*)(void*,void*)>(0xB5D300)(s,data);
 }
}
inline bool ensure(int i){
 if(!verify())return false;
 if(!points[i]){points[i]=reinterpret_cast<void*(__cdecl*)()>(0xA7D6E0)();if(!points[i])return false;scene::retain(points[i]);++allocations;}
 if(!records[i]){
  auto memory=reinterpret_cast<void*(__cdecl*)(unsigned)>(0xAA1070)(0x250);if(!memory)return false;
  records[i]=reinterpret_cast<void*(__thiscall*)(void*)>(0xB9FDA0)(memory);scene::retain(records[i]);
  scene::field<unsigned char>(records[i],0xF4)=1;scene::field<unsigned char>(records[i],0xF5)=0;
  scene::field<void*>(records[i],0xF8)=points[i];scene::retain(points[i]);
  scene::field<unsigned char>(records[i],0xFC)=1;
 }
 return true;
}
inline void apply(int i,scene::Vec position,float radius,float brightness,const float* rgb){
 if(!ensure(i))return;auto s=root();if(!s)return;auto p=points[i];
 scene::field<unsigned>(p,0x30)=(scene::field<unsigned>(p,0x30)&~1u)|0x60000020u;
 scene::field<bool>(p,0x9C)=brightness>.001f;scene::field<unsigned char>(p,0x9D)=2;
 scene::field<bool>(p,0x9E)=false;scene::field<unsigned char>(p,0x9F)=0;
 auto t=scene::world(p);t.p=position;t.scale=1;scene::local(p)=t;scene::field<scene::Transform>(p,0x68)=t;
 scene::field<float>(p,0xC4)=brightness;scene::field<scene::Vec>(p,0xC8)={};
 scene::field<scene::Vec>(p,0xD4)={rgb[0],rgb[1],rgb[2]};scene::field<float>(p,0xE0)=radius;
 scene::field<float>(p,0xE4)=1;scene::field<void*>(p,0xE8)=nullptr;scene::field<void*>(p,0xEC)=nullptr;
 scene::field<float>(p,0xF0)=0;scene::field<float>(p,0xF4)=1;scene::field<float>(p,0xF8)=0;
 if(auto data=registered(s,p))reinterpret_cast<void(__thiscall*)(void*,void*)>(0xB5D300)(s,data);
 else if(brightness>.001f){scene::field<void*>(records[i],0x240)=scene::field<void*>(s,0x1E0);reinterpret_cast<void(__thiscall*)(void*,void*)>(0xB5ECA0)(s,records[i]);}
 if(brightness>.001f)++activeCount;
}
}
