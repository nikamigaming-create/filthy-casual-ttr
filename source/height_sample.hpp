#pragma once
#include <cmath>
#include <cstdint>
namespace height_sample {
// Calibration consumes tracked, stationary samples, not inferred poses retained
// by runtimes across focus/tracking loss. Times are OpenXR nanoseconds.
struct Window {
 int64_t first=0,last=0;float low=0,high=0,value=0;bool ready=false;
 void reset(){first=last=0;ready=false;}
 bool sample(float y,int64_t time,bool tracked){
  if(!tracked||!std::isfinite(y)||y<.25f||y>2.5f){reset();return false;}
  if(time==last)return ready;
  if(!first||time<last||time-last>250000000||y<low-.03f||y>high+.03f){first=time;low=high=y;ready=false;}
  else {if(y<low)low=y;if(y>high)high=y;if(high-low>.03f){first=time;low=high=y;ready=false;}}
  last=time;value=y;ready=time-first>=350000000;return ready;
 }
};
}
