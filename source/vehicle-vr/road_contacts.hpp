#pragma once
#include "../scene_adapter.hpp"
namespace road_contacts {
using scene::Vec;
// The retail terrain query used by JIP GetTerrainHeight also reports
// whether the exterior cell supplied a height. Never treat a missing cell
// or the terrain below a bridge as suspension support.
inline bool landscape(Vec foot,float travel,Vec& point,Vec& normal){
 auto tes=*reinterpret_cast<void**>(0x11DEA10);
 if(!tes||scene::field<void*>(tes,0x34))return false;
 auto height=[&](float x,float y,float& z){
  float xy[]={x,y};return reinterpret_cast<bool(__thiscall*)(void*,float*,float*)>(0x4572E0)(tes,xy,&z)&&std::isfinite(z);
 };
 float z,right,front;
 if(!height(foot.x,foot.y,z)||z>foot.z+12||z<foot.z-travel||
    !height(foot.x+8,foot.y,right)||!height(foot.x,foot.y+8,front))return false;
 normal={z-right,z-front,8};normal=normal*(1/scene::distance(normal,{}));point={foot.x,foot.y,z};
 return normal.z>.35f;
}
inline Form* owner(void* node){for(unsigned n=0;node&&n<64;n++,node=scene::parent(node))if(scene::field<unsigned>(node,0)==0x10A8F90){auto ref=scene::field<Form*>(node,0xCC);if(ref)return ref;}return nullptr;}
inline bool ray(Vec start,Vec& end,Vec& normal,Form*& ref,Form* ignore,Form* ignoreSecond){
 const unsigned char signature[]={0x55,0x8B,0xEC,0x83,0xEC,0x0C,0x89,0x4D,0xF8};if(memcmp(reinterpret_cast<void*>(0x458440),signature,sizeof signature))return false;
 auto tes=*reinterpret_cast<void**>(0x11DEA10);if(!tes)return false;auto delta=end-start;float length=scene::distance(delta,{});if(!std::isfinite(length)||length<1)return false;auto direction=delta*(1/length);
 for(unsigned i=0;i<8;i++){
  alignas(16) unsigned char data[0xB0]{};scene::field<Vec>(data,0)=start*.14287498593330383f;scene::field<Vec>(data,0x10)=end*.14287498593330383f;
  scene::field<unsigned>(data,0x24)=6;scene::field<float>(data,0x40)=1;scene::field<int>(data,0x44)=scene::field<int>(data,0x50)=-1;
  auto node=reinterpret_cast<void*(__thiscall*)(void*,void*,bool)>(0x458440)(tes,data,true);
  auto fraction=scene::field<float>(data,0x40);if(!std::isfinite(fraction)||fraction<0||fraction>1)return false;auto hit=start+(end-start)*fraction;ref=owner(node);
  if(!node&&fraction>=1)return false;
  if(ref!=ignore&&ref!=ignoreSecond){normal=scene::field<Vec>(data,0x30);float n=scene::distance(normal,{});if(!std::isfinite(n)||n<.1f)return false;normal=normal*(1/n);end=hit;return true;}
  start=hit+direction*2;if(scene::distance(start,end)<2)return false;
 }return false;
}

}
