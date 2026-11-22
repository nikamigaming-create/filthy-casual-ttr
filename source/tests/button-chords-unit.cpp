#include "../button_chords.hpp"
#include <cassert>
#include <cstdio>
using button_chords::State;
int main(){
 for(unsigned mask:{5u,9u,10u,6u}){
  State s;unsigned first=mask&(~mask+1u);
  assert(s.update(first,0,true,true)==-1);assert(s.output==0);
  assert(s.update(mask,60,true,true)==-1);
  assert(s.update(mask,559,true,true)==-1);assert(s.output==0);
  assert(s.update(mask,560,true,true)==State::action(mask));assert(s.output==0);
  assert(s.update(mask,2000,true,true)==-1);assert(s.output==0);
  assert(s.update(first,2100,true,true)==-1);assert(s.output==0);
  assert(s.update(0,2200,true,true)==-1);assert(s.output==0);
  assert(s.update(mask,2300,true,true)==-1);assert(s.update(mask,2800,true,true)==State::action(mask));
 }
 for(unsigned bit:{1u,2u,4u,8u})for(unsigned duration:{40u,100u,1000u}){
  State s;unsigned rose=0,fell=0;bool old=false;
  for(unsigned t=0;t<duration+400;t++){
   assert(s.update(t<duration?bit:0,t,true,true)==-1);
   bool v=(s.output&bit)!=0;if(v&&!old)rose=t;if(!v&&old)fell=t;old=v;
  }
  assert(rose==State::WindowMs);assert(fell-rose==duration);
 }
 {State s;s.update(5,0,true,true);assert(s.update(5,499,true,true)==-1);s.update(0,500,true,true);assert(!s.output);assert(s.update(0,1500,true,true)==-1);}
 {State s;s.update(1,0,true,true);s.update(1,160,true,true);assert(s.output==1);s.update(5,200,true,true);assert(s.update(5,900,true,true)==-1);assert(s.output==5);}
 {State s;s.update(5,0,true,false);assert(s.update(5,600,true,false)==-1);assert(s.output==5);}
 {State s;s.update(5,0,true,true);s.update(0,300,false,true);assert(s.update(0,900,true,true)==-1);assert(!s.output);}
 {State s;s.update(5,0,true,true);s.update(7,100,true,true);assert(s.update(5,900,true,true)==-1);assert(!s.output);}
 {State s;s.update(5,0,true,true);assert(s.update(5,500,true,false)==-1);assert(s.update(5,900,true,true)==-1);assert(!s.output);}
 puts("PASS: four held chords, cancel/release/rearm, native tap/hold duration, late pairs, menus, tracking loss, ambiguity");
}
