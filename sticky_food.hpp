#pragma once
#include <cmath>
#include <cstdint>
namespace sticky_food {
// World distances in Fallout units (70 units/metre). Require a deliberate
// outside-to-mouth approach, not food materializing beside the face.
struct MouthGate {
 bool armed=false;uint64_t since=0;
 void reset(){armed=false;since=0;}
 bool update(bool eligible,float distance,uint64_t now){
  if(!eligible||!std::isfinite(distance)){reset();return false;}
  if(distance>=17.5f){armed=true;since=0;return false;}
  if(!armed)return false;
  if(distance>8.4f){since=0;return false;}
  if(!since){since=now;return false;}
  if(now-since<650)return false;
  reset();return true;
 }
};
}
