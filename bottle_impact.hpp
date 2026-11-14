#pragma once
#include <cmath>
#include <algorithm>
namespace bottle_impact {
struct Vec {float x=0,y=0,z=0;Vec operator-(Vec b)const{return{x-b.x,y-b.y,z-b.z};}};
inline float dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline float length(Vec a){return std::sqrt(dot(a,a));}
struct Sample {bool valid=false,held=false,contact=false;unsigned long long body=0;double seconds=0;Vec velocity{},surfaceVelocity{};};
struct Verdict {bool smash=false;float closingSpeed=0,velocityChange=0;};
// Native Havok velocities converted to metres/second by the adapter. A fast
// airborne object never qualifies. A soft/glancing contact with little normal
// velocity change also survives. Invalid/stale/reacquired samples reset history.
inline Verdict judge(const Sample& before,const Sample& now){
 Verdict v;double dt=now.seconds-before.seconds;
 if(!before.valid||!now.valid||before.held||now.held||!now.contact||!now.body||now.body!=before.body||dt<=0||dt>.1)return v;
 Vec relative=before.velocity-now.surfaceVelocity,change=before.velocity-now.velocity;
 v.velocityChange=length(change);if(!std::isfinite(v.velocityChange)||v.velocityChange<1.25f)return v;
 v.closingSpeed=dot(relative,change)/v.velocityChange;
 v.smash=std::isfinite(v.closingSpeed)&&v.closingSpeed>=2.5f;
 return v;
}
}
