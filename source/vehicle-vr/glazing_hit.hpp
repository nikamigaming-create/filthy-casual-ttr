#pragma once
#include <cmath>
#include <algorithm>
// Engine-independent finite-segment/triangle contact. No trigger-state input,
// shooter identity restriction, or broad vehicle-bounds shattering.
namespace glazing_hit {
struct Vec {float x,y,z;};
inline Vec sub(Vec a,Vec b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec cross(Vec a,Vec b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline float dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline bool segment(Vec start,Vec end,Vec a,Vec b,Vec c,float& fraction){
 auto delta=sub(end,start),e1=sub(b,a),e2=sub(c,a),p=cross(delta,e2);
 float det=dot(e1,p);if(!std::isfinite(det)||std::abs(det)<1e-7f)return false;
 float inv=1/det;auto t=sub(start,a);float u=dot(t,p)*inv;
 if(u<-.00001f||u>1.00001f)return false;
 auto q=cross(t,e1);float v=dot(delta,q)*inv;
 if(v<-.00001f||u+v>1.00001f)return false;
 float f=dot(e2,q)*inv;
 if(!std::isfinite(f)||f<0||f>1)return false;fraction=f;return true;
}
}
