#pragma once
#include "mounted_frame.hpp"
// Exact E14567BC native contact history. Match TTR's own snap-turn rebase,
// but use the mounted carrier's full rigid delta, once before contact solving.
namespace vehicle_contact {
using mounted_frame::Mat;using mounted_frame::Vec;
inline bool finite(Vec p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
inline bool finite(Mat m){for(auto& row:m.a)for(float x:row)if(!std::isfinite(x))return false;return true;}
inline Mat rotation(const scene::Transform& t){Mat m;memcpy(m.a,t.r,sizeof m.a);return m;}
struct Delta {Mat r;Vec oldOrigin,newOrigin;Vec point(Vec p)const{return newOrigin+mounted_frame::apply(r,p-oldOrigin);}Vec vector(Vec v)const{return mounted_frame::apply(r,v);}Mat orientation(Mat m)const{return mounted_frame::mul(r,m);}};
inline bool makeDelta(const scene::Transform& old,const scene::Transform& now,Delta& d){
 if(!finite(old.p)||!finite(now.p)||!std::isfinite(old.scale)||!std::isfinite(now.scale)||old.scale<.001f||std::abs(old.scale-now.scale)>.0001f)return false;
 auto a=rotation(old),b=rotation(now);if(!finite(a)||!finite(b))return false;
 auto check=mounted_frame::mul(mounted_frame::transpose(a),a);auto checkB=mounted_frame::mul(mounted_frame::transpose(b),b);
 for(int i=0;i<3;i++)for(int j=0;j<3;j++)if(std::abs(check.a[i][j]-(i==j?1.f:0.f))>.002f||std::abs(checkB.a[i][j]-(i==j?1.f:0.f))>.002f)return false;
 d={mounted_frame::mul(b,mounted_frame::transpose(a)),old.p,now.p};return true;
}
inline bool validState(unsigned char* s){
 if(!s[0])return true;auto n=scene::field<unsigned>(s,0x804),next=scene::field<unsigned>(s,0x808);if(n>40||next>=40)return false;
 for(unsigned o:{0x04u,0x10u,0x20u,0x30u})if(!finite(scene::field<Vec>(s,o)))return false;
 for(unsigned o:{0x3Cu,0x60u})if(!finite(scene::field<Mat>(s,o)))return false;
 for(unsigned i=0;i<n;i++){auto slot=(next+39-i)%40;auto p=s+0x84+slot*0x30;if(!finite(scene::field<Vec>(p,0))||!finite(scene::field<Mat>(p,12)))return false;}return true;
}
inline void carryState(unsigned char* s,const Delta& d){
 if(!s[0])return;
 for(unsigned o:{0x04u,0x30u})scene::field<Vec>(s,o)=d.point(scene::field<Vec>(s,o));
 if(s[0x1C])scene::field<Vec>(s,0x10)=d.point(scene::field<Vec>(s,0x10));
 scene::field<Vec>(s,0x20)=d.vector(scene::field<Vec>(s,0x20));
 for(unsigned o:{0x3Cu,0x60u})scene::field<Mat>(s,o)=d.orientation(scene::field<Mat>(s,o));
 auto n=scene::field<unsigned>(s,0x804),next=scene::field<unsigned>(s,0x808);
 for(unsigned i=0;i<n;i++){auto slot=(next+39-i)%40;auto p=s+0x84+slot*0x30;scene::field<Vec>(p,0)=d.point(scene::field<Vec>(p,0));scene::field<Mat>(p,12)=d.orientation(scene::field<Mat>(p,12));}
}
inline bool verified(){
 auto b=build_adapter::heldModule();if(!b)return false;
 const unsigned char clear[]={0x55,0x8B,0xEC,0x83,0xE4,0xF0,0x81,0xEC,0x98,0x08,0,0};
 const unsigned char previous[]={0x80,0x7F,0x1C,0x00},yaw[]={0xF3,0x0F,0x10,0x9E,0x3C,0x0B,0,0};
 // Native grip query reads translation 19FC+h*12 and rotation 1A14+h*36.
 // 19F8 is an INPUT field, not part of either hand pose. Never transport it.
 const unsigned char translation[]={0xF3,0x0F,0x10,0x94,0x8E,0xFC,0x19,0,0};
 const unsigned char rotation[]={0x8D,0x14,0xC5,0x85,0x06,0,0,0x03,0xD0};
 return !memcmp(b+0x6C1BDF,translation,sizeof translation)&&!memcmp(b+0x6C1A4F,rotation,sizeof rotation)&&!memcmp(b+0x15AEE0,clear,sizeof clear)&&!memcmp(b+0x15D83D,previous,sizeof previous)&&!memcmp(b+0x167585,yaw,sizeof yaw)&&b[0x1675AD]==0xF3&&b[0x1675AE]==0x0F&&b[0x1675AF]==0x5C&&b[0x1675B0]==0x0D&&scene::field<unsigned char*>(b,0x1675B1)==b+0x12A19B4;
}
inline void carryManager(void* manager,const Delta& d){
 for(int h=0;h<2;h++){
  auto& p=scene::field<Vec>(manager,0x19FC+h*12);p=d.vector(p);
  auto& r=scene::field<Mat>(manager,0x1A14+h*36);r=mounted_frame::mul(d.orientation(r),mounted_frame::transpose(d.r));
 }
}
inline bool carry(void* manager,const Delta& d){
 auto b=build_adapter::heldModule();if(!b||!manager)return false;
 for(int h=0;h<2;h++)if(!validState(b+0x129EE18+h*0x81C)||!finite(scene::field<Vec>(manager,0x19FC+h*12))||!finite(scene::field<Mat>(manager,0x1A14+h*36)))return false;
 // The same native snap-turn block carries optional held-item world history.
 if(b[0x129EE14])for(unsigned i=0;i<320;i++)if(!finite(scene::field<Vec>(b,0x12A19B8+i*12)))return false;
 if(b[0x12A38F4]&&(!finite(scene::field<Vec>(b,0x12A38F8))||!finite(scene::field<Mat>(b,0x12A3904))))return false;
 for(int h=0;h<2;h++){
  carryState(b+0x129EE18+h*0x81C,d);
 }
 carryManager(manager,d);
 if(b[0x129EE14])for(unsigned i=0;i<320;i++){auto& p=scene::field<Vec>(b,0x12A19B8+i*12);p=d.point(p);}
 if(b[0x12A38F4]){auto& p=scene::field<Vec>(b,0x12A38F8);p=d.point(p);auto& r=scene::field<Mat>(b,0x12A3904);r=d.orientation(r);}
 return true;
}
inline void ownYaw(void* manager){auto b=build_adapter::heldModule();if(b&&manager)scene::field<float>(b,0x12A19B4)=scene::field<float>(manager,0xB3C);}
struct History {void* actorRoot=nullptr;void* carrier=nullptr;scene::Transform pose{};unsigned sequence=0;};
inline History history;inline unsigned carries=0,misses=0,reseeds=0;
inline void reseed(){auto b=build_adapter::heldModule();if(b){reinterpret_cast<void(__cdecl*)()>(b+0x15AEE0)();++reseeds;}}
inline void reset(){history={};}
inline void before(void* manager,void* actorRoot,void* carrier,unsigned sequence){
 if(!manager||!actorRoot||!carrier){reset();return;}
 if(history.sequence==sequence&&history.actorRoot==actorRoot&&history.carrier==carrier)return;
 auto now=scene::world(carrier);
 if(history.actorRoot==actorRoot&&history.carrier==carrier){Delta d;if(makeDelta(history.pose,now,d)&&carry(manager,d))++carries;else {++misses;reseed();}}
 else reseed(); // No old-frame native ring may survive a new mount/root identity.
 // A rejected transport invalidates the native contacts before a new baseline
 // is recorded. The original solver now seeds its history in this frame.
 history={actorRoot,carrier,now,sequence};ownYaw(manager);
}
}
