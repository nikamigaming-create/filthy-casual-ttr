#pragma once
#include "world_shadow_head_adapter.hpp"
#include <atomic>
// FNV 1.4.0.525 / guarded TTR 1.1.6. Only the native pose transaction writes
// nodes. The next transaction restores every change before TTR runs again.
namespace kill_camera_head {
inline std::atomic<bool> enabled{false},requested{false},dead{false},external{false};
struct Saved {void* node=nullptr;scene::Transform local{},world{};unsigned cull=0;bool transform=false;};
inline std::array<Saved,32> saved{};inline unsigned count=0,publications=0;
inline unsigned attempts=0,reason=0;
inline bool visible=false;inline float cameraDistance=0;
inline void* restHead=nullptr;inline scene::Vec restHeadPosition{},restNeckPosition{};
inline void restore(){
 for(unsigned i=0;i<count;i++){
  auto& s=saved[i];if(s.transform){scene::local(s.node)=s.local;scene::field<scene::Transform>(s.node,0x68)=s.world;}
  auto& flags=scene::field<unsigned>(s.node,0x30);flags=(flags&~1u)|s.cull;
 }
 for(unsigned i=0;i<count;i++)scene::release(saved[i].node);
 count=0;visible=false;
}
inline void remember(void* node,bool transform=false){
 if(!node||count==saved.size())return;
 for(unsigned i=0;i<count;i++)if(saved[i].node==node)return;
 scene::retain(node);saved[count++]={node,scene::local(node),scene::world(node),scene::field<unsigned>(node,0x30)&1u,transform};
}
inline void publishPose(void* node,const scene::Transform& wanted){
 remember(node,true);auto parent=scene::parent(node);if(!parent)return;
 auto base=scene::world(parent);auto local=scene::local(node);
 local.p=scene::inverseRotate(base,wanted.p-base.p)*(1/base.scale);local.scale=wanted.scale/base.scale;
 for(int r=0;r<3;r++)for(int c=0;c<3;c++){local.r[r][c]=0;for(int k=0;k<3;k++)local.r[r][c]+=base.r[k][r]*wanted.r[k][c];}
 scene::local(node)=local;scene::field<scene::Transform>(node,0x68)=wanted;
}
inline void publish(bool completed){
 if(!completed||(!enabled.load()&&!external.load()))return;
 if(!requested.load()&&!external.load()){
  // Preserve the authored head-to-neck offset before TTR clears its tables
  // on death. Its displayed local translation is compensated for .001 scale.
  auto b=build_adapter::heldModule();if(!b)return;
  if(b[0x10E6300+5]&&b[0x10E66B0]){
   auto p=reinterpret_cast<scene::Transform*>(b+0x10E5F58+5*0x34)->p;
   auto n=reinterpret_cast<scene::Transform*>(b+0x10E667C)->p;
   if(std::isfinite(scene::distance(p,{}))&&scene::distance(p,{})<20.f&&
      std::isfinite(scene::distance(n,{}))&&scene::distance(n,{})<20.f){
    restHead=*reinterpret_cast<void**>(b+0x10E5EFC+5*4);restHeadPosition=p;restNeckPosition=n;
   }
  }
  return;
 }
 ++attempts;reason=1;
 auto player=*reinterpret_cast<void**>(0x11DEA3C);if(!player)return;
 // ExtraDataList presence bit for ExtraDismemberedLimbs (0x5F). Never
 // reconstruct a missing head after native dismemberment.
 if(scene::field<unsigned char>(player,0x44+8+0x5F/8)&(1u<<(0x5F%8)))return;
 reason=2;world_shadow_head::View view;view.prepare(player);
 if(!view.ready||dead.load()){
  // Death releases TTR's tracked-bone table. Native animation has restored
  // local scales, but the world transforms still carry TTR's shrink factors.
  // Keep the current ragdoll world positions/rotations, correct only scales.
  if(!build_adapter::heldModule()||!view.root)return;
  auto neck=scene::find(view.root,"Bip01 Neck1"),head=scene::find(view.root,"Bip01 Head");
  if(!neck||!head||scene::parent(head)!=neck||!scene::parent(neck))return;
  float ns=scene::local(neck).scale,hs=scene::local(head).scale;
  if(!std::isfinite(ns)||!std::isfinite(hs)||ns<.1f||ns>10||hs<.1f||hs>10)return;
  auto nl=scene::local(neck),hl=scene::local(head);
  if(restHead==head){hl.p=restHeadPosition;nl.p=restNeckPosition;}
  else if(scene::distance(hl.p,{})>20.f||scene::distance(nl.p,{})>20.f)return;
  auto nw=world_shadow_head::View::join(scene::world(scene::parent(neck)),nl);
  auto hw=world_shadow_head::View::join(nw,hl);
  if(!std::isfinite(hw.scale)||hw.scale<.1f||hw.scale>10)return;
  view.count=0;view.poses[view.count++]={neck,nw};view.children(head,hw);view.ready=true;
 }
 if(view.count<2)return;
 reason=3;auto graph=*reinterpret_cast<void**>(0x11DEB7C);
 auto camera=graph?scene::field<void*>(graph,0xAC):nullptr;if(!camera)return;
 reason=4;cameraDistance=scene::distance(scene::world(camera).p,view.poses[1].value.p);
 // Even in a death/kill state, an eye inside the head must never see a face.
 if(!std::isfinite(cameraDistance)||cameraDistance<35.f)return;
 // Publish local AND world transforms after animation. Calling the native
 // update here re-applies its selective bone transforms and shrinks them again.
 if(view.count+8>saved.size())return;
 for(unsigned i=0;i<view.count;i++)publishPose(view.poses[i].node,view.poses[i].value);
 for(auto name:{"FaceGenFace","FaceGenHairNoHat","FaceGenMouth","FaceGenTeeth",
                "FaceGenTongue","FaceGenEyeLeft","FaceGenEyeRight","BSFaceGenNiNodeBiped"}){
  auto node=scene::find(view.root,name);if(!node)continue;
  remember(node);scene::field<unsigned>(node,0x30)&=~1u;
  // Geometry lives under a separate FaceGen root. Refresh its native bounds
  // and skin after publishing the corrected bone worlds, not the bones again.
  if(strcmp(name,"BSFaceGenNiNodeBiped"))scene::update(node);
 }
 reason=5;visible=true;++publications;
}
}
