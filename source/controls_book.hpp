#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
// Pure input/animation state. Trigger hysteresis and release-to-rearm avoid
// paging continuously while holding a trigger or after leaving another menu.
namespace controls_book {
constexpr int spreads=6;
struct State {
 int page=0,from=0,direction=0,hand=0;bool active=false,armed[2]{},stickArmed=false;
 uint64_t opened=0,turnStarted=0;
 void open(uint64_t now){active=true;opened=now;direction=0;armed[0]=armed[1]=false;stickArmed=false;}
 void close(){active=false;direction=0;armed[0]=armed[1]=false;stickArmed=false;}
 bool turn(int d,uint64_t now){
  if(!active||direction||now-opened<500||d==0)return false;
  int next=std::clamp(page+(d>0?1:-1),0,spreads-1);if(next==page)return false;
  from=page;page=next;direction=d>0?1:-1;turnStarted=now;return true;
 }
 float progress(uint64_t now)const{return std::clamp(float(now-turnStarted)/420.f,0.f,1.f);}
 void tick(uint64_t now,float left,float right,float stick){
  if(!active)return;if(direction&&now-turnStarted>=420)direction=0;
  float values[]{left,right};bool edge[2]{};
  for(int h=0;h<2;h++){if(values[h]<.2f)armed[h]=true;else if(values[h]>.65f&&armed[h]){edge[h]=true;armed[h]=false;}}
  if(std::abs(stick)<.25f)stickArmed=true;
  int d=0;if(edge[0]!=edge[1])d=edge[1]?1:-1;
  if(std::abs(stick)>.7f&&stickArmed){if(!d)d=stick>0?1:-1;stickArmed=false;}
  if(d)turn(d,now);
 }
};
}
