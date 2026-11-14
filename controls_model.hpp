#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <string>
#include <map>
#include "controller_profiles.hpp"
namespace fc_controls {
constexpr int sourceCount=70,nativeCount=21,actionCount=26;
enum Gesture {Level,Press,Tap,Hold,DoubleTap};
inline const char* sourceNames[]={"LeftTrigger","LeftGrip","X","Y","LeftStickClick","RightTrigger","RightGrip","A","B","RightStickClick","LeftMenu","LeftStickLeft","LeftStickRight","LeftStickDown","LeftStickUp","RightStickLeft","RightStickRight","RightStickDown","RightStickUp","LeftThumbRest","RightThumbRest","LeftStickTouch","RightStickTouch","XTouch","YTouch","ATouch","BTouch","LeftTriggerTouch","RightTriggerTouch","RightMenu","LeftTrackpadClick","RightTrackpadClick","LeftTrackpadTouch","RightTrackpadTouch","LeftTrackpadLeft","LeftTrackpadRight","LeftTrackpadDown","LeftTrackpadUp","RightTrackpadLeft","RightTrackpadRight","RightTrackpadDown","RightTrackpadUp","LeftGripForce","RightGripForce","LeftTrackpadForce","RightTrackpadForce","LeftShoulder","RightShoulder","LeftDpadUp","LeftDpadDown","LeftDpadLeft","LeftDpadRight","RightX","RightY","LeftGripTouch","RightGripTouch","LeftShoulderTouch","RightShoulderTouch","LeftDpadUpTouch","LeftDpadDownTouch","LeftDpadLeftTouch","LeftDpadRightTouch","RightXTouch","RightYTouch","LeftMenuTouch","RightMenuTouch","LeftTriggerClick","RightTriggerClick","LeftGripClick","RightGripClick"};
inline const char* actionNames[]={"Left trigger","Left grip","X / left primary","Y / left secondary","Left stick click","Right trigger","Right grip","A / right primary","B / right secondary","Right stick click","Left Menu","Move left","Move right","Move backward","Move forward","Turn left / scroll left","Turn right / scroll right","Crouch / scroll down","Jump / scroll up","Left thumb-rest touch","Right thumb-rest touch","Weapon wheel","Consumable wheel","Reload mode","Height / settings","Weapon tutorial"};
inline const char* descriptions[]={
 "Fire, weapon parts, scope, heavy attack/block, stim activation; near wrist: Pip-Boy/flashlight; pouch: ammo type. Both triggers calibrate.",
 "Grab, holsters, gravity glove, support grip, prime throwable, punch, pouch move; release throws/stows. Spatial rules stay native.",
 "Activate, drop/save ammo, bolt lock, pump, gates, reload-pose learning. Native context selects the action.",
 "Pause / native menu. Also used by Filthy Casuals settings when open.",
 "Wait on click; Console on hold. Both uses move together.",
 "Fire, weapon parts, scope, heavy attack/block, stim activation; near Wrist-Boy: on/off/category. Both triggers calibrate.",
 "Grab, holsters, gravity glove, support grip, prime throwable, punch, pouch move; Wrist-Boy pages; release throws/stows.",
 "Activate, drop/save ammo, bolt lock, pump, gates, reload-pose learning; hold during conversation for voice.",
 "V.A.T.S. / Back. Filthy Casuals also uses this native channel to close selectors.",
 "Favorites: click, point stick, release. Other native uses stay together.",
 "Filthy Casuals: tap reload mode, hold height/settings, triple tap tutorial. Native menu input otherwise.",
 "Left stick negative X. Analog movement is preserved when using a stick direction.","Left stick positive X.","Left stick negative Y.","Left stick positive Y.",
 "Right stick negative X; native turn/loot context.","Right stick positive X; native turn/loot context.","Right stick negative Y; native crouch/loot context.","Right stick positive Y; native jump/loot context.",
 "Native left thumb-rest touch channel; physical finger tracking stays independent.","Native right thumb-rest touch channel; physical finger tracking stays independent.",
 "Default: touch opposite thumb rest and double-tap right thumb rest. A custom binding replaces only this shortcut.",
 "Default: touch opposite thumb rest and double-tap left thumb rest. A custom binding replaces only this shortcut.",
 "Default: short left Menu tap. Changes manual/automatic reload; does not replace native weapon mechanics.",
 "Default: hold left Menu. Height stays saved between runs. Editor access: click left stick in Height/settings.",
 "Default: triple-tap left Menu. Shows the weapon tutorial."};
inline const char* gestureNames[]={"Direct","Press","Tap","Hold","Double tap"};
struct Binding {int source=-1,modifier=-1;Gesture gesture=Level;int delay=600;};
inline bool operator==(const Binding&a,const Binding&b){return a.source==b.source&&a.modifier==b.modifier&&a.gesture==b.gesture&&a.delay==b.delay;}
inline Binding defaults(int row){return {row<nativeCount?row:-1,-1,Level,600};}
inline bool valid(const Binding&b){return b.source>=-2&&b.source<sourceCount&&b.modifier>=-1&&b.modifier<sourceCount&&(b.source<0?b.modifier==-1:b.modifier!=b.source)&&int(b.gesture)>=0&&int(b.gesture)<=4&&b.delay>=200&&b.delay<=2000;}
struct Sample {float value=0;bool active=false;int64_t time=0;};
using Frame=std::array<Sample,sourceCount>;
struct Profile {std::array<Binding,actionCount> bindings;Profile(){for(int i=0;i<actionCount;i++)bindings[i]=defaults(i);}};
inline std::array<std::string,sourceCount> sourceLabels{};
inline std::string sourceLabel(int i){return sourceLabels[i].empty()?sourceNames[i]:sourceLabels[i];}
inline std::string bindingText(const Binding& b){if(b.source==-1)return "TTR / Filthy Casuals default";if(b.source==-2)return "Unbound";std::string s=sourceLabel(b.source);if(b.modifier>=0)s=sourceLabel(b.modifier)+" + "+s;s+=" / ";s+=gestureNames[b.gesture];if(b.gesture==Hold)s+=" "+std::to_string(b.delay)+"ms";return s;}
inline std::string encode(const Profile&p){std::ostringstream out;out<<"FilthyCasualControls 2\n";for(int i=0;i<actionCount;i++){auto b=p.bindings[i];out<<i<<' '<<b.source<<' '<<b.modifier<<' '<<int(b.gesture)<<' '<<b.delay<<'\n';}return out.str();}
inline bool decode(const std::string&s,Profile&out){std::istringstream in(s);std::string magic;int version=0;in>>magic>>version;if(magic!="FilthyCasualControls"||(version!=1&&version!=2))return false;Profile p;int count=version==1?24:actionCount;for(int i=0;i<count;i++){int row=-1,g=-1;Binding b;if(!(in>>row>>b.source>>b.modifier>>g>>b.delay)||row!=i)return false;b.gesture=Gesture(g);if(!valid(b))return false;int destination=version==1&&i>=19?i+2:i;p.bindings[destination]=b;}std::string extra;if(in>>extra)return false;out=p;return true;}

inline bool validModeId(const std::string& id){if(id.empty()||id.size()>80)return false;for(auto c:id)if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-'))return false;return true;}
struct Profiles {std::map<std::string,Profile> modes{{"onfoot",Profile{}}};bool legacy=false;
 Profile& get(const std::string& id){auto p=modes.find(id);if(p==modes.end())p=modes.emplace(id,legacy?modes.at("onfoot"):Profile{}).first;return p->second;}};
inline std::string encode(const Profiles& p){std::ostringstream o;o<<"FilthyCasualControls 3\n"<<p.modes.size()<<'\n';for(auto& m:p.modes){o<<m.first<<'\n';for(int i=0;i<actionCount;i++){auto b=m.second.bindings[i];o<<i<<' '<<b.source<<' '<<b.modifier<<' '<<int(b.gesture)<<' '<<b.delay<<'\n';}}return o.str();}
inline bool decode(const std::string& s,Profiles& out){if(s.size()>131072)return false;std::istringstream in(s);std::string magic;int version=0;in>>magic>>version;if(magic!="FilthyCasualControls")return false;Profiles p;
 if(version==1||version==2){Profile old;if(!decode(s,old))return false;p.modes["onfoot"]=old;p.legacy=true;out=p;return true;}
 if(version!=3)return false;int count=0;if(!(in>>count)||count<1||count>32)return false;p.modes.clear();
 for(int m=0;m<count;m++){std::string id;if(!(in>>id)||!validModeId(id)||p.modes.count(id))return false;Profile bank;for(int i=0;i<actionCount;i++){int row=-1,g=-1;Binding b;if(!(in>>row>>b.source>>b.modifier>>g>>b.delay)||row!=i)return false;b.gesture=Gesture(g);if(!valid(b))return false;bank.bindings[i]=b;}p.modes.emplace(id,bank);}
 std::string extra;if(in>>extra||!p.modes.count("onfoot"))return false;out=p;return true;}

struct State {bool armed=false,down=false,fired=false,first=false;uint64_t since=0,released=0;};
struct Engine {
 Profile profile;std::array<State,actionCount> states{};std::array<Sample,actionCount> output{};
 void reset(){states={};output={};}
 void update(const Frame& f,uint64_t now,bool suspended){
  for(int i=0;i<actionCount;i++){
   auto b=profile.bindings[i];auto& s=states[i];auto& o=output[i];o={};
   if(b.source<0){s={};continue;}
   auto v=f[b.source];bool active=v.active&&(b.modifier<0||f[b.modifier].active);bool down=active&&v.value>.55f&&(b.modifier<0||f[b.modifier].value>.55f);
   o.active=active;o.time=v.time;if(b.modifier>=0)o.time=std::max(o.time,f[b.modifier].time);
   if(suspended||!active){s={};continue;}
   if(!s.armed){if(!down)s.armed=true;continue;}
   bool press=down&&!s.down,release=!down&&s.down;
   if(press){s.since=now;s.fired=false;}
   if(b.gesture==Level)o.value=(b.modifier<0||f[b.modifier].value>.55f)?v.value:0;
   else if(b.gesture==Press)o.value=press?1.f:0.f;
   else if(b.gesture==Hold)o.value=down&&now-s.since>=uint64_t(b.delay)?1.f:0.f;
   else if(b.gesture==Tap)o.value=release&&now-s.since>=35&&now-s.since<uint64_t(b.delay)?1.f:0.f;
   else if(b.gesture==DoubleTap){
    if(s.first&&now-s.released>uint64_t(b.delay))s.first=false;
    if(release&&now-s.since>=35&&now-s.since<=300){if(s.first&&now-s.released>=35){o.value=1;s.first=false;}else{s.first=true;s.released=now;}}
   }
   s.down=down;
  }
 }
};
}
