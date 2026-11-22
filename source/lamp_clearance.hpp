#pragma once
// Small conservative clearance volumes sampled from the live device mesh.
// No collision hooks or changes to the user's hands/animation.
namespace lamp_clearance {
struct Band{float loY=1e9f,hiY=-1e9f,loZ=1e9f,hiZ=-1e9f;};
inline Band bands[12];inline float minX=0,maxX=0;inline bool valid=false;
inline float dot(scene::Vec a,scene::Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline void build(void* pip){
 valid=false;for(auto& b:bands)b=Band{};std::vector<scene::Vec> points;
 auto base=scene::world(pip);if(base.scale<.01f)return;
 for(auto name:{"PipBoyArm:0","PipBoyArm:1","PipBoyArmFemale:0","PipBoyArmFemale:1"})if(auto mesh=scene::find(pip,name)){
  auto data=scene::field<void*>(mesh,0xB8);if(!data)continue;auto count=scene::field<unsigned short>(data,8);auto v=scene::field<scene::Vec*>(data,0x20);if(!v||!count)continue;
  auto w=scene::world(mesh);for(unsigned i=0;i<count;i++)points.push_back(scene::inverseRotate(base,w.p+scene::rotate(w,v[i])*w.scale-base.p)*(1/base.scale));
 }
 if(points.empty())return;minX=maxX=points[0].x;for(auto p:points){minX=std::min(minX,p.x);maxX=std::max(maxX,p.x);}if(maxX-minX<1)return;
 float step=(maxX-minX)/12;
 for(int i=0;i<12;i++){auto& b=bands[i];float center=minX+(i+.5f)*step;
  for(auto p:points)if(std::abs(p.x-center)<step*1.4f){b.loY=std::min(b.loY,p.y);b.hiY=std::max(b.hiY,p.y);b.loZ=std::min(b.loZ,p.z);b.hiZ=std::max(b.hiZ,p.z);}
  if(b.loY>b.hiY)return;
 }valid=true;
}
inline scene::Vec casePush(void* pip,scene::Vec point,float radius,bool routeBelow=false){
 if(!pip||!valid)return {};auto w=scene::world(pip);if(w.scale<.01f)return {};
 auto p=scene::inverseRotate(w,point-w.p)*(1/w.scale);float r=radius/w.scale;
 const float outsideX=std::max({minX-p.x,p.x-maxX,0.f});
 if(outsideX>r+(routeBelow?.80f:0.f))return {};
 float f=std::clamp((p.x-minX)/(maxX-minX)*12-.5f,0.f,11.f);int i=int(f),j=std::min(i+1,11);float t=f-i;
 auto mix=[&](float a,float b){return a+(b-a)*t;};
 float loY=mix(bands[i].loY,bands[j].loY),hiY=mix(bands[i].hiY,bands[j].hiY),loZ=mix(bands[i].loZ,bands[j].loZ),hiZ=mix(bands[i].hiZ,bands[j].hiZ);
 // The underside cable always leaves below this wall. Nearest-face
 // projection would flip its centers to the lid at a case edge.
 if(routeBelow){
  const float outsideZ=std::max({loZ-p.z,p.z-hiZ,0.f});
  const float exit=std::clamp((std::max(outsideX,outsideZ)-r)/.80f,0.f,1.f);
  const float contact=1.f-exit*exit*(3.f-2.f*exit);
  return scene::rotate(w,{0,std::min(0.f,loY-r-p.y)*contact,0})*w.scale;
 }
 float y=p.y-std::clamp(p.y,loY,hiY),z=p.z-std::clamp(p.z,loZ,hiZ),d=std::sqrt(y*y+z*z);scene::Vec push{};
 if(d>=r)return {};
 if(d>.001f)push={0,y*(r-d)/d,z*(r-d)/d};
 else {float distances[]={p.y-loY,hiY-p.y,p.z-loZ,hiZ-p.z};int best=0;for(int k=1;k<4;k++)if(distances[k]<distances[best])best=k;
  if(best<2)push.y=(best?1.f:-1.f)*(distances[best]+r);else push.z=(best==3?1.f:-1.f)*(distances[best]+r);
 }
 return scene::rotate(w,push)*w.scale;
}
inline scene::Vec handPush(scene::Vec point,scene::Vec wrist,scene::Vec knuckles,scene::Vec outward,float radius){
 auto axis=knuckles-wrist;float denom=dot(axis,axis);if(denom<.01f)return {};
 auto on=wrist+axis*std::clamp(dot(point-wrist,axis)/denom,0.f,1.f);auto delta=point-on;float d=scene::distance(delta,{}),required=3.1f+radius;
 if(d>=required)return {};return (d>.001f?delta*(1/d):outward)*(required-d);
}
}
