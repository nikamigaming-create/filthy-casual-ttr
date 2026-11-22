#pragma once
#include "comfort_controls.hpp"
#include "openxr.h"
namespace comfort_xr {
inline XrQuaternionf mul(XrQuaternionf a,XrQuaternionf b){return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};}
inline XrQuaternionf inv(XrQuaternionf q){return {-q.x,-q.y,-q.z,q.w};}
inline XrVector3f rotate(XrQuaternionf q,XrVector3f v){auto r=mul(mul(q,{v.x,v.y,v.z,0}),inv(q));return {r.x,r.y,r.z};}
struct Filter {XrPosef raw{},filtered{};XrSpace base{};XrTime time=0;float ms=0;};inline Filter filters[2];
inline XrPosef filtered(Filter& f,XrPosef p,XrSpace base,XrTime time,float ms){auto delta=fc_comfort::sub({p.position.x,p.position.y,p.position.z},{f.raw.position.x,f.raw.position.y,f.raw.position.z});float dt=float(time-f.time)*1e-9f;
 if(!f.time||base!=f.base||dt<0||dt>.1f||fc_comfort::length(delta)>.3f||ms!=f.ms){f={p,p,base,time,ms};return p;}if(time==f.time)return f.filtered;
 float speed=fc_comfort::length(delta)/std::max(dt,.00001f),a=1-std::exp(-dt/(ms*.001f/(1+speed*8)));
 f.raw=p;f.base=base;f.time=time;auto& o=f.filtered;auto mix=[&](float x,float y){return x+(y-x)*a;};o.position={mix(o.position.x,p.position.x),mix(o.position.y,p.position.y),mix(o.position.z,p.position.z)};
 // Keep native wrist orientation exact. Filtering rotation here can disagree
 // with TTR/VD's separate body/contact solve during a wrist twist.
 o.orientation=p.orientation;return o;
}
inline XrPosef transport(XrPosef raw,XrPosef smooth,XrPosef p){auto q=mul(smooth.orientation,inv(raw.orientation));auto v=rotate(q,{p.position.x-raw.position.x,p.position.y-raw.position.y,p.position.z-raw.position.z});return {mul(q,p.orientation),{smooth.position.x+v.x,smooth.position.y+v.y,smooth.position.z+v.z}};}
}
