#pragma once
#include <cstdint>
#include <deque>
// X/Y/A/B = bits 0/1/2/3. Ordinary edges retain their order and held duration,
// shifted by a small recognition window. A recognized chord consumes its whole
// press/release cycle, including a cancelled short hold. No native button leaks.
namespace button_chords {
enum Action {None=-1,Weapons=0,Consumables=1,Height=2,Reload=3};
struct State {
 static constexpr uint64_t WindowMs=160,HoldMs=500;
 struct Edge {uint64_t due;unsigned bit;bool down;};
 std::deque<Edge> edges;
 unsigned raw=0,output=0,candidate=0;
 uint64_t downAt[4]{},since=0;
 bool locked=false,fired=false;
 int update(unsigned next,uint64_t now,bool active,bool allow){
  next&=15;
  if(!active){*this=State{};return None;}
  if(locked){
   raw=next;
   if(!allow)candidate=0;
   if(!next){locked=false;candidate=0;fired=false;}
   else if(!fired&&candidate&&next==candidate&&now-since>=HoldMs){fired=true;return action(candidate);}
   else if(next!=candidate){candidate=0;}
   return None;
  }
  unsigned changed=raw^next;
  for(unsigned i=0;i<4;i++)if(changed&(1u<<i)){
   bool down=(next&(1u<<i))!=0;if(down)downAt[i]=now;
   edges.push_back({now+WindowMs,1u<<i,down});
  }
  raw=next;
  if(allow&&output==0&&action(next)!=None){
   uint64_t earliest=now;for(unsigned i=0;i<4;i++)if(next&(1u<<i))if(downAt[i]<earliest)earliest=downAt[i];
   if(now-earliest<WindowMs){edges.clear();locked=true;candidate=next;since=now;fired=false;return None;}
  }
  while(!edges.empty()&&edges.front().due<=now){auto e=edges.front();edges.pop_front();if(e.down)output|=e.bit;else output&=~e.bit;}
  return None;
 }
 static int action(unsigned bits){switch(bits){case 5:return Weapons;case 9:return Consumables;case 10:return Height;case 6:return Reload;default:return None;}}
};
}
