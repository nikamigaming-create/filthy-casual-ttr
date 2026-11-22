#pragma once
#include <cstdint>
#include <algorithm>
namespace fc_settings {
enum Page { Home, Height, Sprint, Light, LampPosition, Comfort = 6, Hands, Walking, WristHome, WristFeatures, WristControls, WristVoice, WristSettlement, WristPool, WristEquipment };
enum Kind { Action, Toggle, Number, Unavailable };
struct Row { char label[64]{},value[64]{},detail[192]{}; int kind=Action; };
struct View { unsigned epoch=0,page=0,selected=0,count=0;char title[96]{},note[160]{};Row rows[8]{}; };
// Pointer hits are row*3: body, decrease, increase. Back is separate.
constexpr int Back=100;
enum DeviceCommand { DeviceOpen, DeviceClose, DeviceRead, DeviceSelect, DeviceAdjust, DeviceBack, DeviceMove, DevicePresent };
using DeviceSettings = bool (__cdecl*)(unsigned, int, int, View*, unsigned);
constexpr int DeviceRows=5;
inline int firstVisible(const View& view){return std::clamp(int(view.selected)-2,0,std::max(0,int(view.count)-DeviceRows));}
enum EditorCommand { EditorOpen, EditorClose, EditorRead, EditorChoose, EditorBack };
using DeviceEditor = bool (__cdecl*)(unsigned, int, View*, unsigned);
struct Navigation {
 int selected=0;uint64_t next=0;bool axisHeld=false;
 int step(float x,float y,uint64_t now,int count){
  if(x*x+y*y<.25f){axisHeld=false;return 0;}
  if(axisHeld&&now<next)return 0;
  if(x*x+y*y<.4225f)return 0;
  next=now+(axisHeld?220:380);axisHeld=true;
  if(y*y>x*x){selected=(selected+(y>0?-1:1)+count)%count;return 0;}
  return x>0?1:-1;
 }
};
}
