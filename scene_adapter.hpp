#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
// FalloutNV 1.4.0.525. Layout/vtables verified in JIP LN GameObjects.h,
// netimmerse.h and Ni_types.h. Accessed only on the native game thread.
namespace scene {
struct Vec {float x=0,y=0,z=0;};
inline Vec operator+(Vec a,Vec b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec operator-(Vec a,Vec b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec operator*(Vec a,float s){return {a.x*s,a.y*s,a.z*s};}
inline Vec cross(Vec a,Vec b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline float distance(Vec a,Vec b){auto d=a-b;return std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z);}
struct Transform {float r[3][3];Vec p;float scale;};
static_assert(sizeof(Transform)==0x34);
template<class T> inline T& field(void* p,size_t offset){return *reinterpret_cast<T*>(static_cast<char*>(p)+offset);}
inline Vec rotate(const Transform& t,Vec v){return {t.r[0][0]*v.x+t.r[0][1]*v.y+t.r[0][2]*v.z,t.r[1][0]*v.x+t.r[1][1]*v.y+t.r[1][2]*v.z,t.r[2][0]*v.x+t.r[2][1]*v.y+t.r[2][2]*v.z};}
inline Vec inverseRotate(const Transform& t,Vec v){return {t.r[0][0]*v.x+t.r[1][0]*v.y+t.r[2][0]*v.z,t.r[0][1]*v.x+t.r[1][1]*v.y+t.r[2][1]*v.z,t.r[0][2]*v.x+t.r[1][2]*v.y+t.r[2][2]*v.z};}
inline void* root(void* player){if(!player)return nullptr;auto state=field<void*>(player,0x64);return state?field<void*>(state,0x14):nullptr;}
inline void* cell(void* player){return player?field<void*>(player,0x40):nullptr;}
inline Transform& local(void* node){return field<Transform>(node,0x34);}
inline const char* name(void* node){auto n=node?field<const char*>(node,8):nullptr;return n?n:"";}
inline void* parent(void* node){return node?field<void*>(node,0x18):nullptr;}
inline Transform world(void* node){return field<Transform>(node,0x68);}
inline void* find(void* node,const char* wanted,int depth=0){
 if(!node||depth>48)return nullptr;if(!strcmp(name(node),wanted))return node;
 auto vt=field<void**>(node,0);if(!reinterpret_cast<void*(__thiscall*)(void*)>(vt[3])(node))return nullptr;
 auto count=field<unsigned short>(node,0xA6);auto children=field<void**>(node,0xA0);if(count>4096||!children)return nullptr;
 for(unsigned i=0;i<count;i++)if(auto found=find(children[i],wanted,depth+1))return found;return nullptr;
}
inline void retain(void* node){if(node)InterlockedIncrement(&field<LONG>(node,4));}
inline void release(void* node){if(node&&!InterlockedDecrement(&field<LONG>(node,4)))reinterpret_cast<void(__thiscall*)(void*)>(field<void**>(node,0)[1])(node);}
inline void update(void* node){static unsigned char data[12]{};reinterpret_cast<void(__thiscall*)(void*,const void*,unsigned)>(field<void**>(node,0)[0xA4/4])(node,data,0);}
inline void moveParent(void* node,void* target){
 retain(node);auto old=parent(node);if(old)reinterpret_cast<void(__thiscall*)(void*,void*)>(field<void**>(old,0)[0xE8/4])(old,node);
 reinterpret_cast<void(__thiscall*)(void*,void*,bool)>(field<void**>(target,0)[0xDC/4])(target,node,true);release(node);
}
// NiGeometry::geometryData B8; NiGeometryData vertices 20 / count 08.
// Cache the authored rigid mesh bounds once. Hit-test in each mesh's actual
// rendered frame, not at the NIF origin (which can be far from the object).
struct Box {void* node;Vec lo,hi;};
inline void boxes(void* node,std::vector<Box>& out,int depth=0){
 if(!node||depth>48)return;auto vt=field<void**>(node,0);
 if(reinterpret_cast<void*(__thiscall*)(void*)>(vt[6])(node)){
  auto data=field<void*>(node,0xB8);if(data){auto count=field<unsigned short>(data,8);auto vertices=field<Vec*>(data,0x20);
   if(count&&vertices){Box b{node,vertices[0],vertices[0]};for(unsigned i=1;i<count;i++){auto v=vertices[i];b.lo={std::min(b.lo.x,v.x),std::min(b.lo.y,v.y),std::min(b.lo.z,v.z)};b.hi={std::max(b.hi.x,v.x),std::max(b.hi.y,v.y),std::max(b.hi.z,v.z)};}out.push_back(b);}
  }
 }
 if(!reinterpret_cast<void*(__thiscall*)(void*)>(vt[3])(node))return;
 auto n=field<unsigned short>(node,0xA6);auto children=field<void**>(node,0xA0);if(!children||n>4096)return;
 for(unsigned i=0;i<n;i++)boxes(children[i],out,depth+1);
}
inline Vec boxCenter(const std::vector<Box>& mesh,Vec* extent=nullptr){
 Vec lo{1e20f,1e20f,1e20f},hi{-1e20f,-1e20f,-1e20f};
 for(auto& b:mesh){auto t=world(b.node);for(int i=0;i<8;i++){Vec v{(i&1)?b.hi.x:b.lo.x,(i&2)?b.hi.y:b.lo.y,(i&4)?b.hi.z:b.lo.z};auto p=t.p+rotate(t,v)*t.scale;lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};}}
 if(extent)*extent=hi-lo;return (lo+hi)*.5f;
}
inline float touchDistance(const std::vector<Box>& mesh,Vec point){
 float nearest=1e20f;for(auto& b:mesh){auto t=world(b.node);if(std::abs(t.scale)<.001f)continue;auto p=inverseRotate(t,point-t.p)*(1/t.scale);Vec on{std::clamp(p.x,b.lo.x,b.hi.x),std::clamp(p.y,b.lo.y,b.hi.y),std::clamp(p.z,b.lo.z,b.hi.z)};nearest=std::min(nearest,distance(p,on)*std::abs(t.scale));}return nearest;
}
inline Vec palm(void* rig,int hand){
 auto wrist=find(rig,hand?"Bip01 R Hand":"Bip01 L Hand");if(!wrist)return {};
 auto origin=world(wrist).p;auto index=find(wrist,hand?"Bip01 R Finger1":"Bip01 L Finger1");auto pinky=find(wrist,hand?"Bip01 R Finger4":"Bip01 L Finger4");
 if(!index||!pinky)return origin;
 auto a=world(index).p-origin,b=world(pinky).p-origin;auto normal=cross(a,b);auto length=distance(normal,{});if(length>.01f)normal=normal*((hand?1.f:-1.f)/length);
 return origin+(a+b)*.35f+normal*2.0f;
}
inline bool setWorld(void* node,const Transform& wanted){
 auto p=parent(node);if(!p)return false;auto base=world(p);if(!std::isfinite(base.scale)||std::abs(base.scale)<.01f)return false;
 Transform local{};local.p=inverseRotate(base,wanted.p-base.p)*(1/base.scale);local.scale=wanted.scale/base.scale;
 for(int r=0;r<3;r++)for(int c=0;c<3;c++)for(int k=0;k<3;k++)local.r[r][c]+=base.r[k][r]*wanted.r[k][c];
 field<Transform>(node,0x34)=local;update(node);return true;
}
struct WorldPin {
 void* node=nullptr;void* home=nullptr;void* anchor=nullptr;
 bool attach(void* source){
  if(!source||!parent(source))return false;auto dest=parent(source);while(parent(dest))dest=parent(dest);
  auto pose=world(source);node=source;home=parent(source);anchor=dest;retain(node);retain(home);retain(anchor);
  moveParent(node,anchor);return setWorld(node,pose);
 }
 void restore(){if(!node)return;moveParent(node,home);release(anchor);release(home);release(node);node=home=anchor=nullptr;}
};
}
