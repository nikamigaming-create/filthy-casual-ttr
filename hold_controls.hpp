#pragma once
#include <cstdint>
namespace hold_controls {
enum Action {None=-1,Weapons=0,Consumables=1,Reload=2,Settings=3,Tutorial=4};
struct State {
 struct Hold {bool armed=false,running=false,fired=false;uint64_t since=0;};
 struct Tap {bool armed=false,down=false,first=false,second=false,fired=false;uint64_t since=0,released=0;};
 Hold menu;Tap taps[2];uint64_t delayMs=1000,doubleTapMs=450;bool menuOwned=false;
 unsigned menuClicks=0;uint64_t menuReleased=0;static constexpr uint64_t menuGapMs=450;
 int update(unsigned mask,uint64_t now,bool active,bool allowed){
  mask&=31;menuOwned=menuOwned&&active&&(mask&4);
  if(!active||!allowed){menu={};menuClicks=0;for(auto& t:taps)t={};return None;}
  int result=None;
  // Menu has priority. The opposite thumbrest must stay touched throughout
  // both taps; simultaneous contact alone never arms either wheel.
  bool restBlocked=(mask&4)!=0;
  for(int i=0;i<2;i++){
   auto& t=taps[i];bool down=(mask&(1u<<i))!=0;
   bool oppositeRest=(mask&(1u<<(1-i)))!=0;
   if(restBlocked||!oppositeRest){t={};continue;}
   if(t.first&&now-t.released>doubleTapMs)t.first=false;
   if(!down){
    if(t.down){
     auto duration=now-t.since;bool tap=duration>=40&&duration<=300;
     if(t.second&&tap&&!t.fired)result=i==0?Consumables:Weapons;
     t.first=tap&&!t.second&&!t.fired;t.released=now;
    }
    t.down=false;t.second=false;t.fired=false;t.armed=true;continue;
   }
   if(t.armed){
    t.second=t.first&&now-t.released>=40&&now-t.released<=doubleTapMs;
    t.first=false;t.armed=false;t.down=true;t.fired=false;t.since=now;
   }
   // Fire on the debounced second contact, without another hold or release.
   if(t.down&&t.second&&!t.fired&&now-t.since>=40){t.fired=true;result=i==0?Consumables:Weapons;}
  }
  if(result!=None)menuClicks=0; // A wheel wins; never queue a delayed mode change.
  bool down=(mask&4)!=0;auto& h=menu;
  // Defer a single tap until a possible triple has expired. A double tap is
  // deliberately a no-op: neither an accidental mode change nor two changes.
  if(menuClicks&&!h.running&&now-menuReleased>menuGapMs){
   if(menuClicks==1)result=Reload;
   menuClicks=0;
  }
  if(!down){
   if(h.running&&!h.fired){
    const auto duration=now-h.since;
    if(duration>=delayMs){result=Settings;menuClicks=0;}
    else if(duration>=40){
     ++menuClicks;menuReleased=now;
     if(menuClicks==3){result=Tutorial;menuClicks=0;}
    }else menuClicks=0; // Bounce cancels, never becomes a reload later.
   }
   h={};h.armed=true;
  }else{
   if(h.armed){
    h.armed=false;h.running=true;h.since=now;menuOwned=true;
    if(menuClicks&&now-menuReleased<40)menuClicks=0;
   }
   if(h.running&&!h.fired&&now-h.since>=delayMs){h.fired=true;menuClicks=0;result=Settings;}
  }
  return result;
 }
};
}
