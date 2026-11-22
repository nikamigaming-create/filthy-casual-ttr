#pragma once
#include <algorithm>
#include <cmath>

namespace finger_approach {
// TTR supplies smoothstep(distance) across its native approach interval.
// Keep free gestures through the outer 75%, then blend at the contact end.
// The native contact endpoint, ownership and interaction distances are untouched.
inline float freeWeight(float nativeWeight) {
 if(!std::isfinite(nativeWeight))return 0.f;
 constexpr float endOfBlend=.15625f; // smoothstep(.25)
 float x=std::clamp(nativeWeight/endOfBlend,0.f,1.f);
 return x*x*(3.f-2.f*x);
}
}
