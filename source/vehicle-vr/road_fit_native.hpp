#pragma once
#include "road_fit_math.hpp"
#include "road_contacts.hpp"
namespace road_fit_native {
inline road_fit::V v(scene::Vec a){return {a.x,a.y,a.z};}
inline scene::Vec v(road_fit::V a){return {a.x,a.y,a.z};}
// A zero-time NiUpdateDownwardPass also runs controllers and collision sync.
// Suspension is a render transform: transport the published world hierarchy
// directly, retaining every local animation and every Havok transform.
inline bool setVisualWorld(void* root,const scene::Transform& wanted){
 if(!root)return false;auto old=scene::world(root);
 if(!std::isfinite(old.scale)||old.scale<.001f||std::abs(old.scale-wanted.scale)>.001f)return false;
 std::vector<void*> nodes{root};
 for(size_t i=0;i<nodes.size();++i){
  auto n=nodes[i];auto vt=scene::field<void**>(n,0);
  if(!reinterpret_cast<void*(__thiscall*)(void*)>(vt[3])(n))continue;
  auto count=scene::field<unsigned short>(n,0xA6);auto children=scene::field<void**>(n,0xA0);
  if(count>512||nodes.size()+count>512)return false;
  for(unsigned j=0;children&&j<count;j++)if(children[j])nodes.push_back(children[j]);
 }
 auto point=[&](scene::Vec p){return wanted.p+scene::rotate(wanted,scene::inverseRotate(old,p-old.p));};
 for(auto n:nodes){
  auto t=scene::world(n);auto result=t;result.p=point(t.p);
  for(int c=0;c<3;c++){auto a=scene::rotate(wanted,scene::inverseRotate(old,{t.r[0][c],t.r[1][c],t.r[2][c]}));for(int j=0;j<3;j++)result.r[j][c]=(&a.x)[j];}
  scene::field<scene::Transform>(n,0x68)=result;
  if(auto bound=scene::field<void*>(n,0x20))scene::field<scene::Vec>(bound,0)=point(scene::field<scene::Vec>(bound,0));
 }
 return true;
}
struct State {
 void* node=nullptr;
 scene::Transform nativePose{},shownPose{};
 bool showing=false;
 float offset=0;
 unsigned applied=0;
 float maxGapBefore=0,maxGapAfter=0;
 scene::Vec lastUp{0,0,1};
 float unsupportedSeconds=0;
inline void* visualRoot(Form* actor){return actor?scene::find(scene::root(actor),"Bip01"):nullptr;}
inline bool same(const scene::Transform& a,const scene::Transform& b){
 const float* x=reinterpret_cast<const float*>(&a);const float* y=reinterpret_cast<const float*>(&b);
 for(int i=0;i<13;i++)if(!std::isfinite(x[i])||std::abs(x[i]-y[i])>.0001f)return false;return true;
}
inline void restore(Form* actor){
 if(showing&&actor&&visualRoot(actor)==node&&same(scene::world(node),shownPose))setVisualWorld(node,nativePose);
 showing=false;
}
inline void clear(Form* actor){restore(actor);node=nullptr;offset=0;lastUp={0,0,1};unsupportedSeconds=0;}
inline void finishPublication(Form* actor){
 // Native publication can refresh animation after the camera sampled the
 // fitted eye. Re-publish this exact frame's solved visual transform so the
 // cabin and the camera use the same pose; do not solve or smooth twice.
 if(showing&&visualRoot(actor)==node)setVisualWorld(node,shownPose);
}
inline scene::Vec nativePoint(scene::Vec p)const{
 if(!showing)return p;
 return nativePose.p+scene::rotate(nativePose,scene::inverseRotate(shownPose,p-shownPose.p));
}
inline void presentCached(Form* actor){
 if(!actor||visualRoot(actor)!=node||!applied)return;
 restore(actor);auto pose=scene::world(node),output=pose;
 auto up=road_fit::unit(v(lastUp));
 auto forward=road_fit::unit(v(scene::rotate(pose,{0,1,0}))-up*road_fit::dot(v(scene::rotate(pose,{0,1,0})),up));
 auto right=road_fit::unit(road_fit::cross(forward,up));output.p.z+=offset;
 for(int j=0;j<3;j++){output.r[j][0]=(&right.x)[j];output.r[j][1]=(&forward.x)[j];output.r[j][2]=(&up.x)[j];}
 nativePose=pose;if(setVisualWorld(node,output)){shownPose=scene::world(node);showing=true;}
}
inline void apply(Form* actor,Form* player,float dt){
 if(!actor||!std::isfinite(dt)||dt<=0)return;
 auto actorRoot=scene::root(actor);auto root=visualRoot(actor);
 // Never transform the reference's root/collision/culling origin. Only the
 // visual bone hierarchy receives suspension; native motion owns actorRoot.
 if(!actorRoot||!root||!scene::parent(root))return;
 if(node!=root){node=root;offset=0;lastUp={0,0,1};showing=false;unsupportedSeconds=0;}
 restore(actor);
 auto pose=scene::world(root);bool measured=scene::find(actorRoot,"FCRoadWheel0")!=nullptr;
 // Imported chassis proxies are deliberately excluded from visual fit.
 if(!measured&&scene::find(actorRoot,"FCRoadContact0"))return;
 const char* actual[]={"FCRoadWheel0","FCRoadWheel1","FCRoadWheel2","FCRoadWheel3"};
 const char* radial[]={"FCRoadRadius0","FCRoadRadius1","FCRoadRadius2","FCRoadRadius3"};
 const char* original[]={"Bip01 HandLeft","Bip01 HandRight","Bip01 FootLeft","Bip01 FootRight"};
 road_fit::V centers[4]{},hits[4]{};float radii[4]{};
 bool support=true;
 for(int i=0;i<4;i++){
  auto wheel=scene::find(actorRoot,measured?actual[i]:original[i]);if(!wheel)return;
  auto p=scene::world(wheel).p;float radius=(i<2?27.11194f:45.11193f)*pose.scale;
  if(measured){auto r=scene::find(wheel,radial[i]);if(!r)return;radius=scene::distance(p,scene::world(r).p);}
  scene::Vec hit=p+scene::Vec{0,0,-radius-48},normal{};Form* owner{};
  if(!road_contacts::ray(p+scene::Vec{0,0,24-radius},hit,normal,owner,actor,player)||normal.z<.88f)support=false;
  centers[i]=v(p);hits[i]=v(hit);radii[i]=radius;
 }
 road_fit::Pose originalPose{{pose.r[0][0],pose.r[1][0],pose.r[2][0]},
  {pose.r[0][1],pose.r[1][1],pose.r[2][1]},{pose.r[0][2],pose.r[1][2],pose.r[2][2]},v(pose.p)},desired{};
 support=support&&road_fit::solve(originalPose,centers,radii,hits,desired);
 if(support)unsupportedSeconds=0;
 else {
  // A missed cast at a triangle seam must not snap the entire cabin back to
  // its uncorrected pose. Briefly retain a relative suspension offset, then
  // smoothly release it. Never retain a world anchor over a cliff/airborne.
  unsupportedSeconds+=std::min(dt,.05f);
  desired=originalPose;
  if(unsupportedSeconds<.10f){desired.position.z+=offset;desired.up=v(lastUp);}
 }
 float blend=1-std::exp(-std::min(dt,.05f)/.075f);
 offset+=(desired.position.z-pose.p.z-offset)*blend;
 if(std::abs(offset)<.0001f)offset=0;
 auto up=road_fit::unit(v(lastUp)*(1-blend)+desired.up*blend);lastUp=v(up);
 auto forward=road_fit::unit(originalPose.forward-up*road_fit::dot(originalPose.forward,up));auto right=road_fit::unit(road_fit::cross(forward,up));
 auto output=pose;output.p.z+=offset;
 for(int j=0;j<3;j++){output.r[j][0]=(&right.x)[j];output.r[j][1]=(&forward.x)[j];output.r[j][2]=(&up.x)[j];}
 nativePose=pose;
 if(setVisualWorld(root,output)){
  shownPose=scene::world(root);showing=true;++applied;maxGapBefore=maxGapAfter=-1e10f;
  for(int i=0;support&&i<4;i++){
   maxGapBefore=std::max(maxGapBefore,centers[i].z-radii[i]-hits[i].z);
   auto wheel=scene::find(actorRoot,measured?actual[i]:original[i]);
   maxGapAfter=std::max(maxGapAfter,scene::world(wheel).p.z-radii[i]-hits[i].z);
  }
 }
}
};
inline State current;
inline unsigned& applied=current.applied;
inline bool& showing=current.showing;
inline float& offset=current.offset;
inline float& maxGapBefore=current.maxGapBefore;
inline float& maxGapAfter=current.maxGapAfter;
inline void restore(Form* actor){current.restore(actor);}
inline void clear(Form* actor){current.clear(actor);}
inline void apply(Form* actor,Form* player,float dt){current.apply(actor,player,dt);}
}
