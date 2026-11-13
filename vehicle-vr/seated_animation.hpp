#pragma once
#include <array>
#include "mounted_frame.hpp"
#include "body_adapter.hpp"
// Retail 1.4.0.525 live third-person idle sequence. Layout and Update ABI were
// checked against bounded runtime captures, not inferred from conflicting SDKs.
namespace seated_animation {
struct NativeQT {scene::Vec translation;float w,x,y,z,scale;};
static_assert(sizeof(NativeQT)==32);
using Update=bool(__thiscall*)(void*,float,void*,NativeQT*);
inline const char* names[]={"Bip01 NonAccum","Bip01 Pelvis","Bip01 L Thigh","Bip01 L Calf","Bip01 L Foot","Bip01 L Toe0","Bip01 R Thigh","Bip01 R Calf","Bip01 R Foot","Bip01 R Toe0"};
struct Pose {void* root=nullptr;void* sequence=nullptr;float time=0;std::array<void*,10> nodes{};std::array<scene::Transform,10> local{};};
inline Pose pending;
inline scene::Vec lastTarget{},lastImmediate{};inline float localPredictionError=0,localRewriteError=0;inline unsigned lastFlags=0;inline float lastRootScale=0,lastPelvisScale=0;
inline void* ownedSequence=nullptr;inline void* ownedRoot=nullptr;inline void* ownedActor=nullptr;inline double clipClock=0;
inline void* requestedActor=nullptr;inline void* requestedRoot=nullptr;inline void* requestedIdle=nullptr;inline void* requestedSequence=nullptr;inline bool requestSucceeded=false;
inline void restore(){pending={};if(ownedSequence)scene::release(ownedSequence);ownedSequence=ownedRoot=ownedActor=nullptr;requestedActor=requestedRoot=requestedIdle=requestedSequence=nullptr;requestSucceeded=false;clipClock=0;}
inline void advance(float delta){if(ownedSequence&&std::isfinite(delta))clipClock+=std::clamp(delta,0.f,.1f);}
inline void* thirdPersonData(void* actor){
 if(!actor)return nullptr;auto process=scene::field<void*>(actor,0x68);if(!process)return nullptr;
 auto getter=scene::field<void**>(process,0)[0x1B8/4];if(getter!=reinterpret_cast<void*>(0x602150))return nullptr;
 return reinterpret_cast<void*(__thiscall*)(void*)>(getter)(process);
}
inline bool requestedForm(void* idle,void* actor){
 if(!idle||scene::field<void*>(idle,0x34)!=actor)return false;auto form=scene::field<void*>(idle,0x2C);
 return form&&scene::field<unsigned>(form,0x0C)==0x24F83;
}
// One native request per valid actor/root identity, including failed requests.
// A rebuilt player root gets one fresh acquisition instead of losing the pose
// after the old one-shot has ended, or retrying ForcePlayIdle every frame.
inline bool needsRequest(void* actor,void* root){
 auto data=thirdPersonData(actor);return root&&data&&scene::field<void*>(data,8)==root&&(requestedActor!=actor||requestedRoot!=root);
}
inline void noteRequest(void* actor,void* root,bool succeeded){
 requestedActor=actor;requestedRoot=root;requestSucceeded=succeeded;requestedIdle=requestedSequence=nullptr;
 auto data=succeeded?thirdPersonData(actor):nullptr;if(!data)return;
 for(unsigned offset:{0x128u,0x124u}){auto idle=scene::field<void*>(data,offset);if(requestedForm(idle,actor)){requestedIdle=idle;requestedSequence=scene::field<void*>(idle,0x18);break;}}
}
inline bool ownsActive(void* actor){
 auto data=thirdPersonData(actor);if(!data)return false;auto idle=scene::field<void*>(data,0x124);
 if(idle&&ownedSequence&&ownedActor==actor&&scene::field<void*>(idle,0x18)==ownedSequence)return true;
 // Cover dismount before the first body sample. Never stop another active idle
 // merely because our request is still queued; match this successful request.
 if(!idle)idle=scene::field<void*>(data,0x128);if(!idle)return false;
 auto seq=scene::field<void*>(idle,0x18);
 return requestSucceeded&&requestedActor==actor&&scene::field<void*>(data,8)==requestedRoot&&requestedForm(idle,actor)&&(!requestedIdle||idle==requestedIdle)&&(!requestedSequence||seq==requestedSequence);
}
inline unsigned samples=0,applies=0;inline float sourceTime=0;
inline bool finite(float v){return std::isfinite(v)&&std::abs(v)<1e20f;}
inline bool rotation(const NativeQT& q,mounted_frame::Mat& r){
 if(!finite(q.w)||!finite(q.x)||!finite(q.y)||!finite(q.z))return false;
 float n=q.w*q.w+q.x*q.x+q.y*q.y+q.z*q.z;if(n<.5f||n>1.5f)return false;
 // Use the exact native conversion used by NiMultiTargetTransformController.
 // This avoids relying on conflicting SDK row/column quaternion conventions.
 reinterpret_cast<void(__thiscall*)(const float*,mounted_frame::Mat*)>(0x4F0180)(&q.w,&r);return true;
}
inline bool verified(){
 static bool result=[](){const unsigned char getter[]={0x55,0x8B,0xEC,0x51,0x89,0x4D,0xFC,0x8B,0x45,0xFC,0x8B,0x80,0xC0,0x01,0x00,0x00};
 const unsigned char simple[]={0x83,0xEC,0x20,0x53,0x8B,0xD9,0x80,0x7B,0x44,0x00};
 const unsigned char spline[]={0x83,0xEC,0x20,0x53,0x8B,0xD9,0xD9,0x43,0x08};
 const unsigned char quaternion[]={0x55,0x8B,0xEC,0x83,0xEC,0x58};
 return !memcmp(reinterpret_cast<void*>(0x4F0180),quaternion,sizeof quaternion)&&!memcmp(reinterpret_cast<void*>(0x602150),getter,sizeof getter)&&!memcmp(reinterpret_cast<void*>(0xA3FDB0),simple,sizeof simple)&&!memcmp(reinterpret_cast<void*>(0xA50F60),spline,sizeof spline);}();return result;
}
inline void sample(void* actor,void* root){
 pending={};if(!actor||!root||!verified()||!vehicle_body::verified())return;
 auto process=scene::field<void*>(actor,0x68);if(!process)return;
 auto getter=scene::field<void**>(process,0)[0x1B8/4];if(getter!=reinterpret_cast<void*>(0x602150))return;
 auto data=reinterpret_cast<void*(__thiscall*)(void*)>(getter)(process);
 if(!data||scene::field<void*>(data,8)!=root)return;
 if(ownedSequence&&(ownedRoot!=root||ownedActor!=actor))restore();
 if(!ownedSequence){
  if(!requestSucceeded||requestedActor!=actor||requestedRoot!=root)return;
  auto idle=scene::field<void*>(data,0x124);if(!idle)return;
  auto form=scene::field<void*>(idle,0x2C);if(!form||scene::field<unsigned>(form,0x0C)!=0x24F83)return;
  auto seq=scene::field<void*>(idle,0x18);if(!seq)return;
  unsigned state=scene::field<unsigned>(seq,0x44);if(state!=1&&state!=2&&state!=5)return;
  ownedSequence=seq;ownedRoot=root;ownedActor=actor;scene::retain(seq);clipClock=0;
 }
 // The native idle is a one-shot: its text key ends playback even when the
 // controller cycle is LOOP. Retain its actual loaded sequence and drive only
 // the mounted lower branch with a bounded game-time loop. No file mutation,
 // queued-idle spam, external animation asset, or standing-IK pose reuse.
 auto seq=ownedSequence;unsigned count=scene::field<unsigned>(seq,0x0C);
 float begin=scene::field<float>(seq,0x2C),end=scene::field<float>(seq,0x30),frequency=scene::field<float>(seq,0x28);
 if(!finite(begin)||!finite(end)||end-begin<.1f||!finite(frequency)||frequency<=0||frequency>10||count>256)return;
 float time=begin+float(std::fmod(clipClock*frequency,double(end-begin)));
 auto blocks=scene::field<unsigned char*>(seq,0x14),tags=scene::field<unsigned char*>(seq,0x18);if(!blocks||!tags)return;
 Pose result;result.root=root;result.sequence=seq;result.time=time;
 for(unsigned j=0;j<10;j++){
  auto node=scene::find(root,names[j]);if(!node)return;void* interp=nullptr;
  for(unsigned i=0;i<count;i++){auto name=scene::field<const char*>(tags+i*20,0);if(name&&!strcmp(name,names[j])){interp=scene::field<void*>(blocks+i*16,0);break;}}
  if(!interp)return;auto update=scene::field<void**>(interp,0)[0x8C/4];
  if(update!=reinterpret_cast<void*>(0xA3FDB0)&&update!=reinterpret_cast<void*>(0xA50F60))return;
  NativeQT q{};if(!reinterpret_cast<Update>(update)(interp,time,node,&q))return;
  mounted_frame::Mat r;if(!rotation(q,r)||!finite(q.translation.x)||!finite(q.translation.y)||!finite(q.translation.z))return;
  auto t=scene::local(node);memcpy(t.r,r.a,sizeof t.r);t.p=q.translation;
  if(finite(q.scale)&&q.scale>.01f&&q.scale<10)t.scale=q.scale;
  else if(j){
   // These exact-build saved transforms are rest/bind data, not animation.
   // Only their scale is used when the source clip leaves that channel unset;
   // do not inherit a temporary standing-IK leg compression into a seated clip.
   const unsigned indices[]={0,0,10,12,14,16,11,13,15,17};auto base=build_adapter::heldModule();unsigned index=indices[j];
   if(base&&scene::field<void*>(base+0x101AEEC,index*4)==node&&scene::field<unsigned char>(base+0x101B2F0,index)){
    float bindScale=scene::field<float>(base+0x101AF48,index*0x34+0x30);
    if(finite(bindScale)&&bindScale>.01f&&bindScale<10)t.scale=bindScale;
   }
  }
  result.nodes[j]=node;result.local[j]=t;
 }
 pending=result;++samples;sourceTime=time;
}
inline bool apply(void* root,void* body,void* seat){
 auto pose=pending;pending={};if(!pose.root||pose.root!=root||!body||!seat)return false;
 auto pelvis=pose.nodes[1];auto parent=scene::parent(pelvis);if(parent!=pose.nodes[0])return false;
 auto carrier=scene::world(body),parentWorld=scene::world(parent),oldPelvis=scene::world(pelvis);
 lastRootScale=scene::world(root).scale;lastPelvisScale=oldPelvis.scale;
 float actorScale=oldPelvis.scale;if(!finite(actorScale)||actorScale<.1f||actorScale>10||std::abs(parentWorld.scale)<.01f)return false;
 mounted_frame::Mat c,n,p;memcpy(c.a,carrier.r,sizeof c.a);memcpy(n.a,pose.local[0].r,sizeof n.a);memcpy(p.a,pose.local[1].r,sizeof p.a);
 auto targetR=mounted_frame::mul(c,mounted_frame::mul(n,p));
 auto relative=pose.local[0].p+scene::rotate(pose.local[0],pose.local[1].p)*pose.local[0].scale;
 auto position=scene::world(seat).p+scene::rotate(carrier,relative)*actorScale;
 scene::Transform local{};local.p=scene::inverseRotate(parentWorld,position-parentWorld.p)*(1/parentWorld.scale);local.scale=oldPelvis.scale/parentWorld.scale;
 mounted_frame::Mat parentR;memcpy(parentR.a,parentWorld.r,sizeof parentR.a);auto localR=mounted_frame::mul(mounted_frame::transpose(parentR),targetR);memcpy(local.r,localR.a,sizeof local.r);
 for(unsigned i=1;i<10;i++)vehicle_body::releaseWorldOverride(pose.nodes[i]);
 lastTarget=position;localPredictionError=scene::distance(position,parentWorld.p+scene::rotate(parentWorld,local.p)*parentWorld.scale);lastFlags=scene::field<unsigned>(pelvis,0x30);scene::local(pelvis)=local;
 for(unsigned i=2;i<10;i++)scene::local(pose.nodes[i])=pose.local[i];
 // Pelvis and Spine are siblings: this update preserves native tracked hands,
 // head and upper body while the live seated animation drives the lower branch.
 scene::update(pelvis);localRewriteError=scene::distance(local.p,scene::local(pelvis).p);lastImmediate=scene::world(pelvis).p;++applies;return true;
}
}
