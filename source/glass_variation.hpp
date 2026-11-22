#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace glass_variation {
struct Style {unsigned pattern;float width,height;};
inline Style choose(uint32_t seed,float impact){
 auto next=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
 if(!seed)seed=0x9e3779b9;
 unsigned pattern=next()%3;
 float strength=std::isfinite(impact)?std::clamp((impact-2.5f)/7.5f,0.f,1.f):0;
 float size=(20.f+12.f*strength)*(.85f+float(next()%1001)*.0003f);
 float aspect=.8f+float(next()%1001)*.0004f;
 return {pattern,size*aspect,size/aspect};
}
}
