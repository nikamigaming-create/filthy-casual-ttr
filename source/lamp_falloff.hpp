#pragma once
#include <algorithm>
namespace lamp_falloff {
inline float fade(float distance,float range){
 if(!(range>0)||!(distance>=0))return 0;
 float x=std::clamp(distance/range,0.f,1.f),edge=std::clamp((x-.72f)/.28f,0.f,1.f);
 return (1-edge*edge*(3-2*edge))/(1+1.25f*x*x);
}
}
