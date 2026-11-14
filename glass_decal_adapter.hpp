#pragma once
#include "bottle_impact_adapter.hpp"
#include <string>
// FNV 1.4.0.525, decoded retail text matching SHA256
// 518c87f58a6c4d9826e9ef8fbb7f4213882fa70822675610d45aea2464502a57.
// Layout and public entry point cross-checked with JIP LN GameProcess.h.
namespace glass_decal_adapter {
using scene::Vec;
struct Decal {
 Vec position,normal,point18;void* actor;void* node;unsigned unk2C;void* textureSet;int index;
 float width,height,depth,rng44;void* cell;float parallaxScale;void* skinned;float specular,epsilon,radius;
 Vec color;unsigned hitFlags;unsigned char quadrant,byte71,byte72,parallax,alphaTest,alphaBlend,passes,modelSpace,forceFade,twoSided,pad[2];
};
static_assert(sizeof(Decal)==0x7C);static_assert(offsetof(Decal,textureSet)==0x30);
inline bool supported(){
 static const unsigned char ctor[]={0x55,0x8B,0xEC,0x83,0xEC,0x10,0x89,0x4D,0xF0};
 static const unsigned char add[]={0x55,0x8B,0xEC,0x6A,0xFF,0x68,0x03,0xFC,0xEE,0x00};
 return !memcmp(reinterpret_cast<void*>(0x4A37B0),ctor,sizeof ctor)&&!memcmp(reinterpret_cast<void*>(0x4A10D0),add,sizeof add);
}
inline void* parentNode(void* body){
 using bottle_impact_adapter::readable;if(!readable(body,0x80))return nullptr;
 auto entries=scene::field<unsigned char*>(body,0x78);auto count=scene::field<unsigned>(body,0x7C);
 if(count>128||!readable(entries,count*0x10))return nullptr;
 for(unsigned i=0;i<count;i++){auto e=entries+i*0x10;auto tag=scene::field<unsigned>(e,0);void* node=nullptr;
  if(tag==0x1267B70){auto c=scene::field<void*>(e,8);if(readable(c,0xC))node=scene::field<void*>(c,8);}
  else if(tag==0x11F4280)node=scene::field<void*>(e,8);
  if(readable(node,0x9C))return node;
 }return nullptr;
}
// Native TES ray projection, as used by JIP GetRayCastObject. No live collision
// layer edits. Skip only the breaking bottle's render subtree with bounded rays.
inline void* raySurface(Vec& point,Vec& normal,void* bottle){
 static const unsigned char signature[]={0x55,0x8B,0xEC,0x83,0xEC,0x0C,0x89,0x4D,0xF8};
 if(memcmp(reinterpret_cast<void*>(0x458440),signature,sizeof signature))return nullptr;
 auto tes=*reinterpret_cast<void**>(0x11DEA10);if(!tes)return nullptr;
 Vec start=point+normal*8,end=point-normal*40;
 for(int attempt=0;attempt<12;attempt++){
  alignas(16) unsigned char ray[0xB0]{};
  scene::field<Vec>(ray,0)=start*0.14287498593330383f;scene::field<Vec>(ray,0x10)=end*0.14287498593330383f;
  scene::field<unsigned>(ray,0x24)=6;scene::field<float>(ray,0x40)=1;
  scene::field<int>(ray,0x44)=scene::field<int>(ray,0x50)=-1;
  auto node=reinterpret_cast<void*(__thiscall*)(void*,void*,bool)>(0x458440)(tes,ray,true);
  if(!node)return nullptr;float fraction=scene::field<float>(ray,0x40);
  auto rayStart=scene::field<Vec>(ray,0),rayEnd=scene::field<Vec>(ray,0x10);
  auto hit=(rayStart+(rayEnd-rayStart)*fraction)*6.9991254806518555f;bool own=false;
  for(auto p=node;p;p=scene::parent(p)){if(p==bottle){own=true;break;}}
  if(!own){point=hit;auto n=scene::field<Vec>(ray,0x30);float length=scene::distance(n,{});if(length>.5f&&length<1.5f)normal=n*(1.f/length);return node;}
  start=hit-normal*.5f;if(scene::distance(start,end)<1)return nullptr;
 }return nullptr;
}
inline void* manager(){return supported()?*reinterpret_cast<void**>(0x11C57F8):nullptr;}
inline unsigned count(){auto m=manager();return m?scene::field<unsigned>(m,0x10):0;}
inline unsigned attempts(){return supported()?*reinterpret_cast<unsigned*>(0x11C57E4):0;}
// Native bullet-impact particle factory (cdecl, 12 stack words), used by
// Projectile impact at 88F245. It owns loading, cell attachment and expiry.
inline bool puff(Vec point,Vec normal,void* cell){
 static const unsigned char signature[]={0x55,0x8B,0xEC,0x6A,0xFF,0x68,0xFF,0x11,0xF0,0x00};
 if(!cell||memcmp(reinterpret_cast<void*>(0x6890B0),signature,sizeof signature))return false;
 using Spawn=void*(__cdecl*)(void*,float,const char*,Vec,Vec,float,unsigned,void*);
 return reinterpret_cast<Spawn>(0x6890B0)(cell,.45f,"Effects\\ImpactBallisticGlass01.NIF",normal,point,.4f,0,nullptr)!=nullptr;
}
struct Probe{void* effect;unsigned ticks;};
inline std::vector<Probe> probes;inline bool diagnostics=false;
inline std::vector<std::string> diagnosticTargets;
inline void clearProbes(){for(auto p:probes)scene::release(p.effect);probes.clear();}
template<class Log>void inspect(Log log){
 for(auto& name:diagnosticTargets)log("GLASS TARGET name=%s",name.c_str());diagnosticTargets.clear();
 for(auto i=probes.begin();i!=probes.end();){auto e=i->effect;
 if(i->ticks<5)log("GLASS NATIVE tick=%u age=%.3f life=%.3f init=%u processed=%u ready=%u verts=%u tris=%u geom=%p parent=%p tex=%p",i->ticks,scene::field<float>(e,0x10),scene::field<float>(e,8),scene::field<unsigned char>(e,0x14),scene::field<unsigned char>(e,0x18),scene::field<unsigned char>(e,0x19),scene::field<unsigned>(e,0xC4),scene::field<unsigned>(e,0xC8),scene::field<void*>(e,0x28),scene::field<void*>(e,0x10C),scene::field<void*>(e,0x30));
 if(i->ticks==4&&scene::field<void*>(e,0x28)){auto geom=scene::field<void*>(e,0x28);std::vector<scene::Box> boxes;scene::boxes(geom,boxes);Vec extent;auto c=scene::boxCenter(boxes,&extent);log("GLASS MESH center=%.2f %.2f %.2f size=%.2f %.2f %.2f flags=%08X texpath=%s",c.x,c.y,c.z,extent.x,extent.y,extent.z,scene::field<unsigned>(geom,0x30),scene::field<const char*>(scene::field<void*>(e,0x30),0x3C));}
 if(++i->ticks>5){scene::release(e);i=probes.erase(i);}else ++i;}}
inline bool soundPlaySupported(){
 static const unsigned char entry[]={0x55,0x8B,0xEC,0x51,0x89,0x4D,0xFC};
 auto p=reinterpret_cast<unsigned char*>(0xAD8830);if(!memcmp(p,entry,sizeof entry))return true;
 // Preserve TTR's own sound interception rather than bypassing it.
 if(*p!=0xE9)return false;auto destination=p+5+*reinterpret_cast<int*>(p+1);HMODULE owner=nullptr;
 if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<const char*>(destination),&owner))return false;
 return owner==GetModuleHandleW(L"TTWVR.dll")&&build_adapter::heldModule()!=nullptr;
}
inline int soundStage=0;
inline bool smashSound(Vec point){
 soundStage=1;
 static const unsigned char init[]={0x55,0x8B,0xEC,0x81,0xEC,0x3C,0x01,0,0};
 static const unsigned char small[]={0x55,0x8B,0xEC,0x51,0x89,0x4D,0xFC};
 if(memcmp(reinterpret_cast<void*>(0xAE5870),init,sizeof init)||memcmp(reinterpret_cast<void*>(0xAD8B60),small,sizeof small)||!soundPlaySupported())return false;
 soundStage=2;
 struct Sound{unsigned key=~0u;unsigned flags=0,unk=0;}sound;
 reinterpret_cast<void(__thiscall*)(void*,Sound*,unsigned,unsigned)>(0xAE5870)(reinterpret_cast<void*>(0x11F6EF0),&sound,0xAFA74,0x102);
 if(sound.key==~0u)return false;soundStage=3;
 reinterpret_cast<void(__thiscall*)(Sound*,float,float,float)>(0xAD8B60)(&sound,point.x,point.y,point.z);
 reinterpret_cast<void(__thiscall*)(Sound*,unsigned)>(0xAD8830)(&sound,0);return true;
}
// Only renderable triangles intersected by the impact projection are eligible.
// Bounding boxes alone admitted weather meshes and omitted landscape render siblings.
inline float dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline bool triangleHit(Vec origin,Vec delta,Vec a,Vec b,Vec c){
 auto e=b-a,f=c-a,h=scene::cross(delta,f);float det=dot(e,h);if(std::abs(det)<1e-7f)return false;
 auto q=origin-a;float u=dot(q,h)/det;if(u<-.001f||u>1.001f)return false;
 auto k=scene::cross(q,e);float v=dot(delta,k)/det;if(v<-.001f||u+v>1.001f)return false;
 float t=dot(f,k)/det;return t>=0&&t<=1;
}
inline bool projectedSurface(void* node,Vec point,Vec normal){
 using bottle_impact_adapter::readable;auto vt=scene::field<void**>(node,0);
 if(!reinterpret_cast<void*(__thiscall*)(void*)>(vt[7])(node))return false;
 auto data=scene::field<void*>(node,0xB8);if(!readable(data,0x50))return false;
 unsigned count=scene::field<unsigned short>(data,8);auto vertices=scene::field<Vec*>(data,0x20);
 if(!count||!readable(vertices,count*sizeof(Vec)))return false;
 auto w=scene::world(node);if(!std::isfinite(w.scale)||std::abs(w.scale)<.001f)return false;
 auto o=scene::inverseRotate(w,point+normal*16-w.p)*(1/w.scale);
 auto d=scene::inverseRotate(w,normal*-32)*(1/w.scale);
 auto hit=[&](unsigned a,unsigned b,unsigned c){return a<count&&b<count&&c<count&&triangleHit(o,d,vertices[a],vertices[b],vertices[c]);};
 if(reinterpret_cast<void*(__thiscall*)(void*)>(vt[9])(node)){
  unsigned n=scene::field<unsigned short>(data,0x40);auto ix=scene::field<unsigned short*>(data,0x48);
  if(!readable(ix,n*6))return false;for(unsigned i=0;i<n;i++)if(hit(ix[i*3],ix[i*3+1],ix[i*3+2]))return true;
 }else if(reinterpret_cast<void*(__thiscall*)(void*)>(vt[8])(node)){
  unsigned n=scene::field<unsigned short>(data,0x44);auto lengths=scene::field<unsigned short*>(data,0x48);auto ix=scene::field<unsigned short*>(data,0x4C);
  if(n>4096||!readable(lengths,n*2))return false;unsigned offset=0;
  for(unsigned j=0;j<n;j++){unsigned size=lengths[j];if(offset+size>200000||!readable(ix+offset,size*2))return false;for(unsigned i=2;i<size;i++)if(hit(ix[offset+i-2],ix[offset+i-1],ix[offset+i]))return true;offset+=size;}
 }return false;
}
inline void nearby(void* node,Vec point,Vec normal,std::vector<scene::Box>& result,unsigned& budget,int depth=0){
 using bottle_impact_adapter::readable;if(!budget||depth>48||!readable(node,0x9C))return;--budget;
 // TTR may temporarily hide native nodes between rendering passes.
 // Parent bounds may be stale after TTR publishes render transforms.
 auto vt=scene::field<void**>(node,0);
 if(reinterpret_cast<void*(__thiscall*)(void*)>(vt[6])(node)){
  std::vector<scene::Box> one;scene::boxes(node,one);if(!one.empty()&&scene::touchDistance(one,point)<32&&projectedSurface(node,point,normal))result.push_back(one.front());return;
 }
 if(!reinterpret_cast<void*(__thiscall*)(void*)>(vt[3])(node)||!readable(node,0xAC))return;
 auto count=scene::field<unsigned short>(node,0xA6);auto children=scene::field<void**>(node,0xA0);
 if(count>4096||!readable(children,count*4))return;
 for(unsigned i=0;i<count;i++)nearby(children[i],point,normal,result,budget,depth+1);
}
inline void* impactCell(Vec point){
 static const unsigned char signature[]={0x55,0x8B,0xEC,0x83,0xEC,0x10,0x89,0x4D,0xF0};
 if(memcmp(reinterpret_cast<void*>(0x4519D0),signature,sizeof signature))return nullptr;
 return reinterpret_cast<void*(__thiscall*)(void*,Vec)>(0x4519D0)(*reinterpret_cast<void**>(0x11DEA10),point);
}
inline void* cellRoot(void* cell){using bottle_impact_adapter::readable;auto data=readable(cell,0xC8)?scene::field<void*>(cell,0xC4):nullptr;return readable(data,4)?scene::field<void*>(data,0):nullptr;}
inline bool add(Vec position,Vec normal,void* node,void* cell,void* texture,void* bottle){
 auto m=manager();if(!m||!node||!cell||!texture)return false;
 Decal d;reinterpret_cast<void(__thiscall*)(Decal*)>(0x4A37B0)(&d);
 d.position=position;d.normal=normal;d.node=node;d.cell=cell;d.textureSet=texture;
 d.width=d.height=28;d.depth=16;d.alphaBlend=1;d.alphaTest=1;d.forceFade=1;d.byte71=0;d.byte72=1;d.epsilon=20;d.radius=16;
 d.quadrant=4;d.twoSided=0;d.color={.65f,.8f,.65f};d.specular=0;
 std::vector<scene::Box> targets;unsigned budget=12000;nearby(node,position,normal,targets,budget);
 if(targets.empty())nearby(cellRoot(cell),position,normal,targets,budget);
 // LAND collision proxies and their visible render chunks need not share a cell
 // subtree. Traverse the bounded live render tree only when local projection fails.
 if(targets.empty()){auto root=node;for(unsigned i=0;root&&scene::parent(root)&&i<48;i++)root=scene::parent(root);nearby(root,position,normal,targets,budget);}
 targets.erase(std::remove_if(targets.begin(),targets.end(),[&](const scene::Box& b){for(auto n=b.node;n;n=scene::parent(n))if(n==bottle)return true;return false;}),targets.end());
 std::sort(targets.begin(),targets.end(),[&](const scene::Box& a,const scene::Box& b){return scene::touchDistance({a},position)<scene::touchDistance({b},position);});
 if(diagnostics){for(auto& target:targets)diagnosticTargets.emplace_back(scene::name(target.node));}
 auto before=count();auto oldTail=scene::field<void*>(m,0xC);for(unsigned i=0;i<targets.size()&&i<4;i++){d.node=targets[i].node;reinterpret_cast<void(__thiscall*)(void*,Decal*,unsigned,bool)>(0x4A10D0)(m,&d,1,false);}
 auto entry=diagnostics?(oldTail?scene::field<void*>(oldTail,0):scene::field<void*>(m,8)):nullptr;for(unsigned n=0;entry&&n<16;n++,entry=scene::field<void*>(entry,0)){auto e=scene::field<void*>(entry,8);if(e){scene::retain(e);probes.push_back({e,0});}}
 return count()>before;
}
}
