#include "../bottle_impact.hpp"
#include <cassert>
#include <cstdio>
using namespace bottle_impact;
int main(){
 Sample a{true,false,false,1,1,{0,0,-4},{}},b{true,false,true,1,1.016,{0,0,.2f},{}};
 assert(judge(a,b).smash);
 b.contact=false;assert(!judge(a,b).smash); // fast flight/gesture is not an impact
 b.contact=true;a.velocity={0,0,-.5f};b.velocity={};assert(!judge(a,b).smash);
 a.velocity={8,0,-.2f};b.velocity={7.9f,0,.1f};assert(!judge(a,b).smash); // glancing slide
 a.velocity={20,0,0};b.velocity={18,0,0};b.surfaceVelocity={20,0,0};assert(!judge(a,b).smash); // riding a moving surface
 a.velocity={0,0,-4};b.velocity={};b.surfaceVelocity={};a.held=true;assert(!judge(a,b).smash);
 a.held=false;b.held=true;assert(!judge(a,b).smash);b.held=false;b.body=2;assert(!judge(a,b).smash);
 b.body=1;b.seconds=2;assert(!judge(a,b).smash);b.seconds=1;assert(!judge(a,b).smash);
 puts("PASS: hard contact breaks; flight, soft drop, glancing slide, co-moving surface, held/caught object, body replacement and stale samples do not");
}
