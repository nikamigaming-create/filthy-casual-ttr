#pragma once
#include <algorithm>
#include <cmath>
namespace vehicle_drive {
struct Pedals {float gas=0,brake=0;bool reverse=false,changeGear=false;};
// Controller Y commands signed travel. Opposite input brakes to a stop before
// changing the dependency's gear. Triggers remain entirely available to TTR.
inline Pedals pedals(float y,bool live,bool reverse,float momentum){
 Pedals p{};p.reverse=reverse;
 if(!live||!std::isfinite(y)||!std::isfinite(momentum)){p.brake=1;return p;}
 if(std::abs(y)<=.15f){p.brake=.35f;return p;}
 bool wantReverse=y<0;float amount=std::clamp((std::abs(y)-.15f)/.85f,0.f,1.f);
 if(wantReverse!=reverse&&momentum>.01f){p.brake=1;return p;}
 p.reverse=wantReverse;p.changeGear=wantReverse!=reverse;p.gas=amount;return p;
}
}
