#pragma once
#include <algorithm>
#include <cmath>
namespace dialogue_layout {
// Positive yaw puts the panel to the speaker's right. Choose the side nearer
// the viewer's forward centre; at centre use the native right-hand layout.
inline float sideAngle(float width,float panelDistance,float actorDistance,float actorRight){
 const float half=std::atan2(width*.5f,panelDistance);
 const float clearance=std::atan2(.28f,std::max(actorDistance,.5f))+.0523598776f;
 const float angle=std::clamp((half+clearance)*57.2957795f,30.f,60.f);
 return actorRight>.05f?-angle:angle;
}
}
