#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace fc_comfort {
struct Config {uint32_t size=sizeof(Config),version=1;uint32_t fingers=0,grip=0,smoothing=0,walking=0,autoWalk=0;float press=.70f,release=.25f,smoothMs=12,walkSpeed=.55f,deadzone=.15f;uint32_t gameplay=0,walkRequest=0;float runStart=.60f,sprintStart=.97f;};
inline void sanitize(Config& c){auto clamp=[](float& v,float d,float l,float h){v=std::isfinite(v)?std::clamp(v,l,h):d;};clamp(c.press,.7f,.15f,.95f);clamp(c.release,.25f,.05f,c.press-.05f);clamp(c.smoothMs,12,1,30);clamp(c.walkSpeed,.55f,.2f,.85f);clamp(c.deadzone,.15f,.05f,.4f);
 clamp(c.sprintStart,.97f,.8f,1.f);
 clamp(c.runStart,.60f,c.deadzone+.10f,c.sprintStart-.12f);
}
struct Grip {
 bool down=false,armed=false,enabled=false;float releaseAt=.25f;
 float value(float raw,bool active,const Config& c){
  if(!active||!std::isfinite(raw)){*this={};return 0;}
  if(!c.grip){*this={};return raw;}
  // Enabling/recovering while already holding must not synthesize a release.
  // Adopt the new sensitivity after the first real neutral input.
  if(!enabled){enabled=true;releaseAt=c.release;armed=raw<=releaseAt;return raw;}
  if(!armed){if(raw<=releaseAt)armed=true;return raw;}
  if(down){if(raw<=releaseAt)down=false;}
  else if(raw>=c.press){down=true;releaseAt=c.release;}
  return down?1.f:0.f;
 }
};
struct Walker {
 bool active=false,running=false;
 uint32_t request=0;
 void apply(float& x,float& y,bool valid,const Config& c){
  const bool toggle=request!=c.walkRequest;
  request=c.walkRequest;
  if(!valid||!c.gameplay||!c.walking){active=running=false;return;}
  const float radius=std::sqrt(x*x+y*y);
  if(!std::isfinite(radius)){x=y=0;active=running=false;return;}
  const float walkMax=.23f+.25f*c.walkSpeed;
  if(!c.autoWalk||radius>c.deadzone)active=false;
  else if(toggle)active=!active;
  if(active){x=0;y=walkMax;running=false;return;}
  if(radius<=c.deadzone){x=y=0;running=false;return;}

  const float n=std::min(radius,1.f);
  running=n>=c.runStart-(running?.025f:0);
  // Leave a useful physical band for each gait. Sprint reads the unshaped
  // vector, so the running plateau cannot trigger it early.
  const float walkTravel=std::clamp((n-c.deadzone)/(c.runStart-c.deadzone),0.f,1.f);
  const float runTravel=std::clamp((n-c.runStart)/(c.sprintStart-c.runStart),0.f,1.f);
  const float magnitude=running?.85f+.15f*runTravel:.20f+(walkMax-.20f)*walkTravel;
  x*=magnitude/radius;
  y*=magnitude/radius;
 }
};
struct V {float x,y,z;};inline V sub(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}inline float length(V a){return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);}
inline bool fingerCurl(const V* p,int count,float& curl){float sum=0;for(int i=1;i<count-1;i++){auto a=sub(p[i],p[i-1]),b=sub(p[i+1],p[i]);float la=length(a),lb=length(b);if(!std::isfinite(la)||!std::isfinite(lb)||la<.001f||lb<.001f)return false;sum+=std::acos(std::clamp((a.x*b.x+a.y*b.y+a.z*b.z)/(la*lb),-1.f,1.f));}curl=std::clamp(sum/(count==4?2.4f:3.6f),0.f,1.f);return std::isfinite(curl);}
}
