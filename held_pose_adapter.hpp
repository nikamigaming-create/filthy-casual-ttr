#pragma once
#include "build_adapter.hpp"
#include "scene_adapter.hpp"
// Exact 7EB72860 native grip frame. Copy only a rigid transform, never old
// references, collision state or native grab ownership into a new object.
namespace held_pose_adapter {
inline scene::Transform localPose{};inline bool pending=false;
inline bool grip(scene::Transform& pose){
 auto b=build_adapter::heldModule();if(!b)return false;
 const unsigned char raw[]={0x0F,0xB6,0x44,0x24,0x08,0x83,0xEC,0x30};
 if(memcmp(b+0x694CD0,raw,sizeof raw))return false;
 auto manager=reinterpret_cast<void*(__cdecl*)()>(b+0x13903)();pose.scale=1;
 return manager&&reinterpret_cast<bool(__thiscall*)(void*,bool,bool,scene::Vec*,float*)>(b+0x694CD0)(manager,true,false,&pose.p,&pose.r[0][0]);
}
inline scene::Transform relative(const scene::Transform& g,const scene::Transform& p){
 scene::Transform t{};t.p=scene::inverseRotate(g,p.p-g.p);t.scale=p.scale;
 for(int i=0;i<3;i++)for(int j=0;j<3;j++)for(int k=0;k<3;k++)t.r[i][j]+=g.r[k][i]*p.r[k][j];return t;
}
inline bool capture(void* ref){
 auto root=scene::root(ref);scene::Transform g{};pending=root&&grip(g);
 if(pending)localPose=relative(g,scene::world(root));return pending;
}
inline bool target(scene::Transform& t){
 scene::Transform g{};if(!pending||!grip(g))return false;t={};t.p=g.p+scene::rotate(g,localPose.p);t.scale=localPose.scale;
 for(int i=0;i<3;i++)for(int j=0;j<3;j++)for(int k=0;k<3;k++)t.r[i][j]+=g.r[i][k]*localPose.r[k][j];return true;
}
}
