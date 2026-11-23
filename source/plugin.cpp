#include "book_discovery.hpp"
#include "equipment_wheel_profiles.hpp"
#include "equipment_layout_view.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "empty_bottles.hpp"
#include "feature_store.hpp"
#include "core_perf.hpp"
#include "held_pose_adapter.hpp"
#include <windows.h>
#include <cstddef>
#include <cstdio>
#include <share.h>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <atomic>
#include <mutex>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <algorithm>
#include "nvse_function.hpp"
#include "build_adapter.hpp"
#include "reload_adapter.hpp"
#include "native_worldmag_recovery.hpp"
#include "reload_trace_adapter.hpp"
#include "scene_adapter.hpp"
#include "controls_book_visual.hpp"
#include "field_rig_debug.hpp"
#include "kill_camera_head_adapter.hpp"
#include "physical_reload_adapter.hpp"
#include "bottle_impact_adapter.hpp"
#include "glass_decal_adapter.hpp"
#include "glass_variation.hpp"
#include "wrist_controls.hpp"
#include "finger_adapter.hpp"
#include "hold_controls.hpp"
#include "reload_tutorial.hpp"
#include "tutorial_adapter.hpp"
#include "tutorial_visual.hpp"
#include "native_pose_dispatch.hpp"
#include "sticky_food.hpp"
#include "sprint_controls.hpp"
#include "settings_pages.hpp"
#include "comfort_controls.hpp"
#include "sprint_adapter.hpp"
#include "wrist_light.hpp"
#include "presentation_adapter.hpp"
#include "layer_discovery.hpp"
namespace {
 Scripts* scripts=nullptr;Arrays* arrays=nullptr;Console* consoleApi=nullptr;std::string gameDirectory;
 InventoryRef*(__cdecl* inventoryRef)(u32)=nullptr;
 FILE* logfile=nullptr;FILE* headsetLog=nullptr;std::mutex logMutex,inputMutex,displayMutex;
 std::string displayText;ULONGLONG displayUntil=0;
 std::atomic<bool> controlsEditor{false};
 Form* bookForm=nullptr;Script* bookGive=nullptr;Script* bookWarm=nullptr;bool bookWarmed=false;Script* bookFind[4]{};bool bookFindDone[4]{};u32 bookFindRefs[4]{};ULONGLONG bookFindAt=0;Script *bookPlace=nullptr,*bookPlaced=nullptr;bool bookGiven=false,bookRouted=false;
 unsigned bookWorldRef=0;bool bookDropPending=false;ULONGLONG bookDropDeadline=0;int bookDropHand=0;
 std::atomic<bool> bookActive{false};controls_book::State bookState;bool bookVisible=false;
 ULONGLONG bookCloseUntil=0;
 void initBook();void tickBook(const struct Input&,bool,ULONGLONG);void publishBook(bool);
 std::atomic<uint32_t> featureMask{fc_features::allowedMask};uint32_t addonSessionMask=0;
 std::mutex featureMutex;fc_features::View featureView;fc_features::Selection featureSelection;
 std::atomic<bool> featureMenu{false};std::atomic<int> featureEvent{-1},featureHover{-1};
 std::string featurePath;ULONGLONG featureInputAt=0;
 bool nativeReloadCaptured=false,nativeReloadManual=false;
 bool feature(int id){return (featureMask.load()&fc_features::bit(id))!=0;}
 void openFeatures();uint32_t availableFeatures();void applyFeatures(uint32_t mask);
 void setTutorial(bool want);
 bool nativeSettingsAvailable(){
  using Probe=unsigned(__cdecl*)();
  static std::atomic<Probe> published{nullptr};
  auto probe=published.load();
  if(!probe){
   auto module=GetModuleHandleW(L"FilthyCasualNativeSettings.dll");
   if(module){probe=reinterpret_cast<Probe>(GetProcAddress(module,"FC_WristSettingsHostVersion"));published=probe;}
  }
  return probe&&probe()==1;
 }
 bool toggleReload(bool notify=true);
 void featureTick(const struct Input& current,ULONGLONG now);

 std::atomic<unsigned> controlsOverrides{0};
 float physicalGrip[2]{},physicalTrigger[2]{};ULONGLONG physicalAt[2]{};
 std::atomic<bool> enabled{false},selectorOpen{false},heightMenu{false},showSelectorPopups{false};
 std::atomic<bool> weaponTransition{false};
 std::atomic<int> weaponHand{-1},aidHand{-1};std::atomic<float> heightOffset{0},physicalHeight{0};std::mutex heightSampleMutex;ULONGLONG heightSampleAt=0;
 std::atomic<float> seatOffsets[5]{};int seatContext=0;
 constexpr const char* seatKeys[]={"HeightOffsetCm","CarSeatOffsetCm","FlightSeatOffsetCm","BighornerSeatOffsetCm","DeathclawSeatOffsetCm"};
 constexpr const char* seatNames[]={"DEFAULT","CAR","VERTIBIRD","BIGHORNER","DEATHCLAW"};
 std::atomic<unsigned> uiEpoch{0};bool controlDrain[2][6]{};
 char settingsPath[MAX_PATH]{};float desiredHeight=1.65f;bool automaticReload=true;
 bool beamInserted=false,beamAttached=false;ULONGLONG beamRetry=0;
 void* beamRig=nullptr;bool lightWasOn=false,lightRestorePending=false;ULONGLONG lightRestoreAt=0;
 Script* lightOn=nullptr;
 Script* emptyEventFlag=nullptr;bool emptyFlagDirty=true;
 struct EmptyEvent {unsigned base;Script* read;Script* handler;};std::vector<EmptyEvent> emptyEvents;bool emptyEventsReady=false;unsigned nativeEmptyAwarded=0;
 void initEmptyEvents();void tickEmptyEvents();
 bool reloadPreferenceApplied=false;
 sprint_controls::Config sprintConfig; sprint_controls::State sprintState;
 Script *sprintAP=nullptr,*sprintDrain=nullptr,*sprintAllowed=nullptr;
 bool sprintNoticeEnabled=true;unsigned sprintNoticeCount=0;std::atomic<ULONGLONG> sprintNoticeUntil{0};
 ULONGLONG sprintAt=0;
 std::mutex settingsMutex;fc_settings::View settingsView;fc_settings::Navigation settingsNav;
 fc_settings::Page settingsPage=fc_settings::Home;std::atomic<int> settingsHover{-1};
 std::atomic_bool wristSettings=false,wristSettingsPresented=false;
 bool usingWristSettings(){return wristSettings.load()&&wristSettingsPresented.load();}
 int wristFeaturePage=0;
 fc_features::Selection wristFeatures;
 std::string wristSettingsStatus;
 void settingsText();void openSettings();void settingsTick(const struct Input&,ULONGLONG);

 bool seatedAmmoPouch=true,pouchApplied=false;float pouchForward=.40f,pouchSide=-.18f,pouchBelow=.45f;
 bool wristEnabled=false,wristInserted=false,wristAttached=false;
 hold_controls::State holdState;
 std::atomic<bool> controlsActive{false};
 std::atomic<int> pendingControl{-1};
 int crashStrength=0;
 std::mutex comfortMutex;fc_comfort::Config comfortConfig;
 std::atomic<ULONGLONG> comfortGameplayAt{0};std::atomic<unsigned> walkRequest{0};
 float skeletalCurls[2][5]{};ULONGLONG skeletalAt[2]{};


 bool tutorialEnabled=false,tutorialDataLoaded=false,tutorialInserted=false,tutorialAttached=false;
 ULONGLONG tutorialVisualRetry=0;
 tutorial_data::Database tutorialWeapons;std::string tutorialError,tutorialEditor;
 reload_tutorial::Step tutorialStep=reload_tutorial::Step::Hidden;
 reload_tutorial::Mechanism tutorialMechanism=reload_tutorial::Mechanism::Generic;
 bool tutorialAnchorValid=false;scene::Vec tutorialAnchor{},tutorialAnchorEye{};
 bool tutorialPublished=false;DWORD tutorialThread=0;unsigned tutorialPublications=0;
 void* tutorialRig=nullptr;void* tutorialCarrier=nullptr;
 scene::Vec tutorialLocalAnchor{},tutorialLocalEye{};
 tutorial_adapter::Snapshot tutorialNative;
 const tutorial_data::Weapon* tutorialWeapon=nullptr;unsigned tutorialForm=0;
 Script *tutorialEditorQuery=nullptr,*tutorialRoundsQuery=nullptr,*tutorialCapacityQuery=nullptr;
 bool fingerGestures=true;
 std::atomic<bool> gestureGameplayAllowed{false}; // Gameplay gate; native adapter checks each hand's occupancy.
 struct Gesture {bool active=false,thumb=false,index=false;ULONGLONG time=0,blendTime=0;float thumbCurl=0,indexCurl=0,lift=0;};
 Gesture gestures[2];
 void* wristNode=nullptr;void* wristRig=nullptr;ULONGLONG wristRetry=0;
 wrist_controls::Frame wristFrame;wrist_controls::TouchState wristTouch;
 struct WristPlunger {void* node=nullptr;scene::Vec rest{};float depression=0;};
 WristPlunger wristPlungers[5];
 const char* wristPlungerNames[5]={"FC_Weapons_Press","FC_Consumables_Press","FC_Reload_Auto_Press","FC_Reload_Manual_Press","FC_Settings_Press"};
 const char* wristHolder="FCWristMount";
 // The cuff's authored FC_* node names are already unique. JIP's optional
 // *suffix* syntax renames every child and would hide the plunger/mode nodes
 // from exact scene lookups.
 const char* wristAttachment="FCWristMount|FilthyCasual\\wrist-controls-auto.nif";

 struct Hand {bool active=false,stick=false,primary=false,secondary=false;float grip=0,trigger=0,x=0,y=0;};
 struct Input {Hand h[2];scene::Vec head,mouth;ULONGLONG mouthTime=0;bool headValid=false;ULONGLONG time=0;} input,previous;
 bool ready=false,loaded=false,waitingRelease=false;int openHand=-1,page=0,selected=-1;
 struct Settings {float forward=42,below=10,radius=14,reach=2.1f,size=16,capSize=8.4f,walkAway=85;} settings;
 ULONGLONG heldSince[2]{},lastLoop=0,lastAnnounce=0;
 constexpr int slots=6,capSlot=slots,previewCount=slots+1;constexpr float pi=3.14159265359f;
 static_assert(slots==fc::wheels::slotCount);
 using Vec=scene::Vec;
 void publishTutorial(bool completed);
 void cullNode(void* node,bool hidden);
 Vec selectorPalms[2]{};
 void* wheelCarrier=nullptr;bool wheelCarrierLost=false;scene::Transform wheelCarrierFrame{},wheelLocalBasis{};
 Vec wheelLocalHead{},wheelLocalRoot{};
 Vec wheelHead, wheelRoot,wheelXRHead;bool wheelXRValid=false;scene::Transform wheelBasis{};void* wheelCell=nullptr;
 ULONGLONG openedAt=0,touchSince=0,chordSince=0,heightStepAt=0;int touchSlot=-1,touchHand=-1;bool chordLatch=false,triggerArmed=false;
 struct Item {Form* form=nullptr;u32 id=0;std::string name,model;int count=0;float scale=.25f;};
 std::vector<Item> items;Item heldAid;Form* selectedRef=nullptr;std::atomic<bool> aidGripLatch{false};
 sticky_food::MouthGate mouthGate;bool heldFood=false;float mouthDistance=-1;
 Vec foodMouthPoint{};
 std::atomic<bool> stickyAidButtons{false};
 bool pendingAid=false,aidDropPending=false;ULONGLONG aidDeadline=0;
 Item pendingWeapon;int pendingHand=-1,pendingStage=0;ULONGLONG pendingAt=0;
 u32 selectedWeaponID=0;ULONGLONG redrawAfter=0;
 Script *dialogueTarget=nullptr,*dialoguePartner=nullptr,*combatDisabled=nullptr,*playerDead=nullptr,*killCameraQuery=nullptr,*firstPersonQuery=nullptr;
 bool replayHeadEnabled=true,replayHeadProbe=false;
 struct Preview {std::string holder,attachment;bool inserted=false,attached=false;scene::WorldPin pin;scene::Transform carrierLocal{};Vec trackingCenter{};std::vector<scene::Box> mesh;};
 Preview previews[previewCount];
 bool touchMustLeave[2][previewCount]{};
 Script* getCap=nullptr;
 Script *getPlayer=nullptr,*getMenu=nullptr,*getWeapons=nullptr,*getAid=nullptr,*getName=nullptr,*getModel=nullptr,*getCount=nullptr,*getDimension=nullptr,*rootScale=nullptr,*getEquipped=nullptr,*weaponOut=nullptr,*holster=nullptr,*draw=nullptr;
 Script *insert=nullptr,*removeNode=nullptr,*attach=nullptr,*detach=nullptr,*translate=nullptr,*scale=nullptr,*position=nullptr,*equip=nullptr,*notice=nullptr;
 Script *getInventoryRefs=nullptr,*moveRef=nullptr,*takeRef=nullptr;

 void log(const char* fmt,...){
  std::lock_guard<std::mutex> lock(logMutex);
  const auto ticks=GetTickCount64();
  va_list args;va_start(args,fmt);
  for(auto file:{logfile,headsetLog})if(file){
   fprintf(file,"[%llu] ",ticks);
   va_list copy;va_copy(copy,args);vfprintf(file,fmt,copy);va_end(copy);
   fputc('\n',file);fflush(file);
  }
  va_end(args);
 }
 void release(Element& e){if(e.type==3&&e.value.string)build_adapter::freeGameString(e.value.string);e={};}
 template<typename... Args> bool call(Script* s,Element& out,Args... args){if(!s)return false;return scripts->call(s,nullptr,nullptr,&out,sizeof...(args),args...);}
 template<typename... Args> double number(Script* s,Args... args){Element e;bool ok=call(s,e,args...);double value=ok&&e.type==1?e.value.number:0;release(e);return value;}
 template<typename... Args> std::string string(Script* s,Args... args){Element e;std::string value;if(call(s,e,args...)&&e.type==3&&e.value.string)value=e.value.string;release(e);return value;}
 u32 bits(float f){u32 b;memcpy(&b,&f,4);return b;}
 void tell(const std::string& s,bool height=false){
  if(!height&&(!feature(fc_features::Hints)||!showSelectorPopups)){std::lock_guard<std::mutex> lock(displayMutex);displayText.clear();log("UI (muted): %s",s.c_str());return;}
  {std::lock_guard<std::mutex> lock(displayMutex);
   displayText=openHand>=0?std::string(openHand==1?"WEAPONS":"CONSUMABLES")+"   "+std::to_string(page+1)+" / "+std::to_string(std::max(1,int((items.size()+slots-1)/slots)))+"\n"+s+"\nTouch to select | Center cap clears hand | B close":s;
   displayUntil=GetTickCount64()+2200;}
  if(notice){Element e;call(notice,e,s.c_str());release(e);}log("UI: %s",s.c_str());
 }
 void readSettings(){
  char path[MAX_PATH]{};GetModuleFileNameA(GetModuleHandleA("SeatedControls.dll"),path,MAX_PATH);
  auto dot=strrchr(path,'.');if(dot)strcpy_s(dot,5,".ini");
  // Explicit local test profile: keep its choices out of MO2's shared overwrite.
  char localTest[MAX_PATH]{};DWORD localTestLen=GetEnvironmentVariableA("FC_TEST_CONFIG",localTest,MAX_PATH);
  if(localTestLen>2&&localTestLen<MAX_PATH&&localTest[1]==':'&&(localTest[2]=='\\'||localTest[2]=='/')&&GetFileAttributesA(localTest)!=INVALID_FILE_ATTRIBUTES)strcpy_s(path,localTest);
  strcpy_s(settingsPath,path);
  // Optional installation-local runtime selection for ordinary MO2 launches.
  // Explicit launcher/simulator environments take precedence; never edit HKLM.
  char runtime[MAX_PATH]{};GetPrivateProfileStringA("OpenXR","RuntimeJson","",runtime,MAX_PATH,path);
  if(runtime[0]&&!GetEnvironmentVariableA("XR_RUNTIME_JSON",nullptr,0)){
   auto attributes=GetFileAttributesA(runtime);
   if(attributes!=INVALID_FILE_ATTRIBUTES&&!(attributes&FILE_ATTRIBUTE_DIRECTORY)){
    log("Configured OpenXR runtime: %s (%s)",runtime,SetEnvironmentVariableA("XR_RUNTIME_JSON",runtime)?"applied":"failed");
   }else log("Configured OpenXR runtime missing: %s",runtime);
  }
  auto value=[&](const char* key,float def,float lo,float hi){char tmp[40],fallback[40];sprintf_s(fallback,"%.2f",def);GetPrivateProfileStringA("Selector",key,fallback,tmp,sizeof(tmp),path);float v=float(atof(tmp));return std::isfinite(v)?std::clamp(v,lo,hi):def;};
  settings.forward=value("ForwardCm",60.f,25,65)*.7f;settings.below=value("BelowEyesCm",14.3f,0,35)*.7f;
  settings.radius=value("RadiusCm",20,10,30)*.7f;settings.reach=value("TouchRadiusCm",3.f,1,8)*.7f;
  settings.size=value("PreviewSizeCm",22.9f,10,40)*.7f;
  settings.capSize=value("CenterCapSizeCm",12,6,16)*.7f;
  crashStrength=std::clamp(int(GetPrivateProfileIntA("Driving","CrashStrength",0,path)),0,2);
  showSelectorPopups=GetPrivateProfileIntA("UI","ShowSelectorPopups",0,path)!=0;
  automaticReload=GetPrivateProfileIntA("Weapons","AutomaticReload",1,path)!=0;
  seatedAmmoPouch=GetPrivateProfileIntA("Weapons","SeatedAmmoPouch",1,path)!=0;
  sprintConfig.enabled=GetPrivateProfileIntA("Sprint","Enabled",1,path)!=0;
  sprintNoticeEnabled=GetPrivateProfileIntA("Sprint","ShowNotification",1,path)!=0;
  auto sprintValue=[&](const char* key,float fallback){char text[40],def[40];sprintf_s(def,"%.3f",fallback);GetPrivateProfileStringA("Sprint",key,def,text,sizeof text,path);return float(atof(text));};
  sprintConfig.start=sprintValue("StartThreshold",.97f);sprintConfig.stop=sprintValue("StopThreshold",.90f);
  sprintConfig.multiplier=sprintValue("SpeedMultiplier",2.f);sprintConfig.apPerSecond=sprintValue("APPerSecond",15.f);sprintConfig.sanitize();
  wrist_light::config.enabled=GetPrivateProfileIntA("WristLight","Enabled",1,path)!=0;
  wrist_light::config.projected=GetPrivateProfileIntA("WristLight","Projected",1,path)!=0;
  if(GetPrivateProfileIntA("WristLight","TraceProjection",0,path))
   projected_lighting::contact_trace::log=log;
  wrist_light::config.gloveMount=GetPrivateProfileIntA("WristLight","GloveMount",1,path)!=0;
  auto lightValue=[&](const char* key,float fallback,float lo,float hi){char text[40],def[40];sprintf_s(def,"%.3f",fallback);GetPrivateProfileStringA("WristLight",key,def,text,sizeof text,path);float v=float(atof(text));return std::isfinite(v)?std::clamp(v,lo,hi):fallback;};
  wrist_light::config.brightness=lightValue("Brightness",1.3f,0,5);wrist_light::config.angle=lightValue("BeamDegrees",32,10,90);wrist_light::config.range=lightValue("RangeMetres",35,2,50)*69.99125f;
  wrist_light::config.forward=lightValue("ForwardCm",-4.29f,-10,20)*.6999125f;wrist_light::config.above=lightValue("AboveCm",6.0f,0,10)*.6999125f;
  wrist_light::config.rgb[0]=lightValue("Red",1,0,1);wrist_light::config.rgb[1]=lightValue("Green",.93f,0,1);wrist_light::config.rgb[2]=lightValue("Blue",.78f,0,1);
  wristEnabled=false; // Thumbrest double taps and left Menu replace the cuff.
  holdState.delayMs=std::clamp(GetPrivateProfileIntA("Controls","HoldMs",1000,path),500u,2000u);
  holdState.doubleTapMs=std::clamp(GetPrivateProfileIntA("Controls","DoubleTapMs",450,path),200u,700u);
  replayHeadEnabled=GetPrivateProfileIntA("Recording","ThirdPersonHead",1,path)!=0;
  fingerGestures=GetPrivateProfileIntA("Hands","FingerGestures",1,path)!=0;
  {std::lock_guard<std::mutex> lock(comfortMutex);auto& c=comfortConfig;
   c.fingers=GetPrivateProfileIntA("Hands","IndexFingers",1,path)!=0;c.grip=GetPrivateProfileIntA("Hands","CustomGrip",0,path)!=0;c.smoothing=GetPrivateProfileIntA("Hands","Smoothing",0,path)!=0;c.walking=GetPrivateProfileIntA("Walking","Enabled",0,path)!=0;c.autoWalk=GetPrivateProfileIntA("Walking","AutoWalk",0,path)!=0;
   auto read=[&](const char* section,const char* key,float def){char text[40],fallback[40];sprintf_s(fallback,"%.3f",def);GetPrivateProfileStringA(section,key,fallback,text,sizeof text,path);return float(atof(text));};
   c.press=read("Hands","GripPress",.7f);c.release=read("Hands","GripRelease",.25f);c.smoothMs=read("Hands","SmoothingMs",12);c.walkSpeed=read("Walking","Speed",.55f);c.deadzone=read("Walking","Deadzone",.15f);c.runStart=read("Walking","RunStart",.60f);c.sprintStart=sprintConfig.start;fc_comfort::sanitize(c);
  }
  auto pouchValue=[&](const char* key,float def,float lo,float hi){char text[40],fallback[40];sprintf_s(fallback,"%.1f",def);GetPrivateProfileStringA("Weapons",key,fallback,text,sizeof(text),path);float v=float(atof(text));return (std::isfinite(v)?std::clamp(v,lo,hi):def)*.01f;};
  pouchForward=pouchValue("PouchForwardCm",40,10,70);pouchSide=pouchValue("PouchSideCm",-18,-60,60);pouchBelow=pouchValue("PouchBelowEyesCm",45,20,120);
  char tmp[40]{};GetPrivateProfileStringA("Seated","HeightOffsetCm","0",tmp,sizeof(tmp),path);const float groundOffset=float(atof(tmp))*.01f;
  heightOffset=std::isfinite(groundOffset)?std::clamp(groundOffset,-.5f,1.2f):0;
  seatOffsets[0]=heightOffset.load();
  for(int i=1;i<5;i++){GetPrivateProfileStringA("Seated",seatKeys[i],"0",tmp,sizeof(tmp),path);float v=float(atof(tmp))*.01f;seatOffsets[i]=std::isfinite(v)?std::clamp(v,-.5f,.5f):0;}
  GetPrivateProfileStringA("Seated","TargetEyeHeightCm","165",tmp,sizeof(tmp),path);desiredHeight=std::clamp(float(atof(tmp))*.01f,1.1f,2.1f);
 }
 void readFeatures(){
  featurePath=std::string(settingsPath)+".features.ini";
  uint32_t saved=0;bool exists=fc_features::read(featurePath,saved);
  if(!exists){
   bool fresh=GetPrivateProfileIntA("FeatureSetup","FirstRun",0,settingsPath)!=0;
   saved=fresh?0:(fc_features::allowedMask&~(fc_features::bit(fc_features::Book)|fc_features::bit(fc_features::Workshop)|fc_features::bit(fc_features::Command)));
   if(!fresh){
    auto clear=[&](int id,bool on){if(!on)saved&=~fc_features::bit(id);};
    clear(fc_features::Fingers,fingerGestures);clear(fc_features::Sprint,sprintConfig.enabled);
    clear(fc_features::Pouch,seatedAmmoPouch);clear(fc_features::Light,wrist_light::config.enabled);
    clear(fc_features::Hints,showSelectorPopups);clear(fc_features::Magazines,GetPrivateProfileIntA("Reload","RecoverWorldMagazines",0,settingsPath)!=0);
    clear(fc_features::Presentation,GetPrivateProfileIntA("Presentation","StableWorldPanels",0,settingsPath)!=0);
   }
  }
  featureMask=saved;addonSessionMask=saved&fc_features::addonMask;
  presentation_adapter::enabled=feature(fc_features::Presentation);floating_hud_adapter::enabled=feature(fc_features::Presentation);
  log("FEATURES saved=%d mask=%08X",exists,saved);
 }
 bool enableInputLayer(){
  wchar_t path[MAX_PATH]{};if(!GetModuleFileNameW(GetModuleHandleW(L"SeatedControls.dll"),path,MAX_PATH))return false;
  auto slash=wcsrchr(path,L'\\');if(!slash)return false;*slash=0;
  std::wstring dir=std::wstring(path)+L"\\SeatedControls";
  if(GetFileAttributesW((dir+L"\\SeatedInput.json").c_str())==INVALID_FILE_ATTRIBUTES)return false;
  // xNVSE loads us before TTWVR creates its XR instance. Process-local discovery
  // makes an ordinary MO2 archive install sufficient; no registry changes.
  bool ok=layer_discovery::append(L"XR_API_LAYER_PATH",dir,true)&&layer_discovery::append(L"XR_ENABLE_API_LAYERS",L"XR_APILAYER_LOCAL_seated_controls");
  wchar_t paths[4096]{};GetEnvironmentVariableW(L"XR_API_LAYER_PATH",paths,4096);log("Input layer discovery: %ls",paths);
  return ok;
 }
 Script* compile(const char* label,const char* text,bool expression=false){auto s=expression?scripts->expression(text):scripts->compile(text);log("compile %s: %s",label,s?"OK":"FAILED");if(!s)log("source: %s",text);return s;}
 bool init(){
  getPlayer=compile("player","player",true);getMenu=compile("menu","MenuMode",true);
  getCap=compile("center bottle cap","Caps001",true);
  rootScale=compile("tracked root scale","player.GetNifBlockScale \"Scene Root\" 1",true);
  tutorialEditorQuery=compile("tutorial editor id","ref item\nbegin function {item}\nSetFunctionValue (GetEditorID item)\nend");
  tutorialRoundsQuery=compile("tutorial rounds","player.GetCurrentAmmoRounds",true);
  tutorialCapacityQuery=compile("tutorial capacity","ref item\nbegin function {item}\nSetFunctionValue (GetWeaponClipRounds item)\nend");
  getEquipped=compile("equipped","player.GetEquippedObject 5",true);
  weaponOut=compile("weapon out","player.IsWeaponOut",true);
  dialogueTarget=compile("dialogue target","GetDialogueTarget",true);
  dialoguePartner=compile("dialogue menu partner","GetMenuTargetRef 1009",true);
  combatDisabled=compile("combat controls disabled","GetPlayerControlsDisabled 0 1 0 0 0 0 0",true);
  playerDead=compile("player dead","player.GetDead",true);
  killCameraQuery=compile("kill camera","IsInKillCam",true);
  firstPersonQuery=compile("native camera mode","IsPC1stPerson",true);
  holster=compile("holster","begin function {}\nplayer.SetWeaponOut 0\nSetFunctionValue 1\nend");
  draw=compile("draw","begin function {}\nplayer.SetWeaponOut 1\nSetFunctionValue 1\nend");
  getWeapons=compile("weapons","player.GetAllItems 40 1",true);getAid=compile("aid","player.GetAllItems 47 1",true);
  getName=compile("name","ref item\nbegin function {item}\nSetFunctionValue (GetName item)\nend");
  getModel=compile("model","ref item\nbegin function {item}\nSetFunctionValue (GetModelPath item)\nend");
  getCount=compile("count","ref item\nbegin function {item}\nSetFunctionValue (player.GetItemCount item)\nend");
  getDimension=compile("dimensions","ref item\nbegin function {item}\nSetFunctionValue (GetObjectDimensions X item + GetObjectDimensions Y item + GetObjectDimensions Z item)\nend");
  insert=compile("insert","string_var n\nbegin function {n}\nSetFunctionValue (InsertNode player 3 \"%z\" n)\nend");
  removeNode=compile("remove node","string_var n\nbegin function {n}\nSetFunctionValue (InsertNode player 2 \"%z\" n)\nend");
  attach=compile("attach","string_var n\nbegin function {n}\nSetFunctionValue (AttachModel player 3 \"%z\" n)\nend");
  detach=compile("detach","string_var n\nbegin function {n}\nSetFunctionValue (AttachModel player 2 \"%z\" n)\nend");
  equip=compile("equip or consume","ref item\nint weapon\nbegin function {item,weapon}\nif player.GetItemCount item > 0\nplayer.EquipItemAlt item 0 1\nif weapon\nplayer.SetWeaponOut 1\nendif\nSetFunctionValue 1\nelse\nSetFunctionValue 0\nendif\nend");
  notice=compile("notice","string_var text\nbegin function {text}\nMessageExAlt 2 \"%z\" text\nSetFunctionValue 1\nend");
  getInventoryRefs=compile("owned item instances","ref item\nbegin function {item}\nSetFunctionValue (player.GetInvRefsForItem item)\nend");
  // Selected refs are already in the player cell. MoveTo queues relocation and
  // can strand their rendered root after a native grab/release. SetPosEx performs
  // one synchronous reference + collision placement before native pickup.
  moveRef=compile("position selected item","ref target\nfloat x\nfloat y\nfloat z\nbegin function {target,x,y,z}\ntarget.SetPosEx x y z\nSetFunctionValue 1\nend");
  takeRef=compile("return selected item","ref target\nbegin function {target}\ntarget.Activate player\nSetFunctionValue 1\nend");
  sprintAP=compile("sprint AP","player.GetAV ActionPoints",true);
  lightOn=fc::nvse::loadFunction(*scripts, "Player/RestoreLight");
  sprintDrain=fc::nvse::loadFunction(*scripts, "Player/DrainSprintAP");
  sprintAllowed=compile("sprint allowed","(player.GetDead == 0) && (player.IsSneaking == 0) && (player.IsSwimming == 0) && (player.GetAV InventoryWeight <= player.GetAV CarryWeight) && (GetPlayerControlsDisabled == 0)",true);
  ready=getPlayer&&getMenu&&getWeapons&&getAid&&getName&&getModel&&getCount&&insert&&removeNode&&attach&&detach&&equip&&notice&&rootScale&&getEquipped&&weaponOut&&holster&&draw&&getInventoryRefs&&inventoryRef&&moveRef&&takeRef;
  log("script initialization %s",ready?"READY":"FAILED; controls left unchanged");return ready;
 }
 bool pos(const char* name,int mode,Vec& out){
  static std::unordered_map<std::string,Script*> cache;
  std::string key=std::string(name)+std::to_string(mode);auto& query=cache[key];
  if(!query){std::string text="player.GetNifBlockTranslation \""+std::string(name)+"\" 1 "+std::to_string(mode);query=compile(key.c_str(),text.c_str(),true);}
  Element e;if(!call(query,e)||e.type!=4||!e.value.array){log("position %s mode%d failed type=%u array=%p",name,mode,e.type,e.value.array);release(e);return false;}
  auto count=arrays->size(e.value.array);if(count!=3){log("position %s mode%d size=%u",name,mode,count);release(e);return false;}
  Element coords[3];bool ok=arrays->elements(e.value.array,coords,nullptr);if(ok&&coords[0].type==1&&coords[1].type==1&&coords[2].type==1)out={float(coords[0].value.number),float(coords[1].value.number),float(coords[2].value.number)};else ok=false;
  for(auto& c:coords)release(c);release(e);return ok;
 }

 Form* playerForm(){Element e;Form* p=nullptr;if(call(getPlayer,e)&&e.type==2)p=e.value.form;release(e);return p;}
 // WIP native-worldmag bridge: existing scripts only, no per-drop compilation.
 void* __cdecl worldmagPlayer() noexcept {
  if(!ready||!loaded||!gestureGameplayAllowed||!enabled||selectorOpen||heightMenu||weaponTransition||controlsEditor||selectedRef)return nullptr;
  auto vehicleModule=GetModuleHandleW(L"FilthyCasualVehicles.dll");
  auto vehicleActive=vehicleModule?reinterpret_cast<bool(__cdecl*)()>(GetProcAddress(vehicleModule,"FC_VehicleActive")):nullptr;
  if(vehicleModule&&(!vehicleActive||vehicleActive()))return nullptr;
  {std::lock_guard<std::mutex> lock(inputMutex);if(GetTickCount64()-input.time>=500||!input.h[0].active||!input.h[1].active)return nullptr;}
  Element menu;bool ok=call(getMenu,menu)&&menu.type==1&&menu.value.number==0;release(menu);
  return ok?playerForm():nullptr;
 }
 bool __cdecl worldmagInventory(void* player,void* ammo,int* out) noexcept {
  if(!ready||!getCount||!player||!ammo||!out)return false;
  Element e;bool ok=call(getCount,e,static_cast<Form*>(ammo));
  double count=ok&&e.type==1?e.value.number:-1;release(e);
  if(!std::isfinite(count)||count<0||count>INT_MAX||std::floor(count)!=count)return false;
  *out=int(count);return true;
 }


 // First physical-magazine slice. World references retain engine collision;
 // only their visible geometry is replaced by a separately retained clone.
 struct PhysicalMag {unsigned refID=0,weaponID=0,ammoID=0;bool wasHeld=false,wasNearPouch=false,identityDirty=false;int lastHeldHand=-1;ULONGLONG identityAt=0,contactAt=0;float contactDistance=-1;int contactHand=-1;unsigned contactWeapon=0;void* visual=nullptr;scene::Transform pose{};Vec center{},velocity{};ULONGLONG born=0;bool attached=false;std::string cache;scene::Transform relative{};bool restoring=false;bool fullModel=false,modelAssigned=false;bool metadataDirty=false;ULONGLONG retryAt=0,metadataAt=0;};
 std::vector<PhysicalMag> physicalMags;
 bool physicalPartsEnabled=false;
 Script *partSet=nullptr,*partGet=nullptr,*partKeys=nullptr,*partErase=nullptr,*partModel=nullptr,*partIdentitySet=nullptr,*partIdentityGet=nullptr,*removeEmptyPart=nullptr;ULONGLONG partDiscoverAt=0;
 bool initPartMetadata(){
  if(partSet&&partGet&&partKeys&&partErase&&partModel&&partIdentitySet&&partIdentityGet&&removeEmptyPart)return true;
  static bool attempted=false;if(attempted)return false;attempted=true;
  partSet=compile("save part appearance","ref target\nstring_var value\nbegin function {target,value}\ntarget.RefMapSetStr \"_FC_PhysicalReload_v1\" \"$value\"\nSetFunctionValue 1\nend");
  partGet=compile("read part appearance","ref target\nbegin function {target}\nSetFunctionValue (target.RefMapGetStr \"_FC_PhysicalReload_v1\")\nend");
  partKeys=compile("part appearance keys","RefMapKeys \"_FC_PhysicalReload_v1\"",true);
  partErase=compile("forget removed part appearance","ref target\nbegin function {target}\ntarget.RefMapErase \"_FC_PhysicalReload_v1\"\ntarget.RefMapErase \"_FC_PhysicalReloadWeapon\"\ntarget.RefMapErase \"_FC_PhysicalReloadAmmo\"\nSetFunctionValue 1\nend");
  partModel=compile("physical part model","ref target\nstring_var value\nbegin function {target,value}\ntarget.SetRefrModelPath \"$value\"\nSetFunctionValue 1\nend");
  partIdentitySet=compile("part source identity","ref target\nstring_var key\nref source\nbegin function {target,key,source}\ntarget.RefMapSetRef \"$key\" source\nSetFunctionValue 1\nend");
  partIdentityGet=compile("read part source identity","ref target\nstring_var key\nbegin function {target,key}\nSetFunctionValue (target.RefMapGetRef \"$key\")\nend");
  removeEmptyPart=compile("consume original empty carrier","ref base\nbegin function {base}\nplayer.RemoveItem base 1 1\nSetFunctionValue 1\nend");
  return partSet&&partGet&&partKeys&&partErase&&partModel&&partIdentitySet&&partIdentityGet&&removeEmptyPart;
 }
 unsigned partIdentity(Form* ref,const char* key){Element e;unsigned id=0;if(call(partIdentityGet,e,ref,key)&&e.type==2&&e.value.form)id=e.value.form->id;release(e);return id;}
 void rememberIdentity(PhysicalMag& m,Form* ref){
  m.identityDirty=false;m.identityAt=GetTickCount64();
  const char* keys[]={"_FC_PhysicalReloadWeapon","_FC_PhysicalReloadAmmo"};unsigned ids[]={m.weaponID,m.ammoID};
  for(int i=0;i<2;i++)if(auto source=static_cast<Form*>(physical_reload_adapter::lookup(ids[i]))){Element e;call(partIdentitySet,e,ref,keys[i],source);release(e);if(partIdentity(ref,keys[i])!=ids[i]){m.identityDirty=true;log("PHYSICAL MAG identity write failed ref=%08X key=%s",m.refID,keys[i]);}}
 }
 bool rememberPart(PhysicalMag& m){
  m.metadataDirty=true;m.metadataAt=GetTickCount64();
  auto ref=static_cast<Form*>(physical_reload_adapter::lookup(m.refID));if(!ref||!initPartMetadata()||m.cache.size()<68)return false;
  std::ostringstream text;text<<std::setprecision(9)<<(m.fullModel?"2 ":"1 ")<<((m.attached||m.restoring)?2:1)<<" "<<m.cache.substr(m.cache.size()-68,64);
  const auto& transform=(m.attached||m.restoring)?m.relative:m.pose;const auto values=reinterpret_cast<const float*>(&transform);
  for(unsigned i=0;i<13;i++)text<<" "<<values[i];
  text<<" "<<m.center.x<<" "<<m.center.y<<" "<<m.center.z<<" "<<m.velocity.x<<" "<<m.velocity.y<<" "<<m.velocity.z;
  const auto value=text.str();Element result;bool written=call(partSet,result,ref,value.c_str());release(result);bool ok=written&&string(partGet,ref)==value;if(!ok)log("PHYSICAL MAG metadata roundtrip failed ref=%08X stored=%s",m.refID,string(partGet,ref).substr(0,100).c_str());m.metadataDirty=!ok;return ok;
 }
 Form* emptyReloadPart(){
  static Script* query=nullptr;static bool tried=false;
  if(!tried){tried=true;query=compile("empty reload part","GetFormFromMod \"Filthy Casual Reload Parts.esp\" \"000800\"",true);}
  Element e;Form* base=nullptr;if(query&&call(query,e)&&e.type==2)base=e.value.form;release(e);return base&&base->type==31?base:nullptr;
 }
 bool __cdecl commitPhysicalMag(const physical_reload_adapter::Drop& d);
 int __cdecl recoverPhysicalMag(void* player,int hand);
 const char* installPhysicalParts(){
  // This exact upstream build owns world magazines. Never install competing
  // release/recovery hooks, including under the old private test override.
  physicalPartsEnabled=false;
  return "disabled: upstream TTR owns dropped ammunition";
 }

 void discoverParts(){
  if(!ready||!loaded||!initPartMetadata())return;Element e;
  if(call(partKeys,e)&&e.type==4&&e.value.array){auto n=arrays->size(e.value.array);if(n<=8192){std::vector<Element> entries(n);if(arrays->elements(e.value.array,entries.data(),nullptr))for(auto& key:entries){
   auto ref=key.type==2?key.value.form:nullptr;if(!ref||(ref->flags&0x20))continue;
   if(std::any_of(physicalMags.begin(),physicalMags.end(),[&](const PhysicalMag& m){return m.refID==ref->id;}))continue;
   std::istringstream text(string(partGet,ref));unsigned version=0,mode=0;std::string digest;PhysicalMag m{};text>>version>>mode>>digest;
   if((version!=1&&version!=2)||(mode!=1&&mode!=2)||digest.size()!=64||digest.find_first_not_of("0123456789abcdef")!=std::string::npos)continue;
   auto& transform=mode==1?m.pose:m.relative;auto values=reinterpret_cast<float*>(&transform);bool valid=true;
   for(unsigned i=0;i<13;i++){if(!(text>>values[i])||!std::isfinite(values[i]))valid=false;}
   if(!(text>>m.center.x>>m.center.y>>m.center.z>>m.velocity.x>>m.velocity.y>>m.velocity.z))valid=false;
   if(!valid||!std::isfinite(m.center.x)||!std::isfinite(m.center.y)||!std::isfinite(m.center.z)||transform.scale<=0||transform.scale>100)continue;
   m.fullModel=version==2;m.refID=ref->id;m.cache="FilthyCasualLocal\\Reload\\"+digest+".nif";m.restoring=mode==2;m.born=GetTickCount64();
   m.weaponID=partIdentity(ref,"_FC_PhysicalReloadWeapon");m.ammoID=partIdentity(ref,"_FC_PhysicalReloadAmmo");
   physicalMags.push_back(std::move(m));log("PHYSICAL MAG discovered appearance ref=%08X mode=%u",ref->id,mode);
  }for(auto& key:entries)release(key);}}
  release(e);
 }

 std::vector<std::string> partSourceModels(Form* weapon,Form* ammo=nullptr){
  std::vector<std::string> paths;
  auto add=[&](Form* form){if(!form)return;auto path=string(getModel,form);if(path.empty())return;path="meshes\\"+path;if(std::find(paths.begin(),paths.end(),path)==paths.end())paths.push_back(path);};
  if(weapon&&weapon->type==40){for(unsigned offset=0x250;offset<0x270;offset+=4)add(scene::field<Form*>(weapon,offset));add(weapon);}add(ammo);return paths;
 }
 bool __cdecl commitPhysicalMag(const physical_reload_adapter::Drop& d){
  if(!ready||!loaded||!initPartMetadata())return false;
  auto carrier=physical_reload_adapter::loadVisual("meshes\\FilthyCasual\\reload-carrier.nif");if(!carrier)return false;scene::retain(carrier);
  auto model=physical_reload_adapter::clone(d.visual);if(!model){scene::release(carrier);return false;}scene::retain(model);
  Element sourceWeapon;Form* weapon=nullptr;if(call(getEquipped,sourceWeapon)&&sourceWeapon.type==2)weapon=sourceWeapon.value.form;
  auto sources=partSourceModels(weapon,static_cast<Form*>(d.ammo));release(sourceWeapon);
  if(!physical_reload_adapter::restoreAuthoredUV(model,sources)){log("PHYSICAL MAG authored UV restore failed (%u paths); left native",unsigned(sources.size()));for(auto& path:sources)log("PHYSICAL MAG UV source %s",path.c_str());scene::release(model);scene::release(carrier);return false;}
  physical_reload_adapter::stripControllers(model);
  scene::Transform identity{};identity.r[0][0]=identity.r[1][1]=identity.r[2][2]=1;identity.scale=1;
  scene::local(carrier)=identity;scene::local(model)=identity;scene::local(model).scale=d.pose.scale;scene::update(model);
  std::vector<scene::Box> meshes;scene::boxes(model,meshes);if(meshes.empty()){scene::release(model);scene::release(carrier);return false;}
  Vec extent;auto center=scene::boxCenter(meshes,&extent);auto half=extent*.5f;half={std::max(.3f,half.x),std::max(.3f,half.y),std::max(.3f,half.z)};
  auto target=d.pose;target.p=d.pose.p+scene::rotate(d.pose,center);target.scale=1;
  if(auto visual=scene::find(carrier,"FCReloadCarrierVisual")){scene::retain(visual);reinterpret_cast<void(__thiscall*)(void*,void*)>(scene::field<void**>(carrier,0)[0xE8/4])(carrier,visual);scene::release(visual);}
  scene::moveParent(model,carrier);scene::local(model).p=center*-1.f;scene::field<unsigned>(model,0x30)&=~1u;scene::update(carrier);
  const auto folder=gameDirectory+"Data\\meshes\\FilthyCasualLocal\\Reload";
  CreateDirectoryA((gameDirectory+"Data\\meshes").c_str(),nullptr);CreateDirectoryA((gameDirectory+"Data\\meshes\\FilthyCasualLocal").c_str(),nullptr);CreateDirectoryA(folder.c_str(),nullptr);
  const auto temporary=folder+"\\pending-"+std::to_string(GetCurrentProcessId())+".nif";
  bool exported=physical_reload_adapter::exportVisual(carrier,temporary.c_str());scene::release(model);scene::release(carrier);
  if(exported)exported=physical_reload_adapter::fitCarrierFile(temporary.c_str(),half);
  auto digest=exported?physical_reload_adapter::hashFile(temporary.c_str()):std::string{};
  if(digest.empty()){log("PHYSICAL MAG fitted model cache failed; transaction left native");return false;}
  const auto cache="FilthyCasualLocal\\Reload\\"+digest+".nif";const auto finalPath=folder+"\\"+digest+".nif";
  if(GetFileAttributesA(finalPath.c_str())!=INVALID_FILE_ATTRIBUTES)DeleteFileA(temporary.c_str());else if(!MoveFileA(temporary.c_str(),finalPath.c_str()))return false;
  auto before=d.ammo?number(getCount,static_cast<Form*>(d.ammo)):0;
  if(before<d.rounds)return false;
  Form* ref=nullptr;
  if(d.rounds){ref=static_cast<Form*>(physical_reload_adapter::dropAmmo(d.player,d.ammo,unsigned(d.rounds)));}
  else {
   if(auto base=emptyReloadPart())ref=static_cast<Form*>(physical_reload_adapter::placeEmpty(d.player,base));
  }
  if(!ref){
   const auto after=d.ammo?number(getCount,static_cast<Form*>(d.ammo)):0;
   if(after!=before){log("PHYSICAL MAG unresolved reference after debit %.0f->%.0f; native second debit suppressed",before,after);return true;}
   return false;
  }
  // Native RemoveItem owns the inventory transaction and creates the reference.
  // Position after 3D creation; MoveTo queues a relocation that can overwrite SetPos.
  PhysicalMag part{};part.refID=ref->id;part.pose=target;part.velocity=d.velocity;part.born=GetTickCount64();part.cache=cache;part.fullModel=true;physicalMags.push_back(std::move(part));
  auto& created=physicalMags.back();Element equipped;if(call(getEquipped,equipped)&&equipped.type==2&&equipped.value.form)created.weaponID=equipped.value.form->id;release(equipped);created.ammoID=d.ammo?static_cast<Form*>(d.ammo)->id:0;rememberIdentity(created,ref);
  rememberPart(created);
  physical_reload_adapter::lastRefID=ref->id;
  log("PHYSICAL MAG ref=%08X ammo=%08X rounds=%d inventory=%.0f->%.0f",ref->id,d.ammo?static_cast<Form*>(d.ammo)->id:0,d.rounds,before,d.ammo?number(getCount,static_cast<Form*>(d.ammo)):0);
  return true;
 }

 bool takePhysicalPart(PhysicalMag& m,Form* ref,void* player,bool held){
  auto base=scene::field<Form*>(ref,0x20);bool empty=base&&base==emptyReloadPart();
  if(!base||(!empty&&(base->type!=41||base->id!=m.ammoID)))return false;
  int count=empty?1:int(physical_reload_adapter::stackCount(ref));if(count<1||count>10000)return false;
  double before=number(getCount,base);unsigned id=m.refID;
  // Retire all associations while the exact reference is still alive. Native
  // Activate may destroy it synchronously. A failed transfer restores metadata
  // only if this same live reference remains; never dereference a freed ref.
  auto restoreMetadata=[&](){auto live=static_cast<Form*>(physical_reload_adapter::lookup(id));if(live==ref&&!(live->flags&0x20)){rememberPart(m);rememberIdentity(m,live);}};
  number(partErase,ref);
  if(!string(partGet,ref).empty()||partIdentity(ref,"_FC_PhysicalReloadWeapon")||partIdentity(ref,"_FC_PhysicalReloadAmmo")){restoreMetadata();return false;}
  if(held){if(!build_adapter::stowSelectedGrab(player,ref)){restoreMetadata();return false;}}
  else reinterpret_cast<void(__thiscall*)(void*,void*,int,int,int)>(0x573170)(ref,player,0,0,1);
  double after=number(getCount,base);
  if(after-before!=count){log("PHYSICAL MAG recovery inventory mismatch ref=%08X %.0f->%.0f expected=%d",id,before,after,count);restoreMetadata();return false;}
  if(empty){number(removeEmptyPart,base);if(number(getCount,base)!=before){log("PHYSICAL MAG empty carrier removal failed ref=%08X",id);return false;}}
  m.wasHeld=false;m.wasNearPouch=false;m.refID=0;
  log("PHYSICAL MAG recovered ref=%08X rounds=%d",id,empty?0:count);return true;
 }
 int __cdecl recoverPhysicalMag(void* player,int hand){
  if(!ready||!loaded||!gestureGameplayAllowed||!player||!initPartMetadata())return -1;auto b=build_adapter::heldModule();
  physical_reload_adapter::recoverStage=4;
  auto first=b[0x1538805]?static_cast<Form*>(*reinterpret_cast<void**>(b+0x15395E0)):nullptr;
  auto second=b[0x15388D4]?static_cast<Form*>(*reinterpret_cast<void**>(b+0x15388DC)):nullptr;
  physical_reload_adapter::recoverStage=5;
  auto ref=first?first:second;if(!ref||(first&&second&&first!=second))return -1;
  physical_reload_adapter::recoverStage=6;
  int heldHand=*reinterpret_cast<int*>(b+(first?0x1035E60:0x15388D8));if(heldHand!=hand)return -1;
  physical_reload_adapter::recoverStage=7;
  auto it=std::find_if(physicalMags.begin(),physicalMags.end(),[&](const PhysicalMag& m){return m.refID==ref->id;});
  if(it==physicalMags.end()||!it->weaponID||!it->ammoID)return -1;
  physical_reload_adapter::recoverStage=8;
  Element equipped;unsigned weapon=0;if(call(getEquipped,equipped)&&equipped.type==2&&equipped.value.form)weapon=equipped.value.form->id;release(equipped);
  auto ammo=static_cast<Form*>(reinterpret_cast<void*(__cdecl*)(void*)>(b+0x37F910)(player));
  physical_reload_adapter::recoverStage=9;
  if(weapon!=it->weaponID||!ammo||ammo->id!=it->ammoID)return -1;
  physical_reload_adapter::recoverStage=10;
  auto base=scene::field<Form*>(ref,0x20);bool empty=base==emptyReloadPart();if(!empty&&base!=ammo)return -1;
  physical_reload_adapter::recoverStage=11;
  int count=empty?0:int(physical_reload_adapter::stackCount(ref));int capacity=reinterpret_cast<int(__cdecl*)()>(b+0x3852E0)();
  if(count<0||count>capacity||capacity<1)return -1;
  // Actual native grab ownership is the pickup/catch event. Hand over now,
  // not only after approaching the well with an arbitrary physics grab pose.
  // The native initializer computes this weapon's held offset and orientation.
  physical_reload_adapter::recoverStage=16;
  if(!takePhysicalPart(*it,ref,player,true))return -1;
  return count;
 }
 bool nearPhysicalPouch(void* ref){
  auto b=physical_reload_adapter::module;auto reloadSettings=build_adapter::liveReloadSettings();if(!b||!reloadSettings||reloadSettings[0x4a0]!=1)return false;
  auto pouch=*reinterpret_cast<void**>(b+0x19F6864);auto root=scene::root(ref);
  if(!build_adapter::readPouchNode(pouch).readable&&!*reinterpret_cast<int*>(b+0x19F6948))pouch=*reinterpret_cast<void**>(b+0x19F6850);
  if(!root||!build_adapter::readPouchNode(pouch).readable)return false;
  auto pose=scene::world(pouch);auto center=pose.p+scene::rotate(pose,*reinterpret_cast<Vec*>(b+0x19F6854))*pose.scale;
  auto bound=scene::field<float*>(root,0x20);if(!bound)return false;
  float threshold=*reinterpret_cast<float*>(reloadSettings+0x4d0)**reinterpret_cast<float*>(reloadSettings+0x28);
  float distance=scene::distance(center,{bound[0],bound[1],bound[2]});
  return std::isfinite(threshold)&&threshold>0&&std::isfinite(distance)&&distance<threshold;
 }
 void tickPhysicalMags(){
  if(!physicalPartsEnabled)return;
  if(!build_adapter::heldModule())return;
  if(GetTickCount64()-partDiscoverAt>3000){partDiscoverAt=GetTickCount64();discoverParts();}
  for(auto it=physicalMags.begin();it!=physicalMags.end();){auto ref=static_cast<Form*>(physical_reload_adapter::lookup(it->refID));
   if(!it->refID||(ref&&(ref->flags&0x20))){if(partErase&&ref)number(partErase,ref);if(it->visual)scene::release(it->visual);it=physicalMags.erase(it);}else ++it;}
  Input partInput;{std::lock_guard<std::mutex> lock(inputMutex);partInput=input;}
  for(auto& m:physicalMags){if(!m.refID)continue;
   auto ref=static_cast<Form*>(physical_reload_adapter::lookup(m.refID));if(!ref)continue;
   if(m.metadataDirty&&GetTickCount64()-m.metadataAt>1000)rememberPart(m);
   if(m.identityDirty&&GetTickCount64()-m.identityAt>1000)rememberIdentity(m,ref);
   bool held=physical_reload_adapter::isHeld(ref);bool nearPouch=nearPhysicalPouch(ref);
   m.contactAt=0;m.contactDistance=-1;m.contactHand=-1;m.contactWeapon=0;
   if(held&&gestureGameplayAllowed&&physical_reload_adapter::module){
    auto root=scene::root(ref);scene::Transform rest{};auto reloadSettings=build_adapter::liveReloadSettings();
    if(reloadSettings&&reloadSettings[0x4a0]==1&&build_adapter::readPouchNode(root).readable&&physical_reload_adapter::installedRest(rest)){
     auto bound=scene::field<float*>(root,0x20);float units=*reinterpret_cast<float*>(reloadSettings+0x28);
     if(bound&&std::isfinite(units)&&units>1){m.contactDistance=scene::distance({bound[0],bound[1],bound[2]},rest.p)/units;if(std::isfinite(m.contactDistance)&&m.contactDistance<=.12f){
      auto b=physical_reload_adapter::module;m.contactHand=(b[0x1538805]&&*reinterpret_cast<void**>(b+0x15395E0)==ref)?*reinterpret_cast<int*>(b+0x1035E60):build_adapter::grabbedHand();
      Element equipped;if(call(getEquipped,equipped)&&equipped.type==2&&equipped.value.form)m.contactWeapon=equipped.value.form->id;release(equipped);m.contactAt=GetTickCount64();
     }}
    }
   }
   if(m.wasHeld&&!held&&m.wasNearPouch&&nearPouch&&m.weaponID&&m.ammoID&&m.lastHeldHand>=0&&partInput.h[m.lastHeldHand].active&&partInput.h[m.lastHeldHand].grip<.2f&&GetTickCount64()-partInput.time<100&&gestureGameplayAllowed){if(takePhysicalPart(m,ref,playerForm(),false)){physical_reload_adapter::recoveredPouch++;continue;}}
   if(held){auto b=physical_reload_adapter::module;m.lastHeldHand=(b&&b[0x1538805]&&*reinterpret_cast<void**>(b+0x15395E0)==ref)?*reinterpret_cast<int*>(b+0x1035E60):build_adapter::grabbedHand();}
   m.wasHeld=held;m.wasNearPouch=held&&nearPouch;
   auto root=scene::root(ref);
   if(m.fullModel){
    // Once native pickup owns a pending ref, its current pose wins. Never apply
    // delayed release placement or momentum to an already-held world object.
    if(!m.attached&&!m.restoring&&physical_reload_adapter::isHeld(ref)){
     m.restoring=true;m.relative=root?scene::local(root):m.pose;rememberPart(m);
    }
    if(m.attached&&root!=m.visual){scene::release(m.visual);m.visual=nullptr;m.attached=false;m.restoring=true;m.modelAssigned=false;m.retryAt=0;}
    if(m.restoring&&!root)continue; // JIP clears per-reference paths on destruction; wait for the returning object.
    if(!m.modelAssigned){
     if(physical_reload_adapter::isHeld(ref)||GetTickCount64()<m.retryAt)continue;m.retryAt=GetTickCount64()+3000;
     if(GetFileAttributesA((gameDirectory+"Data\\meshes\\"+m.cache).c_str())==INVALID_FILE_ATTRIBUTES)continue;
     auto candidateCache=m.cache;if(!physical_reload_adapter::worldCache(gameDirectory+"Data\\meshes\\",candidateCache,partSourceModels(static_cast<Form*>(physical_reload_adapter::lookup(m.weaponID)),static_cast<Form*>(physical_reload_adapter::lookup(m.ammoID)))))continue;
     auto checked=physical_reload_adapter::loadVisual(("meshes\\"+candidateCache).c_str());if(!checked)continue;scene::retain(checked);bool fitted=!scene::find(checked,"FCReloadCarrierVisual")&&strcmp(scene::name(checked),"FCReloadPart")==0;scene::release(checked);if(!fitted)continue;
     if(number(partModel,ref,candidateCache.c_str())!=1)continue;
     if(candidateCache!=m.cache){m.cache=candidateCache;rememberPart(m);log("PHYSICAL MAG migrated world shader ref=%08X",m.refID);}
     m.modelAssigned=true;m.retryAt=0;if(root)physical_reload_adapter::rebuildReference(ref);continue;
    }
    if(!root||m.attached)continue;
    if(strcmp(scene::name(root),"FCReloadPart")||scene::find(root,"FCReloadCarrierVisual")){if(GetTickCount64()>m.retryAt){m.retryAt=GetTickCount64()+3000;physical_reload_adapter::rebuildReference(ref);}continue;}
    if(!m.restoring){
     static Script* place=nullptr;if(!place)place=compile("place fitted part","ref target\nfloat x\nfloat y\nfloat z\nfloat ax\nfloat ay\nfloat az\nbegin function {target,x,y,z,ax,ay,az}\ntarget.SetAngleEx ax ay az 1\ntarget.SetPosEx x y z\nSetFunctionValue 1\nend");
     constexpr float deg=57.295779513f;const auto& r=m.pose.r;
     auto ax=std::atan2(r[2][1],r[2][2])*deg,ay=-std::asin(std::clamp(r[2][0],-1.f,1.f))*deg,az=std::atan2(r[1][0],r[0][0])*deg;
     if(!place||number(place,ref,bits(m.pose.p.x),bits(m.pose.p.y),bits(m.pose.p.z),bits(ax),bits(ay),bits(az))!=1)continue;
     physical_reload_adapter::momentum(root,m.velocity);
    }
    m.visual=root;scene::retain(root);m.attached=true;m.relative=scene::local(root);rememberPart(m);log("PHYSICAL MAG fitted model ready ref=%08X restored=%d",m.refID,m.restoring?1:0);continue;
   }
   if(m.attached&&(!root||scene::parent(m.visual)!=root)){scene::release(m.visual);m.visual=nullptr;m.attached=false;m.restoring=true;}
   if(!root||m.attached)continue;
   if(!reinterpret_cast<void*(__thiscall*)(void*)>(scene::field<void**>(root,0)[3])(root))continue;
   if(!m.visual&&!m.cache.empty()){if(GetTickCount64()<m.retryAt)continue;m.retryAt=GetTickCount64()+3000;m.visual=physical_reload_adapter::loadVisual(("meshes\\"+m.cache).c_str());if(m.visual)scene::retain(m.visual);}
   if(m.restoring){
    if(!m.visual)continue;
    std::vector<scene::Box> donor;scene::boxes(root,donor);if(donor.empty())continue;
    scene::moveParent(m.visual,root);scene::local(m.visual)=m.relative;scene::field<unsigned>(m.visual,0x30)&=~1u;scene::update(m.visual);
    for(auto& box:donor)scene::field<unsigned>(box.node,0x30)|=1;scene::update(root);
    m.attached=true;log("PHYSICAL MAG restored appearance ref=%08X",m.refID);continue;
   }
   if(!m.visual)continue;
   if(!reinterpret_cast<void*(__thiscall*)(void*)>(scene::field<void**>(root,0)[3])(root)){log("PHYSICAL MAG visual refused non-NiNode root %08X",m.refID);continue;}
   static bool triedPosition=false;static Script* positionPart=nullptr;
   if(!triedPosition){triedPosition=true;positionPart=compile("position physical part","ref target\nfloat x\nfloat y\nfloat z\nbegin function {target,x,y,z}\ntarget.SetPosEx x y z\nSetFunctionValue 1\nend");}
   if(!positionPart)continue;
   const auto target=m.pose.p+scene::rotate(m.pose,m.center)*m.pose.scale;
   if(number(positionPart,ref,bits(target.x),bits(target.y),bits(target.z))!=1)continue;
   // Preserve donor root and its native Havok collision. Cull only donor meshes.
   std::vector<scene::Box> donor;scene::boxes(root,donor);if(donor.empty())continue;
   auto donorCenter=scene::boxCenter(donor);auto wanted=m.pose;
   wanted.p=donorCenter-scene::rotate(wanted,m.center)*wanted.scale;
   scene::moveParent(m.visual,root);scene::field<unsigned>(m.visual,0x30)&=~1u;
   if(!scene::setWorld(m.visual,wanted)){log("PHYSICAL MAG placement failed; donor retained %08X",m.refID);continue;}
   for(auto& mesh:donor)scene::field<unsigned>(mesh.node,0x30)|=1;
   scene::update(root);
   log("PHYSICAL MAG attach delay=%llu ms rootDelta=%.3f",GetTickCount64()-m.born,scene::distance(donorCenter,m.pose.p));
   physical_reload_adapter::momentum(root,m.velocity);
   m.relative=scene::local(m.visual);m.attached=true;rememberPart(m);log("PHYSICAL MAG visual attached ref=%08X root=%p clone=%p",m.refID,root,m.visual);
  }
 }
 void clearPhysicalMags(){for(auto& m:physicalMags)if(m.visual)scene::release(m.visual);physicalMags.clear();partDiscoverAt=0;physical_reload_adapter::lastRefID=0;}



 struct FragileBottle {unsigned id=0,base=0;bottle_impact::Sample previous{};};
 std::vector<FragileBottle> fragileBottles;
 unsigned bottleBreaks=0;bool bottleTrace=false;Vec lastGlassPoint{};
 bool emptyBottleBase(unsigned id){return id==0x5B6CE||id==0x5B6CD||id==0x22103||id==0x103b1b;}
 void trackBottle(Form* ref){
  if(!ref)return;auto base=scene::field<Form*>(ref,0x20);if(!base||!emptyBottleBase(base->id))return;
  for(auto& b:fragileBottles)if(b.id==ref->id)return;
  if(fragileBottles.size()>=64)fragileBottles.erase(fragileBottles.begin());
  fragileBottles.push_back({ref->id,base->id,{}});
 }
 void clearBottleImpacts(){glass_decal_adapter::clearProbes();fragileBottles.clear();}
 bool glassAt(Vec point,Vec normal,void* surface,Form* ref,float impact=5){
  static Script* textures[3]{};static Script* destroy=nullptr;static Script* sound=nullptr;
  auto style=glass_variation::choose(ref->id^(bottleBreaks*2654435761u)^unsigned(GetTickCount64()),impact);
  auto& texture=textures[style.pattern];
  if(!texture){char command[160];sprintf_s(command,"GetFormFromMod \"Filthy Casual Glass.esp\" \"%06X\"",0x800+style.pattern);texture=compile("glass decal texture",command,true);}
  if(!destroy)destroy=compile("broken empty bottle","ref item\nbegin function {item}\nitem.Disable\nitem.MarkForDelete\nSetFunctionValue 1\nend");
  if(!sound)sound=compile("glass impact sound","PlaySound FXMeleeGenericGlass",true);
  if(!texture||!destroy)return false;
  Element value;Form* tx=nullptr;if(call(texture,value)&&value.type==2)tx=value.value.form;release(value);
  auto node=glass_decal_adapter::raySurface(point,normal,scene::root(ref));if(!node)node=glass_decal_adapter::parentNode(surface);auto before=glass_decal_adapter::attempts();

  auto cell=glass_decal_adapter::impactCell(point);bool ok=glass_decal_adapter::add(point,normal,node,cell,tx,scene::root(ref),style.width,style.height);
  log("GLASS VARIATION pattern=%u size=%.2fx%.2f impact=%.2f",style.pattern,style.width,style.height,impact);
  log("BOTTLE DECAL accepted=%d tx=%p surface=%p node=%p name=%s active=%u attempts=%u/%u normal=%.3f %.3f %.3f pos=%.2f %.2f %.2f",ok,tx,surface,node,node?scene::name(node):"",glass_decal_adapter::count(),before,glass_decal_adapter::attempts(),normal.x,normal.y,normal.z,point.x,point.y,point.z);
  if(!ok)return false;
  bool audioOK=glass_decal_adapter::smashSound(point);log("BOTTLE EFFECTS puff=%d sound=%d stage=%d bytes=%08X/%08X/%08X",glass_decal_adapter::puff(point,normal,cell),audioOK,glass_decal_adapter::soundStage,scene::field<unsigned>((void*)0xAE5870,0),scene::field<unsigned>((void*)0xAD8B60,0),scene::field<unsigned>((void*)0xAD8830,0));Element audio;if(!audioOK&&sound){call(sound,audio);release(audio);}
  if(number(destroy,ref)!=1)return false;lastGlassPoint=point;++bottleBreaks;return true;
 }
 void tickBottleImpacts(bool usable){
  if(!feature(fc_features::Glass)){clearBottleImpacts();return;}
  glass_decal_adapter::inspect([](const char* fmt,auto... args){log(fmt,args...);});
  auto now=GetTickCount64();
  if(!usable){for(auto& b:fragileBottles)b.previous={};return;}
  auto held=static_cast<Form*>(build_adapter::grabbedItem());if(held)trackBottle(held);
  for(auto i=fragileBottles.begin();i!=fragileBottles.end();){
   // A spawned replacement is being moved into the native grip, not thrown.
   // Ignore spawn/teleport contacts until that handoff completes; otherwise a
   // nearby wall can shatter it during the 3D-loading delay.
   if(weaponTransition&&pendingAid&&aidDropPending&&selectedRef&&selectedRef->id==i->id){i->previous={};++i;continue;}
   auto ref=static_cast<Form*>(physical_reload_adapter::lookup(i->id));auto base=ref?scene::field<Form*>(ref,0x20):nullptr;
   if(!ref||!base||base->id!=i->base||(ref->flags&0x820)||!scene::root(ref)){i=fragileBottles.erase(i);continue;}
   void* surface=nullptr;auto sample=bottle_impact_adapter::sample(scene::root(ref),held==ref,now*.001,&surface);auto change=sample.velocity-i->previous.velocity;auto speed=bottle_impact::length(change);Vec normal=speed>.001f?Vec{change.x/speed,change.y/speed,change.z/speed}:Vec{0,0,1};auto verdict=bottle_impact::judge(i->previous,sample);if(bottleTrace)log("BOTTLE SAMPLE ref=%08X valid=%d held=%d contact=%d velocity=%.3f %.3f %.3f surface=%.3f %.3f %.3f closing=%.3f delta=%.3f",i->id,sample.valid,sample.held,sample.contact,sample.velocity.x,sample.velocity.y,sample.velocity.z,sample.surfaceVelocity.x,sample.surfaceVelocity.y,sample.surfaceVelocity.z,verdict.closingSpeed,verdict.velocityChange);i->previous=sample;
   if(bottleTrace){auto root=scene::root(ref);auto b=bottle_impact_adapter::body(root);auto wp=scene::world(root).p;auto rp=scene::field<Vec>(ref,0x30);Vec hp{};unsigned mt=0;if(b){hp=scene::field<Vec>(b,0x120);mt=scene::field<unsigned char>(b,0xE8);}log("BOTTLE POSE ref=%08X held=%d root=%.3f %.3f %.3f refpos=%.3f %.3f %.3f body=%p hk=%.3f %.3f %.3f motion=%u nodeflags=%08X collision=%04X local=%.3f %.3f %.3f",i->id,sample.held,wp.x,wp.y,wp.z,rp.x,rp.y,rp.z,b,hp.x,hp.y,hp.z,mt,scene::field<unsigned>(root,0x30),scene::field<void*>(root,0x1C)?scene::field<unsigned short>(scene::field<void*>(root,0x1C),0xC):0,scene::local(root).p.x,scene::local(root).p.y,scene::local(root).p.z);}
   if(verdict.smash){log("BOTTLE IMPACT ref=%08X contact=%d closing=%.3f deltaV=%.3f",i->id,sample.contact,verdict.closingSpeed,verdict.velocityChange);if(glassAt(scene::world(scene::root(ref)).p,normal,surface,ref,verdict.closingSpeed)){i=fragileBottles.erase(i);continue;}}
   ++i;
  }
 }
 void clearPreviews(){for(auto& p:previews){p.pin.restore();if(p.attached)number(detach,p.attachment.c_str());if(p.inserted)number(removeNode,p.holder.c_str());p={};}}
 void forgetHeldAid(){held_pose_adapter::pending=false;stickyAidButtons=false;mouthGate.reset();mouthDistance=-1;heldFood=false;aidHand=-1;aidGripLatch=false;aidDropPending=false;selectedRef=nullptr;heldAid={};}
 void clearAid(){
  if(aidHand>=0){
   if(heldAid.id==0x15169)build_adapter::clearStim(playerForm());
   else if(selectedRef&&build_adapter::grabbedItem()==selectedRef)build_adapter::stowGrab(playerForm());
   else if(selectedRef&&aidDropPending)number(takeRef,selectedRef);
  }
  forgetHeldAid();
 }
 void close(){clearPreviews();wheelCarrier=nullptr;wheelCarrierLost=false;selectorOpen=false;openHand=-1;selected=-1;items.clear();touchSlot=-1;touchHand=-1;touchSince=0;uiEpoch++;}
 InventoryRef* ownedInstance(Form* item){
  InventoryRef* result=nullptr;Element e;
  if(call(getInventoryRefs,e,item)&&e.type==4&&e.value.array){Element first,key;key.type=1;key.value.number=0;
   if(arrays->get(e.value.array,key,first)&&first.type==2&&first.value.form)result=inventoryRef(first.value.form->id);release(first);}
  release(e);return result&&result->type==item&&result->container==playerForm()?result:nullptr;
 }
 bool holdAid(const Item& item){
  heldAid=item;aidHand=1;aidGripLatch=true;
  // All native ingestibles use their normal inventory-use effect. Food flags
  // describe subcategories, not whether a selected consumable may be used.
  mouthGate.reset();mouthDistance=-1;heldFood=item.form&&item.form->type==47&&item.id!=0x15169;
  if(item.id==0x15169){if(!build_adapter::selectStim(playerForm())){clearAid();return false;}log("NATIVE STIM selected right count=%.0f",number(getCount,item.form));return true;}
  auto instance=ownedInstance(item.form);if(!instance){clearAid();return false;}
  double count=number(getCount,item.form);selectedRef=static_cast<Form*>(build_adapter::dropOneAid(playerForm(),item.form,instance->extra));
  if(selectedRef){instance->removed=true;aidDropPending=true;}
  if(number(getCount,item.form)!=count-1){clearAid();return false;}
  if(!selectedRef){log("NATIVE AID drop could not resolve new reference; item remains in world");clearAid();return false;}
  auto palm=scene::palm(scene::root(playerForm()),1);number(moveRef,selectedRef,bits(palm.x),bits(palm.y),bits(palm.z));
  log("NATIVE AID dropped %08X ref=%08X for right hand",item.id,selectedRef->id);return true;
 }
 // An ordinary game-owned empty reference, handed to native TTR once. No
 // replacement physics, persistent hand latch, custom save records or cloning.
 unsigned lastEmptyBottle=0;
 bool placeEmptyForPickup(Form* ref){
  scene::Transform replacement{};
  if(!held_pose_adapter::target(replacement))return false;
  auto root=scene::root(ref);
  if(!bottle_impact_adapter::body(root))return false;
  // The held offset is computed from the actual rigid body at pickup.
  // Wait for the reference move to reach the loaded scene before binding it.
  static Script* place=nullptr;
  if(!place)place=compile("empty bottle pose","ref target\nfloat x\nfloat y\nfloat z\nfloat ax\nfloat ay\nfloat az\nbegin function {target,x,y,z,ax,ay,az}\ntarget.SetAngleEx ax ay az 1\ntarget.SetPosEx x y z\nSetFunctionValue 1\nend");
  constexpr float deg=57.295779513f;
  const auto& r=replacement.r;
  const auto ax=std::atan2(r[2][1],r[2][2])*deg;
  const auto ay=-std::asin(std::clamp(r[2][0],-1.f,1.f))*deg;
  const auto az=std::atan2(r[1][0],r[0][0])*deg;
  if(!place||number(place,ref,bits(replacement.p.x),bits(replacement.p.y),bits(replacement.p.z),bits(ax),bits(ay),bits(az))!=1)return false;
  root=scene::root(ref);
  if(!root)return false;
  const auto error=scene::distance(scene::world(root).p,replacement.p);
  log("EMPTY BOTTLE placement ref=%08X position error=%.3f",ref->id,error);
  return error<.4f;
 }
 void leaveEmptyBottle(Form* base,double inventoryBefore,const scene::Transform* heldPose){
  if(!base||weaponTransition)return;auto player=playerForm();Form* ref=nullptr;
  // Reuse one empty already returned by another consumption mod, if present.
  if(number(getCount,base)>inventoryBefore){if(auto instance=ownedInstance(base)){ref=static_cast<Form*>(build_adapter::dropOneAid(player,base,instance->extra));if(ref)instance->removed=true;}}
  else {
   static Script* spawn=nullptr;if(!spawn)spawn=compile("empty drink bottle","ref item\nbegin function {item}\nSetFunctionValue (player.PlaceAtMe item 1)\nend");
   Element made;if(spawn&&call(spawn,made,base)&&made.type==2)ref=made.value.form;release(made);
  }
  if(!ref)return;
  if(heldPose){held_pose_adapter::localPose=*heldPose;held_pose_adapter::pending=true;}
  // Use the same staged native pickup as selecting an ordinary inventory item:
  // publish squeeze ownership before grabbing, and allow the new 3D to load.
  heldAid={};heldAid.form=base;heldAid.id=base->id;heldAid.name=string(getName,base);
  trackBottle(ref);selectedRef=ref;lastEmptyBottle=ref->id;aidHand=1;aidGripLatch=true;aidDropPending=true;heldFood=false;
  pendingWeapon=heldAid;pendingAid=true;pendingStage=3;pendingAt=GetTickCount64()+150;aidDeadline=pendingAt+2000;weaponTransition=true;
  auto palm=scene::palm(scene::root(player),1);number(moveRef,ref,bits(palm.x),bits(palm.y),bits(palm.z));
  log("EMPTY BOTTLE staged native pickup ref=%08X base=%08X",ref->id,base->id);
 }
 void eatStickyFood(){
  if(!heldFood||!aidGripLatch||!selectedRef||build_adapter::grabbedItem()!=selectedRef)return;
  // Clear native grab ownership before activation can delete its world ref.
  // Consume through the engine inventory-use path, retaining all normal effects.
  scene::Transform oldPose{};bool preservePose=held_pose_adapter::capture(selectedRef);if(preservePose)oldPose=held_pose_adapter::localPose;
  Item food=heldAid;double before=number(getCount,food.form);
  unsigned emptyID=feature(fc_features::Bottles)?empty_bottles::baseForModel(string(getModel,food.form)):0;auto empty=emptyID?static_cast<Form*>(physical_reload_adapter::lookup(emptyID)):nullptr;
  if(empty&&empty->type!=31)empty=nullptr;double emptyBefore=empty?number(getCount,empty):0;
  if(auto b=build_adapter::heldModule())log("FOOD stow input wanted=%p primary=(%u,%p,%p,%d) secondary=(%u,%p,%p,%d)",selectedRef,b[0x1538805],*reinterpret_cast<void**>(b+0x15395E0),*reinterpret_cast<void**>(b+0x15395E4),*reinterpret_cast<int*>(b+0x1035E60),b[0x15388D4],*reinterpret_cast<void**>(b+0x15388DC),*reinterpret_cast<void**>(b+0x15388E0),*reinterpret_cast<int*>(b+0x15388D8));
  if(!build_adapter::stowSelectedGrab(playerForm(),selectedRef)){
   if(auto b=build_adapter::heldModule())log("FOOD stow refused wanted=%p primary=(%u,%p,%d) secondary=(%u,%p,%d) swap=%02X%02X%02X%02X",selectedRef,b[0x1538805],*reinterpret_cast<void**>(b+0x15395E0),*reinterpret_cast<int*>(b+0x1035E60),b[0x15388D4],*reinterpret_cast<void**>(b+0x15388DC),*reinterpret_cast<int*>(b+0x15388D8),b[0x1CC310],b[0x1CC311],b[0x1CC312],b[0x1CC313]);
   mouthGate.reset();return;
  }
  forgetHeldAid();
  double returned=number(getCount,food.form);
  if(returned!=before+1){log("FOOD mouth cancelled: stow count %.0f -> %.0f",before,returned);return;}
  number(emptyEventFlag,"FC.Empty.Owned",1);
  double result=number(equip,food.form,0);
  number(emptyEventFlag,"FC.Empty.Owned",0);
  double after=number(getCount,food.form);
  log("FOOD mouth %08X native-use=%.0f inventory %.0f -> %.0f -> %.0f",food.id,result,before,returned,after);
  if(after==before){tell(std::string(empty?"Drank: ":"Ate: ")+food.name);if(empty)leaveEmptyBottle(empty,emptyBefore,preservePose?&oldPose:nullptr);}
 }

 void initEmptyEvents(){
  if(emptyEventsReady)return;emptyEventsReady=true;
  // Native CompileScript assigns registered FormIDs. API partial scripts have
  // ID zero and cannot be passed as ref arguments to JIP event registration.
  auto namedScript=[&](const std::string& path)->Script*{
   return fc::nvse::loadFunction(*scripts, path.c_str());
  };
  emptyEventFlag=namedScript("EmptyState");
  auto forms=compile("supported drink catalog","GetLoadedTypeArray 47",true);
  auto registerEvent=compile("native drink event registration","ref handler\nref item\nbegin function {handler,item}\nSetOnUseAidItemEventHandler handler 1 item\nSetFunctionValue 1\nend");
  if(!emptyEventFlag||!forms||!registerEvent)return;
  number(emptyEventFlag,"FC.Empty.Enabled",feature(fc_features::Bottles)?1:0);number(emptyEventFlag,"FC.Empty.Owned",0);
  Element catalog;if(!call(forms,catalog)||catalog.type!=4||!catalog.value.array){release(catalog);return;}
  unsigned registered=0,count=arrays->size(catalog.value.array);if(count>100000){release(catalog);return;}
  for(unsigned i=0;i<count;++i){Element key,item;key.type=1;key.value.number=i;
   if(!arrays->get(catalog.value.array,key,item)||item.type!=2||!item.value.form){release(item);continue;}
   unsigned empty=empty_bottles::baseForModel(string(getModel,item.value.form));if(!empty){release(item);continue;}
   auto existing=std::find_if(emptyEvents.begin(),emptyEvents.end(),[&](const EmptyEvent& e){return e.base==empty;});
   if(existing==emptyEvents.end()){
    char id[16];sprintf_s(id,"%08X",empty);std::string keyName=std::string("FC.Empty.Pending.")+id;
    auto handler=namedScript(std::string("Empty-")+id);
    auto drain=namedScript(std::string("EmptyDrain-")+id);if(!handler||!drain){release(item);continue;}
    number(emptyEventFlag,keyName.c_str(),0);emptyEvents.push_back({empty,drain,handler});existing=emptyEvents.end()-1;
   }
   if(number(registerEvent,existing->handler,item.value.form)){++registered;log("EMPTY EVENT item=%08X bottle=%08X",item.value.form->id,empty);}release(item);
  }release(catalog);log("EMPTY EVENTS registered=%u families=%u",registered,unsigned(emptyEvents.size()));
 }
 void tickEmptyEvents(){
  if(!loaded)return;
  if(emptyFlagDirty){number(emptyEventFlag,"FC.Empty.Enabled",feature(fc_features::Bottles)?1:0);emptyFlagDirty=false;}
  if(!feature(fc_features::Bottles))return;
  if(core_perf::enabled)++core_perf::emptyPolls;
  static Script* award=nullptr;if(!award)award=compile("normal drink empty","ref bottle\nint count\nbegin function {bottle,count}\nplayer.AddItem bottle count 1\nSetFunctionValue 1\nend");
  for(auto& e:emptyEvents){int count=int(number(e.read));if(count<=0||!feature(fc_features::Bottles))continue;count=std::min(count,100);
   auto base=static_cast<Form*>(physical_reload_adapter::lookup(e.base));if(!base||base->type!=31)continue;
   if(number(award,base,count)){nativeEmptyAwarded+=count;log("EMPTY EVENTS native consume base=%08X count=%d",e.base,count);}
  }
 }

 void selectItem(int index){
  int hand=1; // The controls book alone belongs in the left hand.
  if(index<0||index>=int(items.size())||!items[index].form)return;Item chosen=items[index];bool weapon=openHand==1;close();
  if(bookForm&&chosen.id==bookForm->id)hand=0;
  if(number(getCount,chosen.form)<=0){tell("Item no longer in inventory");return;}
  clearAid();selectedWeaponID=0;Element e;u32 equippedID=0;if(call(getEquipped,e)&&e.type==2&&e.value.form)equippedID=e.value.form->id;release(e);
  build_adapter::clearStim(playerForm());build_adapter::stowGrab(playerForm());
  if(weapon&&equippedID==chosen.id&&number(weaponOut)){weaponHand=-1;number(holster);log("TOUCH HOLSTER %08X",chosen.id);tell("Holstered: "+chosen.name);return;}
  // Give TTWVR a native holster frame before replacing its held scene object.
  // Arming draw before EquipItemAlt can be cancelled by the original mod's
  // weapon-change cleanup, especially when switching hands at the same time.
  weaponHand=-1;number(holster);pendingWeapon=chosen;pendingAid=!weapon;pendingHand=hand;pendingStage=1;pendingAt=GetTickCount64()+100;weaponTransition=true;
 }
 void selectNone(){
  close();weaponTransition=false;pendingWeapon={};pendingAid=false;selectedWeaponID=0;
  clearAid();build_adapter::clearStim(playerForm());build_adapter::stowGrab(playerForm());
  weaponHand=-1;number(holster);log("TOUCH NEUTRAL: native holster/stow");tell("Hands empty");
 }
 void initBook(){
  auto lookup=compile("controls book form","GetFormFromMod \"Filthy Casual Controls Book.esp\" \"800\"",true);
  Element e;if(call(lookup,e)&&e.type==2&&e.value.form&&e.value.form->type==40)bookForm=e.value.form;release(e);
  if(!bookForm){log("BOOK optional plugin absent");return;}
  bookGive=compile("controls book once","ref item\nbegin function {item}\nif eval !(player.AuxVarGetFlt \"_FC.ControlsBook.Given\")\nif player.GetItemCount item == 0\nplayer.AddItem item 1 1\nendif\nplayer.AuxVarSetFlt \"_FC.ControlsBook.Given\" 1\nendif\nSetFunctionValue 1\nend");
  bookPlace=compile("book placed reference","ref item\nbegin function {item}\nplayer.AuxVarSetRef \"_FC.ControlsBook.Placed\" item\nSetFunctionValue 1\nend");
  bookPlaced=compile("book placed lookup","player.AuxVarGetRef \"_FC.ControlsBook.Placed\"",true);
  // JIP ModelHasBlock warms the native model/texture cache without cloning,
  // equipping, or changing inventory. First-use upload belongs to save loading.
  bookWarm=compile("book asset warmup","ref model\nbegin function {}\nlet model := GetFormFromMod \"Filthy Casual Controls Book.esp\" \"801\"\nSetFunctionValue (ModelHasBlock model \"FC_ControlsBook\")\nend");
  for(unsigned i=0;i<4;i++){
   const auto& place=book_discovery::locations[i];char text[2048];
   sprintf_s(text,"ref item\nref copy\nbegin function {item}\nlet copy := GetFormFromMod \"Filthy Casual Controls Book.esp\" \"%X\"\nif eval !(IsFormValid copy)\nreturn\nendif\nif eval !(player.AuxVarGetFlt \"_FC.Book.Find.%s\")\ncopy.Enable\nplayer.AuxVarSetRef \"_FC.Book.FindRef.%s\" copy\nplayer.AuxVarSetFlt \"_FC.Book.Find.%s\" 1\nendif\nSetFunctionValue 1\nend",0x802+i,place.key,place.key,place.key);
   bookFind[i]=compile("discoverable book",text);
  }
  log("BOOK form=%08X ready=%d",bookForm->id,bookGive!=nullptr);
 }
 void warmBook(){
  if(bookWarmed||!bookWarm||!feature(fc_features::Book))return;
  bookWarmed=true;auto start=GetTickCount64();auto ready=number(bookWarm);
  log("BOOK asset warmup ready=%.0f ms=%llu",ready,GetTickCount64()-start);
 }
 void publishBook(bool completed){
  if(!completed||!bookActive)return;
  build_adapter::PouchDiagnostics facts;
  void* root=nullptr;if(build_adapter::readPouchDiagnostics(facts))root=facts.weaponRoot;
  if(!root||!scene::find(root,"FC_ControlsBook"))root=scene::root(playerForm());
  bookVisible=root&&controls_book::publish(root,scene::root(playerForm()),bookState,GetTickCount64());
 }
 void tickBook(const Input& current,bool usable,ULONGLONG now){
  if(!bookForm)return;
  // The equipped reader can be handed to native clutter once. After pickup,
  // native grip/release and Havok own the reference; never pin it each frame.
  if(bookDropPending){
   auto ref=static_cast<Form*>(physical_reload_adapter::lookup(bookWorldRef));
   if(!ref||!usable||!feature(fc_features::Book)||current.h[bookDropHand].grip<.2f||now>=bookDropDeadline){bookDropPending=false;return;}
   if(scene::root(ref)){
    Vec head;if(pos("Bip01 Head",1,head))build_adapter::grabSelected(playerForm(),&head.x,ref,bookDropHand);
    if(build_adapter::grabbedItem()==ref&&build_adapter::grabbedHand()==bookDropHand){bookDropPending=false;log("BOOK native world handoff ref=%08X hand=%d",bookWorldRef,bookDropHand);}
   }
   return;
  }
  if(!feature(fc_features::Book)){if(bookActive){weaponHand=-1;number(holster);}bookActive=false;bookState.close();bookVisible=false;bookRouted=false;return;}
  if(loaded&&now>=bookFindAt){bookFindAt=now+1000;
   for(unsigned i=0;i<4;i++)if(!bookFindDone[i]&&bookFind[i]&&number(bookFind[i],bookForm)!=0){
    bookFindDone[i]=true;char expression[128];sprintf_s(expression,"player.AuxVarGetRef \"_FC.Book.FindRef.%s\"",book_discovery::locations[i].key);
    auto script=compile("book discovery reference",expression,true);Element ref;
    if(call(script,ref)&&ref.type==2&&ref.value.form)bookFindRefs[i]=ref.value.form->id;release(ref);
    log("BOOK discovery %s ref=%08X",book_discovery::locations[i].key,bookFindRefs[i]);
   }
  }
  if(loaded&&!bookWarmed)warmBook();
  if(loaded&&!bookGiven&&bookGive){bookGiven=number(bookGive,bookForm)!=0;
   Element placed;if(call(bookPlaced,placed)&&placed.type==2&&placed.value.form)bookWorldRef=placed.value.form->id;release(placed);
  }
  // Native proximity pickup of our saved resting book, on a real squeeze edge.
  // No transform writes or synthetic held grip; TTR owns the resulting grab.
  for(int hand=0;hand<2;hand++)if(usable&&!bookActive&&!heightMenu&&!selectorOpen&&!controlsEditor&&current.h[hand].grip>.65f&&previous.h[hand].grip<=.65f&&!build_adapter::grabbedItem()){
   const u32 refs[]={bookWorldRef,bookFindRefs[0],bookFindRefs[1],bookFindRefs[2],bookFindRefs[3]};
   for(auto id:refs){if(!id)continue;auto ref=static_cast<Form*>(physical_reload_adapter::lookup(id));auto root=ref?scene::root(ref):nullptr;
    if(root&&scene::field<Form*>(ref,0x20)==bookForm&&scene::distance(physical_reload_adapter::gripPoint(hand),scene::world(root).p)<18.f){
     Vec head;if(pos("Bip01 Head",1,head))build_adapter::grabSelected(playerForm(),&head.x,ref,hand);
     if(build_adapter::grabbedItem())break;
    }
   }
  }
  Element e;bool equipped=call(getEquipped,e)&&e.type==2&&e.value.form==bookForm;release(e);
  bool drawn=equipped&&number(weaponOut)!=0;
  if(!equipped){bookRouted=false;if(bookActive&&weaponHand==0)weaponHand=-1;}
  bool canRead=usable&&drawn&&now>=bookCloseUntil&&!heightMenu&&!selectorOpen&&!controlsEditor&&!weaponTransition&&number(playerDead)==0;
  if(!canRead){if(equipped&&!drawn&&bookActive){weaponHand=-1;if(selectedWeaponID==bookForm->id)selectedWeaponID=0;}bookActive=false;bookVisible=false;bookState.close();return;}
  if(!bookRouted){
   if(build_adapter::drawnWeaponHand()==0){bookRouted=true;}
   else{
   pendingWeapon={bookForm,bookForm->id,"Kupo's Controls","FilthyCasual\\controls-book.nif",1,1.f};
   // The book is already equipped. Wait for one native holster transition,
   // then draw left, without re-equipping and holstering it a second time.
   pendingHand=0;pendingAid=false;pendingStage=2;pendingAt=now+100;weaponTransition=true;
   clearAid();weaponHand=-1;number(holster);log("BOOK route to native left hand");return;
   }
  }
  int holdingHand=build_adapter::drawnWeaponHand();if(holdingHand<0)return;
  bookState.hand=holdingHand;
  if(!bookActive){bookState.open(now);bookActive=true;log("BOOK reader opened hand=%d",holdingHand);}
  if(weaponHand==holdingHand&&selectedWeaponID==bookForm->id){
   if(current.h[holdingHand].grip>.65f){weaponHand=-1;selectedWeaponID=0;}
   else build_adapter::ignoreSyntheticBookSlotHold(holdingHand);
  }
  // Finish the grip + A/X gesture before creating loose clutter. TTR uses
  // that same button to take held items, and a loose book falls under Havok.
  if(current.h[holdingHand].grip>.65f&&previous.h[holdingHand].grip>.65f&&!current.h[holdingHand].primary&&previous.h[holdingHand].primary){
   auto instance=ownedInstance(bookForm);
   if(instance){
    auto palm=physical_reload_adapter::gripPoint(holdingHand);bookDropHand=holdingHand;
    bookActive=false;bookState.close();bookVisible=false;bookRouted=false;bookCloseUntil=now+1000;weaponHand=-1;selectedWeaponID=0;number(holster);
    auto ref=static_cast<Form*>(build_adapter::dropOneAid(playerForm(),bookForm,instance->extra));
    if(ref){instance->removed=true;bookWorldRef=ref->id;number(bookPlace,ref);number(moveRef,ref,bits(palm.x),bits(palm.y),bits(palm.z));bookDropPending=true;bookDropDeadline=now+1500;log("BOOK physical transfer ref=%08X",bookWorldRef);}
   }
   return;
  }
  if(current.h[1].secondary&&!previous.h[1].secondary){
   bookActive=false;bookState.close();bookVisible=false;bookCloseUntil=now+1000;weaponHand=-1;selectedWeaponID=0;number(holster);log("BOOK reader holstered");return;
  }
  auto before=bookState.page;bookState.tick(now,current.h[0].trigger,current.h[1].trigger,current.h[1].x);
  if(before!=bookState.page)log("BOOK page %d -> %d",before,bookState.page);
 }
 bool mountedVehicle(){
  auto m=GetModuleHandleW(L"FilthyCasualVehicles.dll");
  auto fn=m?reinterpret_cast<bool(__cdecl*)()>(GetProcAddress(m,"FC_VehicleActive")):nullptr;
  return fn&&fn();
 }
 void* mountedUIFrame(scene::Transform& frame){
  auto m=GetModuleHandleW(L"FilthyCasualVehicles.dll");
  auto fn=m?reinterpret_cast<void*(__cdecl*)(scene::Transform*)>(GetProcAddress(m,"FC_VehicleTutorialFrame")):nullptr;
  return fn?fn(&frame):nullptr;
 }
 scene::Vec cabinPoint(const scene::Transform& f,scene::Vec p){return f.p+scene::rotate(f,p)*f.scale;}
 scene::Transform cabinLocal(const scene::Transform& f,const scene::Transform& world){
  auto t=held_pose_adapter::relative(f,world);t.p=t.p*(1/f.scale);t.scale/=f.scale;return t;
 }
 scene::Transform cabinWorld(const scene::Transform& f,const scene::Transform& local){
  scene::Transform t{};t.p=cabinPoint(f,local.p);t.scale=f.scale*local.scale;
  for(int i=0;i<3;i++)for(int j=0;j<3;j++)for(int k=0;k<3;k++)t.r[i][j]+=f.r[i][k]*local.r[k][j];return t;
 }
 void publishSelector(){
  if(!selectorOpen||!wheelCarrier)return;
  scene::Transform frame{};
  if(mountedUIFrame(frame)!=wheelCarrier){
   wheelCarrierLost=true;for(auto& p:previews)if(p.pin.node)cullNode(p.pin.node,true);return;
  }
  auto rig=scene::root(playerForm());for(int h=0;h<2;h++)selectorPalms[h]=physical_reload_adapter::trackingPoint(scene::palm(rig,h));
  wheelCarrierFrame=frame;wheelHead=cabinPoint(frame,wheelLocalHead);wheelRoot=cabinPoint(frame,wheelLocalRoot);wheelBasis=cabinWorld(frame,wheelLocalBasis);
  for(auto& p:previews)if(p.pin.node){scene::setWorld(p.pin.node,cabinWorld(frame,p.carrierLocal));p.trackingCenter=physical_reload_adapter::trackingPoint(p.mesh.empty()?scene::world(p.pin.node).p:scene::boxCenter(p.mesh));}
 }
 bool buildPage(){
  clearPreviews();for(auto& hand:touchMustLeave)for(auto& slot:hand)slot=true;auto root=scene::root(playerForm());if(!root)return false;int displayed=0;
  // The seated native arm solve has a shorter forward reach than standing.
  // Keep previews on the cabin side of the dashboard, without changing the
  // user's on-foot spacing or increasing invisible touch hitboxes.
  const float forward=wheelCarrier?std::min(settings.forward,30.f):settings.forward;
  Item cap;Element e;if(call(getCap,e)&&e.type==2&&e.value.form){cap.form=e.value.form;cap.id=cap.form->id;cap.model=string(getModel,cap.form);}release(e);
  for(int i=0;i<previewCount;i++){
   bool neutral=i==capSlot;int index=page*slots+i;if(!neutral&&index>=int(items.size()))continue;auto& item=neutral?cap:items[index];if(item.model.empty())continue;
   auto& p=previews[i];p.holder="SeatedPick"+std::to_string(i);p.attachment=p.holder+"|*SC"+std::to_string(i)+"*"+item.model;
   if(p.attachment.size()>=230)continue;p.inserted=number(insert,p.holder.c_str())!=0;if(!p.inserted)continue;
   p.attached=number(attach,p.attachment.c_str())!=0;if(!p.attached)continue;
   auto node=scene::find(root,p.holder.c_str());if(!node)continue;
   float angle=2*pi*float(i)/slots;auto desired=wheelBasis;
   desired.p=wheelHead+scene::rotate(wheelBasis,{settings.radius*std::sin(angle),forward,0})+Vec{0,0,-settings.below+settings.radius*std::cos(angle)};
   if(neutral){
    desired.p=wheelHead+scene::rotate(wheelBasis,{0,forward,0})+Vec{0,0,-settings.below};
    // The cap is authored flat on Z. Present its top toward the player (-Y).
    for(int r=0;r<3;r++){desired.r[r][1]=wheelBasis.r[r][2];desired.r[r][2]=-wheelBasis.r[r][1];}
   }
   desired.scale=neutral?1.f:item.scale;if(!scene::setWorld(node,desired))continue;scene::boxes(node,p.mesh);
   if(neutral&&!p.mesh.empty()){
    Vec extent;scene::boxCenter(p.mesh,&extent);float diameter=std::max({extent.x,extent.y,extent.z});
    if(diameter>.001f){desired.scale=settings.capSize/diameter;if(!scene::setWorld(node,desired))continue;}
   }
   if(!p.mesh.empty()){auto centered=desired;centered.p=centered.p+desired.p-scene::boxCenter(p.mesh);if(!scene::setWorld(node,centered))continue;}
   if(!p.pin.attach(node))continue;
   if(wheelCarrier)p.carrierLocal=cabinLocal(wheelCarrierFrame,scene::world(node));
   // Preview lifetime is retained explicitly, separate from the moving player.
   log("PIN %d %08X world=%.3f %.3f %.3f parent=%s",i,item.id,desired.p.x,desired.p.y,desired.p.z,scene::name(p.pin.anchor));displayed++;
  }
  selected=-1;touchSlot=-1;touchSince=0;openedAt=GetTickCount64();tell(openHand==1?"Touch to equip; center cap clears your hand":"Touch to hold; center cap returns held aid");return displayed>0;
 }
 Item wheelItem(int hand,Form* form,bool preview){
  if(!form || form->type!=(hand==1?40:47) || (hand==0&&(form->flags&0x400)))return {};
  Item item;item.count=int(number(getCount,form));
  if(item.count<=0)return {};
  item.form=form;item.id=form->id;item.name=string(getName,form);
  if(preview){
   item.model=string(getModel,form);
   if(item.model.empty()){
    item.model=hand==1?"Weapons\\1HandPistol\\10mmPistol.NIF":"Clutter\\Chems\\Buffout01.NIF";
    item.name+=" (generic preview)";
   }
   float dimension=float(number(getDimension,form));
   item.scale=dimension>1?std::clamp(settings.size/dimension,.04f,1.5f):.25f;
  }
  return item;
 }
 bool readWheelInventory(int hand,std::vector<Item>& inventory,bool loadModels){
  inventory.clear();
  Element e;
  if(!call(hand==1?getWeapons:getAid,e)||(e.type!=4&&!(e.type==1&&e.value.number==0))){release(e);return false;}
  // An empty native inventory still exposes the neutral center cap.
  auto count=e.type==4&&e.value.array?arrays->size(e.value.array):0;
  if(count>4096){release(e);return false;}
  std::vector<Element> values(count);
  if(count&&!arrays->elements(e.value.array,values.data(),nullptr)){release(e);return false;}
  for(auto& value:values){
   if(value.type==2){
    auto item=wheelItem(hand,value.value.form,loadModels);
    if(item.form)inventory.push_back(std::move(item));
   }
   release(value);
  }
  release(e);
  std::sort(inventory.begin(),inventory.end(),[](const Item&a,const Item&b){return a.name<b.name;});
  return true;
 }
 #include "equipment_wheels.hpp"
 void open(int hand){
  if(!feature(hand==1?fc_features::Weapons:fc_features::Consumables))return;
  if(openHand>=0)close();items.clear();auto player=playerForm();auto root=scene::root(player);if(!root)return;
  Vec head;if(!pos("Bip01 Head",1,head))return;wheelHead=head;wheelRoot=scene::world(root).p;wheelBasis=scene::world(root);wheelBasis.scale=1;wheelCell=scene::cell(player);
  {std::lock_guard<std::mutex> lock(inputMutex);wheelXRHead=input.head;wheelXRValid=input.headValid;}
  wheelCarrierLost=false;wheelCarrier=mountedUIFrame(wheelCarrierFrame);
  if(wheelCarrier){
   wheelLocalHead=scene::inverseRotate(wheelCarrierFrame,wheelHead-wheelCarrierFrame.p)*(1/wheelCarrierFrame.scale);
   wheelLocalRoot=scene::inverseRotate(wheelCarrierFrame,wheelRoot-wheelCarrierFrame.p)*(1/wheelCarrierFrame.scale);
   wheelLocalBasis=cabinLocal(wheelCarrierFrame,wheelBasis);
   tutorialThread=GetCurrentThreadId();
   if(!tutorialPublished)tutorialPublished=native_pose_dispatch::subscribe(kill_camera_head::restore,publishTutorial);
   if(!tutorialPublished){wheelCarrier=nullptr;tell("Vehicle selector unavailable");return;}
  }
  if(!wheelEquipment::readWheel(hand,items)){tell("Inventory unavailable");return;}
  openHand=hand;page=0;uiEpoch++;selectorOpen=true;if(!buildPage())close();
 }
 void saveSprint(){
  WritePrivateProfileStringA("Sprint","Enabled",sprintConfig.enabled?"1":"0",settingsPath);
  WritePrivateProfileStringA("Sprint","ShowNotification",sprintNoticeEnabled?"1":"0",settingsPath);
  char value[40];sprintf_s(value,"%.1f",sprintConfig.apPerSecond);WritePrivateProfileStringA("Sprint","APPerSecond",value,settingsPath);
  sprintf_s(value,"%.2f",sprintConfig.start);WritePrivateProfileStringA("Sprint","StartThreshold",value,settingsPath);
  sprintf_s(value,"%.2f",sprintConfig.stop);WritePrivateProfileStringA("Sprint","StopThreshold",value,settingsPath);
  sprintf_s(value,"%.2f",sprintConfig.multiplier);WritePrivateProfileStringA("Sprint","SpeedMultiplier",value,settingsPath);
 }
 void saveLight(){
  WritePrivateProfileStringA("WristLight","Enabled",wrist_light::config.enabled?"1":"0",settingsPath);
  WritePrivateProfileStringA("WristLight","Projected",wrist_light::config.projected?"1":"0",settingsPath);
  auto write=[&](const char* key,float v){char text[40];sprintf_s(text,"%.3f",v);WritePrivateProfileStringA("WristLight",key,text,settingsPath);};
  auto& c=wrist_light::config;write("Brightness",c.brightness);write("BeamDegrees",c.angle);write("RangeMetres",c.range/69.99125f);write("Red",c.rgb[0]);write("Green",c.rgb[1]);write("Blue",c.rgb[2]);write("ForwardCm",c.forward/.6999125f);write("AboveCm",c.above/.6999125f);
 }
 void saveComfort(){auto& c=comfortConfig;auto put=[&](const char* section,const char* key,float value){char text[32];sprintf_s(text,"%.3f",value);WritePrivateProfileStringA(section,key,text,settingsPath);};
  auto flag=[&](const char* section,const char* key,unsigned value){WritePrivateProfileStringA(section,key,value?"1":"0",settingsPath);};
  flag("Hands","IndexFingers",c.fingers);flag("Hands","CustomGrip",c.grip);flag("Hands","Smoothing",c.smoothing);flag("Walking","Enabled",c.walking);flag("Walking","AutoWalk",c.autoWalk);put("Hands","GripPress",c.press);put("Hands","GripRelease",c.release);put("Hands","SmoothingMs",c.smoothMs);put("Walking","Speed",c.walkSpeed);put("Walking","Deadzone",c.deadzone);put("Walking","RunStart",c.runStart);
 }
 void heightText(){settingsText();}
 void adjustSprintStart(int direction){
  std::lock_guard<std::mutex> lock(comfortMutex);
  auto& c=comfortConfig;
  const float minimum=c.walking?std::max(.80f,c.runStart+.12f):.80f;
  sprintConfig.start=std::clamp(sprintConfig.start+direction*.01f,minimum,1.f);
  sprintConfig.stop=sprintConfig.start-.07f;
  c.sprintStart=sprintConfig.start;
  fc_comfort::sanitize(c);
 }
 void settingsText(){
  fc_settings::View v;v.epoch=uiEpoch;v.page=settingsPage;fc_comfort::Config comfort;
  bool fresh=false;
  if(settingsPage==fc_settings::Hands||settingsPage==fc_settings::Walking){
   std::lock_guard<std::mutex> lock(comfortMutex);
   comfort=comfortConfig;
   const auto now=GetTickCount64();
   fresh=(skeletalAt[0]&&now-skeletalAt[0]<150)||(skeletalAt[1]&&now-skeletalAt[1]<150);
  }
  namespace pages = fc_settings::pages;
  switch(settingsPage){
  case fc_settings::WristHome: {
   bool manual=false;
   const bool canReload=feature(fc_features::Reload)&&build_adapter::readManualReload(manual);
   pages::wristHome(v, seatNames[seatContext], canReload, manual, feature(fc_features::Tutorial), tutorialEnabled);
   break;
  }
  case fc_settings::WristFeatures: {
   pages::features(v, wristFeatures, wristFeaturePage);
   break;
  }
  case fc_settings::WristControls: {
   pages::remapping(v);
   break;
  }
  case fc_settings::Home: {
   pages::home(v, seatNames[seatContext]);
   break;
  }
  case fc_settings::Height: {
   pages::height(v, seatNames[seatContext], seatContext, heightOffset, showSelectorPopups);
   break;
  }
  case fc_settings::Sprint: {
   pages::sprint(v, sprintConfig, sprintNoticeEnabled, feature(fc_features::Sprint));
   break;
  }
  case fc_settings::Light: {
   pages::light(v, wrist_light::config, feature(fc_features::Light));
   break;
  }
  case fc_settings::Comfort: {
   pages::comfort(v);
   if(feature(fc_features::Driving) && GetModuleHandleW(L"FilthyCasualVehicles.dll")){
    const char* levels[]={"NORMAL","HEAVY","INSANE"};
    pages::add(v,"Crash strength",levels[crashStrength],fc_settings::Number,"Change how far a struck car slides. Select to cycle or use the arrows.");
   }
   break;
  }
  case fc_settings::Hands: {
   pages::hands(v, comfort, fresh);
   break;
  }
  case fc_settings::Walking: {
   pages::walking(v, comfort);
   break;
  }
  default: {
   pages::lampPosition(v, wrist_light::config);
   break;
  }
  }
  if(wristSettings&&!wristSettingsStatus.empty())strncpy_s(v.note,wristSettingsStatus.c_str(),_TRUNCATE);
  settingsNav.selected=std::clamp(settingsNav.selected,0,int(v.count)-(wristSettings?1:0));v.selected=settingsNav.selected;
  std::lock_guard<std::mutex> lock(settingsMutex);settingsView=v;
 }
 void openSettings(){if(wristSettings||nativeSettingsAvailable())return;settingsPage=fc_settings::Home;settingsNav={};settingsHover=-1;heightMenu=true;++uiEpoch;settingsText();}
 void saveHeight(float offset){
  if(offset==heightOffset.load())return;
  char value[32];sprintf_s(value,"%.1f",offset*100);
  if(!WritePrivateProfileStringA("Seated",seatKeys[seatContext],value,settingsPath)){
   tell("Could not save the seat height. Previous height kept.",true);
   return;
  }
  const float saved=float(atof(value))*.01f;
  seatOffsets[seatContext]=saved;heightOffset=saved;
  log("HEIGHT saved context=%s offset=%s cm",seatNames[seatContext],value);
 }
 void settingsBack(){
  settingsHover=-1;settingsNav={};
  wristSettingsStatus.clear();
  if(settingsPage==fc_settings::WristHome){wristSettings=false;return;}
  if(settingsPage==fc_settings::Home){heightMenu=false;tell("Settings saved",true);return;}
  settingsPage=settingsPage==fc_settings::LampPosition?fc_settings::Light:(settingsPage==fc_settings::Hands||settingsPage==fc_settings::Walking)?fc_settings::Comfort:wristSettings?fc_settings::WristHome:fc_settings::Home;settingsText();
 }
 void settingsActivate(int direction,bool select){
  int row=settingsNav.selected;auto oldPage=settingsPage;
  unsigned count;{std::lock_guard<std::mutex> lock(settingsMutex);count=settingsView.count;}
  if(row==int(count)){if(select)settingsBack();return;}
  if(wristSettings)wristSettingsStatus.clear();
  if(settingsPage==fc_settings::WristHome){
   if(row==0){
    bool manual=false;
    if(feature(fc_features::Reload)&&(select||direction)&&build_adapter::readManualReload(manual)&&
       (!direction||manual==(direction>0))&&!toggleReload(false))wristSettingsStatus="Could not save reload mode.";
   }else if(row==7){
    if(select&&feature(fc_features::Tutorial))setTutorial(!tutorialEnabled);
   }else if(select){
    constexpr fc_settings::Page pages[]{fc_settings::WristHome,fc_settings::Height,fc_settings::Sprint,
        fc_settings::Light,fc_settings::Comfort,fc_settings::WristFeatures,fc_settings::WristControls};
    settingsPage=pages[row];
    if(settingsPage==fc_settings::WristFeatures){wristFeaturePage=0;const auto available=availableFeatures();wristFeatures.begin(featureMask,available);wristFeatures.available=available;}
   }
  }else if(settingsPage==fc_settings::WristFeatures){
   if(!select)return;
   const auto featureRows=fc_settings::pages::wristFeatureRows(wristFeatures.available,wristFeaturePage);
   if(row<featureRows.count){wristFeatures.toggle(featureRows.ids[row]);}
   else if(row<featureRows.count+2){
    wristFeaturePage=(wristFeaturePage+(row==featureRows.count?featureRows.pages-1:1))%featureRows.pages;
    settingsNav={};
   }else if(fc_features::write(featurePath,wristFeatures.pending)){
    const bool restart=fc_features::restartNeeded(wristFeatures.pending,addonSessionMask);
    applyFeatures(wristFeatures.pending);settingsPage=fc_settings::WristHome;
    wristSettingsStatus=restart?"Saved. Restart for add-on changes.":"Features saved.";
   }else wristSettingsStatus="Could not save features.";
  }else if(settingsPage==fc_settings::WristControls){
   return; // The device host gives the existing input editor its own focus.
  }else if(settingsPage==fc_settings::Home){if(!select)return;
   if(row==0)settingsPage=fc_settings::Height;
   else if(row==1)settingsPage=fc_settings::Sprint;
   else if(row==2)settingsPage=fc_settings::Light;
   else if(row==3){openFeatures();return;}
   else if(row==4){auto m=GetModuleHandleW(L"SeatedInput.dll");using Open=void(__cdecl*)();auto fn=m?reinterpret_cast<Open>(GetProcAddress(m,"SC_OpenControls")):nullptr;if(fn){heightMenu=false;fn();}return;}
   else if(row==5)settingsPage=fc_settings::Comfort;
   else {settingsBack();return;}
  }else if(settingsPage==fc_settings::Height){
   if(row==0&&direction)saveHeight(std::clamp(heightOffset+direction*.01f,-.5f,seatContext?.5f:1.2f));
   else if(row==1&&select&&!seatContext){
    float sample=0;
    {std::lock_guard<std::mutex> lock(heightSampleMutex);if(heightSampleAt&&GetTickCount64()-heightSampleAt<300)sample=physicalHeight.load();}
    if(sample>.2f)saveHeight(std::clamp(desiredHeight-sample,-.5f,1.2f));
    else {tell("Hold your head steady, then calibrate again",true);return;}
   }
   else if(row==2&&select)saveHeight(0);
   else if(row==3&&(select||direction)){showSelectorPopups=direction?direction>0:!showSelectorPopups;WritePrivateProfileStringA("UI","ShowSelectorPopups",showSelectorPopups?"1":"0",settingsPath);}
  }else if(settingsPage==fc_settings::Sprint){
   if(row==0&&(select||direction))sprintConfig.enabled=direction?direction>0:!sprintConfig.enabled;
   else if(row==1&&direction)sprintConfig.multiplier=std::clamp(sprintConfig.multiplier+direction*.1f,1.f,2.5f);
   else if(row==2&&direction)sprintConfig.apPerSecond=std::clamp(sprintConfig.apPerSecond+direction,0.f,100.f);
   else if(row==3&&direction)adjustSprintStart(direction);
   else if(row==4&&(select||direction))sprintNoticeEnabled=direction?direction>0:!sprintNoticeEnabled;
   saveSprint();
  }else if(settingsPage==fc_settings::Light){auto& c=wrist_light::config;
   if(row==0&&(select||direction))c.enabled=direction?direction>0:!c.enabled;
   else if(row==1&&direction)c.brightness=std::clamp(c.brightness+direction*.1f,0.f,5.f);
   else if(row==2&&direction)c.angle=std::clamp(c.angle+direction*2.f,10.f,90.f);
   else if(row==3&&direction)c.range=std::clamp(c.range+direction*350.f,350.f,3500.f);
   else if(row==4&&direction){const float colours[][3]={{1,.93f,.78f},{1,1,1},{.55f,1,.55f},{1,.6f,.25f}};int preset=c.rgb[0]<.8f?2:c.rgb[1]<.8f?3:c.rgb[2]<.9f?0:1;memcpy(c.rgb,colours[(preset+direction+4)%4],sizeof c.rgb);}
   else if(row==5&&select)settingsPage=fc_settings::LampPosition;
   saveLight();
  }else if(settingsPage==fc_settings::Comfort){
   if(row==2 && feature(fc_features::Driving)){
    const int next=select?(crashStrength+1)%3:std::clamp(crashStrength+direction,0,2);
    const char digits[]={char('0'+next),0};
    if(WritePrivateProfileStringA("Driving","CrashStrength",digits,settingsPath))crashStrength=next;
   }else if(select&&row<2)settingsPage=row?fc_settings::Walking:fc_settings::Hands;
  }else if(settingsPage==fc_settings::Walking&&row==3){
   if(direction){adjustSprintStart(direction);saveSprint();}
  }else if(settingsPage==fc_settings::Hands||settingsPage==fc_settings::Walking){
   std::lock_guard<std::mutex> lock(comfortMutex);auto& c=comfortConfig;auto toggle=[&](uint32_t& value){if(select||direction)value=direction?direction>0:!value;};
   if(settingsPage==fc_settings::Hands){if(row==0)toggle(c.fingers);else if(row==1)toggle(c.grip);else if(row==2&&direction)c.press+=direction*.05f;else if(row==3&&direction)c.release+=direction*.05f;else if(row==4)toggle(c.smoothing);else if(row==5&&direction)c.smoothMs+=direction;}
   else {
    if(row==0)toggle(c.walking);
    else if(row==1&&direction)c.deadzone=std::min(c.deadzone+direction*.01f,c.runStart-.10f);
    else if(row==2&&direction)c.runStart+=direction*.01f;
    else if(row==4&&direction)c.walkSpeed+=direction*.05f;
    else if(row==5)toggle(c.autoWalk);
   }
   fc_comfort::sanitize(c);saveComfort();
  }else{auto& c=wrist_light::config;if(direction){if(row==0)c.forward=std::clamp(c.forward+direction*.7f,-7.f,14.f);else c.above=std::clamp(c.above+direction*.7f,0.f,7.f);}saveLight();}
  if(settingsPage!=oldPage){settingsNav={};settingsHover=-1;}
  settingsText();
 }
 void settingsTick(const Input& current,ULONGLONG now){
  const auto& r=current.h[1];const auto& l=current.h[0];const auto& pr=previous.h[1];const auto& pl=previous.h[0];
  if(r.secondary&&!pr.secondary){settingsBack();return;}
  float x=r.x,y=r.y;if(x*x+y*y<l.x*l.x+l.y*l.y){x=l.x;y=l.y;}
  int count;{std::lock_guard<std::mutex> lock(settingsMutex);count=int(settingsView.count);}
  if(!count)return;
  int was=settingsNav.selected;int direction=settingsNav.step(x,y,now,count+1);
  if(direction){settingsActivate(direction,false);return;}
  if(was!=settingsNav.selected)settingsText();
  int pointer=settingsHover.load();bool trigger=false;
  if(pointer>=0){int hand=pointer>>16;trigger=hand==0?(l.trigger>.65f&&pl.trigger<=.65f):(r.trigger>.65f&&pr.trigger<=.65f);}
  bool confirm=(r.primary&&!pr.primary)||(l.primary&&!pl.primary);
  if(trigger){int hit=pointer&65535;if(hit==fc_settings::Back){settingsBack();return;}int row=hit/3;if(row>=count)return;settingsNav.selected=row;settingsActivate(hit%3==1?-1:hit%3==2?1:0,true);}
  else if(confirm)settingsActivate(0,true);
 }

 bool toggleReload(bool notify){
   if(!feature(fc_features::Reload))return false;
   bool manual=false;
   if(build_adapter::readManualReload(manual)&&build_adapter::setManualReload(!manual)){
    if(WritePrivateProfileStringA("Weapons","AutomaticReload",manual?"1":"0",settingsPath)){
     automaticReload=manual;
     if(notify)tell(automaticReload?"Automatic reload ON":"Manual reload ON",true);
     log("RELOAD MODE: %s",automaticReload?"automatic":"manual");
     return true;
    }
    build_adapter::setManualReload(manual);
   }
   if(notify)tell("Reload mode unavailable; unchanged",true);
   log("RELOAD MODE: change refused");
   return false;
 }
 void cullNode(void* node,bool hidden){if(node){auto& flags=scene::field<unsigned>(node,0x30);flags=hidden?flags|1:flags&~1u;}}
 void cullWristBranch(void* node,bool hidden,int depth=0){
  if(!node||depth>32)return;cullNode(node,hidden);
  // The native VR draw list may retain already-collected geometry for an eye.
  // Collapse only this addon's alternate mode branch as well as culling it.
  // Its authored root scale is one; use a finite nonzero hidden scale.
  if(depth==0){scene::local(node).scale=hidden?.0001f:1.f;scene::update(node);}
  auto vt=scene::field<void**>(node,0);
  if(!reinterpret_cast<void*(__thiscall*)(void*)>(vt[3])(node))return;
  auto count=scene::field<unsigned short>(node,0xA6);auto children=scene::field<void**>(node,0xA0);
  if(!children||count>4096)return;
  for(unsigned i=0;i<count;i++)cullWristBranch(children[i],hidden,depth+1);
 }
 void clearWrist(){
  for(auto& p:wristPlungers)p={};
  wristTouch.reset();wristFrame={};wristNode=nullptr;wristRig=nullptr;
  if(wristAttached)number(detach,wristAttachment);if(wristInserted)number(removeNode,wristHolder);
  wristAttached=wristInserted=false;wristRetry=0;
 }
 int updateWrist(void* root,ULONGLONG now,bool usable){
  if(!wristEnabled){wristTouch.reset();return -1;}
  if(wristRig&&root!=wristRig){for(auto& p:wristPlungers)p={};wristNode=wristRig=nullptr;wristInserted=wristAttached=false;wristRetry=0;wristTouch.reset();}
  wristFrame=wrist_controls::sample(root);
  if(!wristFrame.valid||!usable){cullNode(wristNode,true);wristTouch.update(wristFrame,now,false);return -1;}
  if(!wristNode&&now>=wristRetry){
   wristRetry=now+2000;
   if(!wristInserted)wristInserted=number(insert,wristHolder)!=0;
   if(wristInserted&&!wristAttached)wristAttached=number(attach,wristAttachment)!=0;
   if(wristAttached){wristNode=scene::find(root,wristHolder);wristRig=root;if(wristNode){scene::moveParent(wristNode,wristFrame.anchor);
    for(int i=0;i<5;i++){auto& p=wristPlungers[i];p.node=scene::find(wristNode,wristPlungerNames[i]);if(p.node)p.rest=scene::local(p.node).p;}
    log("WRIST native model attached");}}
  }
  if(!wristNode){wristTouch.update(wristFrame,now,false);return -1;}
  if(!scene::setWorld(wristNode,wristFrame.world)){cullNode(wristNode,true);wristTouch.update(wristFrame,now,false);return -1;}
  cullNode(wristNode,false);
  bool manual=false;bool known=build_adapter::readManualReload(manual);
  // TTR also traverses draw geometry directly; hide the variant's actual shapes,
  // not only its parent NiNode, so the alternate letter cannot remain visible.
  cullWristBranch(scene::find(wristNode,"FC_Reload_Auto"),!known||manual);
  cullWristBranch(scene::find(wristNode,"FC_Reload_Manual"),!known||!manual);
  // Move only the button plunger; bezels and cuff stay rigid on the forearm.
  // Rest transforms come from the NIF, so depression never accumulates.
  for(int i=0;i<5;i++){auto& p=wristPlungers[i];if(!p.node)continue;
   int action=i<2?i:(i<4?2:3);
   p.depression=wrist_controls::visualPress(wristFrame,action);
   scene::local(p.node).p=p.rest+scene::Vec{0,0,-wrist_controls::PressTravel*p.depression};
   scene::update(p.node);
  }
  return wristTouch.update(wristFrame,now,!selectorOpen&&!heightMenu&&!weaponTransition);
 }
 void clearTutorialVisual(){
  tutorialAnchorValid=false;tutorialCarrier=nullptr;tutorialRig=nullptr;
  tutorial_visual::clear();
  if(tutorialAttached)number(detach,"FCTutorialMount|FilthyCasual\\tutorial.nif");
  if(tutorialInserted)number(removeNode,"FCTutorialMount");
  tutorialAttached=tutorialInserted=false;
 }
 bool ensureTutorialVisual(){
  if(tutorial_visual::pin.node)return true;
  auto now=GetTickCount64();if(now<tutorialVisualRetry)return false;tutorialVisualRetry=now+2000;
  if(!tutorialInserted)tutorialInserted=number(insert,"FCTutorialMount")!=0;
  if(tutorialInserted&&!tutorialAttached)tutorialAttached=number(attach,"FCTutorialMount|FilthyCasual\\tutorial.nif")!=0;
  auto node=scene::find(scene::root(playerForm()),"FC_Tutorial");
  bool ok=node&&tutorial_visual::bind(node);log("TUTORIAL visual insert=%d attach=%d bind=%d",tutorialInserted,tutorialAttached,ok);return ok;
 }
 void drawTutorial(){
  if(tutorialStep==reload_tutorial::Step::Hidden||!tutorialNative.valid||!tutorialWeapon){tutorial_visual::hide();return;}
  if(!tutorial_visual::pin.node||!tutorialRig)return;
  tutorial_visual::begin();auto& r=tutorialNative.reload;auto frame=scene::world(r.weaponRoot);
  auto point=[&](tutorial_data::Vec p){return tutorial_adapter::position(r.weaponRoot,p);};
  if(tutorialWeapon->charger.present&&!tutorialWeapon->charger.values.empty()){
   auto node=scene::find(r.weaponRoot,tutorialWeapon->charger.node.c_str());
   if(node){auto axis=tutorialWeapon->charger.axis;
    auto entry=scene::world(node).p+scene::rotate(frame,{axis.x,axis.y,axis.z})*(tutorialWeapon->charger.values[0]*frame.scale);
    tutorial_visual::box(0,entry,2.f,frame);}
  }
  else if(tutorialWeapon->mag.present)tutorial_visual::box(0,point(tutorialWeapon->mag.face),std::clamp(tutorialWeapon->mag.spread*.65f,1.5f,3.5f),frame);
  else if(tutorialWeapon->hasRounds)tutorial_visual::box(0,tutorial_adapter::loadingPoint(tutorialNative,*tutorialWeapon),2.0f,frame);
  if(tutorialWeapon->handle.present)tutorial_visual::box(1,point(tutorialWeapon->handle.face),2.0f,frame);
  else if(tutorialMechanism==reload_tutorial::Mechanism::Cylinder||tutorialMechanism==reload_tutorial::Mechanism::Gate){
   std::vector<void*> parts;tutorial_adapter::namedParts(r.weaponRoot,parts);
   const char* token=tutorialMechanism==reload_tutorial::Mechanism::Cylinder?"cylinder":"loadinggate";
   for(auto node:parts){auto label=tutorial_data::key(scene::name(node));
    if(label.find(token)==std::string::npos||label.find(":0")==std::string::npos)continue;
    std::vector<scene::Box> mesh;scene::boxes(node,mesh);
    if(!mesh.empty()){auto& box=mesh.front();auto t=scene::world(box.node);auto center=t.p+scene::rotate(t,(box.lo+box.hi)*.5f)*t.scale;
     tutorial_visual::box(1,center,2.0f,t);}
    break;
   }
  }
  if(r.primary.readable&&r.primaryShown){auto pouch=scene::world(r.primaryNode);auto p=pouch.p;
   if(r.spareCenterValid)p=p+scene::rotate(pouch,{r.spareLocalCenter[0],r.spareLocalCenter[1],r.spareLocalCenter[2]})*pouch.scale;
   tutorial_visual::box(2,p,4.0f,pouch);
  }
  auto head=scene::find(tutorialRig,"Bip01 Head");if(!head)return;
  const auto eye=scene::world(head).p;
  // A reading card stays in its world zone; manipulating the gun must not
  // pull it into the face or across the action. Only the part outlines follow.
  float units=r.metresToUnits;
  if(!std::isfinite(units)||units<1)return;
  // Cabin-local anchor uses the same completed native pose as the gun.
  // On foot the reading card retains its original stable world zone.
  using Carrier=void*(__cdecl*)(scene::Transform*);
  auto module=GetModuleHandleW(L"FilthyCasualVehicles.dll");
  auto query=module?reinterpret_cast<Carrier>(GetProcAddress(module,"FC_VehicleTutorialFrame")):nullptr;
  scene::Transform carrierFrame{};auto carrier=query?query(&carrierFrame):nullptr;
  if(carrier!=tutorialCarrier){tutorialAnchorValid=false;tutorialCarrier=carrier;}
  if(carrier&&tutorialAnchorValid){
   tutorialAnchor=carrierFrame.p+scene::rotate(carrierFrame,tutorialLocalAnchor)*carrierFrame.scale;
   tutorialAnchorEye=carrierFrame.p+scene::rotate(carrierFrame,tutorialLocalEye)*carrierFrame.scale;
  }
  if(!tutorialAnchorValid||scene::distance(eye,tutorialAnchorEye)>.8f*units){
   // Reuse the selector's established player-facing frame. A weapon may
   // still be at the hip when drawn, so its direction cannot place the card.
   auto body=scene::world(tutorialRig);
   tutorialAnchor=eye+scene::rotate(body,{-.10f*units,.72f*units,0});tutorialAnchor.z+=.15f*units;
   tutorialAnchorEye=eye;tutorialAnchorValid=true;
   if(carrier){
    tutorialLocalAnchor=scene::inverseRotate(carrierFrame,tutorialAnchor-carrierFrame.p)*(1/carrierFrame.scale);
    tutorialLocalEye=scene::inverseRotate(carrierFrame,tutorialAnchorEye-carrierFrame.p)*(1/carrierFrame.scale);
   }
  }
  tutorial_visual::card(reload_tutorial::card(tutorialStep,tutorialMechanism),tutorialAnchor,tutorialAnchorEye);
  scene::update(tutorial_visual::pin.node); // Recompute aggregate bounds after moving children.
 }
 void publishTutorial(bool completed){
  kill_camera_head::publish(completed);
  wrist_light::publish(completed);
  publishBook(completed);
  if(GetCurrentThreadId()!=tutorialThread)return;
  if(!completed){tutorial_visual::hide();if(wheelCarrier){wheelCarrierLost=true;for(auto& p:previews)if(p.pin.node)cullNode(p.pin.node,true);}return;}
  publishSelector();
  if(tutorialStep==reload_tutorial::Step::Hidden||!tutorial_visual::pin.node)return;
  tutorialNative=tutorial_adapter::read();
  drawTutorial();++tutorialPublications;
 }
 void setTutorial(bool want){
  want=want&&feature(fc_features::Tutorial);
  if(want&&!tutorialDataLoaded){
   std::ifstream pack(gameDirectory+"Data\\NVSE\\Plugins\\TTWVR\\TTWVR.dat",std::ios::binary);
   tutorialDataLoaded=tutorial_data::readPack(pack,tutorialWeapons,tutorialError);
   log("TUTORIAL metadata %s: weapons=%u %s",tutorialDataLoaded?"loaded":"unavailable",unsigned(tutorialWeapons.size()),tutorialError.c_str());
  }
  if(!want)clearTutorialVisual();
  tutorialEnabled=want&&tutorialDataLoaded&&tutorialEditorQuery&&tutorialRoundsQuery&&tutorialCapacityQuery;
  tutorialStep=reload_tutorial::Step::Hidden;tutorialForm=0;tutorialWeapon=nullptr;tutorialEditor.clear();
  if(!usingWristSettings())tell(tutorialEnabled?"Weapon tutorial ON":want?"Tutorial metadata unavailable":"Weapon tutorial OFF",true);
 }
 void tickTutorial(){
  tutorialNative={};
  bool visible=tutorialEnabled&&enabled&&!selectorOpen&&!heightMenu&&!weaponTransition&&!bookActive;
  if(!visible){tutorialStep=reload_tutorial::Step::Hidden;tutorial_visual::hide();return;}
  Element e;Form* weapon=nullptr;if(call(getEquipped,e)&&e.type==2)weapon=e.value.form;release(e);
  if(!number(weaponOut))weapon=nullptr;
  if(tutorialForm!=(weapon?weapon->id:0)){
   tutorialAnchorValid=false;tutorialStep=reload_tutorial::Step::Hidden;
   tutorialForm=weapon?weapon->id:0;tutorialEditor=weapon?tutorial_data::key(string(tutorialEditorQuery,weapon)):"";
   auto i=tutorialWeapons.find(tutorialEditor);tutorialWeapon=i==tutorialWeapons.end()?nullptr:&i->second;
   log("TUTORIAL weapon=%08X editor=%s mapped=%d",tutorialForm,tutorialEditor.c_str(),tutorialWeapon!=nullptr);
  }
  tutorialNative=tutorial_adapter::read();auto& r=tutorialNative.reload;
  reload_tutorial::State state;state.visible=visible;state.weapon=weapon!=nullptr;
  build_adapter::readManualReload(state.manual);
  state.chamberKnown=tutorialNative.chamberKnown;state.chambered=tutorialNative.chambered;
  state.rounds=weapon?int(number(tutorialRoundsQuery)):0;state.capacity=weapon?int(number(tutorialCapacityQuery,weapon)):0;
  state.installed=tutorialNative.valid?r.installedMode:-1;state.installedRounds=r.installedRounds;state.held=r.nativeMode;
  auto facts=tutorial_adapter::facts(weapon);state.weaponType=facts.type;state.reloadAnimation=facts.reload;
  tutorialMechanism=reload_tutorial::mechanism(tutorialWeapon,state);
  auto next=reload_tutorial::choose(tutorialWeapon,state);
  Input tutorialInput;{std::lock_guard<std::mutex> lock(inputMutex);tutorialInput=input;}
  const bool cyclingControls=tutorialInput.h[0].trigger>.55f||tutorialInput.h[1].trigger>.55f||
   ((tutorialInput.h[0].primary||tutorialInput.h[1].primary)&&(tutorialInput.h[0].grip>.55f||tutorialInput.h[1].grip>.55f));
  next=reload_tutorial::settle(tutorialStep,next,cyclingControls);
  if(next!=tutorialStep)log("TUTORIAL step=%s rounds=%d installed=%d held=%d chamber=%d",reload_tutorial::name(next),state.rounds,state.installed,state.held,state.chambered);
  tutorialStep=next;
  tutorialThread=GetCurrentThreadId();tutorialRig=scene::root(playerForm());
  if(next!=reload_tutorial::Step::Hidden){
   ensureTutorialVisual();
   if(!tutorialPublished)tutorialPublished=native_pose_dispatch::subscribe(kill_camera_head::restore,publishTutorial);
   if(!tutorialPublished)tutorial_visual::hide();
  }else tutorial_visual::hide();
 }
 void tutorialDiagnostics(std::ostream& out){
  auto anchor=physical_reload_adapter::trackingPoint(tutorialAnchor);
  out<<",\"tutorialPublications\":"<<tutorialPublications<<",\"tutorialMounted\":"<<(tutorialCarrier?"true":"false");
  out<<",\"tutorialCardLocal\":["<<tutorialLocalAnchor.x<<","<<tutorialLocalAnchor.y<<","<<tutorialLocalAnchor.z<<"]";
  out<<",\"tutorialCardWorld\":["<<tutorialAnchor.x<<","<<tutorialAnchor.y<<","<<tutorialAnchor.z<<"]";
  out<<",\"tutorialCardAnchor\":["<<anchor.x<<","<<anchor.y<<","<<anchor.z<<"]";

  out<<",\"tutorial\":{\"enabled\":"<<(tutorialEnabled?"true":"false")<<",\"visualElements\":"<<tutorial_visual::shown<<",\"metadataCount\":"<<tutorialWeapons.size()
     <<",\"weapon\":"<<tutorialForm<<",\"editor\":"<<std::quoted(tutorialEditor)<<",\"step\":\""<<reload_tutorial::name(tutorialStep)
     <<"\",\"card\":"<<std::quoted(reload_tutorial::card(tutorialStep,tutorialMechanism))<<",\"valid\":"<<(tutorialNative.valid?"true":"false")<<",\"chamberKnown\":"<<(tutorialNative.chamberKnown?"true":"false")
     <<",\"chambered\":"<<(tutorialNative.chambered?"true":"false")<<",\"parts\":[";
  if(tutorialNative.valid&&tutorialWeapon){
   const tutorial_data::Part* parts[]={&tutorialWeapon->mag,&tutorialWeapon->handle,&tutorialWeapon->loader};
   bool first=true;for(auto p:parts)if(p->present){
    if(!first)out<<",";first=false;auto node=scene::find(tutorialNative.reload.weaponRoot,p->node.c_str());
    auto partPosition=tutorial_adapter::position(tutorialNative.reload.weaponRoot,p->face);
    auto track=physical_reload_adapter::trackingPoint(partPosition);
    auto pullPoint=tutorial_adapter::position(tutorialNative.reload.weaponRoot,{p->face.x-p->axis.x*10.f,p->face.y-p->axis.y*10.f,p->face.z-p->axis.z*10.f});
    auto pullTrack=physical_reload_adapter::trackingPoint(pullPoint);
    auto nodeTrack=node?physical_reload_adapter::trackingPoint(scene::world(node).p):Vec{};
    out<<"{\"node\":"<<std::quoted(p->node)<<",\"found\":"<<(node?"true":"false")<<",\"tracking\":["<<track.x<<","<<track.y<<","<<track.z<<"],\"pullTracking\":["<<pullTrack.x<<","<<pullTrack.y<<","<<pullTrack.z<<"],\"nodeTracking\":["<<nodeTrack.x<<","<<nodeTrack.y<<","<<nodeTrack.z<<"]}";
   }
  }
  if(tutorialNative.valid&&tutorialWeapon&&tutorialWeapon->hasRounds){
   auto track=physical_reload_adapter::trackingPoint(tutorial_adapter::loadingPoint(tutorialNative,*tutorialWeapon));
   if(tutorialWeapon->mag.present||tutorialWeapon->handle.present||tutorialWeapon->loader.present)out<<",";
   out<<"{\"node\":\"loading-port\",\"found\":true,\"tracking\":["<<track.x<<","<<track.y<<","<<track.z<<"]}";
  }
  Element equipped;Form* weapon=nullptr;if(call(getEquipped,equipped)&&equipped.type==2)weapon=equipped.value.form;release(equipped);
  auto facts=tutorial_adapter::facts(weapon);
  out<<"],\"weaponType\":"<<facts.type<<",\"reloadAnimation\":"<<facts.reload<<",\"namedParts\":[";
  std::vector<void*> named;if(tutorialNative.valid)tutorial_adapter::namedParts(tutorialNative.reload.weaponRoot,named);
  for(size_t i=0;i<named.size();i++){
   if(i)out<<",";auto node=named[i];auto p=physical_reload_adapter::trackingPoint(scene::world(node).p);
   out<<"{\"node\":"<<std::quoted(scene::name(node))<<",\"tracking\":["<<p.x<<","<<p.y<<","<<p.z<<"],\"basis\":[";
   auto t=scene::world(node);
   for(int c=0;c<3;c++){if(c)out<<",";Vec axis{};if(c==0)axis.x=70;else if(c==1)axis.y=70;else axis.z=70;
    auto end=physical_reload_adapter::trackingPoint(t.p+scene::rotate(t,axis));auto v=tutorial_visual::unit(end-p);
    out<<"["<<v.x<<","<<v.y<<","<<v.z<<"]";}
   out<<"]}";
  }
  out<<"],\"pointer\":[";
  auto rig=scene::root(playerForm());const char* names[]={"Bip01 L Finger1","Bip01 L Finger12"};
  for(int i=0;i<2;i++){
   if(i)out<<",";auto node=scene::find(rig,names[i]);auto p=node?physical_reload_adapter::trackingPoint(scene::world(node).p):Vec{};
   out<<"["<<p.x<<","<<p.y<<","<<p.z<<"]";
  }
  out<<"]}";
 }
 void updateSeatContext(){
  using Context=int(__cdecl*)();auto mod=GetModuleHandleW(L"FilthyCasualVehicles.dll");auto fn=mod?reinterpret_cast<Context>(GetProcAddress(mod,"FC_SeatContext")):nullptr;
  int next=fn?fn():0;if(next<0||next>4)next=0;
  if(next!=seatContext){
   seatContext=next;heightOffset=seatOffsets[next].load();mouthGate.reset();
   if(heightMenu&&settingsPage==fc_settings::Height){settingsNav={};settingsHover=-1;heightText();}
  }
 }
 void clearBeam(){
  wrist_light::restore();wrist_light::beamPin.restore();wrist_light::clearSocket();
  wrist_light::lamp=nullptr;wrist_light::gloveBand=nullptr;wrist_light::mount=nullptr;wrist_light::plug=nullptr;for(auto& n:wrist_light::cable)n=nullptr;
  // Return the pinned visual before removing our mount. A surviving mount
  // otherwise makes InsertNode fail on reload and prevents the beam rebind.
  if(beamAttached)number(detach,"FCWristBeamMount|FilthyCasual\\wrist-beam.nif");
  if(beamInserted)number(removeNode,"FCWristBeamMount");
  beamInserted=beamAttached=false;beamRetry=0;
  scene::release(beamRig);beamRig=nullptr;
 }
 void tickSprint(const Input& current,bool usable,ULONGLONG now){
  float dt=sprintAt&&now>=sprintAt?float(now-sprintAt)*.001f:0;sprintAt=now;
  bool allowed=feature(fc_features::Sprint)&&usable&&seatContext==0&&!selectorOpen&&!heightMenu&&!weaponTransition&&!controlsEditor&&pendingControl.load()<0;
  allowed=allowed&&sprint_adapter::installed&&sprintAP&&sprintDrain&&sprintAllowed&&number(sprintAllowed)!=0;
  float ap=allowed?float(number(sprintAP)):0;
  bool wasSprinting=sprintState.active;
  float cost=sprintState.update(sprintConfig,allowed,current.h[0].x,current.h[0].y,ap,dt);
  if(cost>0&&!number(sprintDrain,bits(cost)))sprintState.reset();
  sprint_adapter::factor=sprintState.active?sprintConfig.multiplier:1.f;
  if(sprintState.active&&!wasSprinting&&sprintNoticeEnabled){sprintNoticeUntil=now+2200;++sprintNoticeCount;log("SPRINT start cue");}
  if(!sprintState.active)sprintNoticeUntil=0;
 }
 uint32_t availableFeatures(){
  uint32_t available=fc_features::coreMask;
  if(!bookForm)available&=~fc_features::bit(fc_features::Book);
  if(auto module=GetModuleHandleW(L"FilthyCasualVehicles.dll"))if(GetProcAddress(module,"FC_FeatureGateVersion")){
   static auto cars=compile("driving requirement","IsModLoaded \"UmDrivingVIIIb.esp\"",true);
   if(number(cars))available|=fc_features::bit(fc_features::Driving);
  }
  if(auto module=GetModuleHandleW(L"FilthyCasualPool.dll"))if(GetProcAddress(module,"FC_FeatureGateVersion"))available|=fc_features::bit(fc_features::Pool);
  if(auto module=GetModuleHandleW(L"FilthyCasualSettlements.dll"))if(GetProcAddress(module,"FC_SettlementDesk")){
   static auto master=compile("Workshop requirement","IsModLoaded \"Wasteland Forts.esm\"",true);
   if(number(master))available|=fc_features::bit(fc_features::Workshop);
  }
  if(auto module=GetModuleHandleW(L"FilthyCasualEnclave.dll"))if(GetProcAddress(module,"FC_EnclaveMode")){
   static auto master=compile("Command requirement","IsModLoaded \"EnclaveCommanderNV.esm\"",true);
   if(number(master))available|=fc_features::bit(fc_features::Command);
  }
  return available;
 }
 void openFeatures(){
  if(usingWristSettings())return;
  if(selectorOpen)close();settingsHover=-1;pendingControl=-1;featureHover=-1;featureEvent=-1;
  const auto available=availableFeatures();
  std::lock_guard<std::mutex> lock(featureMutex);featureSelection.begin(featureMask,available);
  featureView={};featureView.visible=1;featureView.epoch=++uiEpoch;featureView.pending=featureSelection.pending;featureView.available=available;
  strcpy_s(featureView.status,"Stick: page / select | A / trigger: toggle | B: cancel");
  featureMenu=true;heightMenu=true;featureInputAt=GetTickCount64()+300;
 }
 void applyFeatures(uint32_t mask){
  uint32_t old=featureMask.exchange(mask&fc_features::allowedMask);
  if((old^mask)&fc_features::vehicleMask){
   const auto vehicles=GetModuleHandleW(L"FilthyCasualVehicles.dll");
   using Notify=void(__cdecl*)(bool,bool,bool);
   const auto notify=vehicles?reinterpret_cast<Notify>(GetProcAddress(vehicles,"FC_VehicleFeaturesChanged")):nullptr;
   if(notify)notify(feature(fc_features::Driving),feature(fc_features::Flight),feature(fc_features::Riding));
  }
  if((old^mask)&fc_features::bit(fc_features::Bottles)){
   number(emptyEventFlag,"FC.Empty.Enabled",feature(fc_features::Bottles)?1:0);
   // Clear once at the transition, not by polling every disabled frame.
   for(auto& e:emptyEvents)number(e.read);
  }
  presentation_adapter::enabled=feature(fc_features::Presentation);floating_hud_adapter::enabled=feature(fc_features::Presentation);
  if(!feature(fc_features::Presentation))presentation_adapter::setSpeaker(0,{});
  if(!feature(fc_features::Tutorial)){tutorialEnabled=false;clearTutorialVisual();}
  if(!feature(fc_features::Light)){wrist_light::gameplay=false;clearBeam();}
  if(!feature(fc_features::Magazines))native_worldmag_recovery::reset();
  if(!feature(fc_features::Glass))clearBottleImpacts();
  if(!feature(fc_features::Sprint)){sprintState.suspend();sprint_adapter::factor=1.f;}
  if(!feature(fc_features::Weapons)){weaponHand=-1;selectedWeaponID=0;}
  if(!feature(fc_features::Consumables)){aidGripLatch=false;stickyAidButtons=false;}
  mouthGate.reset();
  if((old^mask)&fc_features::bit(fc_features::Reload)){if(!feature(fc_features::Reload)&&nativeReloadCaptured)build_adapter::setManualReload(nativeReloadManual);reloadPreferenceApplied=false;}
  log("FEATURES applied core=%08X addons-next-launch=%08X",mask&fc_features::coreMask,mask&fc_features::addonMask);
 }
 void featureTick(const Input& current,ULONGLONG now){
  pendingControl=-1;mouthGate.reset();if(now<featureInputAt)return;
  auto& r=current.h[1];auto& l=current.h[0];auto& pr=previous.h[1];auto& pl=previous.h[0];
  int event=featureEvent.exchange(-1);bool cancel=r.secondary&&!pr.secondary;
  std::lock_guard<std::mutex> lock(featureMutex);
  if(event>=0&&event<=fc_features::Count+4)featureSelection.selected=event;
  int hover=featureHover.load();bool press=(r.primary&&!pr.primary)||(l.primary&&!pl.primary);
  bool trigger=(r.trigger>.65f&&pr.trigger<=.65f)||(l.trigger>.65f&&pl.trigger<=.65f);
  if(trigger&&hover>=0){featureSelection.selected=hover;press=true;}
  float axis=std::abs(r.y)>.65f?r.y:l.y;
  if(std::abs(axis)>.65f){featureSelection.selected=(featureSelection.selected+(axis>0?-1:1)+fc_features::Count+3)%(fc_features::Count+3);featureInputAt=now+220;}
  float pageAxis=std::abs(r.x)>.65f?r.x:l.x;
  if(std::abs(pageAxis)>.65f){int nextPage=(int(featureView.page)+(pageAxis>0?1:2))%3;featureSelection.selected=nextPage*8;featureInputAt=now+300;}
  if(press||event>=0){
   int row=featureSelection.selected;
   if(row<fc_features::Count){if(!featureSelection.toggle(row))strcpy_s(featureView.status,fc_features::definitions[row].locked?"Unavailable in this release.":"Required module is unavailable. Check installation.");}
   else if(row==fc_features::Count)featureSelection.clear();
   else if(row==fc_features::Count+1){
    if(fc_features::write(featurePath,featureSelection.pending)){
     bool restart=fc_features::restartNeeded(featureSelection.pending,addonSessionMask);
     applyFeatures(featureSelection.pending);featureMenu=false;heightMenu=false;featureView.visible=0;
     tell(restart?"Features saved. Restart the game to apply addon changes.":"Features saved",true);return;
    }else strcpy_s(featureView.status,"Could not save settings. Selections have not been applied.");
   }else if(row==fc_features::Count+3||row==fc_features::Count+4){
    int nextPage=(int(featureView.page)+(row==fc_features::Count+4?1:2))%3;
    featureSelection.selected=nextPage*8;
   }else cancel=true;
  }
  if(cancel){featureMenu=false;heightMenu=false;featureView.visible=0;tell("Feature changes cancelled",true);return;}
  featureView.pending=featureSelection.pending;featureView.selected=featureSelection.selected;
  if(featureSelection.selected<fc_features::Count)featureView.page=featureSelection.selected/8;
  featureView.restart=fc_features::restartNeeded(featureSelection.pending,addonSessionMask);
 }
 void tick(){
  updateSeatContext();
  native_worldmag_recovery::tick(feature(fc_features::Magazines)&&ready&&loaded&&gestureGameplayAllowed&&!selectorOpen&&!heightMenu&&!weaponTransition&&!controlsEditor);
  if(!ready||!loaded){kill_camera_head::enabled=false;kill_camera_head::requested=false;kill_camera_head::external=false;lamp_shadow_renderer::gameplay=false;++lamp_shadow_renderer::generation;sprintState.suspend();sprint_adapter::factor=1.f;sprintAt=0;sprintNoticeUntil=0;enabled=false;gestureGameplayAllowed=false;return;}auto now=GetTickCount64();if(now-lastLoop<10)return;lastLoop=now;
  if(feature(fc_features::Presentation)&&presentation_adapter::base){
   if(core_perf::enabled)++core_perf::speakerQueries;
   unsigned id=0;Vec namePoint{};
   if(reinterpret_cast<int(__cdecl*)()>(presentation_adapter::base+0xDFE4)()==0x3F1){
    Element e;if(call(dialoguePartner,e)&&e.type==2&&e.value.form){
     auto actorRoot=scene::root(e.value.form);auto head=scene::find(actorRoot,"Bip01 Head");
     if(head){namePoint=scene::world(head).p;auto nativeSettings=reinterpret_cast<unsigned char*(__cdecl*)()>(presentation_adapter::base+0xB235)();
      // Head joint is inside the skull: allow crown height plus a visible air gap.
      namePoint.z+=.32f*(*reinterpret_cast<float*>(nativeSettings+0x28));id=e.value.form->id;}
    }release(e);
   }
   presentation_adapter::setSpeaker(id,namePoint);
  }
  Input current;{std::lock_guard<std::mutex> lock(inputMutex);current=input;}
  if(!nativeReloadCaptured)nativeReloadCaptured=build_adapter::readManualReload(nativeReloadManual);
  if(feature(fc_features::Reload)&&!reloadPreferenceApplied&&nativeReloadCaptured&&build_adapter::setManualReload(!automaticReload)){reloadPreferenceApplied=true;log("RELOAD preference applied: %s",automaticReload?"automatic":"manual");}
  auto player=playerForm();auto root=scene::root(player);bool usable=root&&number(getMenu)==0&&now-current.time<500&&current.h[0].active&&current.h[1].active;
  if(beamRig&&beamRig!=root)clearBeam();
  const bool dead=number(playerDead)!=0;
  kill_camera_head::enabled=false;
  kill_camera_head::dead=dead;
  kill_camera_head::requested=kill_camera_head::enabled.load()&&root&&(dead||number(killCameraQuery)!=0);
  // The adapter additionally refuses cameras inside the head. Gameplay
  // first-person visibility stays unchanged even with this setting enabled.
  Element cameraMode;const bool validCameraMode=call(firstPersonQuery,cameraMode);
  kill_camera_head::external=replayHeadEnabled&&root&&!dead&&((validCameraMode&&cameraMode.type==1&&cameraMode.value.number==0)||replayHeadProbe);release(cameraMode);
  lamp_shadow_renderer::gameplay=usable&&!dead;
  wrist_light::player=player;wrist_light::gameplay=feature(fc_features::Light)&&usable&&number(playerDead)==0&&!selectorOpen&&!controlsEditor;
  if(wrist_light::gameplay){
   if(lightRestorePending&&wrist_light::config.enabled){
    if(!lightRestoreAt)lightRestoreAt=now+500;
    if(wrist_light::nativeLight())lightRestorePending=false;
    else if(now>=lightRestoreAt){number(lightOn);lightRestorePending=false;log("WRIST LIGHT restored after reload");}
   }
   if(!lightRestorePending)lightWasOn=wrist_light::nativeLight()!=nullptr;
  }
  if(feature(fc_features::Light)&&usable&&wrist_light::config.enabled&&!wrist_light::beamPin.node&&now>=beamRetry){
   beamRetry=now+3000;
   if(!beamRig){beamRig=root;scene::retain(beamRig);}
   if(!beamInserted)beamInserted=scene::find(root,"FCWristBeamMount")||number(insert,"FCWristBeamMount")!=0;
   if(beamInserted&&!beamAttached)beamAttached=scene::find(root,"FCWristBeam")||number(attach,"FCWristBeamMount|FilthyCasual\\wrist-beam.nif")!=0;
   if(beamAttached){bool bound=wrist_light::bindBeam(scene::find(root,"FCWristBeam"));log("WRIST LIGHT beam insert=%d attach=%d bind=%d",beamInserted,beamAttached,bound);if(!bound)beamAttached=false;}
  }
  tickSprint(current,usable,now);
  tickBottleImpacts(usable);
  enabled=usable;
  tickBook(current,usable,now);
  finger_adapter::bookContact=bookActive.load()?bookState.hand:-1;
  auto grabbedBook=static_cast<Form*>(build_adapter::grabbedItem());
  bool isPhysicalBook=usable&&feature(fc_features::Book)&&bookForm&&grabbedBook&&scene::field<Form*>(grabbedBook,0x20)==bookForm;
  finger_adapter::physicalBookHand=isPhysicalBook?build_adapter::grabbedHand():-1;
  gestureGameplayAllowed=usable&&!weaponTransition;
  comfortGameplayAt=usable&&seatContext==0&&!selectorOpen&&!heightMenu&&!featureMenu&&!weaponTransition&&!controlsEditor?now:0;
  native_worldmag_recovery::tick(feature(fc_features::Magazines)&&usable&&!selectorOpen&&!heightMenu&&!weaponTransition&&!controlsEditor);
  static ULONGLONG nextGestureLog=0;static unsigned gestureLogs=0;
  if(now>=nextGestureLog&&gestureLogs<90){nextGestureLog=now+2000;++gestureLogs;
   std::lock_guard<std::mutex> lock(inputMutex);
   log("GESTURE gameplay=%d usable=%d controls=%d L(active=%d thumb=%d index=%d grip=%.2f trigger=%.2f applied=%lu) R(active=%d thumb=%d index=%d grip=%.2f trigger=%.2f applied=%lu)",int(gestureGameplayAllowed.load()),usable,int(controlsActive.load()),gestures[0].active,gestures[0].thumb,gestures[0].index,input.h[0].grip,input.h[0].trigger,finger_adapter::applied[0].load(),gestures[1].active,gestures[1].thumb,gestures[1].index,input.h[1].grip,input.h[1].trigger,finger_adapter::applied[1].load());
  }
  if(featureMenu){if(!usable){featureMenu=false;heightMenu=false;std::lock_guard<std::mutex> lock(featureMutex);featureView.visible=0;}else featureTick(current,now);previous=current;return;}
  if(controlsEditor.load()){mouthGate.reset();pendingControl=-1;previous=current;return;}
  int wristAction=-1;
  int action=pendingControl.exchange(-1);
  if(selectorOpen||heightMenu||weaponTransition||action>=0)mouthGate.reset();
  if(usable&&!selectorOpen&&!heightMenu&&!weaponTransition)wristAction=action;
  if(!usable){mouthGate.reset();stickyAidButtons=false;redrawAfter=now+250;if(selectorOpen)close();if(aidDropPending)forgetHeldAid();heightMenu=false;weaponTransition=false;pendingWeapon={};previous=current;return;}
  if(wristAction>=0){
   log("HOLD CONTROL action=%d",action);
   if(wristAction==wrist_controls::Weapons)open(1);
   else if(wristAction==wrist_controls::Consumables)open(0);
   else if(wristAction==wrist_controls::Reload)toggleReload();
   else if(wristAction==hold_controls::Tutorial)setTutorial(!tutorialEnabled);
   else if(wristAction==wrist_controls::Settings){openSettings();}
   previous=current;return;
  }
  bool manualNow=false;
  if(bookActive){mouthGate.reset();previous=current;return;}
  if(build_adapter::readManualReload(manualNow)){
   bool wantPouch=feature(fc_features::Pouch)&&seatedAmmoPouch&&manualNow;
   if(build_adapter::setSeatedPouch(wantPouch,pouchForward,pouchSide,pouchBelow,.8f)&&pouchApplied!=wantPouch){pouchApplied=wantPouch;log("SEATED POUCH: %s",pouchApplied?"native torso anchor applied":"native position restored");}
  }
  if(weaponTransition){
   if(now>=pendingAt){
    if(pendingAid){
     if(pendingStage==1){
      if(!holdAid(pendingWeapon)){weaponTransition=false;tell("Item could not be placed in hand");}
      else if(pendingWeapon.id==0x15169){weaponTransition=false;tell("Stimpak ready in right hand");}
      else {pendingStage=3;pendingAt=now+150;aidDeadline=now+2000;}
     }else{
      const bool preservePose=held_pose_adapter::pending;
      Vec head;
      int result=0;
      if((!preservePose||placeEmptyForPickup(selectedRef))&&pos("Bip01 Head",1,head))
       result=build_adapter::grabSelected(playerForm(),&head.x,selectedRef);
      if(build_adapter::grabbedItem()==selectedRef&&build_adapter::grabbedHand()==1){aidDropPending=false;weaponTransition=false;held_pose_adapter::pending=false;if(current.h[1].grip>.65f)aidGripLatch=false;stickyAidButtons=aidGripLatch.load();log("NATIVE AID handoff %08X result=%d",heldAid.id,result);tell(heldAid.name+" in right hand");}
      else if(now>=aidDeadline){clearAid();weaponTransition=false;tell("Item returned to inventory; native handoff unavailable");}
      else if(preservePose){pendingAt=now+30;}
      else {pendingAt=now+100;auto palm=scene::palm(root,1);number(moveRef,selectedRef,bits(palm.x),bits(palm.y),bits(palm.z));}
     }
    }else if(pendingStage==1){
     // A holstered copy is already equipped. Do not enqueue a second native
     // equip/unequip transaction for the same item; only request its draw.
     Element currentEquipped;u32 currentID=0;
     if(call(getEquipped,currentEquipped)&&currentEquipped.type==2&&currentEquipped.value.form)currentID=currentEquipped.value.form->id;
     release(currentEquipped);
     double result=currentID==pendingWeapon.id?1:number(equip,pendingWeapon.form,0);
     number(holster);log("TOUCH EQUIP %08X hand=%d result=%.0f already=%d",pendingWeapon.id,pendingHand,result,currentID==pendingWeapon.id);
     if(!result){weaponTransition=false;tell("Weapon could not be equipped");}else{pendingStage=2;pendingAt=now+100;}
    }else{weaponHand=pendingHand;selectedWeaponID=pendingWeapon.id;if(bookForm&&pendingWeapon.id==bookForm->id)bookRouted=true;redrawAfter=now+1000;log("Draw request: %s",build_adapter::requestHandDraw(pendingHand));number(draw);weaponTransition=false;tell("Equipped: "+pendingWeapon.name);pendingWeapon={};}
   }previous=current;return;
  }
  // A deliberate fresh squeeze takes over from the selector's convenience
  // hold. Native release/holster behavior must not be undone by menu resume.
  if(!selectorOpen&&!heightMenu&&weaponHand==1&&current.h[1].grip>.65f&&previous.h[1].grip<=.65f){
   weaponHand=-1;selectedWeaponID=0;log("Weapon grip: native control resumed");
  }
  // Remember only selection intent, never an inventory copy or save record.
  // TTWVR may holster on dialogue/menu teardown after our forced grip stops.
  // Resume its native draw only for the same equipped weapon in live gameplay.
  if(selectedWeaponID&&weaponHand==1&&now>=redrawAfter&&dialogueTarget&&combatDisabled&&playerDead){
   Element equipped;u32 id=0;if(call(getEquipped,equipped)&&equipped.type==2&&equipped.value.form)id=equipped.value.form->id;release(equipped);
   if(id!=selectedWeaponID||(id==0x4346&&!number(weaponOut))){
    // Once TTR puts the cue away, the selector must forget its convenience hold.
    selectedWeaponID=0;weaponHand=-1;
   }
   else if(!selectorOpen&&!heightMenu&&!number(weaponOut)){
    Element target;bool talking=call(dialogueTarget,target)&&target.type==2&&target.value.form;release(target);
    if(!talking&&!number(combatDisabled)&&!number(playerDead)){
     log("RESUME selected weapon %08X: %s",id,build_adapter::requestHandDraw(1));number(draw);redrawAfter=now+1000;
    }
   }
  }
  const auto& l=current.h[0];const auto& r=current.h[1];const auto& oldL=previous.h[0];const auto& oldR=previous.h[1];
  bool b=r.secondary&&!oldR.secondary,a=r.primary&&!oldR.primary,x=l.primary&&!oldL.primary;
  if(heightMenu){settingsTick(current,now);previous=current;return;}
  if(b&&selectorOpen){close();tell("Selector closed");previous=current;return;}
  if(selectorOpen){
   if(wheelXRValid&&current.headValid&&scene::distance(current.head,wheelXRHead)>1.2f){close();tell("Selector dismissed");previous=current;return;}
   if(!wheelCarrier&&l.x*l.x+l.y*l.y>.16f){close();tell("Selector dismissed");previous=current;return;}
   if(wheelCarrierLost||(wheelCarrier?!mountedVehicle():(scene::cell(player)!=wheelCell||scene::distance(scene::world(root).p,wheelRoot)>settings.walkAway))){close();tell("Selector dismissed");previous=current;return;}
   Vec head;if(!pos("Bip01 Head",1,head)||scene::distance(head,wheelHead)>settings.walkAway){close();tell("Selector dismissed");previous=current;return;}
   if(a||x){int pages=std::max(1,int((items.size()+slots-1)/slots));page=(page+(a?1:pages-1))%pages;if(!buildPage())close();previous=current;return;}
   int pick=-1,hand=-1;float nearest=settings.reach;
   for(int h=0;h<2;h++){Vec hp;if(!pos(h?"Bip01 R Hand":"Bip01 L Hand",1,hp))continue;
    Vec points[3]{hp,scene::palm(root,h),hp};if(auto tip=scene::find(root,h?"Bip01 R Finger12":"Bip01 L Finger12"))points[2]=scene::world(tip).p;
    for(int i=0;i<previewCount;i++){auto& preview=previews[i];auto node=preview.pin.node;if(!node)continue;float distance=1e20f;for(auto point:points)distance=std::min(distance,preview.mesh.empty()?scene::distance(point,scene::world(node).p):scene::touchDistance(preview.mesh,point));
     if(distance>settings.reach+.7f)touchMustLeave[h][i]=false;
     if(!touchMustLeave[h][i]&&distance<nearest){nearest=distance;pick=i;hand=h;}}}
   if(pick!=selected){selected=pick;if(pick==capSlot)tell("Empty hand");else if(pick>=0){auto& item=items[page*slots+pick];tell(item.name+" x"+std::to_string(item.count));}else tell("Touch an item");}
   if(pick!=touchSlot||hand!=touchHand){touchSlot=pick;touchHand=hand;touchSince=now;}
   // Short contact debounce rejects a one-frame sweep. No grip press required.
   if(pick>=0&&now-openedAt>180&&now-touchSince>=70){if(pick==capSlot)selectNone();else selectItem(page*slots+pick);}
  }else if(aidHand>=0){
   bool held=heldAid.id==0x15169?build_adapter::stimHand()==1:build_adapter::grabbedItem()==selectedRef;
   if(!held)forgetHeldAid();
   else if(aidGripLatch&&r.grip>.65f){aidGripLatch=false;stickyAidButtons=false;mouthGate.reset();log("Aid grip: native control resumed");}
   else if(aidGripLatch&&heldAid.id!=0x15169){
    stickyAidButtons=true;
    if(a||x){if(build_adapter::stowSelectedGrab(player,selectedRef)){forgetHeldAid();tell("Item returned to inventory");}else mouthGate.reset();}
    else if(feature(fc_features::Mouth)) {
     auto head=scene::find(root,"Bip01 Head");auto itemRoot=scene::root(selectedRef);
     mouthDistance=-1;
     Vec mouth{};
     if(heldFood&&head&&itemRoot&&current.mouthTime&&now-current.mouthTime<250&&physical_reload_adapter::worldTrackingPoint(current.mouth,scene::world(head).p,mouth)){
      foodMouthPoint=mouth;
      if(core_perf::enabled)++core_perf::mouthMeshes;
      std::vector<scene::Box> mesh;scene::boxes(itemRoot,mesh);
      mouthDistance=mesh.empty()?scene::distance(scene::world(itemRoot).p,mouth):scene::touchDistance(mesh,mouth);
     }
     if(mouthGate.update(feature(fc_features::Mouth)&&heldFood&&mouthDistance>=0,mouthDistance,now))eatStickyFood();
    }
   }
  }
  previous=current;
 }

 bool querySkeletal(int h,float* curls){if(h<0||h>1||!gestureGameplayAllowed)return false;std::lock_guard<std::mutex> lock(comfortMutex);if(!comfortConfig.fingers||!skeletalAt[h]||GetTickCount64()-skeletalAt[h]>150)return false;memcpy(curls,skeletalCurls[h],sizeof skeletalCurls[h]);return true;}
 bool queryGesture(int hand,float& thumb,float& index,float& lift,float& freeGrip){
  if(((!feature(fc_features::Fingers)||!fingerGestures)&&!bookActive)||!gestureGameplayAllowed.load())return false;
  std::lock_guard<std::mutex> lock(inputMutex);auto now=GetTickCount64();auto& g=gestures[hand];
  if(!g.active||now-g.time>250){g.blendTime=0;return false;}
  float trigger=std::clamp(now-physicalAt[hand]<250?physicalTrigger[hand]:input.h[hand].trigger,0.f,1.f);
  float grip=std::clamp(now-physicalAt[hand]<250?physicalGrip[hand]:input.h[hand].grip,0.f,1.f);
  auto vehicleModule=GetModuleHandleW(L"FilthyCasualVehicles.dll");
  // All free fingers need the undamped physical squeeze near a weapon, too.
  // finger_adapter's per-hand ownership checks exclude actual grips/contacts.
  freeGrip=grip;
  // Thumb contact closes from relaxed to the full authored fist as grip rises.
  // A lifted thumb stays extended, independently of the finger grip.
  float targetThumb=g.thumb?(.20f+.80f*grip):.12f;
  float targetIndex=g.index?std::max(grip,.20f+.80f*trigger):trigger;
  float targetLift=g.thumb?0.f:grip;
  auto contact=vehicleModule?reinterpret_cast<bool(__cdecl*)(int)>(GetProcAddress(vehicleModule,"FC_VehicleHandContact")):nullptr;
  if(contact&&contact(hand)){targetThumb=.62f;targetIndex=.65f+.25f*trigger;targetLift=0;freeGrip=.72f;}
  auto field=GetModuleHandleW(L"FilthyCasualEnclave.dll");auto fieldContact=field?reinterpret_cast<bool(__cdecl*)(int)>(GetProcAddress(field,"FC_FieldHandContact")):nullptr;
  if(fieldContact&&fieldContact(hand)){targetThumb=.72f;targetIndex=hand?.55f+.25f*trigger:.72f;targetLift=0;freeGrip=.82f;}


  float dt=g.blendTime?std::min(float(now-g.blendTime)*.001f,.1f):.016f;g.blendTime=now;
  float blend=1.f-std::exp(-dt/0.09f);
  g.thumbCurl+=(targetThumb-g.thumbCurl)*blend;g.indexCurl+=(targetIndex-g.indexCurl)*blend;
  g.lift+=(targetLift-g.lift)*blend;
  thumb=g.thumbCurl;index=g.indexCurl;lift=g.lift;return true;
 }
 void testMailbox(){
  // Only enabled by the private simulator launcher, on the game's own thread.
  static char dir[MAX_PATH]{};static bool checked=false;static ULONGLONG last=0;
  if(!checked){GetEnvironmentVariableA("SEATED_TEST_DIR",dir,MAX_PATH);checked=true;}
  if(!dir[0]||!GetModuleHandleW(L"bvr_xrsim32.dll")||GetTickCount64()-last<200)return;
  last=GetTickCount64();std::string path=std::string(dir)+"\\game-command.txt";
  std::ifstream file(path);if(!file)return;std::vector<std::string> lines;std::string line;
  while(std::getline(file,line))if(!line.empty())lines.push_back(line);file.close();
  if(!DeleteFileA(path.c_str()))return;
  for(const auto& cmd:lines){
   if(cmd=="sc_lamp_unshadowed"||cmd=="sc_lamp_shadowed"){
    projected_lighting::lampShadow=cmd=="sc_lamp_shadowed"?lamp_shadow_renderer::bindLamp:nullptr;
    continue;
   }
   if(cmd=="sc_lamp_contact"){
    auto& map=lamp_shadow_renderer::lampMap;
    auto device=projected_render_adapter::device();
    IDirect3DSurface9* staging=nullptr;
    float distance=0;
    if(device&&lamp_shadow_renderer::valid&&map.surface&&SUCCEEDED(device->CreateOffscreenPlainSurface(map.w,map.h,D3DFMT_R32F,D3DPOOL_SYSTEMMEM,&staging,nullptr))){
     D3DLOCKED_RECT locked{};
     if(SUCCEEDED(device->GetRenderTargetData(map.surface,staging))&&SUCCEEDED(staging->LockRect(&locked,nullptr,D3DLOCK_READONLY))){
      const auto row=reinterpret_cast<const float*>(static_cast<const char*>(locked.pBits)+(map.h/2)*locked.Pitch);
      const float z=row[map.w/2], farPlane=wrist_light::config.range;
      if(z>=0&&z<1)distance=farPlane/(farPlane-z*(farPlane-1));
      staging->UnlockRect();
     }
     staging->Release();
    }
    std::ofstream result(std::string(dir)+"\\lamp-contact.json");
    result<<"{\"shadowDistance\":"<<distance<<"}";continue;
   }
   if(cmd.rfind("sc_light_power ",0)==0){wrist_light::config.brightness=std::clamp(float(atof(cmd.c_str()+15)),0.f,5.f);continue;}
   if(cmd=="sc_projected_on"||cmd=="sc_projected_off"){wrist_light::config.projected=cmd=="sc_projected_on";continue;}
    if(cmd=="sc_light_baseline"||cmd=="sc_light_enhanced"){wrist_light::baselineComparison=cmd=="sc_light_baseline";log("LIGHT A/B baseline=%d",wrist_light::baselineComparison);continue;}
   if(cmd.rfind("sc_target ",0)==0){
    unsigned refId=0;sscanf_s(cmd.c_str()+10,"%x",&refId);
    auto ref=static_cast<Form*>(physical_reload_adapter::lookup(refId));
    auto b=build_adapter::heldModule();Vec centre{},tracking{};float extent=0;
    bool ok=ref&&b;
    if(ok){auto cell=scene::field<Form*>(ref,0x40);auto base=scene::field<Form*>(ref,0x20);log("TARGET ref=%08X flags=%08X cell=%08X base=%08X pos=%.2f,%.2f,%.2f root=%p",ref->id,scene::field<u32>(ref,8),cell?cell->id:0,base?base->id:0,scene::field<float>(ref,0x30),scene::field<float>(ref,0x34),scene::field<float>(ref,0x38),scene::root(ref));}
    if(ok)ok=reinterpret_cast<bool(__cdecl*)(void*,Vec*,float*)>(b+0x1C8650)(ref,&centre,&extent);
    if(ok){auto manager=reinterpret_cast<void*(__cdecl*)()>(b+0x141CD)();
     reinterpret_cast<Vec*(__thiscall*)(void*,Vec*,const Vec*)>(b+0x711040)(manager,&tracking,&centre);}
    std::ofstream probe(std::string(dir)+"\\target-probe.json");
    probe<<"{\"ok\":"<<(ok?"true":"false")<<",\"tracking\":["<<tracking.x<<","<<tracking.y<<","<<tracking.z<<"],\"extent\":"<<extent<<"}";
    continue;
   }
   if(cmd=="sc_perf_begin"){core_perf::begin();continue;}
   if(cmd=="sc_perf_end"){core_perf::finish(std::string(dir)+"\\core-perf.json");continue;}
   if(cmd=="sc_fingers_off"||cmd=="sc_fingers_on"){fingerGestures=cmd=="sc_fingers_on";continue;}
   if(cmd=="sc_rig_snapshot"){std::ofstream out(std::string(dir)+"\\rig-state.json");out<<"[";bool first=true;field_rig_debug::dump(out,scene::root(playerForm()),first);out<<"]";continue;}
#ifdef FC_SCOPE_DIAGNOSTICS
   if(cmd=="sc_scope_guard"||cmd=="sc_scope_native"){scope_surface_adapter::enabled=(cmd=="sc_scope_guard");continue;}
#endif
   if(cmd=="sc_scope_inline"||cmd=="sc_scope_separate"){presentation_adapter::testScopeSchedule(cmd=="sc_scope_inline");continue;}
   if(cmd=="sc_controls"||cmd=="sc_controls_state"){
    auto m=GetModuleHandleW(L"SeatedInput.dll");if(m){if(cmd=="sc_controls"){using F=void(__cdecl*)();auto f=reinterpret_cast<F>(GetProcAddress(m,"SC_OpenControls"));if(f)f();}else{using F=void(__cdecl*)(char*,unsigned);auto f=reinterpret_cast<F>(GetProcAddress(m,"SC_ControlsDebug"));if(f){char data[4096]{};f(data,sizeof(data));std::ofstream out(std::string(dir)+"\\controls-state.json");out<<data;}}}
   }
   else if(cmd.rfind("sc_track_bottle ",0)==0){unsigned id=0;sscanf_s(cmd.c_str()+16,"%x",&id);trackBottle(static_cast<Form*>(physical_reload_adapter::lookup(id)));bottleTrace=true;glass_decal_adapter::diagnostics=true;}
   else if(cmd=="sc_glass_state"){auto center=lastGlassPoint;auto t=physical_reload_adapter::trackingPoint(center);std::ofstream out(std::string(dir)+"\\glass-state.json");out<<"[{\"world\":["<<center.x<<","<<center.y<<","<<center.z<<"],\"tracking\":["<<t.x<<","<<t.y<<","<<t.z<<"]}]";}
   else if(cmd=="sc_state"){
    std::ofstream state(std::string(dir)+"\\addon-state.json");auto rig=scene::root(playerForm());
    state<<"{\"seatContext\":"<<seatContext<<",\"groundHeightOffset\":"<<seatOffsets[0].load()<<",\"pid\":"<<GetCurrentProcessId()<<",\"openHand\":"<<openHand<<",\"aidHand\":"<<aidHand.load()<<",\"weaponHand\":"<<weaponHand.load()<<",\"heightMenu\":"<<(heightMenu?"true":"false")<<",\"heightOffset\":"<<heightOffset.load()<<",\"physicalHeight\":"<<physicalHeight.load()<<",\"menu\":"<<number(getMenu)<<",\"uiEpoch\":"<<uiEpoch.load();
    state<<",\"featureMask\":"<<featureMask.load()<<",\"nativeEmptyAwarded\":"<<nativeEmptyAwarded<<",\"featureMenu\":"<<(featureMenu?"true":"false");
    state<<",\"book\":{\"form\":"<<(bookForm?bookForm->id:0)<<",\"active\":"<<(bookActive?"true":"false")<<",\"visible\":"<<(bookVisible?"true":"false")<<",\"page\":"<<bookState.page<<",\"turn\":"<<bookState.direction<<",\"worldRef\":"<<bookWorldRef<<",\"dropPending\":"<<(bookDropPending?"true":"false")<<"}";
    state<<",\"dialoguePlacements\":"<<presentation_adapter::placements.load()<<",\"dialogueTargetDistance\":"<<presentation_adapter::lastDistance.load()<<",\"dialogueSideDegrees\":"<<presentation_adapter::lastAngle.load();
    tutorialDiagnostics(state);presentation_adapter::diagnostics(state);reload_trace_adapter::diagnostics(state);physical_reload_adapter::diagnostics(state);
    {const auto d=native_worldmag_recovery::readInsertion();state<<",\"insertion\":{\"valid\":"<<(d.valid?"true":"false");
     if(d.valid){auto t=physical_reload_adapter::trackingPoint({d.rest.p[0],d.rest.p[1],d.rest.p[2]});auto h=physical_reload_adapter::trackingPoint({d.held.p[0],d.held.p[1],d.held.p[2]});
      state<<",\"target\":["<<t.x<<","<<t.y<<","<<t.z<<"],\"held\":["<<h.x<<","<<h.y<<","<<h.z<<"]";
      scene::Transform rest{},held{};memcpy(&rest,&d.rest,sizeof(rest));memcpy(&held,&d.held,sizeof(held));
      state<<",\"restBasis\":";physical_reload_adapter::trackingBasis(state,rest);state<<",\"heldBasis\":";physical_reload_adapter::trackingBasis(state,held);}
     state<<",\"pouchValid\":"<<(d.pouchValid?"true":"false");if(d.pouchValid){auto p=physical_reload_adapter::trackingPoint({d.pouch[0],d.pouch[1],d.pouch[2]});state<<",\"pouchTarget\":["<<p.x<<","<<p.y<<","<<p.z<<"]";}state<<"}";}

    {DialogueAnchor anchor{};bool valid=presentation_adapter::readSpeaker(anchor);state<<",\"speakerSourceId\":"<<presentation_adapter::speakerId<<",\"speakerFrameUpdates\":"<<presentation_adapter::speakerFrameUpdates.load()<<",\"speakerNameActive\":"<<(valid?"true":"false")<<",\"speakerNameTracking\":["<<anchor.position[0]<<","<<anchor.position[1]<<","<<anchor.position[2]<<"]";
     using Debug=void(__cdecl*)(char*,unsigned);auto layer=GetModuleHandleW(L"SeatedInput.dll");auto debug=layer?reinterpret_cast<Debug>(GetProcAddress(layer,"SC_LabelDebug")):nullptr;
     if(debug){char json[512]{};debug(json,sizeof(json));state<<",\"speakerNameCrop\":"<<json;}}
    state<<",\"nativeGripPoints\":[";for(int h=0;h<2;h++){auto p=physical_reload_adapter::gripPoint(h);if(h)state<<",";state<<"["<<p.x<<","<<p.y<<","<<p.z<<"]";}state<<"]";
    state<<",\"nativeGripTracking\":[";for(int h=0;h<2;h++){auto p=physical_reload_adapter::trackingPoint(physical_reload_adapter::gripPoint(h));if(h)state<<",";state<<"["<<p.x<<","<<p.y<<","<<p.z<<"]";}state<<"]";
    state<<",\"physicalParts\":[";bool firstPart=true;
    for(const auto& m:physicalMags){auto ref=physical_reload_adapter::lookup(m.refID);auto root=ref?scene::root(ref):nullptr;if(!root||!m.attached||(m.fullModel?m.visual!=root:scene::parent(m.visual)!=root))continue;
     std::vector<scene::Box> mesh;scene::boxes(m.visual,mesh);if(mesh.empty())continue;auto c=scene::boxCenter(mesh);
     if(!firstPart)state<<",";firstPart=false;auto bound=scene::field<float*>(root,0x20);auto track=physical_reload_adapter::trackingPoint(bound?Vec{bound[0],bound[1],bound[2]}:c);state<<"{\"ref\":"<<m.refID<<",\"stackCount\":"<<physical_reload_adapter::stackCount(ref)<<",\"center\":["<<c.x<<","<<c.y<<","<<c.z<<"],\"bound\":["<<(bound?bound[0]:0)<<","<<(bound?bound[1]:0)<<","<<(bound?bound[2]:0)<<","<<(bound?bound[3]:0)<<"],\"tracking\":["<<track.x<<","<<track.y<<","<<track.z<<"]";physical_reload_adapter::physicsDiagnostics(state,root);physical_reload_adapter::meshDiagnostics(state,root);state<<"}";
    }state<<"]";
    auto engineGrab=scene::field<Form*>(playerForm(),0x638);state<<",\"engineGrabRef\":"<<(engineGrab?engineGrab->id:0);
    auto grab=static_cast<Form*>(build_adapter::grabbedItem());auto grabRoot=scene::root(grab);
    state<<",\"nativeStimHand\":"<<build_adapter::stimHand()<<",\"nativeGrabHand\":"<<build_adapter::grabbedHand()<<",\"nativeGrabRef\":"<<(grab?grab->id:0)<<",\"nativeGrabBase\":"<<(grab&&scene::field<Form*>(grab,0x20)?scene::field<Form*>(grab,0x20)->id:0)<<",\"nativeGrabScale\":"<<(grabRoot?scene::world(grabRoot).scale:0)<<",\"aidGripLatch\":"<<(aidGripLatch?"true":"false");
    if(grabRoot&&grab->id==lastEmptyBottle&&lastEmptyBottle){scene::Transform grip{};if(held_pose_adapter::grip(grip)){auto pose=held_pose_adapter::relative(grip,scene::world(grabRoot));float rotationError=0;for(int i=0;i<3;i++)for(int j=0;j<3;j++)rotationError=std::max(rotationError,std::abs(pose.r[i][j]-held_pose_adapter::localPose.r[i][j]));state<<",\"emptyPoseError\":"<<scene::distance(pose.p,held_pose_adapter::localPose.p)<<",\"emptyRotationError\":"<<rotationError;}}
    state<<",\"bottleBreaks\":"<<bottleBreaks<<",\"glassPending\":"<<glass_decal_adapter::count()<<",\"trackedBottles\":"<<fragileBottles.size()<<",\"emptyBottleRef\":"<<lastEmptyBottle<<",\"heldFood\":"<<(heldFood?"true":"false")<<",\"mouthDistance\":"<<mouthDistance<<",\"mouthArmed\":"<<(mouthGate.armed?"true":"false")<<",\"freePoseWeight\":["<<finger_adapter::freePoseWeight[0].load()<<","<<finger_adapter::freePoseWeight[1].load()<<"]";
    state<<",\"nativeApproachWeight\":["<<finger_adapter::nativeApproachWeight[0].load()<<","<<finger_adapter::nativeApproachWeight[1].load()<<"]";
    state<<",\"bookHoldingHand\":"<<bookState.hand<<",\"nativeWeaponHand\":"<<build_adapter::drawnWeaponHand();
    {native_worldmag_recovery::WorldRef refs[256]{};unsigned count=native_worldmag_recovery::readWorldRefs(refs,256);
     state<<",\"nativeWorldMags\":[";
     for(unsigned i=0;i<count;i++){const auto& r=refs[i];if(i)state<<",";
      state<<"{\"ref\":"<<r.id<<",\"ammo\":"<<r.ammoID<<",\"count\":"<<r.recordedCount<<",\"actualCount\":"<<r.actualCount
           <<",\"valid\":"<<int(r.valid)<<",\"rootValid\":"<<int(r.rootValid)<<",\"boundValid\":"<<int(r.boundValid)
           <<",\"center\":["<<r.rootCenter[0]<<","<<r.rootCenter[1]<<","<<r.rootCenter[2]<<"],\"bound\":["<<r.bound[0]<<","<<r.bound[1]<<","<<r.bound[2]<<","<<r.bound[3]<<"]";
      if(r.boundValid){auto point=physical_reload_adapter::trackingPoint({r.bound[0],r.bound[1],r.bound[2]});
       if(std::isfinite(point.x)&&std::isfinite(point.y)&&std::isfinite(point.z))state<<",\"tracking\":["<<point.x<<","<<point.y<<","<<point.z<<"]";}
      state<<"}";}
     state<<"]";}
    {const auto& d=native_worldmag_recovery::diagnostics;
     auto chamber=tutorial_adapter::read();state<<",\"chamberKnown\":"<<(chamber.chamberKnown?"true":"false")<<",\"chambered\":"<<(chamber.chambered?"true":"false");
     state<<",\"worldmagRecovery\":{\"installed\":"<<int(d.installed)<<",\"poisoned\":"<<int(d.poisoned)
          <<",\"candidate\":"<<d.candidate<<",\"transferred\":"<<d.transferred<<",\"rounds\":"<<d.rounds<<",\"hand\":"<<d.hand
          <<",\"firstEntries\":"<<d.firstEntries<<",\"secondEntries\":"<<d.secondEntries<<",\"prepared\":"<<d.prepared
          <<",\"attempts\":"<<d.attempts<<",\"committed\":"<<d.committed<<",\"failures\":"<<d.failures
          <<",\"inventoryBefore\":"<<d.inventoryBefore<<",\"inventoryAfter\":"<<d.inventoryAfter<<",\"reason\":\""<<d.reason<<"\"}";}
    bool manual=false;bool reloadKnown=build_adapter::readManualReload(manual);state<<",\"manualReloadKnown\":"<<(reloadKnown?"true":"false")<<",\"manualReload\":"<<(manual?"true":"false")<<",\"seatedPouchApplied\":"<<(pouchApplied?"true":"false");
    build_adapter::PouchDiagnostics pouch;
    if(build_adapter::readPouchDiagnostics(pouch)){
     auto node=[&](const build_adapter::PouchNodeDiagnostics& n){state<<"{\"readable\":"<<(n.readable?"true":"false")<<",\"culled\":"<<(n.culled?"true":"false")<<",\"position\":["<<n.world[0]<<","<<n.world[1]<<","<<n.world[2]<<"],\"scale\":"<<n.scale<<"}";};
     state<<",\"pouch\":{\"spareEnabled\":"<<(pouch.spareEnabled?"true":"false")<<",\"anchor\":"<<(pouch.torsoAnchor?"true":"false")<<",\"shown\":"<<(pouch.primaryShown?"true":"false")<<",\"mode\":"<<pouch.nativeMode<<",\"unitsPerMetre\":"<<pouch.metresToUnits<<",\"primary\":";node(pouch.primary);state<<",\"copy\":";node(pouch.copy);
     state<<",\"heldRounds\":"<<pouch.heldRounds<<",\"detachedMode\":"<<pouch.detachedMode<<",\"detachedRounds\":"<<pouch.detachedRounds<<",\"installedMode\":"<<pouch.installedMode<<",\"installedRounds\":"<<pouch.installedRounds<<",\"discardAmmo\":"<<(pouch.discardAmmo?"true":"false")<<",\"flightAge\":"<<pouch.flightAge;
     if(pouch.primary.readable){auto t=scene::world(pouch.primaryNode);auto center=t.p;if(pouch.spareCenterValid)center=center+scene::rotate(t,{pouch.spareLocalCenter[0],pouch.spareLocalCenter[1],pouch.spareLocalCenter[2]})*t.scale;state<<",\"center\":["<<center.x<<","<<center.y<<","<<center.z<<"]";}
     state<<",\"roundFlags\":[";for(int i=0;i<5;i++){if(i)state<<",";state<<unsigned(pouch.roundFlags[i]);}state<<"]";
     state<<",\"roundTargetValid\":"<<(pouch.roundTargetValid?"true":"false");
     if(pouch.roundTargetValid){auto q=physical_reload_adapter::trackingPoint(tutorial_adapter::position(pouch.weaponRoot,{pouch.roundTargetLocal[0],pouch.roundTargetLocal[1],pouch.roundTargetLocal[2]}));state<<",\"roundTargetTracking\":["<<q.x<<","<<q.y<<","<<q.z<<"]";}
     state<<",\"heldBoundValid\":"<<(pouch.primary.boundValid?"true":"false");
     if(pouch.primary.boundValid){auto q=physical_reload_adapter::trackingPoint({pouch.primary.bound[0],pouch.primary.bound[1],pouch.primary.bound[2]});state<<",\"heldBoundTracking\":["<<q.x<<","<<q.y<<","<<q.z<<"]";}
     state<<"}";
    }
    auto vec=[&](Vec p){state<<"["<<p.x<<","<<p.y<<","<<p.z<<"]";};
    state<<",\"trackedHead\":";{std::lock_guard<std::mutex> lock(inputMutex);vec(input.head);}state<<",\"mouthPoint\":";vec(foodMouthPoint);state<<",\"heldCenter\":";
    {std::vector<scene::Box> mesh;if(grabRoot)scene::boxes(grabRoot,mesh);auto center=mesh.empty()?Vec{}:scene::boxCenter(mesh);vec(center);state<<",\"heldTracking\":";vec(physical_reload_adapter::trackingPoint(center));state<<",\"mouthTracking\":";vec(physical_reload_adapter::trackingPoint(foodMouthPoint));}
    state<<",\"wrist\":{\"attached\":"<<(wristNode?"true":"false")<<",\"valid\":"<<(wristFrame.valid?"true":"false")<<",\"facing\":"<<(wristFrame.facing?"true":"false")<<",\"tip\":";vec(wristFrame.fingertip);state<<",\"origin\":";vec(wristFrame.world.p);state<<",\"normal\":";vec(wrist_controls::axis(wristFrame.world,2));state<<",\"targets\":[";for(int i=0;i<4;i++){if(i)state<<",";vec(wrist_controls::buttonWorld(wristFrame,i));}state<<"]}";
    state<<",\"wristEye\":";vec(wristFrame.eye);
    state<<",\"leftFingertips\":[";const char* fingers[]={"Bip01 L Thumb12","Bip01 L Finger12","Bip01 L Finger22","Bip01 L Finger32","Bip01 L Finger42"};
    for(int i=0;i<5;i++){if(i)state<<",";auto n=scene::find(rig,fingers[i]);vec(n?scene::world(n).p:Vec{});}state<<"]";
    state<<",\"gestureGameplayAllowed\":"<<(gestureGameplayAllowed?"true":"false")<<",\"fingerGestures\":[";
    {std::lock_guard<std::mutex> lock(inputMutex);for(int h=0;h<2;h++){if(h)state<<",";auto& g=gestures[h];state<<"{\"active\":"<<(g.active?"true":"false")<<",\"thumb\":"<<g.thumbCurl<<",\"index\":"<<g.indexCurl<<",\"lift\":"<<g.lift<<",\"applied\":"<<finger_adapter::applied[h].load()<<",\"tips\":[";
     for(int f=0;f<5;f++){if(f)state<<",";char name[40];sprintf_s(name,"Bip01 %c %s",h?'R':'L',f==0?"Thumb12":f==1?"Finger12":f==2?"Finger22":f==3?"Finger32":"Finger42");auto n=scene::find(rig,name);auto w=scene::find(rig,h?"Bip01 R Hand":"Bip01 L Hand");vec(n&&w?scene::inverseRotate(scene::world(w),scene::world(n).p-scene::world(w).p):Vec{});}state<<"]}";}}state<<"]";
    state<<",\"wristPlungers\":[";for(int i=0;i<5;i++){if(i)state<<",";auto& p=wristPlungers[i];state<<"{\"found\":"<<(p.node?"true":"false")<<",\"depression\":"<<p.depression<<",\"localZ\":"<<(p.node?scene::local(p.node).p.z:0)<<"}";}state<<"]";
    state<<",\"hands\":[";for(int h=0;h<2;h++){if(h)state<<",";auto wrist=scene::find(rig,h?"Bip01 R Hand":"Bip01 L Hand");vec(wrist?scene::world(wrist).p:Vec{});}state<<"],\"palms\":[";vec(scene::palm(rig,0));state<<",";vec(scene::palm(rig,1));
    state<<"],\"selectorMounted\":"<<(wheelCarrier?"true":"false")<<",\"selectorPalms\":[";vec(selectorPalms[0]);state<<",";vec(selectorPalms[1]);state<<"],\"previews\":[";bool first=true;for(int i=0;i<previewCount;i++){auto& p=previews[i];if(!p.pin.node)continue;if(!first)state<<",";first=false;state<<"{\"slot\":"<<i<<",\"form\":"<<(i==capSlot?15:items[page*slots+i].id)<<",\"center\":";Vec extent;vec(p.mesh.empty()?scene::world(p.pin.node).p:scene::boxCenter(p.mesh,&extent));state<<",\"tracking\":";vec(p.trackingCenter);state<<",\"extent\":";vec(extent);state<<"}";}state<<"]";
    {std::lock_guard<std::mutex> lock(displayMutex);state<<",\"selectorPopups\":"<<(showSelectorPopups?"true":"false")<<",\"popupVisible\":"<<(!displayText.empty()&&(selectorOpen||heightMenu||GetTickCount64()<=displayUntil)?"true":"false");}
    {std::lock_guard<std::mutex> guard(comfortMutex);state<<",\"comfort\":{\"grip\":"<<comfortConfig.grip<<",\"press\":"<<comfortConfig.press<<",\"smoothing\":"<<comfortConfig.smoothing<<"}";}
    state<<",\"settingsPage\":"<<int(settingsPage)<<",\"settingsRow\":"<<settingsNav.selected<<",\"settingsHover\":"<<settingsHover.load()<<",\"sprintSpeed\":"<<sprintConfig.multiplier<<",\"sprintStart\":"<<sprintConfig.start;
    state<<",\"lampShadow\":{\"active\":"<<lamp_shadow_renderer::valid<<",\"maps\":"<<lamp_shadow_renderer::maps<<",\"failures\":"<<lamp_shadow_renderer::failures<<"}";
    state<<",\"sprint\":{\"enabled\":"<<(sprintConfig.enabled?"true":"false")<<",\"active\":"<<(sprintState.active?"true":"false")<<",\"installed\":"<<(sprint_adapter::installed?"true":"false")<<",\"apPerSecond\":"<<sprintConfig.apPerSecond<<",\"showNotification\":"<<(sprintNoticeEnabled?"true":"false")<<",\"notifications\":"<<sprintNoticeCount<<",\"factor\":"<<sprint_adapter::factor<<",\"applied\":"<<sprint_adapter::applied<<",\"ap\":"<<number(sprintAP)<<"}";
    unsigned visibleSheets = 0;
    for (auto mesh : wrist_light::airBeam)
        if (mesh && !(scene::field<unsigned>(mesh,0x30)&1)) ++visibleSheets;
    state<<",\"visibleBeamSheets\":"<<visibleSheets;
    if (wrist_light::lamp) {
        const auto frame = scene::world(wrist_light::lamp);
        const auto lens = frame.p + scene::rotate(frame,{0,-.12f,0}) * frame.scale;
        state<<",\"lampLens\":["<<lens.x<<","<<lens.y<<","<<lens.z<<"]";
    }
    state<<",\"wristLight\":{\"installed\":"<<(wrist_light::installed?"true":"false")<<",\"calls\":"<<wrist_light::calls<<",\"projectedDraws\":"<<projected_lighting::draws<<",\"projectedCpuMeanUs\":"<<(projected_lighting::cpuSamples?projected_lighting::cpuTotalUs/projected_lighting::cpuSamples:0)<<",\"projectedCpuMaxUs\":"<<projected_lighting::cpuMaxUs<<",\"projectedGpuSamples\":"<<projected_lighting::gpuSamples<<",\"projectedGpuMeanMs\":"<<(projected_lighting::gpuSamples?projected_lighting::gpuTotalMs/projected_lighting::gpuSamples:0)<<",\"projectedGpuMaxMs\":"<<projected_lighting::gpuMaxMs<<",\"projectedFailures\":"<<projected_lighting::failures<<",\"projectedResult\":"<<projected_lighting::lastResult<<",\"depthSwaps\":"<<projected_depth_adapter::swaps<<",\"temporalEyeCopy\":"<<(wrist_light::temporalCopy?"true":"false")<<",\"fillSupported\":"<<(lamp_lights::supported?"true":"false")<<",\"fillHits\":"<<wrist_light::fillHits<<",\"fillActive\":"<<lamp_lights::activeCount<<",\"fillAllocated\":"<<lamp_lights::allocations<<",\"lightCpuMeanUs\":"<<(wrist_light::lightingSamples?wrist_light::lightingMicros/wrist_light::lightingSamples:0)<<",\"lightCpuMaxUs\":"<<wrist_light::lightingMaxMicros<<",\"lampBound\":"<<(wrist_light::lamp?"true":"false")<<",\"fixedSocket\":"<<(wrist_light::fixedSocket?"true":"false")<<",\"cordNodes\":"<<std::count_if(std::begin(wrist_light::cable),std::end(wrist_light::cable),[](void* n){return n!=nullptr;})<<",\"cordSpan\":"<<scene::distance(wrist_light::cordStart,wrist_light::cordEnd)<<",\"lampScale\":"<<(wrist_light::lamp?scene::world(wrist_light::lamp).scale:0)<<",\"beamBound\":"<<(wrist_light::beamPin.node?"true":"false")<<",\"beamVisible\":"<<((wrist_light::beamPin.node && !(scene::field<unsigned>(wrist_light::beamPin.node,0x30)&1))?"true":"false")<<",\"stockBeamDetached\":"<<(wrist_light::stockBeamShape&&!scene::parent(wrist_light::stockBeamShape)?"true":"false")<<",\"nativeOn\":"<<(wrist_light::nativeLight()?"true":"false")<<",\"direction\":["<<wrist_light::lastDirection.x<<","<<wrist_light::lastDirection.y<<","<<wrist_light::lastDirection.z<<"],\"draws\":"<<wrist_light::draws<<",\"fail\":"<<wrist_light::fail<<",\"depthFormat\":"<<wrist_light::depthFormat<<",\"hit\":"<<(wrist_light::hit?"true":"false")<<",\"origin\":["<<wrist_light::lastOrigin.x<<","<<wrist_light::lastOrigin.y<<","<<wrist_light::lastOrigin.z<<"],\"target\":["<<wrist_light::lastHit.x<<","<<wrist_light::lastHit.y<<","<<wrist_light::lastHit.z<<"]}";
    state<<"}";log("TEST STATE captured");
   }
   else if(cmd=="sc_weapons")open(1);
   else if(cmd=="sc_aid")open(0);
   else if(cmd=="sc_features"){openFeatures();}
   else if(cmd.rfind("sc_feature_event ",0)==0){featureEvent=atoi(cmd.c_str()+17);}
   else if(cmd=="sc_recording_camera_on"){replayHeadProbe=true;}
   else if(cmd=="sc_recording_camera_off"){replayHeadProbe=false;}
   else if(cmd=="sc_settings"){if(selectorOpen)close();openSettings();}
   else if(cmd=="sc_reload")toggleReload();
   else if(cmd.rfind("sc_pick ",0)==0){unsigned id=0;sscanf_s(cmd.c_str()+8,"%x",&id);for(size_t i=0;i<items.size();i++)if(items[i].id==id){selectItem(static_cast<int>(i));break;}}
   else if(cmd.rfind("expr ",0)==0){auto s=compile("test expression",cmd.c_str()+5,true);Element e;bool ok=call(s,e);log("TEST EXPR %s => ok=%d type=%u number=%g form=%08X",cmd.c_str()+5,ok,e.type,e.type==1?e.value.number:0,e.type==2&&e.value.form?e.value.form->id:0);release(e);}
   else {bool ok=consoleApi&&consoleApi->run(cmd.c_str(),nullptr);log("TEST CMD %s => %d",cmd.c_str(),ok);}
  }
 }
 void __cdecl message(Message* m){
  static bool seen[64]{};if(m->type<64&&!seen[m->type]){seen[m->type]=true;log("NVSE message %u dataLen=%u",m->type,m->len);}
  if(m->type==DeferredInit){init();initBook();initEmptyEvents();log("WRIST LIGHT adapter %s",wrist_light::install()?"installed":"REFUSED");tutorialPublished=native_pose_dispatch::subscribe(kill_camera_head::restore,publishTutorial);log("SPRINT adapter %s",sprint_adapter::install()?"installed":"REFUSED");log("RELOAD TRACE %s",reload_trace_adapter::install());log("PHYSICAL RELOAD %s",installPhysicalParts());log("FINGERS %s",(finger_adapter::skeletal=querySkeletal,finger_adapter::install(queryGesture)));log("PRESENTATION %s",presentation_adapter::install(true));return;}
  if(m->type==NewGame||m->type==PreLoadGame||m->type==ExitMenu||m->type==ExitGame){wheelEquipment::editor={};bookActive=false;finger_adapter::bookContact=-1;bookState.close();bookVisible=false;bookRouted=false;bookGiven=false;bookWarmed=false;memset(bookFindDone,0,sizeof(bookFindDone));memset(bookFindRefs,0,sizeof(bookFindRefs));bookFindAt=0;bookDropPending=false;bookWorldRef=0;}
  // xNVSE Serialization.cpp sends the boolean value cast to void*, not a pointer.
  if(m->type==PostLoadGame){loaded=m->data!=nullptr;if(loaded)warmBook();log("PostLoadGame loaded=%d",loaded);return;}
  if(m->type==NewGame||m->type==PreLoadGame||m->type==ExitMenu||m->type==ExitGame){kill_camera_head::enabled=false;kill_camera_head::requested=false;kill_camera_head::external=false;replayHeadProbe=false;kill_camera_head::restore();emptyFlagDirty=true;lamp_shadow_renderer::gameplay=false;++lamp_shadow_renderer::generation;sprintState.suspend();sprint_adapter::factor=1.f;sprintAt=0;sprintNoticeUntil=0;wristSettings=false;wristSettingsStatus.clear();settingsPage=fc_settings::Home;settingsNav={};lightRestorePending=m->type==PreLoadGame&&lightWasOn;lightRestoreAt=0;if(m->type!=PreLoadGame)lightWasOn=false;clearBeam();wrist_light::player=nullptr;wrist_light::gameplay=false;native_worldmag_recovery::reset();}
  if(m->type==NewGame){clearPhysicalMags();clearWrist();loaded=true;forgetHeldAid();log("NewGame");return;}
  // Native TTWVR owns held-item save/load and teardown. Only discard addon
  // pointers here; never activate/stow a native object during lifecycle events.
  if(m->type==PreLoadGame||m->type==ExitMenu||m->type==ExitGame){featureMenu=false;for(auto& e:emptyEvents)number(e.read);{std::lock_guard<std::mutex> lock(featureMutex);featureView.visible=0;}clearBottleImpacts();lastEmptyBottle=0;clearTutorialVisual();tutorialStep=reload_tutorial::Step::Hidden;tutorialForm=0;tutorialWeapon=nullptr;clearPhysicalMags();clearWrist();enabled=false;gestureGameplayAllowed=false;weaponHand=-1;selectedWeaponID=0;weaponTransition=false;pendingWeapon={};if(selectorOpen)close();forgetHeldAid();heightMenu=false;loaded=false;return;}
  if(m->type==MainLoop){testMailbox();core_perf::Scope timing;tickEmptyEvents();tick();if(loaded){tickPhysicalMags();tickTutorial();}}
 }
}
extern "C" __declspec(dllexport) u32 __cdecl SC_Input(const char* name,int hand,int kind,float x,float y){
 if(hand<0||hand>1||!name)return 0;
 static std::atomic<bool> announced{false};if(!announced.exchange(true))log("OpenXR input bridge connected");
 bool active=(kind&8)!=0;kind&=7;
 if(!strncmp(name,"fc_diag:",8)){log("GESTURE XR %s result=%d",name+8,int(x));return 0;}
 if(!strcmp(name,"fc_controls")){
  auto now=GetTickCount64();int action=holdState.update(unsigned(x),now,active,enabled&&!selectorOpen&&!heightMenu&&!weaponTransition&&!controlsEditor&&!usingWristSettings(),false);
  static unsigned lastMask=~0u,events=0;if(unsigned(x)!=lastMask&&events++<100){lastMask=unsigned(x);log("SHORTCUT mask=%u active=%d enabled=%d action=%d",unsigned(x),active,int(enabled.load()),action);}
  controlsActive=active;if(action>=0&&!(controlsOverrides.load()&(1u<<action)))pendingControl=action;return 0;
 }
 if(!strcmp(name,"fc_gestures")){
  std::lock_guard<std::mutex> lock(inputMutex);auto& g=gestures[hand];g.active=active;g.thumb=x>.5f;g.index=y>.5f;g.time=GetTickCount64();if(!active)g.blendTime=0;return 0;
 }
 if(!strcmp(name,"fc_physical")){std::lock_guard<std::mutex> lock(inputMutex);physicalGrip[hand]=x;physicalTrigger[hand]=y;physicalAt[hand]=active?GetTickCount64():0;return 0;}
 bool stick=!strcmp(name,"thumbstick_click");
 const bool lootHorizontal=hand==1&&kind==2&&!strcmp(name,"thumbstick")&&floating_hud_adapter::horizontalInput(x,active);
 {std::lock_guard<std::mutex> lock(inputMutex);input.time=GetTickCount64();auto& h=input.h[hand];
  if(stick){h.stick=x>.5f;h.active=active;}else if(!strcmp(name,"primary_click"))h.primary=x>.5f;else if(!strcmp(name,"secondary_click"))h.secondary=x>.5f;else if(!strcmp(name,"squeeze"))h.grip=x;else if(!strcmp(name,"trigger"))h.trigger=x;else if(!strcmp(name,"thumbstick")){h.x=x;h.y=y;}
 }
 // Menu/dialogue suspension must not manufacture a grip-release edge for
 // already selected aid. Lifecycle cleanup still clears this latch, and
 // inactive XR actions are never forced active.
 if(active&&!strcmp(name,"squeeze")&&aidHand.load()==hand&&aidGripLatch.load())return 2;
 if(!enabled.load())return 0;
 bool modal=controlsEditor.load()||selectorOpen.load()||heightMenu.load()||weaponTransition.load();
 int control=stick?0:!strcmp(name,"primary_click")?1:!strcmp(name,"secondary_click")?2:!strcmp(name,"squeeze")?3:!strcmp(name,"trigger")?4:!strcmp(name,"thumbstick")?5:-1;
 if(control<0)return 0;
 bool pressed=kind==2?(x*x+y*y>.09f):x>.15f;
 bool vehicleMotion=control==5&&mountedVehicle()&&!heightMenu.load()&&!controlsEditor.load();
 bool locomotion=vehicleMotion||(selectorOpen.load()&&!heightMenu.load()&&hand==0&&control==5);
 bool bookOwned=bookActive&&(control==1||(hand==1&&control==2)||control==4||(hand==1&&control==5));
 bool owned=bookOwned||(modal&&!locomotion)||(stickyAidButtons.load()&&aidGripLatch.load()&&control==1)||(control==4&&native_worldmag_recovery::cycleRequiredHand.load()==hand);
 bool suppress=owned;
 {std::lock_guard<std::mutex> lock(inputMutex);auto& drain=controlDrain[hand][control];
  if(vehicleMotion)drain=false;
  if(owned&&pressed)drain=true;if(drain){suppress=true;if(!pressed)drain=false;}}
 if(active&&weaponHand.load()==hand&&control==3)return 2;
 return suppress?1:lootHorizontal?3:0;
}
extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Query(const NVSE* nvse,PluginInfo* info){info->infoVersion=1;info->name="SeatedControls";info->version=2;return !nvse->isEditor&&nvse->runtime==0x040020D0&&nvse->version>=6;}
extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Load(const NVSE* nvse){
 gameDirectory=nvse->runtimeDirectory();std::string path=gameDirectory+"SeatedControls.log";logfile=_fsopen(path.c_str(),"w",_SH_DENYNO);
 if(!GetEnvironmentVariableA("SEATED_TEST_DIR",nullptr,0)&&!GetEnvironmentVariableA("BVR_XRSIM_DIR",nullptr,0))
  headsetLog=_fsopen((gameDirectory+"SeatedControls-headset.log").c_str(),"w",_SH_DENYNO);
 scripts=static_cast<Scripts*>(nvse->queryInterface(6));arrays=static_cast<Arrays*>(nvse->queryInterface(5));consoleApi=static_cast<Console*>(nvse->queryInterface(1));auto messaging=static_cast<Messaging*>(nvse->queryInterface(2));
 if(auto data=static_cast<DataInterface*>(nvse->queryInterface(7)))inventoryRef=reinterpret_cast<InventoryRef*(__cdecl*)(u32)>(data->function(2));
 log("SeatedControls 1.8.0-beta.4-rc.3; runtime %08X; nvse %u",nvse->runtime,nvse->version);
 if(!scripts||!arrays||!messaging||!inventoryRef)return false;
 auto image=build_adapter::heldModule();
 if(!image)return false;
 if(!enableInputLayer()){log("Input layer missing; reinstall the complete addon archive");return false;}
 log("OpenXR input layer enabled for this process");
 readSettings();readFeatures();
 if(!messaging->listen(nvse->getPluginHandle(),"NVSE",message))return false;
 log("Reload preference: %s (applied after native initialization)",automaticReload?"automatic":"manual");
 log("Renderer isolation: %s",build_adapter::testDisableUpscaler());
 // Install only at startup after all load-failure paths. Default off until native QA passes.
 log("WORLD MAG RECOVERY %s",native_worldmag_recovery::install({worldmagPlayer,worldmagInventory}));
 return true;
}
extern "C" __declspec(dllexport) u32 __cdecl SC_UI(char* text,u32 capacity){
 if(!text||capacity<2||!enabled.load())return 0;
 std::lock_guard<std::mutex> lock(displayMutex);
 if(displayText.empty()||(!selectorOpen.load()&&!heightMenu.load()&&GetTickCount64()>displayUntil)){
  if(GetTickCount64()<sprintNoticeUntil.load()){strncpy_s(text,capacity,"@FC_SPRINT@",_TRUNCATE);return 1;}
  auto pool=GetModuleHandleW(L"FilthyCasualPool.dll");auto status=pool?reinterpret_cast<u32(__cdecl*)(char*,u32)>(GetProcAddress(pool,"FC_PoolStatus")):nullptr;
  return status?status(text,capacity):0;
 }
 strncpy_s(text,capacity,displayText.c_str(),_TRUNCATE);return 1;
}

extern "C" __declspec(dllexport) float __cdecl SC_Height(float rawHeadHeight){if(!std::isfinite(rawHeadHeight)||rawHeadHeight>0){
 std::lock_guard<std::mutex> lock(heightSampleMutex);
 if(!std::isfinite(rawHeadHeight)){physicalHeight=0;heightSampleAt=0;}
 else {physicalHeight=rawHeadHeight;heightSampleAt=GetTickCount64();}
}return feature(fc_features::Height)?seatOffsets[0].load():0;}
extern "C" __declspec(dllexport) float __cdecl SC_SeatOffset(int mode){return feature(fc_features::Height)&&mode>0&&mode<5?seatOffsets[mode].load():0;}
extern "C" __declspec(dllexport) bool __cdecl SC_DialogueAnchor(DialogueAnchor* out){return out&&presentation_adapter::readSpeaker(*out);}
extern "C" __declspec(dllexport) void __cdecl SC_Head(float x,float y,float z){std::lock_guard<std::mutex> lock(inputMutex);input.head={x,y,z};input.headValid=std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z);}
extern "C" __declspec(dllexport) unsigned __cdecl SC_UIState(){return (uiEpoch.load()<<2)|(selectorOpen.load()?1:0)|(heightMenu.load()?2:0);}

extern "C" __declspec(dllexport) void __cdecl SC_ControlsState(unsigned modal,unsigned overrides){controlsEditor=modal!=0;controlsOverrides=feature(fc_features::Remapping)?overrides:0;}
extern "C" __declspec(dllexport) void __cdecl SC_ControlAction(int action){if(action==5){if(!controlsEditor.load()&&comfortGameplayAt.load()&&GetTickCount64()-comfortGameplayAt.load()<200)++walkRequest;return;}if(action>=0&&action<=4&&!controlsEditor.load())pendingControl=action;}

extern "C" __declspec(dllexport) bool __cdecl FC_NativePoseOwner(){return native_pose_dispatch::original!=nullptr;}
extern "C" __declspec(dllexport) bool __cdecl FC_NativePoseSubscribe(native_pose_dispatch::Callback before,native_pose_dispatch::Completed after){return native_pose_dispatch::local(before,after);}

extern "C" __declspec(dllexport) void __cdecl SC_Mouth(float x,float y,float z){
 std::lock_guard<std::mutex> lock(inputMutex);
 if(std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)){input.mouth={x,y,z};input.mouthTime=GetTickCount64();}else input.mouthTime=0;
}



extern "C" __declspec(dllexport) bool __cdecl SC_FeaturesView(fc_features::View* out,unsigned size){if(!out||size!=sizeof(*out))return false;std::lock_guard<std::mutex> lock(featureMutex);*out=featureView;return featureMenu&&enabled;}
extern "C" __declspec(dllexport) void __cdecl SC_FeatureHover(int row){featureHover=row;}
extern "C" __declspec(dllexport) bool __cdecl FC_FeatureEnabled(const char* key){int id=fc_features::find(key);if(id<0)return false;const auto bit=fc_features::bit(id);
 if(bit&fc_features::vehicleMask)return (addonSessionMask&featureMask.load()&bit)!=0;
 return ((fc_features::definitions[id].addon?addonSessionMask:featureMask.load())&bit)!=0;}

extern "C" __declspec(dllexport) bool __cdecl SC_SettingsView(fc_settings::View* out,unsigned size){if(!out||size!=sizeof(*out)||!heightMenu||featureMenu||!enabled)return false;std::lock_guard<std::mutex> lock(settingsMutex);*out=settingsView;return true;}
extern "C" __declspec(dllexport) void __cdecl SC_SettingsHover(int hit){settingsHover=hit;}

// Called by the native menu on the game thread, alongside FC_WristSettings.
extern "C" __declspec(dllexport) int __cdecl FC_WheelProfile(int hand){return wheelEquipment::activeProfile(hand);}
extern "C" __declspec(dllexport) int __cdecl FC_WheelProfiles(int hand){return wheelEquipment::profileMask(hand);}
extern "C" __declspec(dllexport) bool __cdecl FC_UseWheelProfile(int hand,int profile){return wheelEquipment::useProfile(hand,profile);}
extern "C" __declspec(dllexport) bool __cdecl FC_EquipmentWheels(unsigned command,int row,int direction,fc_settings::View* out,unsigned size){return wheelEquipment::handle(command,row,direction,out,size);}
extern "C" __declspec(dllexport) bool __cdecl FC_WheelLayoutView(int hand,int profile,fc_equipment::LayoutView* out,unsigned size){
    return out && size==sizeof(*out) && wheelEquipment::preview(hand,profile,*out);
}
// Called only by the native menu's main-loop owner. This never raises an XR panel.
extern "C" __declspec(dllexport) bool __cdecl FC_WristSettings(unsigned command,int row,int direction,fc_settings::View* out,unsigned size){
 using namespace fc_settings;
 if(command==DeviceClose){wristSettingsPresented=false;wristSettings=false;return true;}
 if(command==DevicePresent){wristSettingsPresented=direction!=0&&wristSettings.load();return true;}
 if(!out||size!=sizeof(*out)||!ready||!loaded)return false;
 if(command==DeviceOpen){
  heightMenu=false;featureMenu=false;
  if(selectorOpen)close();
  wristSettings=true;wristSettingsPresented=true;settingsPage=WristHome;settingsNav={};settingsHover=-1;wristSettingsStatus.clear();
  settingsText();
 }
 if(!wristSettings)return false;
 if(command==DeviceBack)settingsBack();
 else if(command==DeviceMove&&direction){
  const int count=int(settingsView.count);
  settingsNav.selected=(settingsNav.selected+(direction>0?1:count-1))%count;settingsText();
 }else if((command==DeviceSelect||command==DeviceAdjust)&&row>=0&&row<int(settingsView.count)){
  settingsNav.selected=row;
  settingsActivate(std::clamp(direction,-1,1),command==DeviceSelect);
 }
 if(!wristSettings)return false;
 std::lock_guard<std::mutex> lock(settingsMutex);*out=settingsView;
 return true;
}

extern "C" __declspec(dllexport) int __cdecl FC_CrashStrength(){return crashStrength;}
extern "C" __declspec(dllexport) bool __cdecl FC_ComfortConfig(fc_comfort::Config* out,unsigned size){if(!out||size!=sizeof(*out))return false;std::lock_guard<std::mutex> lock(comfortMutex);*out=comfortConfig;auto at=comfortGameplayAt.load();out->gameplay=at&&GetTickCount64()-at<200&&!heightMenu&&!featureMenu&&!selectorOpen&&!controlsEditor;out->walkRequest=walkRequest.load();return true;}
extern "C" __declspec(dllexport) void __cdecl FC_SkeletalFingers(int hand,const float* curls){if(hand<0||hand>1)return;std::lock_guard<std::mutex> lock(comfortMutex);skeletalAt[hand]=0;if(!curls)return;for(int f=0;f<5;f++)if(!std::isfinite(curls[f])||curls[f]<0||curls[f]>1)return;memcpy(skeletalCurls[hand],curls,sizeof skeletalCurls[hand]);skeletalAt[hand]=GetTickCount64();}
