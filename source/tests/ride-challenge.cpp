#include "../vehicle-vr/ride_challenge.hpp"
#include <cstdio>
int main(){using namespace creature_ride;Challenge a;
 for(float seconds:{bighornerDefaultSeconds,deathclawDefaultSeconds,120.f,2.5f}){
  a.reset(seconds);for(int i=0;i<int(seconds*100)-2;i++)a.update(.01f,true,true);
  if(a.controlled||a.thrown)return 1;
  for(int i=0;i<6;i++)a.update(.01f,true,true);if(!a.controlled||a.thrown)return 2;
 }
 a.reset();for(int i=0;i<900;i++)a.update(.01f,true,false);if(!a.thrown||a.controlled)return 3;
 a.reset();for(int i=0;i<12000;i++)a.update(.01f,false,true);if(a.elapsed!=0)return 4;
 a.reset(0);if(!a.controlled||a.thrown||a.warning||a.bucking||a.elapsed)return 5;
 for(int i=0;i<900;i++)a.update(.01f,true,false);if(!a.controlled||a.thrown||a.elapsed)return 6;
 a.reset();if(a.controlled||a.elapsed||a.thrown||a.duration!=30)return 7;
 if(configuredSeconds(L"0",60)!=0||configuredSeconds(L" 2.5 ",30)!=2.5f||configuredSeconds(L"120",30)!=120)return 8;
 for(auto text:{L"",L"bad",L"-1",L"nan",L"inf",L"1e999",L"60oops",L"1e50"})if(configuredSeconds(text,60)!=60)return 9;
 puts("PASS: configurable 30/60/custom seconds, immediate zero unlock, body hold, buck ejection, pause, reset and invalid INI fallback");
}
