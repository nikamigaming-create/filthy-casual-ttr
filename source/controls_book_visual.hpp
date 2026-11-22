#pragma once
#include "controls_book.hpp"
#include "scene_adapter.hpp"
namespace controls_book {
inline void show(void* p,bool yes){if(p){auto& flags=scene::field<unsigned>(p,0x30);if(yes)flags&=~1u;else flags|=1u;}}
inline void rotateY(void* p,float a){if(!p)return;auto& t=scene::local(p);float c=std::cos(a),s=std::sin(a);float r[3][3]={{c,0,s},{0,1,0},{-s,0,c}};memcpy(t.r,r,sizeof r);scene::update(p);}
inline bool publish(void* root,void* rig,const State& state,uint64_t now){
 auto book=scene::find(root,"FC_ControlsBook");if(!book)return false;
 auto hand=scene::find(rig,state.hand?"Bip01 R Hand":"Bip01 L Hand");if(!hand)return false;
 // Same completed native pose as the visible hand. The palm's authored +Y
 // faces the pages, +X follows the fingers, and +Z runs across the book.
 auto h=scene::world(hand);scene::Transform held{};held.scale=h.scale;
 // Both native palms face +Y. Only their radial/pinky layout is mirrored;
 // flipping the right palm normal put the cover behind/through the fingers.
 for(int row=0;row<3;row++){held.r[row][0]=h.r[row][2];held.r[row][1]=h.r[row][0];held.r[row][2]=h.r[row][1];}
 // Right thumb sits on the radial edge, outside the printed page. Matching
 // the left offset numerically buried it through the right-hand page surface.
 float across=state.hand?-9.f:6.f;
 held.p=scene::palm(rig,state.hand)+scene::rotate(held,{across,-6,1.6f})*h.scale;
 if(!scene::setWorld(book,held))return false;
 auto left=scene::find(book,"FCBook_LeftCover");
 float opening=std::clamp(float(now-state.opened)/500.f,0.f,1.f);opening=opening*opening*(3-2*opening);
 rotateY(left,3.14159265f*(1-opening)+.24f*opening);
 int l=state.page*2,r=l+1;
 if(state.direction>0)l=state.from*2;
 if(state.direction<0)r=state.from*2+1;
 for(int i=0;i<12;i++){char n[32];sprintf_s(n,"FCBook_Page%d",i);show(scene::find(book,n),i==l||i==r);}
 for(int i=0;i<5;i++){
  char n[32];sprintf_s(n,"FCBook_Turn%d",i);auto leaf=scene::find(book,n);
  bool visible=state.direction&&i==std::min(state.page,state.from);show(leaf,visible);
  if(visible){float p=state.progress(now);p=p*p*(3-2*p);float a=(-3.14159265f+.24f)*(state.direction>0?p:1-p);rotateY(leaf,a);}
 }
 scene::update(book);return true;
}
}
