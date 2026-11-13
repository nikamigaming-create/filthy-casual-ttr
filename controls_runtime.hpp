#pragma once
#include "controls_model.hpp"
#include "controls_catalog.hpp"
#include <mutex>
#include <fstream>
#include <filesystem>
namespace controls_runtime {
using namespace fc_controls;
inline std::mutex mutex;
inline Engine engine;
inline Profiles profiles;inline std::string bank="onfoot",liveMode="onfoot";inline bool followMode=true;
inline Frame physical{},previous{};
inline int pointerRow=-1,pointerHand=1;inline float pointerX=0,pointerY=0;inline uint64_t pointerAt=0;
inline bool available=false,captureTouches=false;
inline bool visible=false,draining=false,initialized=false,dirty=false,openRequested=false;
inline unsigned epoch=0;
inline int selected=0,slot=1,phase=0; // browse, release, capture, confirm
inline Binding proposal;
inline bool captured=false;
inline uint64_t recoveryAt=0,navigationAt=0,captureAt=0;
inline bool recoveryFired=false,recoveryArmed=false;
inline std::string status="Native defaults. Select a control to change its binding.",directory;
inline std::array<std::string,9> originals;
inline std::array<bool,9> observed{};
inline bool addonPrevious[5]{};
constexpr int rows=actionCount+4;
inline void pluginState(){
 auto m=GetModuleHandleW(L"SeatedControls.dll");if(!m)return;
 using State=void(__cdecl*)(unsigned,unsigned);auto f=reinterpret_cast<State>(GetProcAddress(m,"SC_ControlsState"));
 unsigned overrides=0;for(int i=0;i<5;i++)if(engine.profile.bindings[nativeCount+i].source!=-1)overrides|=1u<<i;
 if(f)f(visible||draining?1:0,overrides);
}
inline std::string path(int n){return directory+"\\profile-"+std::to_string(n)+".fcp";}
inline std::string read(const std::string&p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
inline bool atomicWrite(const std::string&p,const std::string&data){
 auto tmp=p+".tmp-"+std::to_string(GetCurrentProcessId());auto h=CreateFileA(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(h==INVALID_HANDLE_VALUE)return false;
 DWORD written=0;bool ok=WriteFile(h,data.data(),DWORD(data.size()),&written,nullptr)&&written==data.size()&&FlushFileBuffers(h);CloseHandle(h);
 if(ok){if(GetFileAttributesA(p.c_str())!=INVALID_FILE_ATTRIBUTES){auto backup=p+".bak";ok=ReplaceFileA(p.c_str(),tmp.c_str(),backup.c_str(),REPLACEFILE_IGNORE_MERGE_ERRORS,nullptr,nullptr)!=0;}else ok=MoveFileExA(tmp.c_str(),p.c_str(),MOVEFILE_WRITE_THROUGH)!=0;}
 if(!ok)DeleteFileA(tmp.c_str());return ok;
}

inline bool storeBank(){if(!profiles.modes.count(bank)&&profiles.modes.size()>=32){status="Profile contains 32 modes. Load a new slot before adding another.";return false;}profiles.get(bank)=engine.profile;return true;}
inline bool fits(const Profiles& p){size_t count=p.modes.size();for(auto& m:controls_catalog::modes)if(!p.modes.count(m.id))++count;return count<=32;}
inline void changeBank(const std::string& id){if(bank==id)return;storeBank();bank=id;selected=0;engine.profile=profiles.modes.count(bank)||profiles.modes.size()<32?profiles.get(bank):Profile{};engine.reset();draining=true;for(auto& v:addonPrevious)v=false;phase=0;captureAt=0;captured=false;pluginState();}
inline std::vector<int> displayed(const std::string& id){std::vector<int> out;auto& m=controls_catalog::find(id);for(int i=0;i<actionCount;i++)if(m.labels[i]!="Unused channel")out.push_back(i);for(int i=actionCount;i<rows;i++)out.push_back(i);return out;}
inline int position(const std::vector<int>& list,int row){auto p=std::find(list.begin(),list.end(),row);return p==list.end()?0:int(p-list.begin());}
inline std::string jsonString(const std::string& value){std::string out="\"";for(unsigned char c:value){if(c=='"'||c=='\\'){out+='\\';out+=char(c);}else if(c<32){const char* hex="0123456789abcdef";out+="\\u00";out+=hex[c>>4];out+=hex[c&15];}else out+=char(c);}return out+'"';}
inline std::string catalogJson(){std::ostringstream o;o<<"{\"version\":1,\"sources\":[";for(int i=0;i<sourceCount;i++){if(i)o<<',';o<<jsonString(sourceNames[i]);}o<<"],\"modes\":[";bool first=true;for(auto& m:controls_catalog::modes){if(!first)o<<',';first=false;o<<"{\"id\":"<<jsonString(m.id)<<",\"title\":"<<jsonString(m.title)<<",\"available\":"<<(m.available?"true":"false")<<",\"actions\":[";for(int i=0;i<actionCount;i++){if(i)o<<',';o<<jsonString(m.labels[i]);}o<<"]}";}o<<"]}";return o.str();}
inline void pollMode(uint64_t now){auto before=controls_catalog::scanned;controls_catalog::discover(now);if(before!=controls_catalog::scanned){for(auto& m:controls_catalog::modes)if(profiles.modes.count(m.id)||profiles.modes.size()<32)profiles.get(m.id);static std::string written;auto json=catalogJson();if(json!=written&&atomicWrite(directory+"\\catalog.json",json))written=json;}
 liveMode=controls_catalog::current();if(!visible||followMode)changeBank(liveMode);}
inline void init(){if(initialized)return;initialized=true;controls_catalog::discover(GetTickCount64());char root[MAX_PATH]{};
 if(GetEnvironmentVariableA("SEATED_TEST_DIR",root,MAX_PATH))directory=std::string(root)+"\\controls-profiles";
 else{GetEnvironmentVariableA("LOCALAPPDATA",root,MAX_PATH);directory=std::string(root)+"\\FilthyCasualTTR\\Controls";}
 std::error_code error;std::filesystem::create_directories(directory,error);
 auto active=read(directory+"\\active.txt");if(active.size()==1&&active[0]>='1'&&active[0]<='8')slot=active[0]-'0';
 auto data=read(path(slot));originals[slot]=data;observed[slot]=true;Profiles p;if(!data.empty()){if(decode(data,p)&&fits(p)){profiles=p;engine.profile=profiles.get(bank);}else status="Profile rejected; using native defaults. Saved file was not changed.";}
 pluginState();
}
inline void open(){followMode=true;changeBank(liveMode);visible=true;draining=true;phase=1;captured=false;openRequested=false;engine.reset();++epoch;status="Release buttons, then choose a binding. Changes stay temporary until saved.";pluginState();}
inline void close(){changeBank(liveMode);visible=false;draining=true;phase=0;engine.reset();status=dirty?"Unsaved changes remain active this session.":"Profile active.";pluginState();}
inline bool neutral(const Frame&f){for(int i=0;i<19;i++)if(f[i].active&&std::abs(f[i].value)>.15f)return false;return true;}
inline void load(){auto data=read(path(slot));Profiles p;if(data.empty()||!decode(data,p)||!fits(p)){status="No valid saved profile in this slot. Current controls kept.";return;}
 profiles=p;engine.profile=profiles.get(bank);engine.reset();originals[slot]=data;observed[slot]=true;dirty=false;draining=true;status="Profile "+std::to_string(slot)+" loaded. Release controls to apply.";atomicWrite(directory+"\\active.txt",std::to_string(slot));pluginState();}
inline void save(){auto existing=read(path(slot));if(!observed[slot]){originals[slot]=existing;if(!existing.empty()){status="This slot contains a profile. Load it first before overwriting.";return;}observed[slot]=true;}
 if(existing!=originals[slot]){status="Profile changed on disk; load it before saving. Nothing overwritten.";return;}
 if(!storeBank())return;auto data=encode(profiles);Profiles checked;if(!decode(data,checked)){status="Profile validation failed; saved file kept.";return;}if(!atomicWrite(path(slot),data)){status="Save failed. Current bindings kept; check profile folder permissions.";return;}
 originals[slot]=data;dirty=false;status="Saved profile "+std::to_string(slot)+" for next launch.";if(!atomicWrite(directory+"\\active.txt",std::to_string(slot)))status="Profile saved; could not select it for next launch.";
}
inline int conflicts(const Binding&b,int row){int n=0;if(b.source<0)return n;for(int i=0;i<actionCount;i++)if(i!=row){auto other=engine.profile.bindings[i];if(other.source==b.source&&other.modifier==b.modifier)++n;}return n;}
inline void invalidate(){std::lock_guard<std::mutex> lock(mutex);available=false;visible=false;draining=false;openRequested=false;phase=0;captureAt=0;recoveryAt=0;recoveryArmed=false;engine.reset();physical={};previous={};for(auto& v:addonPrevious)v=false;status="Input session interrupted; release controls before using a profile.";pluginState();}
inline void update(const Frame&f,uint64_t now){std::lock_guard<std::mutex> lock(mutex);init();pollMode(now);physical=f;
 if(auto module=GetModuleHandleW(L"SeatedControls.dll")){using Raw=unsigned(__cdecl*)(const char*,int,int,float,float);auto fn=reinterpret_cast<Raw>(GetProcAddress(module,"SC_Input"));if(fn)for(int h=0;h<2;h++)fn("fc_physical",h,f[h*5].active?8:0,f[h*5+1].value,f[h*5].value);}
 bool pointing=pointerAt&&now-pointerAt<180&&pointerRow>=0;
 auto down=[&](int i){return f[i].active&&f[i].value>.55f;};auto press=[&](int i){return down(i)&&(!previous[i].active||previous[i].value<=.55f);};
 bool focused=f[0].active&&f[5].active;if(!available&&focused){available=true;draining=true;engine.reset();pluginState();}bool recovery=down(10)&&down(9);
 if(!recovery){recoveryAt=0;recoveryFired=false;recoveryArmed=true;}
 if(recovery&&recoveryArmed&&!recoveryFired){if(!recoveryAt)recoveryAt=now;if(now-recoveryAt>=1000){recoveryFired=true;if(visible)close();else open();}}
 if(openRequested&&focused)open();
 if(!focused){engine.reset();draining=true;recoveryAt=0;recoveryArmed=false;if(visible){phase=1;captured=false;status="Tracking paused. Release controls when focus returns.";}}
 if(draining&&focused&&neutral(f)&&!recovery){draining=false;if(visible&&phase==1)phase=0;pluginState();}
 if(visible&&!draining&&focused){
  bool pointedClick=pointing&&press(pointerHand?5:0);
  if(phase==0){
   if(pointedClick&&(pointerRow==rows+5||pointerRow==rows+6||pointerRow==rows+7)){
    if(pointerRow==rows+7){followMode=true;changeBank(liveMode);}else{followMode=false;size_t n=0;for(size_t i=0;i<controls_catalog::modes.size();i++)if(controls_catalog::modes[i].id==bank)n=i;auto count=controls_catalog::modes.size();n=(n+(pointerRow==rows+6?1:count-1))%count;changeBank(controls_catalog::modes[n].id);}status="Editing "+controls_catalog::find(bank).title+". Saved independently; gameplay follows the active mode.";
   }
   if(pointedClick&&pointerRow<rows)selected=pointerRow;
   if(pointedClick&&pointerRow==rows){auto list=displayed(bank);selected=list[std::max(0,(position(list,selected)/6-1)*6)];}
   if(pointedClick&&pointerRow==rows+1){auto list=displayed(bank);selected=list[std::min(int(list.size())-1,(position(list,selected)/6+1)*6)];}
   if(press(8)){close();}
   else{
    int step=down(18)?-1:down(17)?1:0;
    if(step&&(now-navigationAt>260||press(18)||press(17))){auto list=displayed(bank);selected=list[(position(list,selected)+step+int(list.size()))%int(list.size())];navigationAt=now;}
    if(press(11)||press(12)){slot=(slot+(press(12)?1:7)-1)%8+1;status="Profile slot "+std::to_string(slot)+". Use Save or Load below.";}
    if(selected<actionCount&&(profiles.modes.count(bank)||profiles.modes.size()<32)){
     if(press(4)){captureTouches=!captureTouches;status=captureTouches?"Capture includes capacitive touch sensors.":"Capture buttons, sticks and separate thumb rests.";}
     if(press(15)||press(16)){auto& b=engine.profile.bindings[selected];b.delay=std::clamp(b.delay+(press(16)?100:-100),200,2000);dirty=true;engine.reset();status="Gesture threshold: "+std::to_string(b.delay)+"ms";}
     if(press(2)){auto& b=engine.profile.bindings[selected];if(b.source>=0){b.gesture=Gesture((b.gesture+1)%5);dirty=true;engine.reset();status=bindingText(b);}else status="Assign a button before choosing Tap / Hold.";}
     if(press(3)){engine.profile.bindings[selected]=defaults(selected);dirty=true;engine.reset();status="Selected binding restored to default.";pluginState();}
     if(pointedClick&&pointerRow==rows+4){engine.profile.bindings[selected]={-2,-1,Level,600};dirty=true;engine.reset();status="Selected binding disabled. Y restores it.";pluginState();}
    }
    if(press(7)||(pointedClick&&pointerRow<rows)){
     if(selected<actionCount&& !profiles.modes.count(bank)&&profiles.modes.size()>=32){status="No room for another mode. Load a smaller profile before editing.";}
     else if(selected<actionCount){proposal=engine.profile.bindings[selected];proposal.source=-2;proposal.modifier=-1;captured=false;phase=1;draining=true;captureAt=now;status="Release buttons, then press the new input or two-input combo.";}
     else if(selected==actionCount)save();else if(selected==actionCount+1)load();
     else if(selected==actionCount+2){engine.profile=Profile{};engine.reset();dirty=true;status="All bindings restored to TTR / Filthy Casuals defaults.";pluginState();}
     else close();
    }
   }
  }else if(phase==2){
   for(int i=0;i<sourceCount;i++)if(press(i)){
    // A button contact sensor is not a second input for its own click.
    if(i>=21&&!captureTouches)continue;
    if(!captured){proposal.source=i;captured=true;captureAt=now;}
    else if(i!=proposal.source&&proposal.modifier<0&&now-captureAt<350)proposal.modifier=i;
   }
   if(captured&&!down(proposal.source)&&(proposal.modifier<0||!down(proposal.modifier))){phase=3;status="Proposed: "+bindingText(proposal);int n=conflicts(proposal,selected);if(n)status+=" | Shared by "+std::to_string(n)+" other binding(s). A keeps both.";}
   if(now-captureAt>15000){phase=0;status="Capture cancelled; no change.";}
  }else if(phase==3){
   if(press(8)||(pointedClick&&pointerRow==rows+3)){phase=0;status="Change cancelled.";}
   else if(press(7)||(pointedClick&&pointerRow==rows+2)){engine.profile.bindings[selected]=proposal;dirty=true;engine.reset();phase=0;status="Binding applied. Save profile to keep it for next launch.";pluginState();}
   else if(press(2)){proposal.gesture=Gesture((proposal.gesture+1)%5);status="Proposed: "+bindingText(proposal);}
  }
 }
 // Phase 1 uses a release barrier; then capture starts only for an edited row.
 if(visible&&!draining&&phase==0&&proposal.source==-2&&captureAt&&now>=captureAt){phase=2;captureAt=now;proposal.source=-3;}
 if(phase!=1&&phase!=2)captureAt=0;
 engine.update(f,now,visible||draining||!focused);
 for(int i=0;i<5;i++){auto v=engine.output[nativeCount+i];bool on=v.active&&v.value>.55f;if(on&&!addonPrevious[i]&&!visible&&!draining&&liveMode=="onfoot"){auto m=GetModuleHandleW(L"SeatedControls.dll");using Fn=void(__cdecl*)(int);auto fn=m?reinterpret_cast<Fn>(GetProcAddress(m,"SC_ControlAction")):nullptr;if(fn)fn(i);}addonPrevious[i]=on;}
 previous=f;
}
inline bool route(int destination,Sample&sample){std::lock_guard<std::mutex> lock(mutex);if(destination<0||destination>=nativeCount)return false;
 if(!available){if(engine.profile.bindings[destination]==defaults(destination)||engine.profile.bindings[destination].source==-1)return false;sample={};return true;}
 if(visible||draining){sample.value=0;return true;}
 if(recoveryAt&&(destination==9||destination==10)){sample.value=0;return true;}
 if(engine.profile.bindings[destination]==defaults(destination)||engine.profile.bindings[destination].source==-1)return false;
 if(engine.profile.bindings[destination].source<0){sample.value=0;return true;}
 sample=engine.output[destination];return true;
}
struct View {bool visible=false;unsigned epoch=0;int selected=0,slot=1,phase=0;bool dirty=false;Profile profile;Binding proposal;Frame physical;std::string status;std::string mode,title,description,live;bool automatic;std::array<std::string,actionCount> labels;std::vector<int> displayedRows;};
inline void pointer(int row,int hand,float x,float y){std::lock_guard<std::mutex> lock(mutex);pointerRow=row;pointerHand=hand;pointerX=x;pointerY=y;pointerAt=row>=0?GetTickCount64():0;}
inline View snapshot(){std::lock_guard<std::mutex> lock(mutex);if(controls_catalog::modes.empty())controls_catalog::modes.push_back(controls_catalog::native());return {visible,epoch,selected,slot,phase,dirty,engine.profile,proposal,physical,status,bank,controls_catalog::find(bank).title,controls_catalog::find(bank).description,liveMode,followMode,controls_catalog::find(bank).labels,displayed(bank)};}
inline void request(){std::lock_guard<std::mutex> lock(mutex);openRequested=true;}
inline std::string diagnostics(){std::lock_guard<std::mutex> lock(mutex);std::ostringstream out;out<<"{\"mode\":\""<<bank<<"\",\"liveMode\":\""<<liveMode<<"\",\"modeCount\":"<<controls_catalog::modes.size()<<",\"visible\":"<<(visible?"true":"false")<<",\"draining\":"<<(draining?"true":"false")<<",\"phase\":"<<phase<<",\"selected\":"<<selected<<",\"pointerRow\":"<<pointerRow<<",\"slot\":"<<slot<<",\"bindings\":[";for(int i=0;i<actionCount;i++){if(i)out<<',';auto b=engine.profile.bindings[i];out<<'['<<b.source<<','<<b.modifier<<','<<b.gesture<<']';}out<<"],\"outputs\":[";for(int i=0;i<actionCount;i++){if(i)out<<',';out<<engine.output[i].value;}out<<"]}";return out.str();}
}
