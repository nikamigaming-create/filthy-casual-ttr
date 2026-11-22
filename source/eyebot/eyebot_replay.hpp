#pragma once
// Eyebot portable replay capture. No engine calls, audio, encoder or renderer.
// Native sources publish a complete same-frame scene sample into this buffer.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace eyebot {
struct V3 { float x=0,y=0,z=0; };
inline V3 operator-(V3 a,V3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline float dot(V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline bool finite(V3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
inline float length(V3 p){return std::sqrt(dot(p,p));}
struct Lens {
 V3 eye{},right{1,0,0},up{0,0,1},front{0,1,0};
 float tanH=1.02f,tanV=.574f,nearZ=.08f,farZ=36.f;
 bool valid()const{
  return finite(eye)&&finite(right)&&finite(up)&&finite(front)&&
   std::abs(length(right)-1.f)<.01f&&std::abs(length(up)-1.f)<.01f&&std::abs(length(front)-1.f)<.01f&&
   std::abs(dot(right,up))<.01f&&std::abs(dot(right,front))<.01f&&std::abs(dot(up,front))<.01f&&
   tanH>.05f&&tanH<4.f&&tanV>.05f&&tanV<4.f&&nearZ>0&&farZ>nearZ&&farZ<=300.f;
 }
 bool sees(V3 p,float r)const{
  if(!finite(p)||!(r>=0.f&&r<=25.f))return false;
  auto d=p-eye;float z=dot(d,front);
  if(z+r<nearZ||z-r>farZ)return false;
  return std::abs(dot(d,right))<=tanH*z+r*std::sqrt(1+tanH*tanH)&&
         std::abs(dot(d,up))<=tanV*z+r*std::sqrt(1+tanV*tanV);
 }
};
struct Pose {
 V3 position{};
 float qx=0,qy=0,qz=0,qw=1,scale=1.f;
 bool valid()const{
  const float n=qx*qx+qy*qy+qz*qz+qw*qw;
  return finite(position)&&std::abs(n-1.f)<.01f&&scale>.0001f&&scale<25.f;
 }
};
struct Entity {
 // ID is persistent for a captured lifetime. An asset/rig change must get a
 // new incarnation; joined-joint samples use rig order for this asset.
 uint64_t id=0,asset=0;
 Pose pose{};
 V3 centre{};
 float radius=0;
 const Pose* joints=nullptr;
 uint32_t jointCount=0,flags=0;
};
struct ActorPose {
 uint64_t id=0,asset=0;
 Pose pose{};
 uint32_t jointFirst=0,jointCount=0,flags=0;
};
inline Pose blend(Pose a,Pose b,float t){
 auto l=[t](float x,float y){return x+(y-x)*t;};
 const float side=(a.qx*b.qx+a.qy*b.qy+a.qz*b.qz+a.qw*b.qw)<0.f?-1.f:1.f;
 Pose p{{l(a.position.x,b.position.x),l(a.position.y,b.position.y),l(a.position.z,b.position.z)},
        l(a.qx,b.qx*side),l(a.qy,b.qy*side),l(a.qz,b.qz*side),l(a.qw,b.qw*side),l(a.scale,b.scale)};
 const float norm=std::sqrt(p.qx*p.qx+p.qy*p.qy+p.qz*p.qz+p.qw*p.qw);
 if(!(norm>.001f))return a;
 p.qx/=norm;p.qy/=norm;p.qz/=norm;p.qw/=norm;return p;
}
struct Frame {
 uint64_t timeUs=0,sceneGeneration=0;
 uint32_t first=0,count=0;
};
enum class State {Off,Placed,Parked,Recording,Review,Exporting};
enum class Sample {Skipped,Stored,Full,Incompatible};
struct SnapTicket {
 uint64_t id=0,lensEpoch=0;
 Lens view{};
};
class Snapshot {
 public:
 bool request(const Lens& lens,uint64_t epoch){
  if(!epoch||!lens.valid()||pending||inFlight)return false;
  ticket={++serial,epoch,lens};pending=true;return true;
 }
 bool take(SnapTicket& out,bool renderThreadHasHeadroom){
  if(!pending||!renderThreadHasHeadroom)return false;
  out=ticket;pending=false;inFlight=true;return true;
 }
 bool finish(SnapTicket from,uint64_t currentEpoch,bool rendered){
  if(!inFlight||from.id!=ticket.id||from.lensEpoch!=ticket.lensEpoch)return false;
  inFlight=false;
  if(!rendered||from.lensEpoch!=currentEpoch)return false;
  published=from.id;return true;
 }
 void cancel(){pending=false;inFlight=false;}
 uint64_t shown()const{return published;}
 bool waiting()const{return pending;}
 private:
 SnapTicket ticket{};
 uint64_t serial=0,published=0;
 bool pending=false,inFlight=false;
};
// Calls are owned by one scheduler thread. The render thread receives only a
// value-copy SnapTicket; the encoder/CRT uploader consumes a completed buffer.

class Replay {
 public:
 static constexpr uint32_t MaxActors=96,MaxJointsPerActor=112;
 static constexpr uint64_t PeriodUs=41667,MaxTimeUs=600000000;
 static constexpr size_t MaxBytes=32u*1024*1024;
 State state()const{return mode;}
 const Lens& lens()const{return camera;}
 const std::vector<Frame>& frames()const{return timeline;}
 const std::vector<ActorPose>& actors()const{return dynamic;}
 const std::vector<Pose>& joints()const{return skeleton;}

 bool place(Lens p){
  if((mode!=State::Off&&mode!=State::Placed&&mode!=State::Parked)||!p.valid())return false;
  camera=p;mode=State::Placed;return true;
 }
 bool park(){if(mode!=State::Placed)return false;mode=State::Parked;return true;}
 bool record(uint64_t now,uint64_t cell,size_t budget=MaxBytes){
  if(mode!=State::Parked||!cell||budget<4096||budget>MaxBytes)return false;
  timeline.clear();dynamic.clear();skeleton.clear();
  const size_t timelineBytes=budget/16;
  const size_t actorBytes=budget/4;
  frameCap=std::min<uint32_t>(uint32_t(MaxTimeUs/PeriodUs)+1,uint32_t(timelineBytes/sizeof(Frame)));
  actorCap=uint32_t(actorBytes/sizeof(ActorPose));
  jointCap=uint32_t((budget-timelineBytes-actorBytes)/sizeof(Pose));
  try{timeline.reserve(frameCap);dynamic.reserve(actorCap);skeleton.reserve(jointCap);}
  catch(...){return false;}
  generation=cell;start=now;last=0;mode=State::Recording;return true;
 }
 Sample sample(uint64_t now,uint64_t cell,const Entity* entity,uint32_t count){
  if(mode!=State::Recording)return Sample::Skipped;
  if(cell!=generation)return end(Sample::Incompatible);
  if(now<start||(!timeline.empty()&&now-start<last+PeriodUs))return Sample::Skipped;
  if(!entity||count>MaxActors)return end(Sample::Incompatible);
  if(now-start>MaxTimeUs||timeline.size()==frameCap)return end(Sample::Full);
  uint32_t n=0,j=0;const Entity* visible[MaxActors]{};
  for(uint32_t i=0;i<count;++i){
   const auto& e=entity[i];if(!camera.sees(e.centre,e.radius))continue;
   if(!e.id||!e.asset||!e.pose.valid()||e.jointCount>MaxJointsPerActor||(e.jointCount&&!e.joints))return end(Sample::Incompatible);
   for(uint32_t k=0;k<e.jointCount;++k)if(!e.joints[k].valid())return end(Sample::Incompatible);
   for(uint32_t k=0;k<n;++k)if(visible[k]->id==e.id)return end(Sample::Incompatible);
   visible[n++]=&e;j+=e.jointCount;
  }
  if(dynamic.size()+n>actorCap||skeleton.size()+j>jointCap)return end(Sample::Full);
  const uint32_t begin=uint32_t(dynamic.size());
  for(uint32_t i=0;i<n;++i){
   const auto& e=*visible[i];
   dynamic.push_back({e.id,e.asset,e.pose,uint32_t(skeleton.size()),e.jointCount,e.flags});
   if(e.jointCount)skeleton.insert(skeleton.end(),e.joints,e.joints+e.jointCount);
  }
  last=now-start; timeline.push_back({last,cell,begin,n});return Sample::Stored;
 }
 // The render worker requests a timestamp on the recorded clock. Removal,
 // spawn, asset change and dropped-pose gaps are never interpolated.
 template<class Emit> bool playback(uint64_t at,Emit emit)const{
  if((mode!=State::Review&&mode!=State::Exporting)||timeline.empty())return false;
  auto next=std::upper_bound(timeline.begin(),timeline.end(),at,[](uint64_t t,const Frame& f){return t<f.timeUs;});
  if(next==timeline.begin())return false;
  const auto& a=*(next-1);const bool join=next!=timeline.end()&&next->timeUs-a.timeUs<=2*PeriodUs;
  const float t=join?float(at-a.timeUs)/float(next->timeUs-a.timeUs):0.f;
  Pose staged[MaxJointsPerActor]{};
  for(uint32_t i=0;i<a.count;i++){
   const auto& p=dynamic[a.first+i];const ActorPose* match=nullptr;
   if(join)for(uint32_t k=0;k<next->count;k++){
    const auto& q=dynamic[next->first+k];
    if(q.id==p.id&&q.asset==p.asset&&q.jointCount==p.jointCount){match=&q;break;}
   }
   const Pose* source=p.jointCount?skeleton.data()+p.jointFirst:nullptr;
   if(match&&p.jointCount){for(uint32_t j=0;j<p.jointCount;j++)staged[j]=blend(source[j],skeleton[match->jointFirst+j],t);source=staged;}
   emit(p.id,p.asset,match?blend(p.pose,match->pose,t):p.pose,source,p.jointCount,p.flags);
  }
  return true;
 }
 bool recall(){
  if(mode!=State::Recording)return false;
  mode=timeline.empty()?State::Parked:State::Review;
  return !timeline.empty();
 }
 bool save(){if(mode!=State::Review||timeline.empty())return false;mode=State::Exporting;return true;}
 bool finishExport(bool completed){if(mode!=State::Exporting)return false;mode=State::Review;return completed;}
 bool discard(){
  if(mode!=State::Review)return false;
  timeline.clear();dynamic.clear();skeleton.clear();mode=State::Parked;return true;
 }
 private:
 Sample end(Sample reason){recall();return reason;}
 State mode=State::Off;
 Lens camera{};
 std::vector<Frame> timeline;
 std::vector<ActorPose> dynamic;
 std::vector<Pose> skeleton;
 uint32_t frameCap=0,actorCap=0,jointCap=0;
 uint64_t generation=0,start=0,last=0;
};
}
