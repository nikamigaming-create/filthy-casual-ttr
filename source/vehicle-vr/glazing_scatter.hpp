#pragma once
#include "../glass_decal_adapter.hpp"
#include "../glass_variation.hpp"
// Temporary impact scatter. Native particles provide the airborne burst;
// ballistic samples place fading glass decals only on native collision hits.
namespace glazing_scatter {
struct Piece {scene::Vec p,v;float age;unsigned texture,seed;};
inline std::vector<Piece> pieces;inline unsigned landed=0;inline ULONGLONG last=0;
inline float random(unsigned& s){s=s*1664525u+1013904223u;return float(s>>8)/16777216.f;}
inline void burst(scene::Vec point,scene::Vec normal,unsigned texture,unsigned seed){
 // Low fixed cap across all windows. No permanent references or save records.
 for(unsigned i=0;i<12&&pieces.size()<64;i++){
  scene::Vec velocity{(random(seed)-.5f)*170,(random(seed)-.5f)*170,40+random(seed)*120};
  velocity=velocity+normal*(25+random(seed)*100);
  pieces.push_back({point+normal*2,velocity,0,texture,seed});
 }
 auto cell=glass_decal_adapter::impactCell(point);
 // Reuse the game's actual glass impact particles, not generic smoke.
 glass_decal_adapter::puff(point+scene::Vec{3,0,2},normal,cell);
 glass_decal_adapter::puff(point+scene::Vec{-3,0,0},normal,cell);
}
inline void tick(ULONGLONG now){
 float dt=last?float(now-last)*.001f:0;last=now;if(dt<=0)return;
 if(dt>.15f){pieces.clear();return;}
 for(auto it=pieces.begin();it!=pieces.end();){
  it->age+=dt;if(it->age>3.5f){it=pieces.erase(it);continue;}
  auto next=it->p+it->v*dt+scene::Vec{0,0,-343*dt*dt};it->v.z-=686*dt;
  scene::Vec point{},normal{};void* node=nullptr;
  if(glass_decal_adapter::traceSegment(it->p,next,point,normal,node)){
   auto tx=vehicle_adapter::lookup(it->texture);auto cell=glass_decal_adapter::impactCell(point);
   auto style=glass_variation::choose(it->seed,4+random(it->seed)*10);
   if(tx&&glass_decal_adapter::add(point,normal,node,cell,tx,nullptr,style.width*.65f,style.height*.65f))++landed;
   it=pieces.erase(it);
  }else{it->p=next;++it;}
 }
}
inline void clear(){pieces.clear();last=0;landed=0;}
}
