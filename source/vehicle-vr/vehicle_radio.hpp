#pragma once
// Native actor radio playback, shared by original car creatures and aircraft.
// No music files, alternate mixer, network traffic or global station edits.
namespace vehicle_radio {
inline Script *available=nullptr,*defaultStation=nullptr,*set=nullptr,*notice=nullptr,*offNotice=nullptr;
inline Form* host=nullptr;inline Form* station=nullptr;inline bool on=false;
inline std::vector<Form*> stations;inline std::atomic<int> nearKnob[2]{};
inline std::atomic<unsigned> pending{0};inline unsigned randomState=0x51c03a5u;
inline std::atomic<int> claimed[2]{};inline std::atomic<bool> was[2]{};inline std::atomic<ULONGLONG> pressedAt[2]{};
inline void reset(){host=station=nullptr;on=false;stations.clear();pending=0;for(int i=0;i<2;i++){nearKnob[i]=0;claimed[i]=0;was[i]=false;}}
inline bool init(){
 available=compile("player.GetAvailableRadios",true);defaultStation=compile("RNVTARef",true);
 set = fc::nvse::loadFunction(*scripts, "Radio/Tune");
 notice = fc::nvse::loadFunction(*scripts, "Radio/Notify");
 offNotice = fc::nvse::loadFunction(*scripts, "Radio/NotifyOff");
 return available&&defaultStation&&set&&notice&&offNotice;
}
inline void refresh(){
 stations.clear();auto result=call(available);
 if(result.type==4&&result.value.array&&rideArrays){auto n=rideArrays->size(result.value.array);if(n<=128){std::vector<Element> values(n);if(n&&rideArrays->elements(result.value.array,values.data(),nullptr))for(auto& v:values)if(v.type==2&&v.value.form&&std::find(stations.begin(),stations.end(),v.value.form)==stations.end())stations.push_back(v.value.form);}}
 auto fallback=call(defaultStation);if(fallback.type==2&&fallback.value.form&&std::find(stations.begin(),stations.end(),fallback.value.form)==stations.end())stations.insert(stations.begin(),fallback.value.form);
}
inline void tune(Form* next,bool enabled,bool announce){
 if(!host||!set)return;
 if(station)callOn(set,host,0,station);
 station=next;on=enabled&&station;
 if(on)callOn(set,host,1,station);
 if(announce){if(on)callOn(notice,host,station);else callOn(offNotice,host);}
}
inline void tick(){
 if(!active||riding||!car){reset();return;}
 if(host!=car){
  reset();host=car;refresh();
  // Clear known native stations on this vehicle only, avoiding two radios at
  // once when returning from the dependency's own radio menu.
  for(auto s:stations)callOn(set,host,0,s);
  auto initial=call(defaultStation);tune(initial.type==2?initial.value.form:nullptr,true,false);
 }
 auto command=pending.exchange(0);if(command&&!nativeMenu){
  if(command&1)tune(station,!on,true);
  if(command&6){
   refresh();if(!stations.empty()){
    auto it=std::find(stations.begin(),stations.end(),station);size_t index=it==stations.end()?0:(size_t(it-stations.begin())+1)%stations.size();
    if((command&4)&&stations.size()>1){randomState^=randomState<<13;randomState^=randomState>>17;randomState^=randomState<<5;index=(index+randomState%(stations.size()-1))%stations.size();}
    tune(stations[index],true,true);
   }
  }
 }
}
inline void contacts(void* manager,void* carrier){
 for(int h=0;h<2;h++){
  int closest=0;float distance=6.f;scene::Transform grip{};
  if(active&&!riding&&!nativeMenu&&!controlHeld[h]&&!handOccupied[h]&&carrier&&vehicle_grip::raw(manager,h,grip)){
   for(int i=1;i<=2;i++)if(auto marker=scene::find(carrier,i==1?"FCRadioPower":"FCRadioTune")){
    float d=scene::distance(grip.p,scene::world(marker).p);if(d<distance){distance=d;closest=i;}
   }
  }nearKnob[h]=closest;
 }
 if(!testDir.empty()){
  static ULONGLONG last=0;auto now=GetTickCount64();if(now-last<200)return;last=now;
  std::ofstream o(testDir+"\\vehicle-radio.json");o<<"{\"pid\":"<<GetCurrentProcessId()<<",\"host\":"<<(host?host->id:0)<<",\"on\":"<<on<<",\"station\":"<<(station?station->id:0)<<",\"available\":"<<stations.size()<<",\"nearKnob\":["<<nearKnob[0]<<","<<nearKnob[1]<<"],\"knobs\":[";
  for(int i=0;i<2;i++){if(i)o<<",";auto n=scene::find(carrier,i==0?"FCRadioPower":"FCRadioTune");auto p=n?vehicle_adapter::tracking(scene::world(n).p):scene::Vec{};o<<"["<<p.x<<","<<p.y<<","<<p.z<<"]";}o<<"]}";
 }
}
// Called on the input thread while the host's input lock is held. A click only
// belongs to the radio if it BEGAN over a knob; moving past it cannot steal A/X.
inline bool input(int hand,bool pressed,bool enabled){
 if(!enabled){claimed[hand]=0;was[hand]=pressed;return false;}
 if(pressed&&!was[hand]){claimed[hand]=nearKnob[hand].load();pressedAt[hand]=GetTickCount64();}
 bool owns=claimed[hand]!=0;
 if(!pressed&&was[hand]&&owns){unsigned command=claimed[hand]==1?1:(GetTickCount64()-pressedAt[hand]>=700?4:2);pending.fetch_or(command);claimed[hand]=0;}
 was[hand]=pressed;return owns;
}
}
