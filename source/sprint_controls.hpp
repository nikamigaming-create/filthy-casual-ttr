#pragma once
#include <algorithm>
#include <cmath>
namespace sprint_controls {
struct Config {
 bool enabled=true;
 float start=.97f,stop=.90f,multiplier=2.f,apPerSecond=15.f;
 void sanitize(){
  auto clamp=[](float v,float fallback,float lo,float hi){return std::isfinite(v)?std::clamp(v,lo,hi):fallback;};
  start=clamp(start,.97f,.8f,1.f);stop=clamp(stop,.90f,.5f,start-.02f);
  multiplier=clamp(multiplier,2.f,1.f,2.5f);apPerSecond=clamp(apPerSecond,15.f,0.f,100.f);
 }
};
struct State {
 bool active=false,exhausted=false,awaitNeutral=false;
 float releasedFor=0;
 void reset(){active=false;exhausted=false;awaitNeutral=false;releasedFor=0;}
 void suspend(){reset();awaitNeutral=true;}
 float update(const Config& c,bool allowed,float x,float y,float ap,float dt){
  if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(ap)||!std::isfinite(dt)){suspend();return 0;}
  if(!allowed){suspend();return 0;}
  dt=std::clamp(dt,0.f,.1f);
  const float radius=std::sqrt(x*x+y*y),forward=radius>.001f?y/radius:0;
  // Real sticks move on a circle. Forward diagonals must not fall out of
  // sprint merely because their Y component is less than the tip threshold.
  if(radius<.65f)releasedFor+=dt;else releasedFor=0;
  if(releasedFor>=.10f){exhausted=false;awaitNeutral=false;}
  if(!c.enabled||awaitNeutral||radius<c.stop||forward<(active?.60f:.75f)){active=false;return 0;}
  if(c.apPerSecond>0&&ap<=.01f){active=false;exhausted=true;return 0;}
  if(exhausted)return 0;
  if(!active&&radius>=c.start)active=c.apPerSecond==0||ap>=std::max(1.f,c.apPerSecond*.5f);
  float cost=active?c.apPerSecond*dt:0.f;
  // Latch on the debit that empties AP, before regeneration can refill it.
  if(active&&c.apPerSecond>0&&cost>=ap){active=false;exhausted=true;return std::max(0.f,ap);}
  return cost;
 }
};
}
