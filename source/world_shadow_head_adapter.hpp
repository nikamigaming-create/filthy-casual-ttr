#pragma once
#include "scene_adapter.hpp"
#include "build_adapter.hpp"
#include <array>
// Exact TTR 1.1.7: tracked bone index 5 is the head. Native writes
// local neck/head scales at 6A481/6A654 from a shared scale value. Original scales
// are retained in the original-local tables. This adapter only COPIES poses.
namespace world_shadow_head {
struct Pose {void* node=nullptr;scene::Transform value{};};
struct View {
 std::array<Pose,128> poses{};unsigned count=0;void* root=nullptr;void* face=nullptr;bool ready=false;
 static scene::Transform join(const scene::Transform& p,const scene::Transform& l){
  scene::Transform t{};t.p=p.p+scene::rotate(p,l.p)*p.scale;t.scale=p.scale*l.scale;
  for(int r=0;r<3;r++)for(int c=0;c<3;c++)for(int k=0;k<3;k++)t.r[r][c]+=p.r[r][k]*l.r[k][c];return t;
 }
 void children(void* node,const scene::Transform& pose,unsigned depth=0){
  if(!node||depth>24||count==poses.size())return;poses[count++]={node,pose};
  auto vt=scene::field<void**>(node,0);if(!reinterpret_cast<void*(__thiscall*)(void*)>(vt[3])(node))return;
  unsigned n=scene::field<unsigned short>(node,0xA6);auto child=scene::field<void**>(node,0xA0);if(n>128||!child)return;
  for(unsigned i=0;i<n;i++)if(child[i])children(child[i],join(pose,scene::local(child[i])),depth+1);
 }
 void prepare(void* player){
  count=0;root=scene::root(player);face=nullptr;ready=false;if(!root)return;
  auto b=build_adapter::heldModule();if(!b)return;
  const unsigned char neckHide[]={0xF3,0x0F,0x11,0x47,0x64};
  const unsigned char headHide[]={0xF3,0x0F,0x11,0x4E,0x64};
  if(memcmp(b+0x6A481,neckHide,sizeof neckHide)||memcmp(b+0x6A654,headHide,sizeof headHide))return;
  auto head=scene::find(root,"Bip01 Head"),neck=scene::find(root,"Bip01 Neck1");
  if(!head||!neck||scene::parent(head)!=neck||!scene::parent(neck))return;
  if(*reinterpret_cast<void**>(b+0x10E5EFC+5*4)!=head||*reinterpret_cast<void**>(b+0x10E6678)!=neck)return;
  if(!b[0x10E6300+5]||!b[0x10E66B0])return;
  auto nw=scene::world(neck),hw=scene::world(head);
  float ns=reinterpret_cast<scene::Transform*>(b+0x10E667C)->scale;
  float hs=reinterpret_cast<scene::Transform*>(b+0x10E5F58+5*0x34)->scale;
  if(!std::isfinite(ns)||!std::isfinite(hs)||ns<.1f||ns>10||hs<.1f||hs>10)return;
  nw.scale=scene::world(scene::parent(neck)).scale*ns;hw.scale=nw.scale*hs;
  if(!std::isfinite(hw.scale)||hw.scale<.1f||hw.scale>10)return;
  poses[count++]={neck,nw};children(head,hw);face=scene::find(root,"FaceGenFace");ready=true;
 }
 scene::Transform world(void* node)const{for(unsigned i=0;i<count;i++)if(poses[i].node==node)return poses[i].value;return scene::world(node);}
 bool includeHidden(void* node)const{
  if(!ready)return false;if(node==face)return true;
  // Mouth/eyes do not change the silhouette. Do not expose gore caps or both
  // hair variants. Native visibility remains untouched for either VR eye.
  const char* n=scene::name(node);
  if(strcmp(n,"FaceGenHairNoHat")&&strcmp(n,"BSFaceGenNiNodeBiped"))return false;
  for(unsigned i=0;i<count;i++)if(poses[i].node==node)return true;return false;
 }
};
}
