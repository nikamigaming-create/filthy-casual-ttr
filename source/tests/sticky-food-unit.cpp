#include "../sticky_food.hpp"
#include <cassert>
#include <limits>
#include <cstdio>
int main(){
 sticky_food::MouthGate g;
 assert(!g.update(true,2,100));assert(!g.update(true,2,1000)); // spawn near face
 assert(!g.update(true,20,1010));assert(!g.update(true,2,1100));
 assert(!g.update(true,2,1749));assert(g.update(true,2,1750));
 assert(!g.update(true,2,2500)); // one shot until a new outside approach
 assert(!g.update(true,20,2510));assert(!g.update(true,2,2520));
 assert(!g.update(false,2,2600));assert(!g.update(true,2,4000)); // native handoff
 assert(!g.update(true,20,4010));assert(!g.update(true,2,4020));
 assert(!g.update(true,14,4600));assert(!g.update(true,2,4700));
 assert(!g.update(true,2,5300));assert(g.update(true,2,5350)); // dwell interrupted
 assert(!g.update(true,20,5400));assert(!g.update(true,std::numeric_limits<float>::quiet_NaN(),5500));
 assert(!g.update(true,2,6500));
 g.reset();assert(!g.update(true,20,7000));assert(!g.update(true,11,7100));
 assert(!g.update(true,12.5f,7500));assert(g.update(true,12.5f,7750)); // tolerance while dwelling
 puts("PASS: mouth approach, dwell, single-use, native handoff and invalid tracking guards");
}
