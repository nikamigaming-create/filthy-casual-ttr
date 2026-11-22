#pragma once
#include <cmath>
#include <algorithm>
// Conservative sphere rejection in D3D clip space (z in [0,w]).
// Tests all six light planes once per batch, before issuing a GPU draw.
namespace shadow_visibility {
// Light-space bounds of the actual loaded casters. Unlike a player-centred
// reach, these include distant geometry and do not change when the head turns.
struct Bounds {
 float lo[3]={1e30f,1e30f,1e30f},hi[3]={-1e30f,-1e30f,-1e30f};
 bool valid=false;
 void sphere(const float* basis,float x,float y,float z,float radius){
  if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||!std::isfinite(radius)||radius<0)return;
  for(int r=0;r<3;r++){float p=basis[r*3]*x+basis[r*3+1]*y+basis[r*3+2]*z;lo[r]=std::min(lo[r],p-radius);hi[r]=std::max(hi[r],p+radius);}valid=true;
 }
};
struct Coverage {
 float x=0,y=0,z=0,half=0,depth=0;
 bool contains(const Bounds& b,float margin=0)const{
  return b.valid&&half>margin&&depth>margin*2&&b.lo[0]>=x-half+margin&&b.hi[0]<=x+half-margin&&
   b.lo[1]>=y-half+margin&&b.hi[1]<=y+half-margin&&b.lo[2]>=z+margin&&b.hi[2]<=z+depth-margin;
 }
 static Coverage fit(const Bounds& b){
  Coverage c;if(!b.valid)return c;
  float span=std::max(b.hi[0]-b.lo[0],b.hi[1]-b.lo[1]);
  // Discrete sizes plus a guard band avoid rescaling for every animated bone.
  c.half=1024;while(c.half<(span*.5f+256)*1.05f)c.half*=2;
  float step=c.half/1024;
  c.x=std::round((b.lo[0]+b.hi[0])*.5f/step)*step;c.y=std::round((b.lo[1]+b.hi[1])*.5f/step)*step;
  c.z=std::floor((b.lo[2]-256)/256)*256;c.depth=std::ceil((b.hi[2]+256-c.z)/256)*256;
  return c;
 }
};
struct Frustum {
 float planes[6][4]{};float lengths[6]{};
 explicit Frustum(const float* m){
  for(int j=0;j<4;j++){
   planes[0][j]=m[12+j]+m[j];planes[1][j]=m[12+j]-m[j];
   planes[2][j]=m[12+j]+m[4+j];planes[3][j]=m[12+j]-m[4+j];
   planes[4][j]=m[8+j];planes[5][j]=m[12+j]-m[8+j];
  }
  for(int i=0;i<6;i++)lengths[i]=std::sqrt(planes[i][0]*planes[i][0]+planes[i][1]*planes[i][1]+planes[i][2]*planes[i][2]);
 }
 bool intersects(float x,float y,float z,float radius)const{
  if(!std::isfinite(radius)||radius<0)return true;
  for(int i=0;i<6;i++)if(planes[i][0]*x+planes[i][1]*y+planes[i][2]*z+planes[i][3]<-radius*lengths[i])return false;
  return true;
 }
};
inline void rebase(float* result,const float* original,float x,float y,float z){
 for(int r=0;r<4;r++){
  for(int c=0;c<4;c++)result[r*4+c]=original[r*4+c];
  result[r*4+3]+=original[r*4]*x+original[r*4+1]*y+original[r*4+2]*z;
 }
}
}
