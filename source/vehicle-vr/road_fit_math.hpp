#pragma once
#include <algorithm>
#include <cmath>
// Presentation suspension over the native collision controller. A rigid
// chassis rests on a support plane; it never follows an isolated ray or cliff.
namespace road_fit {
struct V {float x=0,y=0,z=0;};
inline V operator+(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V operator-(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V operator*(V a,float f){return {a.x*f,a.y*f,a.z*f};}
inline float dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline float length(V a){return std::sqrt(dot(a,a));}
inline V unit(V a){float n=length(a);return n>.00001f?a*(1/n):V{};}
struct Pose {V right{1,0,0},forward{0,1,0},up{0,0,1},position{};};
inline V rotate(const Pose& p,V v){return p.right*v.x+p.forward*v.y+p.up*v.z;}
inline V inverse(const Pose& p,V v){return {dot(v,p.right),dot(v,p.forward),dot(v,p.up)};}
inline bool finite(V v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
inline bool solve(const Pose& original,const V centers[4],const float radii[4],const V hits[4],Pose& output){
 V middle{};for(int i=0;i<4;i++){if(!finite(centers[i])||!finite(hits[i])||!std::isfinite(radii[i])||radii[i]<0)return false;middle=middle+hits[i]*.25f;}
 // Least-squares road z = ax + by + c around its centroid.
 float xx=0,xy=0,yy=0,xz=0,yz=0;
 for(int i=0;i<4;i++){V d=hits[i]-middle;xx+=d.x*d.x;xy+=d.x*d.y;yy+=d.y*d.y;xz+=d.x*d.z;yz+=d.y*d.z;}
 float det=xx*yy-xy*xy;if(!std::isfinite(det)||det<1)return false;
 float a=(xz*yy-yz*xy)/det,b=(yz*xx-xz*xy)/det;
 V up=unit({-a,-b,1});if(up.z<.88f)return false; // no wall/cliff attachment
 V forward=unit(original.forward-up*dot(original.forward,up));if(length(forward)<.9f)return false;
 output=original;output.up=up;output.forward=forward;output.right=unit(cross(forward,up));
 float z=-1e20f;
 for(int i=0;i<4;i++){
  float gap=centers[i].z-radii[i]-hits[i].z;
  if(gap < -15 || gap > 45)return false; // airborne/invalid support, use native
  V local=inverse(original,centers[i]-original.position);
  z=std::max(z,hits[i].z+radii[i]-rotate(output,local).z);
 }
 if(!std::isfinite(z)||std::abs(z-original.position.z)>35)return false;
 output.position.z=z+1;return true;
}
}
