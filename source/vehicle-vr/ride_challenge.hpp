#pragma once
#include <algorithm>
#include <cmath>
#include <cwchar>
#include <cwctype>
#include <cerrno>
#include <limits>
namespace creature_ride {
constexpr float bighornerDefaultSeconds=30.f, deathclawDefaultSeconds=60.f;
inline float configuredSeconds(const wchar_t* text,float fallback){
 if(!text||!*text)return fallback;
 wchar_t* end=nullptr;errno=0;double seconds=std::wcstod(text,&end);
 if(end==text||errno==ERANGE||!std::isfinite(seconds)||seconds<0||seconds>std::numeric_limits<float>::max())return fallback;
 while(*end&&std::iswspace(*end))++end;
 return *end?fallback:static_cast<float>(seconds);
}
struct Challenge {
 float elapsed=0,unheld=0,duration=bighornerDefaultSeconds;bool controlled=false,thrown=false,warning=false,bucking=false;unsigned cue=0;
 void reset(float seconds=bighornerDefaultSeconds){*this={};duration=std::isfinite(seconds)&&seconds>=0?seconds:bighornerDefaultSeconds;controlled=duration==0;}
 void update(float dt,bool live,bool bodyGrip){
  if(!live||controlled||thrown)return;dt=std::clamp(dt,0.f,.1f);elapsed+=dt;
  if(elapsed>=duration){elapsed=duration;controlled=true;warning=bucking=false;++cue;return;}
  float phase=std::fmod(elapsed,10.f);bool warn=phase>=7.f&&phase<8.f;bool buck=phase>=8.f;
  if(warn&&!warning)++cue;warning=warn;bucking=buck;
  unheld=buck&&!bodyGrip?unheld+dt:0;
  if(unheld>.65f)thrown=true;
 }
};
}
