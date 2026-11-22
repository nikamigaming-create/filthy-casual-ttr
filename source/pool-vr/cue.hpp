#pragma once
#include <algorithm>
#include <cmath>
namespace poolcue {
constexpr float MaxStanceReach=135.f; // includes room for native nav stopping tolerance

struct V {float x=0,y=0,z=0;};
inline V operator+(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V operator-(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V operator*(V a,float s){return {a.x*s,a.y*s,a.z*s};}
inline float dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline float len(V a){return std::sqrt(dot(a,a));}
// Distance from cue ball back to the rail plus room for a standing actor.
// Coordinates are the upstream 1000x500 physics plane; result is game units.
inline float stanceDistance(float x,float y,float nx,float ny){
 float bx=-109.777778f+x/4.5f,by=55.111111f-y/4.5f,dx=-nx,dy=ny,exit=1e6f;
 if(std::abs(dx)>.0001f)exit=std::min(exit,((dx>0?115.f:-115.f)-bx)/dx);
 if(std::abs(dy)>.0001f)exit=std::min(exit,((dy>0?62.f:-62.f)-by)/dy);
 return std::max(0.f,exit)+42.f;
}
struct Stroke {
 bool valid=false,armed=false; V previous{};
 void reset(){valid=armed=false;}
 // Metres, sampled from the same native game frame.
 float update(V tip,V axis,V ball,float dt,bool usable){
  if(!usable||!std::isfinite(dt)||dt<.001f||dt>.10f){reset();return 0;}
  if(!valid){previous=tip;valid=true;return 0;}
  V step=tip-previous; float distance=len(step);
  if(distance>.25f){previous=tip;armed=false;return 0;}
  float behind=dot(ball-tip,axis);
  if(behind>.055f&&behind<.45f)armed=true;
  V start=previous;previous=tip;
  float speed=dot(step,axis)/dt;
  float t=distance>.00001f?std::clamp(dot(ball-start,step)/(distance*distance),0.f,1.f):0;
  bool hit=len(start+step*t-ball)<.041f;
  if(armed&&speed>.08f&&hit&&dot(ball-start,axis)>0){armed=false;return std::min(speed,4.f);}
  return 0;
 }
};
}
