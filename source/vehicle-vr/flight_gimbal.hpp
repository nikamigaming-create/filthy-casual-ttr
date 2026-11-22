#pragma once
#include <algorithm>
#include <cmath>
// Cabin-local control geometry. Z is the upright shaft axis, Y is forward.
// The input is the raw tracked grip, never the already-constrained render hand.
namespace flight_gimbal {
struct Axis {float x=0,y=0;};
inline Axis response(float x,float y,float dead=.06f){
 if(!std::isfinite(x)||!std::isfinite(y))return {};
 float length=std::sqrt(x*x+y*y);if(length<=dead)return {};
 float magnitude=std::clamp((length-dead)/(1-dead),0.f,1.f);
 return {x/length*magnitude,y/length*magnitude};
}
inline Axis fromLever(float x,float y,float z,float maxAngleRadians=.34906585f){
 if(!std::isfinite(z)||z<=1.f)return {};
 float scale=std::tan(maxAngleRadians)*z;
 return response(x/scale,y/scale);
}
// A circular gimbal cannot exceed its angular stop on diagonals.
inline Axis angles(Axis input,float limit=.34906585f){
 float radial=std::sqrt(input.x*input.x+input.y*input.y);
 if(radial>1){input.x/=radial;input.y/=radial;}
 return {-std::atan(input.y*std::tan(limit)),std::atan(input.x*std::tan(limit))};
}
}
