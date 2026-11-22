#pragma once
#include <cstdint>
#include <cstring>
namespace fc_features {
enum Id {Height,Weapons,Consumables,Fingers,Sprint,Presentation,Hints,Reload,Pouch,Magazines,Tutorial,Mouth,Bottles,Glass,Light,Driving,Flight,Riding,Pool,Workshop,Command,Remapping,Book,Count};
struct Definition {const char* key;const char* label;const char* detail;bool addon;bool locked;};
inline constexpr Definition definitions[]={
 {"SeatedHeight","Seated height","Saved offsets for ground, vehicles and riding.",false,false},
 {"WeaponWheel","Weapon wheel","Touch to select a weapon; centre cap clears selection.",false,false},
 {"ConsumableWheel","Consumable wheel","Select into the right hand without consuming.",false,false},
 {"FingerGestures","Finger gestures","Additional finger poses; native grips remain available.",false,false},
 {"Sprint","Stick-tip sprint","Full-forward sprint with configurable speed and AP drain.",false,false},
 {"WorldPanels","Captions / HUD placement","Stable panels and dialogue/name placement. Off restores native layout.",false,false},
 {"Hints","Routine hints","Selector reminders. Settings messages remain visible.",false,false},
 {"ReloadMode","Reload mode switching","Allows the manual / automatic reload shortcut.",false,false},
 {"SeatedPouch","Seated ammo pouch","Moves the native spare pouch into seated reach.",false,false},
 {"RecoveredMagazines","Recovered magazines","Reuse compatible native dropped magazines.",false,false},
 {"WeaponTutorial","Weapon tutorials","Enables the tutorial shortcut and reload guides.",false,false},
 {"MouthConsumption","Food / drink interaction","Consume selected ingestibles at your mouth.",false,false},
 {"EmptyBottles","Throwable empty bottles","Supported drinks leave a matching empty bottle.",false,false},
 {"GlassImpacts","Glass impact effects","Strong contacts break tracked bottles with glass effects.",false,false},
 {"WristLight","Wrist flashlight","Wrist-guided lamp with brightness, colour and range settings.",false,false},
 {"Driving","Driving (experimental)","Bundled vehicle module; original car mod and local assets required. Off applies now. Restart for first activation.",true,false},
 {"Flight","Vertibird (experimental)","Bundled vehicle module; original aircraft mods and local assets required. Off applies now. Restart for first activation.",true,false},
 {"Riding","Creature riding (experimental)","Bundled vehicle module. Off applies now. Restart for first activation.",true,false},
 {"Pool","Pool","Saloon pool against Sunny, practice, trick shots (Pool V.A.T.S.), and two-player hotseat. Restart to apply.",true,false},
 {"Workshop","Settlement builder (test)","Place buildings and manage the Forts settler roster. Requires the optional bridge. Restart to apply.",true,false},
 {"Command","Troop command (test)","Enclave radio and orders through the Field Kit. Requires the optional bridge. Restart to apply.",true,false},
 {"ControllerRemapping","Controller remapping","Optional saved button profiles. Off keeps native assignments.",false,false},
 {"ControlsBook","Kupo's controls book","Inventory book: left hand holds; left/right trigger turns back/forward.",false,false}
};
static_assert(sizeof(definitions)/sizeof(definitions[0])==Count);
constexpr uint32_t bit(int id){return id>=0&&id<Count?1u<<id:0;}
constexpr uint32_t coreMask=((1u<<Driving)-1)|bit(Remapping)|bit(Book);
constexpr uint32_t addonMask=bit(Driving)|bit(Flight)|bit(Riding)|bit(Pool)|bit(Workshop)|bit(Command);
constexpr uint32_t vehicleMask=bit(Driving)|bit(Flight)|bit(Riding);
inline bool restartNeeded(uint32_t selected,uint32_t session){
 // A disabled vehicle can resume this run only when its adapter was installed at startup.
 return ((selected&~session&vehicleMask)|((selected^session)&addonMask&~vehicleMask))!=0;
}
constexpr uint32_t allowedMask=coreMask|addonMask;
inline int find(const char* key){if(key)for(int i=0;i<Count;++i)if(!strcmp(key,definitions[i].key))return i;return -1;}
struct View {uint32_t size=sizeof(View),version=1;uint32_t visible=0,epoch=0,selected=0,pending=0,available=coreMask,restart=0,page=0;char status[160]{};};
struct Selection {
 uint32_t pending=0,available=coreMask;int selected=0;
 void begin(uint32_t current,uint32_t present){pending=current&allowedMask;available=(present|coreMask)&allowedMask;selected=0;}
 bool toggle(int id){if(!(available&bit(id)))return false;pending^=bit(id);return true;}
 void clear(){pending=0;}
};
}
