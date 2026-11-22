#pragma once
#include "scene_adapter.hpp"
#include "reload_tutorial.hpp"
// Original annotation geometry lives beside the game scene, never under a gun
// or native part. It has no collision, inventory record or interaction handler.
namespace tutorial_visual {
inline scene::WorldPin pin;
inline void* edges[36]{};
inline std::unordered_map<std::string,void*> cards;
inline unsigned shown=0;
inline void hideNode(void* n,bool hide){if(!n)return;auto& flags=scene::field<unsigned>(n,0x30);if(hide)flags|=1;else flags&=~1u;}
inline void hide(){hideNode(pin.node,true);shown=0;}
inline bool bind(void* n){
 if(!pin.attach(n))return false;
 for(int i=0;i<36;i++){char name[32];sprintf_s(name,"FCT_Edge%d",i);edges[i]=scene::find(n,name);}
 for(auto key:reload_tutorial::cardNames){auto name=std::string("FCT_Card_")+key;cards[key]=scene::find(n,name.c_str());}
 hide();return true;
}
inline void clear(){pin.restore();for(auto& n:edges)n=nullptr;cards.clear();shown=0;}
inline scene::Vec unit(scene::Vec v){float length=scene::distance(v,{});return length>.00001f?v*(1/length):scene::Vec{};}
inline scene::Transform basis(scene::Vec x,scene::Vec y,scene::Vec z,scene::Vec p,float scale=1){
 return {{{x.x,y.x,z.x},{x.y,y.y,z.y},{x.z,y.z,z.z}},p,scale};
}
inline void line(unsigned i,scene::Vec a,scene::Vec b){
 auto node=edges[i];if(!node)return;float length=scene::distance(a,b);if(length<.01f)return;
 auto x=unit(b-a);auto up=std::abs(x.z)>.9f?scene::Vec{0,1,0}:scene::Vec{0,0,1};auto y=unit(scene::cross(up,x));auto z=scene::cross(x,y);
 if(scene::setWorld(node,basis(x,y,z,a,length))){hideNode(node,false);++shown;}
}
inline void box(unsigned group,scene::Vec centre,float radius,const scene::Transform& frame){
 if(group>=3)return;scene::Vec corners[8];for(int i=0;i<8;i++)corners[i]=centre+scene::rotate(frame,{(i&1)?radius:-radius,(i&2)?radius:-radius,(i&4)?radius:-radius});
 unsigned edge=group*12;for(unsigned i=0;i<8;i++)for(unsigned axis=1;axis<=4;axis<<=1)if(!(i&axis))line(edge++,corners[i],corners[i|axis]);
}
inline void begin(){shown=0;if(!pin.node)return;hideNode(pin.node,false);for(auto n:edges)hideNode(n,true);for(auto& n:cards)hideNode(n.second,true);}
inline void card(const char* key,scene::Vec point,scene::Vec eye){
 auto i=cards.find(key);if(i==cards.end()||!i->second)return;auto node=i->second;
 auto z=unit(eye-point);auto x=unit(scene::cross({0,0,1},z));auto y=scene::cross(z,x);
 if(scene::distance(x,{})<.9f)return;
 if(scene::setWorld(node,basis(x,y,z,point))){hideNode(node,false);++shown;}
}
}
