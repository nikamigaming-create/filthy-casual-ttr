#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "empty_bottles.hpp"
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
#include "nvse_api.hpp"
#include "build_adapter.hpp"
#include "reload_adapter.hpp"
#include "native_worldmag_recovery.hpp"
#include "reload_trace_adapter.hpp"
#include "scene_adapter.hpp"
#include "physical_reload_adapter.hpp"
#include "bottle_impact_adapter.hpp"
#include "glass_decal_adapter.hpp"
#include "wrist_controls.hpp"
#include "finger_adapter.hpp"
#include "hold_controls.hpp"
#include "reload_tutorial.hpp"
#include "tutorial_adapter.hpp"
#include "tutorial_visual.hpp"
#include "native_pose_dispatch.hpp"
#include "sticky_food.hpp"
#include "presentation_adapter.hpp"
#include "layer_discovery.hpp"
namespace {
 Scripts* scripts=nullptr;Arrays* arrays=nullptr;Console* consoleApi=nullptr;std::string gameDirectory;
 InventoryRef*(__cdecl* inventoryRef)(u32)=nullptr;
 FILE* logfile=nullptr;std::mutex logMutex,inputMutex,displayMutex;
 std::string displayText;ULONGLONG displayUntil=0;
 std::atomic<bool> controlsEditor{false};
 std::atomic<unsigned> controlsOverrides{0};
 float physicalGrip[2]{},physicalTrigger[2]{};ULONGLONG physicalAt[2]{};
 std::atomic<bool> enabled{false},selectorOpen{false},heightMenu{false},showSelectorPopups{false};
 std::atomic<bool> weaponTransition{false};
 std::atomic<int> weaponHand{-1},aidHand{-1};std::atomic<float> heightOffset{0},physicalHeight{0};
 std::atomic<float> seatOffsets[5]{};int seatContext=0;
 constexpr const char* seatKeys[]={"HeightOffsetCm","CarSeatOffsetCm","FlightSeatOffsetCm","BighornerSeatOffsetCm","DeathclawSeatOffsetCm"};
 constexpr const char* seatNames[]={"GROUND","CAR","VERTIBIRD","BIGHORNER","DEATHCLAW"};
 std::atomic<unsigned> uiEpoch{0};bool controlDrain[2][6]{};
 char settingsPath[MAX_PATH]{};float desiredHeight=1.65f;bool automaticReload=true;
 bool reloadPreferenceApplied=false;
 bool seatedAmmoPouch=true,pouchApplied=false;float pouchForward=.40f,pouchSide=-.18f,pouchBelow=.45f;
 bool wristEnabled=false,wristInserted=false,wristAttached=false;
 hold_controls::State holdState;
 std::atomic<ULONGLONG> controlsAt{0};
 std::atomic<bool> controlsActive{false},menuOwned{false};
 std::atomic<int> pendingControl{-1};
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
 Script *dialogueTarget=nullptr,*dialoguePartner=nullptr,*combatDisabled=nullptr,*playerDead=nullptr;
 struct Preview {std::string holder,attachment;bool inserted=false,attached=false;scene::WorldPin pin;scene::Transform carrierLocal{};Vec trackingCenter{};std::vector<scene::Box> mesh;};
 Preview previews[previewCount];
 bool touchMustLeave[2][previewCount]{};
 Script* getCap=nullptr;
 Script *getPlayer=nullptr,*getMenu=nullptr,*getWeapons=nullptr,*getAid=nullptr,*getName=nullptr,*getModel=nullptr,*getCount=nullptr,*getDimension=nullptr,*rootScale=nullptr,*getEquipped=nullptr,*weaponOut=nullptr,*holster=nullptr,*draw=nullptr;
 Script *insert=nullptr,*removeNode=nullptr,*attach=nullptr,*detach=nullptr,*translate=nullptr,*scale=nullptr,*position=nullptr,*equip=nullptr,*notice=nullptr;
 Script *getInventoryRefs=nullptr,*moveRef=nullptr,*takeRef=nullptr;
 
 void log(const char* fmt,...){std::lock_guard<std::mutex> lock(logMutex);if(!logfile)return;fprintf(logfile,"[%llu] ",GetTickCount64());va_list ap;va_start(ap,fmt);vfprintf(logfile,fmt,ap);va_end(ap);fputc('\n',logfile);fflush(logfile);}
 void release(Element& e){if(e.type==3&&e.value.string)build_adapter::freeGameString(e.value.string);e={};}
 template<typename... Args> bool call(Script* s,Element& out,Args... args){if(!s)return false;return scripts->call(s,nullptr,nullptr,&out,sizeof...(args),args...);}
 template<typename... Args> double number(Script* s,Args... args){Element e;bool ok=call(s,e,args...);double value=ok&&e.type==1?e.value.number:0;release(e);return value;}
 template<typename... Args> std::string string(Script* s,Args... args){Element e;std::string value;if(call(s,e,args...)&&e.type==3&&e.value.string)value=e.value.string;release(e);return value;}
 u32 bits(float f){u32 b;memcpy(&b,&f,4);return b;}
 void tell(const std::string& s,bool height=false){
  if(!height&&!showSelectorPopups){std::lock_guard<std::mutex> lock(displayMutex);displayText.clear();log("UI (muted): %s",s.c_str());return;}
  {std::lock_guard<std::mutex> lock(displayMutex);
   displayText=openHand>=0?std::string(openHand==1?"WEAPONS":"CONSUMABLES")+"   "+std::to_string(page+1)+" / "+std::to_string(std::max(1,int((items.size()+slots-1)/slots)))+"\n"+s+"\nTouch to select | Center cap clears hand | B close":s;
   displayUntil=GetTickCount64()+2200;}
  if(notice){Element e;call(notice,e,s.c_str());release(e);}log("UI: %s",s.c_str());
 }
 void readSettings(){
  char path[MAX_PATH]{};GetModuleFileNameA(GetModuleHandleA("SeatedControls.dll"),path,MAX_PATH);
  auto dot=strrchr(path,'.');if(dot)strcpy_s(dot,5,".ini");strcpy_s(settingsPath,path);
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
  showSelectorPopups=GetPrivateProfileIntA("UI","ShowSelectorPopups",0,path)!=0;
  automaticReload=GetPrivateProfileIntA("Weapons","AutomaticReload",1,path)!=0;
  seatedAmmoPouch=GetPrivateProfileIntA("Weapons","SeatedAmmoPouch",1,path)!=0;
  wristEnabled=false; // Thumbrest double taps and left Menu replace the cuff.
  holdState.delayMs=std::clamp(GetPrivateProfileIntA("Controls","HoldMs",1000,path),500u,2000u);
  holdState.doubleTapMs=std::clamp(GetPrivateProfileIntA("Controls","DoubleTapMs",450,path),200u,700u);
  fingerGestures=GetPrivateProfileIntA("Hands","FingerGestures",1,path)!=0;
  auto pouchValue=[&](const char* key,float def,float lo,float hi){char text[40],fallback[40];sprintf_s(fallback,"%.1f",def);GetPrivateProfileStringA("Weapons",key,fallback,text,sizeof(text),path);float v=float(atof(text));return (std::isfinite(v)?std::clamp(v,lo,hi):def)*.01f;};
  pouchForward=pouchValue("PouchForwardCm",40,10,70);pouchSide=pouchValue("PouchSideCm",-18,-60,60);pouchBelow=pouchValue("PouchBelowEyesCm",45,20,120);
  char tmp[40]{};GetPrivateProfileStringA("Seated","HeightOffsetCm","0",tmp,sizeof(tmp),path);heightOffset=std::clamp(float(atof(tmp))*.01f,-.5f,1.2f);
  seatOffsets[0]=heightOffset.load();
  for(int i=1;i<5;i++){GetPrivateProfileStringA("Seated",seatKeys[i],"0",tmp,sizeof(tmp),path);float v=float(atof(tmp))*.01f;seatOffsets[i]=std::isfinite(v)?std::clamp(v,-.5f,.5f):0;}
  GetPrivateProfileStringA("Seated","TargetEyeHeightCm","165",tmp,sizeof(tmp),path);desiredHeight=std::clamp(float(atof(tmp))*.01f,1.1f,2.1f);
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
  auto first=b[0x1416775]?static_cast<Form*>(*reinterpret_cast<void**>(b+0x1417410)):nullptr;
  auto second=b[0x1416844]?static_cast<Form*>(*reinterpret_cast<void**>(b+0x141684C)):nullptr;
  physical_reload_adapter::recoverStage=5;
  auto ref=first?first:second;if(!ref||(first&&second&&first!=second))return -1;
  physical_reload_adapter::recoverStage=6;
  int heldHand=*reinterpret_cast<int*>(b+(first?0xF6B520:0x1416848));if(heldHand!=hand)return -1;
  physical_reload_adapter::recoverStage=7;
  auto it=std::find_if(physicalMags.begin(),physicalMags.end(),[&](const PhysicalMag& m){return m.refID==ref->id;});
  if(it==physicalMags.end()||!it->weaponID||!it->ammoID)return -1;
  physical_reload_adapter::recoverStage=8;
  Element equipped;unsigned weapon=0;if(call(getEquipped,equipped)&&equipped.type==2&&equipped.value.form)weapon=equipped.value.form->id;release(equipped);
  auto ammo=static_cast<Form*>(reinterpret_cast<void*(__cdecl*)(void*)>(b+0x315CF0)(player));
  physical_reload_adapter::recoverStage=9;
  if(weapon!=it->weaponID||!ammo||ammo->id!=it->ammoID)return -1;
  physical_reload_adapter::recoverStage=10;
  auto base=scene::field<Form*>(ref,0x20);bool empty=base==emptyReloadPart();if(!empty&&base!=ammo)return -1;
  physical_reload_adapter::recoverStage=11;
  int count=empty?0:int(physical_reload_adapter::stackCount(ref));int capacity=reinterpret_cast<int(__cdecl*)()>(b+0x31B6A0)();
  if(count<0||count>capacity||capacity<1)return -1;
  // Actual native grab ownership is the pickup/catch event. Hand over now,
  // not only after approaching the well with an arbitrary physics grab pose.
  // The native initializer computes this weapon's held offset and orientation.
  physical_reload_adapter::recoverStage=16;
  if(!takePhysicalPart(*it,ref,player,true))return -1;
  return count;
 }
 bool nearPhysicalPouch(void* ref){
  auto b=physical_reload_adapter::module;auto reloadSettings=build_adapter::liveReloadSettings();if(!b||!reloadSettings||reloadSettings[0x49C]!=1)return false;
  auto pouch=*reinterpret_cast<void**>(b+0x18B3E94);auto root=scene::root(ref);
  if(!build_adapter::readPouchNode(pouch).readable&&!*reinterpret_cast<int*>(b+0x18B3F60))pouch=*reinterpret_cast<void**>(b+0x18B3E80);
  if(!root||!build_adapter::readPouchNode(pouch).readable)return false;
  auto pose=scene::world(pouch);auto center=pose.p+scene::rotate(pose,*reinterpret_cast<Vec*>(b+0x18B3E84))*pose.scale;
  auto bound=scene::field<float*>(root,0x20);if(!bound)return false;
  float threshold=*reinterpret_cast<float*>(reloadSettings+0x4CC)**reinterpret_cast<float*>(reloadSettings+0x28);
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
    if(reloadSettings&&reloadSettings[0x49C]==1&&build_adapter::readPouchNode(root).readable&&physical_reload_adapter::installedRest(rest)){
     auto bound=scene::field<float*>(root,0x20);float units=*reinterpret_cast<float*>(reloadSettings+0x28);
     if(bound&&std::isfinite(units)&&units>1){m.contactDistance=scene::distance({bound[0],bound[1],bound[2]},rest.p)/units;if(std::isfinite(m.contactDistance)&&m.contactDistance<=.12f){
      auto b=physical_reload_adapter::module;m.contactHand=(b[0x1416775]&&*reinterpret_cast<void**>(b+0x1417410)==ref)?*reinterpret_cast<int*>(b+0xF6B520):build_adapter::grabbedHand();
      Element equipped;if(call(getEquipped,equipped)&&equipped.type==2&&equipped.value.form)m.contactWeapon=equipped.value.form->id;release(equipped);m.contactAt=GetTickCount64();
     }}
    }
   }
   if(m.wasHeld&&!held&&m.wasNearPouch&&nearPouch&&m.weaponID&&m.ammoID&&m.lastHeldHand>=0&&partInput.h[m.lastHeldHand].active&&partInput.h[m.lastHeldHand].grip<.2f&&GetTickCount64()-partInput.time<100&&gestureGameplayAllowed){if(takePhysicalPart(m,ref,playerForm(),false)){physical_reload_adapter::recoveredPouch++;continue;}}
   if(held){auto b=physical_reload_adapter::module;m.lastHeldHand=(b&&b[0x1416775]&&*reinterpret_cast<void**>(b+0x1417410)==ref)?*reinterpret_cast<int*>(b+0xF6B520):build_adapter::grabbedHand();}
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
 bool emptyBottleBase(unsigned id){return id==0x5B6CE||id==0x5B6CD||id==0x22103||id==0x103B1B;}
 void trackBottle(Form* ref){
  if(!ref)return;auto base=scene::field<Form*>(ref,0x20);if(!base||!emptyBottleBase(base->id))return;
  for(auto& b:fragileBottles)if(b.id==ref->id)return;
  if(fragileBottles.size()>=64)fragileBottles.erase(fragileBottles.begin());
  fragileBottles.push_back({ref->id,base->id,{}});
 }
 void clearBottleImpacts(){glass_decal_adapter::clearProbes();fragileBottles.clear();}
 bool glassAt(Vec point,Vec normal,void* surface,Form* ref){
  static Script* texture=nullptr;static Script* destroy=nullptr;static Script* sound=nullptr;
  if(!texture)texture=compile("glass decal texture","GetFormFromMod \"Filthy Casual Glass.esp\" \"000800\"",true);
  if(!destroy)destroy=compile("broken empty bottle","ref item\nbegin function {item}\nitem.Disable\nitem.MarkForDelete\nSetFunctionValue 1\nend");
  if(!sound)sound=compile("glass impact sound","PlaySound FXMeleeGenericGlass",true);
  if(!texture||!destroy)return false;
  Element value;Form* tx=nullptr;if(call(texture,value)&&value.type==2)tx=value.value.form;release(value);
  auto node=glass_decal_adapter::raySurface(point,normal,scene::root(ref));if(!node)node=glass_decal_adapter::parentNode(surface);auto before=glass_decal_adapter::attempts();

  auto cell=glass_decal_adapter::impactCell(point);bool ok=glass_decal_adapter::add(point,normal,node,cell,tx,scene::root(ref));
  log("BOTTLE DECAL accepted=%d tx=%p surface=%p node=%p name=%s active=%u attempts=%u/%u normal=%.3f %.3f %.3f pos=%.2f %.2f %.2f",ok,tx,surface,node,node?scene::name(node):"",glass_decal_adapter::count(),before,glass_decal_adapter::attempts(),normal.x,normal.y,normal.z,point.x,point.y,point.z);
  if(!ok)return false;
  bool audioOK=glass_decal_adapter::smashSound(point);log("BOTTLE EFFECTS puff=%d sound=%d stage=%d bytes=%08X/%08X/%08X",glass_decal_adapter::puff(point,normal,cell),audioOK,glass_decal_adapter::soundStage,scene::field<unsigned>((void*)0xAE5870,0),scene::field<unsigned>((void*)0xAD8B60,0),scene::field<unsigned>((void*)0xAD8830,0));Element audio;if(!audioOK&&sound){call(sound,audio);release(audio);}
  if(number(destroy,ref)!=1)return false;lastGlassPoint=point;++bottleBreaks;return true;
 }
 void tickBottleImpacts(bool usable){
  glass_decal_adapter::inspect([](const char* fmt,auto... args){log(fmt,args...);});
  auto now=GetTickCount64();
  if(!usable){for(auto& b:fragileBottles)b.previous={};return;}
  auto held=static_cast<Form*>(build_adapter::grabbedItem());if(held)trackBottle(held);
  for(auto i=fragileBottles.begin();i!=fragileBottles.end();){
   auto ref=static_cast<Form*>(physical_reload_adapter::lookup(i->id));auto base=ref?scene::field<Form*>(ref,0x20):nullptr;
   if(!ref||!base||base->id!=i->base||(ref->flags&0x820)||!scene::root(ref)){i=fragileBottles.erase(i);continue;}
   void* surface=nullptr;auto sample=bottle_impact_adapter::sample(scene::root(ref),held==ref,now*.001,&surface);auto change=sample.velocity-i->previous.velocity;auto speed=bottle_impact::length(change);Vec normal=speed>.001f?Vec{change.x/speed,change.y/speed,change.z/speed}:Vec{0,0,1};auto verdict=bottle_impact::judge(i->previous,sample);if(bottleTrace)log("BOTTLE SAMPLE ref=%08X valid=%d held=%d contact=%d velocity=%.3f %.3f %.3f surface=%.3f %.3f %.3f closing=%.3f delta=%.3f",i->id,sample.valid,sample.held,sample.contact,sample.velocity.x,sample.velocity.y,sample.velocity.z,sample.surfaceVelocity.x,sample.surfaceVelocity.y,sample.surfaceVelocity.z,verdict.closingSpeed,verdict.velocityChange);i->previous=sample;
   if(bottleTrace){auto root=scene::root(ref);auto b=bottle_impact_adapter::body(root);auto wp=scene::world(root).p;auto rp=scene::field<Vec>(ref,0x30);Vec hp{};unsigned mt=0;if(b){hp=scene::field<Vec>(b,0x120);mt=scene::field<unsigned char>(b,0xE8);}log("BOTTLE POSE ref=%08X held=%d root=%.3f %.3f %.3f refpos=%.3f %.3f %.3f body=%p hk=%.3f %.3f %.3f motion=%u nodeflags=%08X collision=%04X local=%.3f %.3f %.3f",i->id,sample.held,wp.x,wp.y,wp.z,rp.x,rp.y,rp.z,b,hp.x,hp.y,hp.z,mt,scene::field<unsigned>(root,0x30),scene::field<void*>(root,0x1C)?scene::field<unsigned short>(scene::field<void*>(root,0x1C),0xC):0,scene::local(root).p.x,scene::local(root).p.y,scene::local(root).p.z);}
   if(verdict.smash){log("BOTTLE IMPACT ref=%08X contact=%d closing=%.3f deltaV=%.3f",i->id,sample.contact,verdict.closingSpeed,verdict.velocityChange);if(glassAt(scene::world(scene::root(ref)).p,normal,surface,ref)){i=fragileBottles.erase(i);continue;}}
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
  unsigned emptyID=empty_bottles::baseForModel(string(getModel,food.form));auto empty=emptyID?static_cast<Form*>(physical_reload_adapter::lookup(emptyID)):nullptr;
  if(empty&&empty->type!=31)empty=nullptr;double emptyBefore=empty?number(getCount,empty):0;
  if(auto b=build_adapter::heldModule())log("FOOD stow input wanted=%p primary=(%u,%p,%p,%d) secondary=(%u,%p,%p,%d)",selectedRef,b[0x1416775],*reinterpret_cast<void**>(b+0x1417410),*reinterpret_cast<void**>(b+0x1417414),*reinterpret_cast<int*>(b+0xF6B520),b[0x1416844],*reinterpret_cast<void**>(b+0x141684C),*reinterpret_cast<void**>(b+0x1416850),*reinterpret_cast<int*>(b+0x1416848));
  if(!build_adapter::stowSelectedGrab(playerForm(),selectedRef)){
   if(auto b=build_adapter::heldModule())log("FOOD stow refused wanted=%p primary=(%u,%p,%d) secondary=(%u,%p,%d) swap=%02X%02X%02X%02X",selectedRef,b[0x1416775],*reinterpret_cast<void**>(b+0x1417410),*reinterpret_cast<int*>(b+0xF6B520),b[0x1416844],*reinterpret_cast<void**>(b+0x141684C),*reinterpret_cast<int*>(b+0x1416848),b[0x1A95A0],b[0x1A95A1],b[0x1A95A2],b[0x1A95A3]);
   mouthGate.reset();return;
  }
  forgetHeldAid();
  double returned=number(getCount,food.form);
  if(returned!=before+1){log("FOOD mouth cancelled: stow count %.0f -> %.0f",before,returned);return;}
  double result=number(equip,food.form,0);
  double after=number(getCount,food.form);
  log("FOOD mouth %08X native-use=%.0f inventory %.0f -> %.0f -> %.0f",food.id,result,before,returned,after);
  if(after==before){tell(std::string(empty?"Drank: ":"Ate: ")+food.name);if(empty)leaveEmptyBottle(empty,emptyBefore,preservePose?&oldPose:nullptr);}
 }
 void selectItem(int index){
  constexpr int hand=1; // Either hand may touch; selected items always go right.
  if(index<0||index>=int(items.size()))return;Item chosen=items[index];bool weapon=openHand==1;close();
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
 void open(int hand){
  if(openHand>=0)close();items.clear();auto player=playerForm();auto root=scene::root(player);if(!root)return;
  Vec head;if(!pos("Bip01 Head",1,head))return;wheelHead=head;wheelRoot=scene::world(root).p;wheelBasis=scene::world(root);wheelBasis.scale=1;wheelCell=scene::cell(player);
  {std::lock_guard<std::mutex> lock(inputMutex);wheelXRHead=input.head;wheelXRValid=input.headValid;}
  wheelCarrierLost=false;wheelCarrier=mountedUIFrame(wheelCarrierFrame);
  if(wheelCarrier){
   wheelLocalHead=scene::inverseRotate(wheelCarrierFrame,wheelHead-wheelCarrierFrame.p)*(1/wheelCarrierFrame.scale);
   wheelLocalRoot=scene::inverseRotate(wheelCarrierFrame,wheelRoot-wheelCarrierFrame.p)*(1/wheelCarrierFrame.scale);
   wheelLocalBasis=cabinLocal(wheelCarrierFrame,wheelBasis);
   tutorialThread=GetCurrentThreadId();
   if(!tutorialPublished)tutorialPublished=native_pose_dispatch::subscribe(nullptr,publishTutorial);
   if(!tutorialPublished){wheelCarrier=nullptr;tell("Vehicle selector unavailable");return;}
  }
  Element e;if(!call(hand==1?getWeapons:getAid,e)||(e.type!=4&&!(e.type==1&&e.value.number==0))){release(e);tell("Inventory unavailable");return;}
  // JIP returns a null array (or numeric zero) for an empty category. The cap
  // remains usable without any outer items; no owned cap is required either.
  auto count=e.type==4&&e.value.array?arrays->size(e.value.array):0;if(count>4096){release(e);return;}std::vector<Element> values(count);
  if(count&&!arrays->elements(e.value.array,values.data(),nullptr)){release(e);return;}
  for(auto& v:values){if(v.type==2&&v.value.form){Item item;item.form=v.value.form;item.id=item.form->id;item.name=string(getName,item.form);item.model=string(getModel,item.form);item.count=int(number(getCount,item.form));
   if(item.count>0&&(hand==1||!(item.form->flags&0x400))){if(item.model.empty()){item.model=hand==1?"Weapons\\1HandPistol\\10mmPistol.NIF":"Clutter\\Chems\\Buffout01.NIF";item.name+=" (generic preview)";}
    float dim=float(number(getDimension,item.form));item.scale=dim>1?std::clamp(settings.size/dim,.04f,1.5f):.25f;items.push_back(std::move(item));}}
   release(v);
  }release(e);std::sort(items.begin(),items.end(),[](const Item&a,const Item&b){return a.name<b.name;});
  openHand=hand;page=0;uiEpoch++;selectorOpen=true;if(!buildPage())close();
 }
 void heightText(){if(seatContext){char text[320];sprintf_s(text,"%s SEAT: %+d cm\nRight stick adjusts | A/X resets this seat\nY toggles popups | Left stick: Controls\nB saves this seat and closes",seatNames[seatContext],int(std::round(heightOffset*100)));tell(text,true);return;}char text[320];sprintf_s(text,"%s HEIGHT: %+d cm | Popups: %s\nRight stick adjusts | A calibrates to %d cm\nX resets | Y toggles popups | Left stick: Controls\nB saves and closes",seatNames[seatContext],int(std::round(heightOffset*100)),showSelectorPopups?"ON":"OFF",int(std::round(desiredHeight*100)));tell(text,true);}
 void saveHeight(){char value[32];sprintf_s(value,"%.1f",heightOffset*100);seatOffsets[seatContext]=heightOffset.load();WritePrivateProfileStringA("Seated",seatKeys[seatContext],value,settingsPath);log("HEIGHT saved context=%s offset=%s cm",seatNames[seatContext],value);}
 void toggleReload(){
   bool manual=false;
   if(build_adapter::readManualReload(manual)&&build_adapter::setManualReload(!manual)){
    automaticReload=manual;
    WritePrivateProfileStringA("Weapons","AutomaticReload",automaticReload?"1":"0",settingsPath);
    tell(automaticReload?"Automatic reload ON":"Manual reload ON",true);
    log("RELOAD MODE: %s",automaticReload?"automatic":"manual");
   }else{tell("Reload mode unavailable; unchanged",true);log("RELOAD MODE: native toggle refused");}
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
  if(GetCurrentThreadId()!=tutorialThread)return;
  if(!completed){tutorial_visual::hide();if(wheelCarrier){wheelCarrierLost=true;for(auto& p:previews)if(p.pin.node)cullNode(p.pin.node,true);}return;}
  publishSelector();
  if(tutorialStep==reload_tutorial::Step::Hidden||!tutorial_visual::pin.node)return;
  tutorialNative=tutorial_adapter::read();
  drawTutorial();++tutorialPublications;
 }
 void setTutorial(bool want){
  if(want&&!tutorialDataLoaded){
   std::ifstream pack(gameDirectory+"Data\\NVSE\\Plugins\\TTWVR\\TTWVR.dat",std::ios::binary);
   tutorialDataLoaded=tutorial_data::readPack(pack,tutorialWeapons,tutorialError);
   log("TUTORIAL metadata %s: weapons=%u %s",tutorialDataLoaded?"loaded":"unavailable",unsigned(tutorialWeapons.size()),tutorialError.c_str());
  }
  if(!want)clearTutorialVisual();
  tutorialEnabled=want&&tutorialDataLoaded&&tutorialEditorQuery&&tutorialRoundsQuery&&tutorialCapacityQuery;
  tutorialStep=reload_tutorial::Step::Hidden;tutorialForm=0;tutorialWeapon=nullptr;tutorialEditor.clear();
  tell(tutorialEnabled?"Weapon tutorial ON\nTriple-tap left Menu to turn it off":want?"Tutorial metadata unavailable; native controls unchanged":"Weapon tutorial OFF",true);
 }
 void tickTutorial(){
  tutorialNative={};
  bool visible=tutorialEnabled&&enabled&&!selectorOpen&&!heightMenu&&!weaponTransition;
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
   if(!tutorialPublished)tutorialPublished=native_pose_dispatch::subscribe(nullptr,publishTutorial);
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
  seatOffsets[seatContext]=heightOffset.load();
  using Context=int(__cdecl*)();auto mod=GetModuleHandleW(L"FilthyCasualVehicles.dll");auto fn=mod?reinterpret_cast<Context>(GetProcAddress(mod,"FC_SeatContext")):nullptr;
  int next=fn?fn():0;if(next<0||next>4)next=0;
  if(next!=seatContext){seatContext=next;heightOffset=seatOffsets[next].load();mouthGate.reset();if(heightMenu)heightText();}
 }
 void tick(){
  updateSeatContext();
  native_worldmag_recovery::tick(ready&&loaded&&gestureGameplayAllowed&&!selectorOpen&&!heightMenu&&!weaponTransition&&!controlsEditor);
  if(!ready||!loaded){enabled=false;gestureGameplayAllowed=false;return;}auto now=GetTickCount64();if(now-lastLoop<10)return;lastLoop=now;
  if(presentation_adapter::base){
   unsigned id=0;Vec namePoint{};
   if(reinterpret_cast<int(__cdecl*)()>(presentation_adapter::base+0xDA03)()==0x3F1){
    Element e;if(call(dialoguePartner,e)&&e.type==2&&e.value.form){
     auto actorRoot=scene::root(e.value.form);auto head=scene::find(actorRoot,"Bip01 Head");
     if(head){namePoint=scene::world(head).p;auto nativeSettings=reinterpret_cast<unsigned char*(__cdecl*)()>(presentation_adapter::base+0xADC1)();
      // Head joint is inside the skull: allow crown height plus a visible air gap.
      namePoint.z+=.32f*(*reinterpret_cast<float*>(nativeSettings+0x28));id=e.value.form->id;}
    }release(e);
   }
   presentation_adapter::setSpeaker(id,namePoint);
  }
  Input current;{std::lock_guard<std::mutex> lock(inputMutex);current=input;}
  if(!reloadPreferenceApplied&&build_adapter::setManualReload(!automaticReload)){reloadPreferenceApplied=true;log("RELOAD preference applied: %s",automaticReload?"automatic":"manual");}
  auto player=playerForm();auto root=scene::root(player);bool usable=root&&number(getMenu)==0&&now-current.time<500&&current.h[0].active&&current.h[1].active;
  tickBottleImpacts(usable);
  enabled=usable;
  gestureGameplayAllowed=usable&&!weaponTransition;
  native_worldmag_recovery::tick(usable&&!selectorOpen&&!heightMenu&&!weaponTransition&&!controlsEditor);
  static ULONGLONG nextGestureLog=0;static unsigned gestureLogs=0;
  if(now>=nextGestureLog&&gestureLogs<90){nextGestureLog=now+2000;++gestureLogs;
   std::lock_guard<std::mutex> lock(inputMutex);
   log("GESTURE gameplay=%d usable=%d controls=%d L(active=%d thumb=%d index=%d grip=%.2f trigger=%.2f applied=%lu) R(active=%d thumb=%d index=%d grip=%.2f trigger=%.2f applied=%lu)",int(gestureGameplayAllowed.load()),usable,int(controlsActive.load()),gestures[0].active,gestures[0].thumb,gestures[0].index,input.h[0].grip,input.h[0].trigger,finger_adapter::applied[0].load(),gestures[1].active,gestures[1].thumb,gestures[1].index,input.h[1].grip,input.h[1].trigger,finger_adapter::applied[1].load());
  }
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
   else if(wristAction==wrist_controls::Settings){heightMenu=true;uiEpoch++;heightText();}
   previous=current;return;
  }
  bool manualNow=false;
  if(build_adapter::readManualReload(manualNow)){
   bool wantPouch=seatedAmmoPouch&&manualNow;
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
      // Keep the replacement in the previous bottle's hand-relative frame
      // while its native 3D loads. The pickup then owns it normally.
      scene::Transform replacement{};if(held_pose_adapter::target(replacement)){
       // Set the actual reference/Havok pose, not just the visible NiNode;
       // native grab derives its attachment from the rigid body.
       static Script* placeEmpty=nullptr;if(!placeEmpty)placeEmpty=compile("empty bottle pose","ref target\nfloat x\nfloat y\nfloat z\nfloat ax\nfloat ay\nfloat az\nbegin function {target,x,y,z,ax,ay,az}\ntarget.SetAngleEx ax ay az 1\ntarget.SetPosEx x y z\nSetFunctionValue 1\nend");
       constexpr float deg=57.295779513f;auto& r=replacement.r;
       auto ax=std::atan2(r[2][1],r[2][2])*deg,ay=-std::asin(std::clamp(r[2][0],-1.f,1.f))*deg,az=std::atan2(r[1][0],r[0][0])*deg;
       if(placeEmpty)number(placeEmpty,selectedRef,bits(replacement.p.x),bits(replacement.p.y),bits(replacement.p.z),bits(ax),bits(ay),bits(az));
      }
      Vec head;int result=0;if(pos("Bip01 Head",1,head))result=build_adapter::grabSelected(playerForm(),&head.x,selectedRef);
      if(build_adapter::grabbedItem()==selectedRef&&build_adapter::grabbedHand()==1){aidDropPending=false;weaponTransition=false;held_pose_adapter::pending=false;if(current.h[1].grip>.65f)aidGripLatch=false;stickyAidButtons=aidGripLatch.load();log("NATIVE AID handoff %08X result=%d",heldAid.id,result);tell(heldAid.name+" in right hand");}
      else if(now>=aidDeadline){clearAid();weaponTransition=false;tell("Item returned to inventory; native handoff unavailable");}
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
    }else{weaponHand=pendingHand;selectedWeaponID=pendingWeapon.id;redrawAfter=now+1000;log("Draw request: %s",build_adapter::requestHandDraw(pendingHand));number(draw);weaponTransition=false;tell("Equipped: "+pendingWeapon.name);pendingWeapon={};}
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
   if(id!=selectedWeaponID){selectedWeaponID=0;weaponHand=-1;}
   else if(!selectorOpen&&!heightMenu&&!number(weaponOut)){
    Element target;bool talking=call(dialogueTarget,target)&&target.type==2&&target.value.form;release(target);
    if(!talking&&!number(combatDisabled)&&!number(playerDead)){
     log("RESUME selected weapon %08X: %s",id,build_adapter::requestHandDraw(1));number(draw);redrawAfter=now+1000;
    }
   }
  }
  const auto& l=current.h[0];const auto& r=current.h[1];const auto& oldL=previous.h[0];const auto& oldR=previous.h[1];
  bool b=r.secondary&&!oldR.secondary,a=r.primary&&!oldR.primary,x=l.primary&&!oldL.primary,y=l.secondary&&!oldL.secondary;
  if(heightMenu){
   if(current.h[0].stick&&!previous.h[0].stick){auto m=GetModuleHandleW(L"SeatedInput.dll");using Open=void(__cdecl*)();auto fn=m?reinterpret_cast<Open>(GetProcAddress(m,"SC_OpenControls")):nullptr;if(fn){heightMenu=false;fn();previous=current;return;}}
   if(b){heightMenu=false;saveHeight();tell("Seated height saved",true);}
   else if(y){showSelectorPopups=!showSelectorPopups.load();WritePrivateProfileStringA("UI","ShowSelectorPopups",showSelectorPopups?"1":"0",settingsPath);heightText();}
   else if(a&&physicalHeight>.2f){heightOffset=seatContext?0:std::clamp(desiredHeight-physicalHeight.load(),-.5f,1.2f);heightText();}
   else if(x){heightOffset=0;heightText();}
   else if(std::abs(r.y)>.65f&&(std::abs(oldR.y)<=.65f||now-heightStepAt>300)){heightOffset=std::clamp(heightOffset+(r.y>0?.05f:-.05f),-.5f,seatContext?.5f:1.2f);heightStepAt=now;heightText();}
   previous=current;return;
  }
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
    else {
     auto head=scene::find(root,"Bip01 Head");auto itemRoot=scene::root(selectedRef);
     mouthDistance=-1;
     Vec mouth{};
     if(heldFood&&head&&itemRoot&&current.mouthTime&&now-current.mouthTime<250&&physical_reload_adapter::worldTrackingPoint(current.mouth,scene::world(head).p,mouth)){
      foodMouthPoint=mouth;
      std::vector<scene::Box> mesh;scene::boxes(itemRoot,mesh);
      mouthDistance=mesh.empty()?scene::distance(scene::world(itemRoot).p,mouth):scene::touchDistance(mesh,mouth);
     }
     if(mouthGate.update(heldFood&&mouthDistance>=0,mouthDistance,now))eatStickyFood();
    }
   }
  }
  previous=current;
 }

 bool queryGesture(int hand,float& thumb,float& index,float& lift,float& freeGrip){
  if(!fingerGestures||!gestureGameplayAllowed.load())return false;
  std::lock_guard<std::mutex> lock(inputMutex);auto now=GetTickCount64();auto& g=gestures[hand];
  if(!g.active||now-g.time>250){g.blendTime=0;return false;}
  float trigger=std::clamp(now-physicalAt[hand]<250?physicalTrigger[hand]:input.h[hand].trigger,0.f,1.f);
  float grip=std::clamp(now-physicalAt[hand]<250?physicalGrip[hand]:input.h[hand].grip,0.f,1.f);
  auto vehicleModule=GetModuleHandleW(L"FilthyCasualVehicles.dll");
  auto vehicleActive=vehicleModule?reinterpret_cast<bool(__cdecl*)()>(GetProcAddress(vehicleModule,"FC_VehicleActive")):nullptr;
  freeGrip=(controlsEditor.load()||selectorOpen.load()||heightMenu.load()||controlDrain[hand][3]||(vehicleActive&&vehicleActive()))?grip:-1.f;
  // Thumb contact closes from relaxed to the full authored fist as grip rises.
  // A lifted thumb stays extended, independently of the finger grip.
  float targetThumb=g.thumb?(.20f+.80f*grip):.12f;
  float targetIndex=g.index?std::max(grip,.20f+.80f*trigger):trigger;
  float targetLift=g.thumb?0.f:grip;
  auto contact=vehicleModule?reinterpret_cast<bool(__cdecl*)(int)>(GetProcAddress(vehicleModule,"FC_VehicleHandContact")):nullptr;
  if(contact&&contact(hand)){targetThumb=.62f;targetIndex=.65f+.25f*trigger;targetLift=0;freeGrip=.72f;}

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
   if(cmd.rfind("sc_target ",0)==0){
    const auto expression=std::string("StringToRef \"")+cmd.substr(10)+"\"";
    auto script=compile("target probe",expression.c_str(),true);Element e;
    auto b=build_adapter::heldModule();Vec centre{},tracking{};float extent=0;
    bool ok=call(script,e)&&e.type==2&&e.value.form&&b;
    if(ok)ok=reinterpret_cast<bool(__cdecl*)(void*,Vec*,float*)>(b+0x1A5CB0)(e.value.form,&centre,&extent);
    if(ok){auto manager=reinterpret_cast<void*(__cdecl*)()>(b+0x13903)();
     reinterpret_cast<Vec*(__thiscall*)(void*,Vec*,const Vec*)>(b+0x69AAC0)(manager,&tracking,&centre);}
    std::ofstream probe(std::string(dir)+"\\target-probe.json");
    probe<<"{\"ok\":"<<(ok?"true":"false")<<",\"tracking\":["<<tracking.x<<","<<tracking.y<<","<<tracking.z<<"],\"extent\":"<<extent<<"}";
    release(e);continue;
   }
   if(cmd=="sc_scope_guard"||cmd=="sc_scope_native"){scope_surface_adapter::enabled=(cmd=="sc_scope_guard");continue;}
   if(cmd=="sc_scope_inline"||cmd=="sc_scope_separate"){presentation_adapter::testScopeSchedule(cmd=="sc_scope_inline");continue;}
   if(cmd=="sc_controls"||cmd=="sc_controls_state"){
    auto m=GetModuleHandleW(L"SeatedInput.dll");if(m){if(cmd=="sc_controls"){using F=void(__cdecl*)();auto f=reinterpret_cast<F>(GetProcAddress(m,"SC_OpenControls"));if(f)f();}else{using F=void(__cdecl*)(char*,unsigned);auto f=reinterpret_cast<F>(GetProcAddress(m,"SC_ControlsDebug"));if(f){char data[4096]{};f(data,sizeof(data));std::ofstream out(std::string(dir)+"\\controls-state.json");out<<data;}}}
   }
   else if(cmd.rfind("sc_track_bottle ",0)==0){unsigned id=0;sscanf_s(cmd.c_str()+16,"%x",&id);trackBottle(static_cast<Form*>(physical_reload_adapter::lookup(id)));bottleTrace=true;glass_decal_adapter::diagnostics=true;}
   else if(cmd=="sc_glass_state"){auto center=lastGlassPoint;auto t=physical_reload_adapter::trackingPoint(center);std::ofstream out(std::string(dir)+"\\glass-state.json");out<<"[{\"world\":["<<center.x<<","<<center.y<<","<<center.z<<"],\"tracking\":["<<t.x<<","<<t.y<<","<<t.z<<"]}]";}
   else if(cmd=="sc_state"){
    std::ofstream state(std::string(dir)+"\\addon-state.json");auto rig=scene::root(playerForm());
    state<<"{\"seatContext\":"<<seatContext<<",\"groundHeightOffset\":"<<seatOffsets[0].load()<<",\"pid\":"<<GetCurrentProcessId()<<",\"openHand\":"<<openHand<<",\"aidHand\":"<<aidHand.load()<<",\"weaponHand\":"<<weaponHand.load()<<",\"heightMenu\":"<<(heightMenu?"true":"false")<<",\"heightOffset\":"<<heightOffset.load()<<",\"physicalHeight\":"<<physicalHeight.load()<<",\"menu\":"<<number(getMenu)<<",\"uiEpoch\":"<<uiEpoch.load();
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
    state<<"}";log("TEST STATE captured");
   }
   else if(cmd=="sc_weapons")open(1);
   else if(cmd=="sc_aid")open(0);
   else if(cmd=="sc_settings"){if(selectorOpen)close();heightMenu=true;uiEpoch++;heightText();}
   else if(cmd=="sc_reload")toggleReload();
   else if(cmd.rfind("sc_pick ",0)==0){unsigned id=0;sscanf_s(cmd.c_str()+8,"%x",&id);for(size_t i=0;i<items.size();i++)if(items[i].id==id){selectItem(static_cast<int>(i));break;}}
   else if(cmd.rfind("expr ",0)==0){auto s=compile("test expression",cmd.c_str()+5,true);Element e;bool ok=call(s,e);log("TEST EXPR %s => ok=%d type=%u number=%g form=%08X",cmd.c_str()+5,ok,e.type,e.type==1?e.value.number:0,e.type==2&&e.value.form?e.value.form->id:0);release(e);}
   else {bool ok=consoleApi&&consoleApi->run(cmd.c_str(),nullptr);log("TEST CMD %s => %d",cmd.c_str(),ok);}
  }
 }
 void __cdecl message(Message* m){
  static bool seen[64]{};if(m->type<64&&!seen[m->type]){seen[m->type]=true;log("NVSE message %u dataLen=%u",m->type,m->len);}
  if(m->type==DeferredInit){init();log("RELOAD TRACE %s",reload_trace_adapter::install());log("PHYSICAL RELOAD %s",installPhysicalParts());if(fingerGestures)log("FINGERS %s",finger_adapter::install(queryGesture));log("PRESENTATION %s",presentation_adapter::install(GetPrivateProfileIntA("Presentation","StableWorldPanels",0,settingsPath)!=0));return;}
  // xNVSE Serialization.cpp sends the boolean value cast to void*, not a pointer.
  if(m->type==PostLoadGame){loaded=m->data!=nullptr;log("PostLoadGame loaded=%d",loaded);return;}
  if(m->type==NewGame||m->type==PreLoadGame||m->type==ExitMenu||m->type==ExitGame)native_worldmag_recovery::reset();
  if(m->type==NewGame){clearPhysicalMags();clearWrist();loaded=true;forgetHeldAid();log("NewGame");return;}
  // Native TTWVR owns held-item save/load and teardown. Only discard addon
  // pointers here; never activate/stow a native object during lifecycle events.
  if(m->type==PreLoadGame||m->type==ExitMenu||m->type==ExitGame){clearBottleImpacts();lastEmptyBottle=0;clearTutorialVisual();tutorialStep=reload_tutorial::Step::Hidden;tutorialForm=0;tutorialWeapon=nullptr;clearPhysicalMags();clearWrist();enabled=false;gestureGameplayAllowed=false;weaponHand=-1;selectedWeaponID=0;weaponTransition=false;pendingWeapon={};if(selectorOpen)close();forgetHeldAid();heightMenu=false;loaded=false;return;}
  if(m->type==MainLoop){testMailbox();tick();if(loaded){tickPhysicalMags();tickTutorial();}}
 }
}
extern "C" __declspec(dllexport) u32 __cdecl SC_Input(const char* name,int hand,int kind,float x,float y){
 if(hand<0||hand>1||!name)return 0;
 static std::atomic<bool> announced{false};if(!announced.exchange(true))log("OpenXR input bridge connected");
 bool active=(kind&8)!=0;kind&=7;
 if(!strncmp(name,"fc_diag:",8)){log("GESTURE XR %s result=%d",name+8,int(x));return 0;}
 if(!strcmp(name,"fc_controls")){
  auto now=GetTickCount64();int action=holdState.update(unsigned(x),now,active,enabled&&!selectorOpen&&!heightMenu&&!weaponTransition&&!controlsEditor);
  menuOwned=holdState.menuOwned;controlsAt=now;controlsActive=active;if(action>=0&&!(controlsOverrides.load()&(1u<<action)))pendingControl=action;return 0;
 }
 if(!strcmp(name,"fc_gestures")){
  std::lock_guard<std::mutex> lock(inputMutex);auto& g=gestures[hand];g.active=active;g.thumb=x>.5f;g.index=y>.5f;g.time=GetTickCount64();if(!active)g.blendTime=0;return 0;
 }
 if(!strcmp(name,"fc_physical")){std::lock_guard<std::mutex> lock(inputMutex);physicalGrip[hand]=x;physicalTrigger[hand]=y;physicalAt[hand]=active?GetTickCount64():0;return 0;}
 bool stick=!strcmp(name,"thumbstick_click");
 const bool lootHorizontal=hand==1&&kind==2&&!strcmp(name,"thumbstick")&&floating_hud_adapter::horizontalInput(x,active);
 if(!strcmp(name,"menu_click")&&hand==0){
  bool own=menuOwned||(enabled&&controlsActive&&GetTickCount64()-controlsAt.load()<250);
  return own?1:0;
 }
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
 bool owned=(modal&&!locomotion)||(stickyAidButtons.load()&&aidGripLatch.load()&&control==1)||(control==4&&native_worldmag_recovery::cycleRequiredHand.load()==hand);
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
 scripts=static_cast<Scripts*>(nvse->queryInterface(6));arrays=static_cast<Arrays*>(nvse->queryInterface(5));consoleApi=static_cast<Console*>(nvse->queryInterface(1));auto messaging=static_cast<Messaging*>(nvse->queryInterface(2));
 if(auto data=static_cast<DataInterface*>(nvse->queryInterface(7)))inventoryRef=reinterpret_cast<InventoryRef*(__cdecl*)(u32)>(data->function(2));
 log("SeatedControls 1.6.0-dev.1; runtime %08X; nvse %u",nvse->runtime,nvse->version);
 if(!scripts||!arrays||!messaging||!inventoryRef||!build_adapter::verifiedTTWVR(GetModuleHandleW(L"TTWVR.dll")))return false;
 if(!enableInputLayer()){log("Input layer missing; reinstall the complete addon archive");return false;}
 log("OpenXR input layer enabled for this process");
 readSettings();
 if(!messaging->listen(nvse->getPluginHandle(),"NVSE",message))return false;
 log("Reload preference: %s (applied after native initialization)",automaticReload?"automatic":"manual");
 log("Renderer isolation: %s",build_adapter::testDisableUpscaler());
 // Install only at startup after all load-failure paths. Default off until native QA passes.
 if(GetPrivateProfileIntA("Reload","RecoverWorldMagazines",0,settingsPath))log("WORLD MAG RECOVERY %s",native_worldmag_recovery::install({worldmagPlayer,worldmagInventory}));
 return true;
}
extern "C" __declspec(dllexport) u32 __cdecl SC_UI(char* text,u32 capacity){
 if(!text||capacity<2||!enabled.load())return 0;
 std::lock_guard<std::mutex> lock(displayMutex);
 if(displayText.empty()||(!selectorOpen.load()&&!heightMenu.load()&&GetTickCount64()>displayUntil)){
  auto pool=GetModuleHandleW(L"FilthyCasualPool.dll");auto status=pool?reinterpret_cast<u32(__cdecl*)(char*,u32)>(GetProcAddress(pool,"FC_PoolStatus")):nullptr;
  return status?status(text,capacity):0;
 }
 strncpy_s(text,capacity,displayText.c_str(),_TRUNCATE);return 1;
}

extern "C" __declspec(dllexport) float __cdecl SC_Height(float rawHeadHeight){if(rawHeadHeight>0)physicalHeight=rawHeadHeight;return seatOffsets[0].load();}
extern "C" __declspec(dllexport) float __cdecl SC_SeatOffset(int mode){return mode>0&&mode<5?seatOffsets[mode].load():0;}
extern "C" __declspec(dllexport) bool __cdecl SC_DialogueAnchor(DialogueAnchor* out){return out&&presentation_adapter::readSpeaker(*out);}
extern "C" __declspec(dllexport) void __cdecl SC_Head(float x,float y,float z){std::lock_guard<std::mutex> lock(inputMutex);input.head={x,y,z};input.headValid=std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z);}
extern "C" __declspec(dllexport) unsigned __cdecl SC_UIState(){return (uiEpoch.load()<<2)|(selectorOpen.load()?1:0)|(heightMenu.load()?2:0);}

extern "C" __declspec(dllexport) void __cdecl SC_ControlsState(unsigned modal,unsigned overrides){controlsEditor=modal!=0;controlsOverrides=overrides;}
extern "C" __declspec(dllexport) void __cdecl SC_ControlAction(int action){if(action>=0&&action<=4&&!controlsEditor.load())pendingControl=action;}

extern "C" __declspec(dllexport) bool __cdecl FC_NativePoseOwner(){return native_pose_dispatch::original!=nullptr;}
extern "C" __declspec(dllexport) bool __cdecl FC_NativePoseSubscribe(native_pose_dispatch::Callback before,native_pose_dispatch::Completed after){return native_pose_dispatch::local(before,after);}

extern "C" __declspec(dllexport) void __cdecl SC_Mouth(float x,float y,float z){
 std::lock_guard<std::mutex> lock(inputMutex);
 if(std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)){input.mouth={x,y,z};input.mouthTime=GetTickCount64();}else input.mouthTime=0;
}
