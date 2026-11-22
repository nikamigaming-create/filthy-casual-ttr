#pragma once
#include "mounted_frame.hpp"
// Presentation only: stick input still owns native car physics. Blend the
// existing seated lower-body pose toward the actual rim. Never move the HMD,
// torso, pelvis, hands or bone lengths. All positions are engine world units.
namespace knee_steering {
using Mat=mounted_frame::Mat;using Vec=scene::Vec;
inline float weight=0;inline ULONGLONG last=0;inline Vec knees[2]{};
inline float dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec unit(Vec a){float n=scene::distance(a,{});return n>.0001f?a*(1/n):Vec{};}
inline Mat align(Vec a,Vec b){
 a=unit(a);b=unit(b);float c=std::clamp(dot(a,b),-1.f,1.f);auto v=scene::cross(a,b);float s=scene::distance(v,{});if(s<.00001f)return mounted_frame::identity();v=v*(1/s);
 float q[]={v.x,v.y,v.z};Mat m{};for(int i=0;i<3;i++)for(int j=0;j<3;j++)m.a[i][j]=(i==j?c:0)+(1-c)*q[i]*q[j];
 m.a[0][1]-=v.z*s;m.a[0][2]+=v.y*s;m.a[1][0]+=v.z*s;m.a[1][2]-=v.x*s;m.a[2][0]-=v.y*s;m.a[2][1]+=v.x*s;return m;
}
inline Mat rotation(const scene::Transform& t){Mat m;memcpy(m.a,t.r,sizeof m.a);return m;}
inline void orient(void* node,Mat world){auto parent=scene::world(scene::parent(node));auto local=mounted_frame::mul(mounted_frame::transpose(rotation(parent)),world);memcpy(scene::local(node).r,local.a,sizeof local.a);scene::update(node);}
inline void reset(){weight=0;last=0;}
inline void apply(void* root,void* carrier,void* wheel,bool driving,float steer){
 auto now=GetTickCount64();float dt=last?std::min(float(now-last)*.001f,.05f):.016f;last=now;
 float target=driving?1.f:0.f;weight+=std::clamp(target-weight,-dt/0.4f,dt/0.4f);if(weight<.0001f||!root||!carrier||!wheel)return;
 auto frame=scene::world(carrier);auto center=scene::world(wheel).p;
 const char* names[2][3]={{"Bip01 L Thigh","Bip01 L Calf","Bip01 L Foot"},{"Bip01 R Thigh","Bip01 R Calf","Bip01 R Foot"}};
 for(int h=0;h<2;h++){
  auto thigh=scene::find(root,names[h][0]),calf=scene::find(root,names[h][1]),foot=scene::find(root,names[h][2]);if(!thigh||!calf||!foot)continue;
  auto hip=scene::world(thigh),knee=scene::world(calf),ankle=scene::world(foot);float sign=h?1.f:-1.f;
  // Inside of each knee meets the lower outside of the rim. A small opposite
  // knee lift follows steering; no impossible full-wheel knee rotation.
  auto desired=center+scene::rotate(frame,{sign*12.f,-2.f,-6.f+sign*steer*2.f})*frame.scale;
  auto oldDir=knee.p-hip.p,desiredDir=unit(desired-hip.p)*scene::distance(oldDir,{});
  auto blended=unit(oldDir*(1-weight)+desiredDir*weight)*scene::distance(oldDir,{});
  orient(thigh,mounted_frame::mul(align(oldDir,blended),rotation(hip)));
  // Preserve the native lower-leg/foot directions as the knee rises. This
  // changes joint bend while retaining bone lengths and a relaxed ankle.
  orient(calf,rotation(knee));orient(foot,rotation(ankle));knees[h]=scene::world(calf).p;
 }
}
}
