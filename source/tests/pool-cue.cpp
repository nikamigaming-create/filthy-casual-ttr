#include "../pool-vr/cue.hpp"
#include <cassert>
#include <cstdio>
int main(){
 using namespace poolcue;
 assert(stanceDistance(500,250,1,0)>150);
 assert(stanceDistance(500,250,0,1)<110);
 assert(stanceDistance(100,250,1,0)<75);
 assert(stanceDistance(100,250,-1,0)>230);const V axis{1,0,0},ball{};Stroke s;
 auto at=[&](float x,float dt=.016f,bool usable=true){return s.update({x,0,0},axis,ball,dt,usable);};
 assert(at(-.12f)==0);assert(at(-.10f)==0);assert(at(-.07f)==0);
 assert(at(-.03f)>0);assert(at(.01f)==0);assert(at(.02f)==0);
 s.reset();assert(at(-.02f)==0);assert(at(-.06f)==0);assert(at(-.12f)==0);
 // Tracking loss and reacquisition cannot turn a changed pose into a stroke.
 assert(at(-.12f,.016f,false)==0);assert(at(.01f)==0);
 s.reset();at(-.12f);at(-.1f);assert(at(.3f)==0);assert(at(.32f)==0);
 s.reset();at(-.12f);at(-.1f);assert(at(-.02f,.5f)==0);
 s.reset();at(-.12f);at(-.1f);assert(s.update({-.02f,.2f,0},axis,ball,.016f,true)==0);
 s.reset();at(-.12f);at(-.10f);
 for(float x=-.099f;x<.01f;x+=.001f)assert(at(x,.02f)==0);
 puts("PASS cue: one forward hit, backswing, tracking loss, jump, stale frame, miss, slow contact");
}
