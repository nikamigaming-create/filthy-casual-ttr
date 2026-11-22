#pragma once
#include <algorithm>
#include <cmath>
// Metres/seconds. The executor supplies native wheel casts and retains native
// character collision/gravity. This controller never teleports a vehicle.
namespace arcade_drive {
struct Config {float topSpeed=16,reverseSpeed=4,acceleration=9,braking=24,coast=.22f,steerDegrees=30,wheelbase=3.55f,drag=.0018f;};
struct State {float speed=0,steer=0;};
struct Result {float yawRate=0,frequency=0;bool reverse=false;};
// Remember an uphill approach through the last axle leaving a crest.
// The vertical impulse goes to the native character controller once.
struct Crest {
 float rise=0,groundTime=0,singleSupportTime=0;
 bool armed=false;
 float sample(float dt,float speed,int supports,float grade,bool live){
  if(!live||dt<=0||dt>.1f||speed<8){*this={};return 0;}
  if(supports>=2){
   singleSupportTime=0;
   if(grade>.07f){
    groundTime+=dt;
    rise+=(speed*grade-rise)*(1-std::exp(-dt/.06f));
    armed=groundTime>=.10f;
   }else{*this={};}
   return 0;
  }
  if(supports==1){
   singleSupportTime+=dt;
   if(singleSupportTime>.10f){*this={};}
   return 0;
  }
  float launch=armed?std::clamp(rise,0.f,7.f):0.f;
  *this={};return launch;
 }
};
inline float axis(float v){if(!std::isfinite(v))return 0;float a=std::abs(v);return a<=.12f?0:std::copysign(std::clamp((a-.12f)/.88f,0.f,1.f),v);}
inline float approach(float value,float target,float amount){return value+std::clamp(target-value,-amount,amount);}
inline Result step(State& s,const Config& c,float pedal,float steering,float contact,float dt,bool live){
 if(!std::isfinite(dt)||dt<=0)return {};dt=std::min(dt,.05f);
 if(!std::isfinite(contact))contact=0;contact=std::clamp(contact,0.f,1.f);
 pedal=live?axis(pedal):0;steering=live?axis(steering):0;
 s.steer=approach(s.steer,steering,dt*7.f);
 // Neutral is a free roll, not a commanded zero-speed cruise target. Rolling
 // resistance needs tyre contact; air drag remains in flight. Integrate the
 // quadratic drag analytically so changing render cadence does not change it.
 if(!live){s.speed=approach(s.speed,0,c.braking*dt);}
 else if(s.speed*pedal<0){
  // Opposite pedal brakes to zero before reversing on a later update.
  s.speed=approach(s.speed,0,c.braking*std::abs(pedal)*contact*dt);
 }else{
  float v=std::abs(s.speed),rolling=std::max(0.f,c.coast)*contact,drag=std::max(0.f,c.drag);
  auto resist=[&](float t){
   if(rolling>0 && drag>0){float k=std::sqrt(rolling*drag);v=std::sqrt(rolling/drag)*std::tan(std::max(0.f,std::atan(v*std::sqrt(drag/rolling))-k*t));}
   else if(drag>0)v=v/(1+drag*v*t);else v=std::max(0.f,v-rolling*t);
  };
  resist(dt*.5f);
  float limit=pedal<0?c.reverseSpeed:c.topSpeed;
  // Pedal position commands engine effort. Releasing some throttle never
  // applies brakes or instantly removes speed accumulated under more throttle.
  if(std::abs(pedal)>.001f && v<limit)v=std::min(limit,v+c.acceleration*std::abs(pedal)*contact*dt);
  resist(dt*.5f);
  s.speed=std::copysign(v,s.speed!=0?s.speed:pedal);
 }
 if(std::abs(s.speed)<.015f&&std::abs(pedal)<.001f)s.speed=0;
 float attenuation=1.f/(1.f+std::abs(s.speed)*.035f);
 float yaw=s.speed/c.wheelbase*std::tan(s.steer*c.steerDegrees*.01745329252f*attenuation)*57.2957795f*contact;
 yaw=std::clamp(yaw,-85.f,85.f);
 // Measured original Roadster root travel: Left=480 units/.5s;
 // Backward=240 units/1.433333s. Change frequency, not world coordinates.
 float frequency=std::abs(s.speed)*69.99125f/(s.speed<0?167.44186f:960.f);
 return {yaw,std::clamp(frequency,.001f,4.f),s.speed<-.001f};
}
}
