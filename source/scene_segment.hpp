#pragma once
#include "scene_adapter.hpp"

// Finite segment against the loaded FalloutNV 1.4.0.525 collision world.
namespace scene {
inline bool traceSegment(Vec start,Vec end,Vec& point,Vec& normal,void*& node){
 const unsigned char sig[]={0x55,0x8B,0xEC,0x83,0xEC,0x0C,0x89,0x4D,0xF8};
 if(memcmp(reinterpret_cast<void*>(0x458440),sig,sizeof sig))return false;
 auto tes=*reinterpret_cast<void**>(0x11DEA10);if(!tes)return false;
 alignas(16) unsigned char ray[0xB0]{};
 scene::field<Vec>(ray,0)=start*.14287498593330383f;scene::field<Vec>(ray,0x10)=end*.14287498593330383f;
 scene::field<unsigned>(ray,0x24)=6;scene::field<float>(ray,0x40)=1;
 scene::field<int>(ray,0x44)=scene::field<int>(ray,0x50)=-1;
 node=reinterpret_cast<void*(__thiscall*)(void*,void*,bool)>(0x458440)(tes,ray,true);
 if(!node)return false;float fraction=scene::field<float>(ray,0x40);
 if(!std::isfinite(fraction)||fraction<0||fraction>1)return false;
 point=start+(end-start)*fraction;normal=scene::field<Vec>(ray,0x30);
 float length=scene::distance(normal,{});if(!std::isfinite(length)||length<.5f||length>1.5f)return false;
 normal=normal*(1/length);return true;
}
}
