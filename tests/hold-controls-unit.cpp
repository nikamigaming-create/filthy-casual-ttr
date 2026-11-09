#include "../hold_controls.hpp"
#include <cassert>
#include <cstdio>
#include <initializer_list>
using namespace hold_controls;
int main(){
 for(unsigned rest:{1u,2u}){
  unsigned modifier=rest==1?2u:1u,combo=rest|modifier;int want=rest==1?Consumables:Weapons;
  State s;s.update(modifier,0,true,true);
  assert(s.update(combo,100,true,true)==None);
  assert(s.update(modifier,180,true,true)==None);
  assert(s.update(combo,280,true,true)==None);
  assert(s.update(combo,319,true,true)==None);
  assert(s.update(combo,320,true,true)==want);
  assert(s.update(combo,2000,true,true)==None);
  assert(s.update(modifier,2100,true,true)==None);
  s.update(combo,2200,true,true);s.update(modifier,2280,true,true);s.update(combo,2380,true,true);
  assert(s.update(modifier,2460,true,true)==want);
  assert(s.update(modifier,2500,true,true)==None);
  State held;held.update(modifier,0,true,true);held.update(combo,10,true,true);
  assert(held.update(combo,5000,true,true)==None);held.update(modifier,5010,true,true);
  held.update(combo,5110,true,true);assert(held.update(combo,5160,true,true)==None);
  State bare;bare.update(0,0,true,true);bare.update(rest,10,true,true);bare.update(0,100,true,true);
  bare.update(rest,200,true,true);assert(bare.update(rest,250,true,true)==None);
  unsigned oldModifier=rest==1?16u:8u;
  State stick;stick.update(oldModifier,0,true,true);stick.update(oldModifier|rest,100,true,true);
  stick.update(oldModifier,180,true,true);stick.update(oldModifier|rest,280,true,true);
  assert(stick.update(oldModifier|rest,330,true,true)==None);
  for(int mode=0;mode<4;mode++){
   State t;t.update(modifier,0,true,true);t.update(combo,100,true,true);t.update(modifier,180,true,true);
   if(mode==0){t.update(combo,700,true,true);assert(t.update(combo,750,true,true)==None);}
   if(mode==1){t.update(combo,190,true,true);assert(t.update(combo,240,true,true)==None);}
   if(mode==2){t.update(0,200,true,true);t.update(modifier,240,true,true);t.update(combo,280,true,true);assert(t.update(combo,330,true,true)==None);}
   if(mode==3){t.update(31,200,true,true);t.update(combo,280,true,true);assert(t.update(combo,330,true,true)==None);}
  }
  for(bool inactive:{false,true}){
   State t;t.update(modifier,0,true,true);t.update(combo,100,true,true);t.update(modifier,180,true,true);
   t.update(combo,280,!inactive,inactive);assert(t.update(combo,330,true,true)==None);
   assert(t.update(modifier,400,true,true)==None);
  }
  State bounce;bounce.update(modifier,0,true,true);bounce.update(combo,100,true,true);bounce.update(modifier,120,true,true);
  bounce.update(combo,220,true,true);assert(bounce.update(combo,300,true,true)==None);
  State resumed;resumed.update(combo,0,true,true);resumed.update(modifier,100,true,true);resumed.update(combo,200,true,true);assert(resumed.update(combo,250,true,true)==None);
 }
 {State s;s.update(0,0,true,true);s.update(4,10,true,true);assert(s.menuOwned);assert(s.update(0,110,true,true)==Reload);assert(!s.menuOwned);}
 {State s;s.update(0,0,true,true);s.update(4,10,true,true);assert(s.update(4,1009,true,true)==None);assert(s.update(4,1010,true,true)==Settings);assert(s.update(0,2000,true,true)==None);}
 {State s;s.update(0,0,true,true);s.update(4,10,true,true);s.update(4,1000,true,true);assert(s.update(0,1020,true,true)==Settings);}
 {State s;s.update(4,0,true,true);assert(s.update(4,2000,true,true)==None);assert(s.update(0,2100,true,true)==None);}
 {State s;s.delayMs=1500;s.update(0,0,true,true);s.update(4,10,true,true);assert(s.update(4,1010,true,true)==None);assert(s.update(4,1510,true,true)==Settings);}
 {State s;s.update(2,0,true,true);s.update(3,100,true,true);s.update(2,180,true,true);s.update(6,220,true,true);s.update(7,280,true,true);assert(s.update(7,330,true,true)==None);assert(s.update(7,1220,true,true)==Settings);}
 {State s;s.update(0,0,true,true);assert(s.update(3,100,true,true)==None);assert(s.update(3,5000,true,true)==None);assert(s.update(0,5100,true,true)==None);}
 puts("PASS: opposite-thumbrest double taps, old stick modifier rejected, debounce, timeout, cancellation, single-fire and Menu exclusion");
}
