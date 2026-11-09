#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
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
#include <unordered_map>
#include <algorithm>
#include "nvse_api.hpp"
#include "build_adapter.hpp"
#include "reload_adapter.hpp"
#include "scene_adapter.hpp"
#include "wrist_controls.hpp"
#include "finger_adapter.hpp"
#include "hold_controls.hpp"
#include "sticky_food.hpp"
#include "presentation_adapter.hpp"
#include "layer_discovery.hpp"
namespace {
 Scripts* scripts=nullptr;Arrays* arrays=nullptr;Console* consoleApi=nullptr;
 InventoryRef*(__cdecl* inventoryRef)(u32)=nullptr;
 FILE* logfile=nullptr;std::mutex logMutex,inputMutex,displayMutex;
 std::string displayText;ULONGLONG displayUntil=0;
 std::atomic<bool> enabled{false},selectorOpen{false},heightMenu{false},showSelectorPopups{false};
 std::atomic<bool> weaponTransition{false};
 std::atomic<int> weaponHand{-1},aidHand{-1};std::atomic<float> heightOffset{0},physicalHeight{0};
 std::atomic<unsigned> uiEpoch{0};bool controlDrain[2][6]{};
 char settingsPath[MAX_PATH]{};float desiredHeight=1.65f;bool automaticReload=true;
 bool reloadPreferenceApplied=false;
 bool seatedAmmoPouch=true,pouchApplied=false;float pouchForward=.25f,pouchSide=0,pouchBelow=.45f;
 bool wristEnabled=false,wristInserted=false,wristAttached=false;
 hold_controls::State holdState;
 std::atomic<ULONGLONG> controlsAt{0};
 std::atomic<bool> controlsActive{false},menuOwned{false};
 std::atomic<int> pendingControl{-1};
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
 struct Input {Hand h[2];scene::Vec head;bool headValid=false;ULONGLONG time=0;} input,previous;
 bool ready=false,loaded=false,waitingRelease=false;int openHand=-1,page=0,selected=-1;
 struct Settings {float forward=42,below=10,radius=14,reach=2.1f,size=16,capSize=8.4f,walkAway=85;} settings;
 ULONGLONG heldSince[2]{},lastLoop=0,lastAnnounce=0;
 constexpr int slots=6,capSlot=slots,previewCount=slots+1;constexpr float pi=3.14159265359f;
 using Vec=scene::Vec;
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
 struct Preview {std::string holder,attachment;bool inserted=false,attached=false;scene::WorldPin pin;std::vector<scene::Box> mesh;};
 Preview previews[previewCount];
 bool touchMustLeave[2][previewCount]{};
 Script* getCap=nullptr;
 Script *getPlayer=nullptr,*getMenu=nullptr,*getWeapons=nullptr,*getAid=nullptr,*getName=nullptr,*getModel=nullptr,*getCount=nullptr,*getDimension=nullptr,*rootScale=nullptr,*getEquipped=nullptr,*weaponOut=nullptr,*holster=nullptr,*draw=nullptr;
 Script *insert=nullptr,*removeNode=nullptr,*attach=nullptr,*detach=nullptr,*translate=nullptr,*scale=nullptr,*position=nullptr,*equip=nullptr,*notice=nullptr;
 Script *getInventoryRefs=nullptr,*moveRef=nullptr,*takeRef=nullptr;
 Script* isFood=nullptr;
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
  pouchForward=pouchValue("PouchForwardCm",25,10,70);pouchSide=pouchValue("PouchSideCm",0,-60,60);pouchBelow=pouchValue("PouchBelowEyesCm",45,20,120);
  char tmp[40]{};GetPrivateProfileStringA("Seated","HeightOffsetCm","0",tmp,sizeof(tmp),path);heightOffset=std::clamp(float(atof(tmp))*.01f,-.5f,1.2f);
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
  isFood=compile("food flag","ref item\nbegin function {item}\nSetFunctionValue (GetIngestibleFlag item 2)\nend");
  getDimension=compile("dimensions","ref item\nbegin function {item}\nSetFunctionValue (GetObjectDimensions X item + GetObjectDimensions Y item + GetObjectDimensions Z item)\nend");
  insert=compile("insert","string_var n\nbegin function {n}\nSetFunctionValue (InsertNode player 3 \"%z\" n)\nend");
  removeNode=compile("remove node","string_var n\nbegin function {n}\nSetFunctionValue (InsertNode player 2 \"%z\" n)\nend");
  attach=compile("attach","string_var n\nbegin function {n}\nSetFunctionValue (AttachModel player 3 \"%z\" n)\nend");
  detach=compile("detach","string_var n\nbegin function {n}\nSetFunctionValue (AttachModel player 2 \"%z\" n)\nend");
  equip=compile("equip or consume","ref item\nint weapon\nbegin function {item,weapon}\nif player.GetItemCount item > 0\nplayer.EquipItemAlt item 0 1\nif weapon\nplayer.SetWeaponOut 1\nendif\nSetFunctionValue 1\nelse\nSetFunctionValue 0\nendif\nend");
  notice=compile("notice","string_var text\nbegin function {text}\nMessageExAlt 2 \"%z\" text\nSetFunctionValue 1\nend");
  getInventoryRefs=compile("owned item instances","ref item\nbegin function {item}\nSetFunctionValue (player.GetInvRefsForItem item)\nend");
  moveRef=compile("position selected item","ref target\nfloat x\nfloat y\nfloat z\nbegin function {target,x,y,z}\ntarget.MoveTo player\ntarget.SetPos X x\ntarget.SetPos Y y\ntarget.SetPos Z z\nSetFunctionValue 1\nend");
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
 void clearPreviews(){for(auto& p:previews){p.pin.restore();if(p.attached)number(detach,p.attachment.c_str());if(p.inserted)number(removeNode,p.holder.c_str());p={};}}
 void forgetHeldAid(){stickyAidButtons=false;mouthGate.reset();mouthDistance=-1;heldFood=false;aidHand=-1;aidGripLatch=false;aidDropPending=false;selectedRef=nullptr;heldAid={};}
 void clearAid(){
  if(aidHand>=0){
   if(heldAid.id==0x15169)build_adapter::clearStim(playerForm());
   else if(selectedRef&&build_adapter::grabbedItem()==selectedRef)build_adapter::stowGrab(playerForm());
   else if(selectedRef&&aidDropPending)number(takeRef,selectedRef);
  }
  forgetHeldAid();
 }
 void close(){clearPreviews();selectorOpen=false;openHand=-1;selected=-1;items.clear();touchSlot=-1;touchHand=-1;touchSince=0;uiEpoch++;}
 InventoryRef* ownedInstance(Form* item){
  InventoryRef* result=nullptr;Element e;
  if(call(getInventoryRefs,e,item)&&e.type==4&&e.value.array){Element first,key;key.type=1;key.value.number=0;
   if(arrays->get(e.value.array,key,first)&&first.type==2&&first.value.form)result=inventoryRef(first.value.form->id);release(first);}
  release(e);return result&&result->type==item&&result->container==playerForm()?result:nullptr;
 }
 bool holdAid(const Item& item){
  heldAid=item;aidHand=1;aidGripLatch=true;
  mouthGate.reset();mouthDistance=-1;heldFood=item.form&&item.form->type==47&&isFood&&number(isFood,item.form)!=0;
  if(item.id==0x15169){if(!build_adapter::selectStim(playerForm())){clearAid();return false;}log("NATIVE STIM selected right count=%.0f",number(getCount,item.form));return true;}
  auto instance=ownedInstance(item.form);if(!instance){clearAid();return false;}
  double count=number(getCount,item.form);selectedRef=static_cast<Form*>(build_adapter::dropOneAid(playerForm(),item.form,instance->extra));
  if(selectedRef){instance->removed=true;aidDropPending=true;}
  if(number(getCount,item.form)!=count-1){clearAid();return false;}
  if(!selectedRef){log("NATIVE AID drop could not resolve new reference; item remains in world");clearAid();return false;}
  auto palm=scene::palm(scene::root(playerForm()),1);number(moveRef,selectedRef,bits(palm.x),bits(palm.y),bits(palm.z));
  log("NATIVE AID dropped %08X ref=%08X for right hand",item.id,selectedRef->id);return true;
 }
 void eatStickyFood(){
  if(!heldFood||!aidGripLatch||!selectedRef||build_adapter::grabbedItem()!=selectedRef)return;
  // Clear native grab ownership before activation can delete its world ref.
  // Consume through the engine inventory-use path, retaining all normal effects.
  Item food=heldAid;double before=number(getCount,food.form);
  if(auto b=build_adapter::heldModule())log("FOOD stow input wanted=%p primary=(%u,%p,%p,%d) secondary=(%u,%p,%p,%d)",selectedRef,b[0x1114215],*reinterpret_cast<void**>(b+0x11146D0),*reinterpret_cast<void**>(b+0x11146D4),*reinterpret_cast<int*>(b+0xDC53E8),b[0x11142E4],*reinterpret_cast<void**>(b+0x11142EC),*reinterpret_cast<void**>(b+0x11142F0),*reinterpret_cast<int*>(b+0x11142E8));
  if(!build_adapter::stowSelectedGrab(playerForm(),selectedRef)){
   if(auto b=build_adapter::heldModule())log("FOOD stow refused wanted=%p primary=(%u,%p,%d) secondary=(%u,%p,%d) swap=%02X%02X%02X%02X",selectedRef,b[0x1114215],*reinterpret_cast<void**>(b+0x11146D0),*reinterpret_cast<int*>(b+0xDC53E8),b[0x11142E4],*reinterpret_cast<void**>(b+0x11142EC),*reinterpret_cast<int*>(b+0x11142E8),b[0x195D10],b[0x195D11],b[0x195D12],b[0x195D13]);
   mouthGate.reset();return;
  }
  forgetHeldAid();
  double returned=number(getCount,food.form);
  if(returned!=before+1){log("FOOD mouth cancelled: stow count %.0f -> %.0f",before,returned);return;}
  double result=number(equip,food.form,0);
  double after=number(getCount,food.form);
  log("FOOD mouth %08X native-use=%.0f inventory %.0f -> %.0f -> %.0f",food.id,result,before,returned,after);
  if(after==before)tell("Ate: "+food.name);
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
 bool buildPage(){
  clearPreviews();for(auto& hand:touchMustLeave)for(auto& slot:hand)slot=true;auto root=scene::root(playerForm());if(!root)return false;int displayed=0;
  Item cap;Element e;if(call(getCap,e)&&e.type==2&&e.value.form){cap.form=e.value.form;cap.id=cap.form->id;cap.model=string(getModel,cap.form);}release(e);
  for(int i=0;i<previewCount;i++){
   bool neutral=i==capSlot;int index=page*slots+i;if(!neutral&&index>=int(items.size()))continue;auto& item=neutral?cap:items[index];if(item.model.empty())continue;
   auto& p=previews[i];p.holder="SeatedPick"+std::to_string(i);p.attachment=p.holder+"|*SC"+std::to_string(i)+"*"+item.model;
   if(p.attachment.size()>=230)continue;p.inserted=number(insert,p.holder.c_str())!=0;if(!p.inserted)continue;
   p.attached=number(attach,p.attachment.c_str())!=0;if(!p.attached)continue;
   auto node=scene::find(root,p.holder.c_str());if(!node)continue;
   float angle=2*pi*float(i)/slots;auto desired=wheelBasis;
   desired.p=wheelHead+scene::rotate(wheelBasis,{settings.radius*std::sin(angle),settings.forward,0})+Vec{0,0,-settings.below+settings.radius*std::cos(angle)};
   if(neutral){
    desired.p=wheelHead+scene::rotate(wheelBasis,{0,settings.forward,0})+Vec{0,0,-settings.below};
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
   // Preview lifetime is retained explicitly, separate from the moving player.
   log("PIN %d %08X world=%.3f %.3f %.3f parent=%s",i,item.id,desired.p.x,desired.p.y,desired.p.z,scene::name(p.pin.anchor));displayed++;
  }
  selected=-1;touchSlot=-1;touchSince=0;openedAt=GetTickCount64();tell(openHand==1?"Touch to equip; center cap clears your hand":"Touch to hold; center cap returns held aid");return displayed>0;
 }
 void open(int hand){
  if(openHand>=0)close();items.clear();auto player=playerForm();auto root=scene::root(player);if(!root)return;
  Vec head;if(!pos("Bip01 Head",1,head))return;wheelHead=head;wheelRoot=scene::world(root).p;wheelBasis=scene::world(root);wheelBasis.scale=1;wheelCell=scene::cell(player);
  {std::lock_guard<std::mutex> lock(inputMutex);wheelXRHead=input.head;wheelXRValid=input.headValid;}
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
 void heightText(){char text[320];sprintf_s(text,"SEATED HEIGHT: %+d cm | Y: selector popups %s\nRight stick up/down adjusts | A calibrates to %d cm\nX resets | B saves and closes",int(std::round(heightOffset*100)),showSelectorPopups?"ON":"OFF",int(std::round(desiredHeight*100)));tell(text,true);}
 void saveHeight(){char value[32];sprintf_s(value,"%.1f",heightOffset*100);WritePrivateProfileStringA("Seated","HeightOffsetCm",value,settingsPath);log("HEIGHT saved offset=%s cm",value);}
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
 void tick(){
  if(!ready||!loaded){enabled=false;gestureGameplayAllowed=false;return;}auto now=GetTickCount64();if(now-lastLoop<10)return;lastLoop=now;
  if(presentation_adapter::base){
   unsigned id=0;Vec namePoint{};
   if(reinterpret_cast<int(__cdecl*)()>(presentation_adapter::base+0xB3E8)()==0x3F1){
    Element e;if(call(dialoguePartner,e)&&e.type==2&&e.value.form){
     auto actorRoot=scene::root(e.value.form);auto head=scene::find(actorRoot,"Bip01 Head");
     if(head){namePoint=scene::world(head).p;auto nativeSettings=reinterpret_cast<unsigned char*(__cdecl*)()>(presentation_adapter::base+0x8F7B)();
      namePoint.z+=.22f*(*reinterpret_cast<float*>(nativeSettings+0x28));id=e.value.form->id;}
    }release(e);
   }
   presentation_adapter::setSpeaker(id,namePoint);
  }
  Input current;{std::lock_guard<std::mutex> lock(inputMutex);current=input;}
  if(!reloadPreferenceApplied&&build_adapter::setManualReload(!automaticReload)){reloadPreferenceApplied=true;log("RELOAD preference applied: %s",automaticReload?"automatic":"manual");}
  auto player=playerForm();auto root=scene::root(player);bool usable=root&&number(getMenu)==0&&now-current.time<500&&current.h[0].active&&current.h[1].active;
  enabled=usable;
  gestureGameplayAllowed=usable&&!weaponTransition;
  static ULONGLONG nextGestureLog=0;static unsigned gestureLogs=0;
  if(now>=nextGestureLog&&gestureLogs<90){nextGestureLog=now+2000;++gestureLogs;
   std::lock_guard<std::mutex> lock(inputMutex);
   log("GESTURE gameplay=%d usable=%d controls=%d L(active=%d thumb=%d index=%d grip=%.2f trigger=%.2f applied=%lu) R(active=%d thumb=%d index=%d grip=%.2f trigger=%.2f applied=%lu)",int(gestureGameplayAllowed.load()),usable,int(controlsActive.load()),gestures[0].active,gestures[0].thumb,gestures[0].index,input.h[0].grip,input.h[0].trigger,finger_adapter::applied[0].load(),gestures[1].active,gestures[1].thumb,gestures[1].index,input.h[1].grip,input.h[1].trigger,finger_adapter::applied[1].load());
  }
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
      Vec head;int result=0;if(pos("Bip01 Head",1,head))result=build_adapter::grabSelected(playerForm(),&head.x,selectedRef);
      if(build_adapter::grabbedItem()==selectedRef&&build_adapter::grabbedHand()==1){aidDropPending=false;weaponTransition=false;stickyAidButtons=true;log("NATIVE AID handoff %08X result=%d",heldAid.id,result);tell(heldAid.name+" in right hand");}
      else if(now>=aidDeadline){clearAid();weaponTransition=false;tell("Item returned to inventory; native handoff unavailable");}
      else {pendingAt=now+100;auto palm=scene::palm(root,1);number(moveRef,selectedRef,bits(palm.x),bits(palm.y),bits(palm.z));}
     }
    }else if(pendingStage==1){double result=number(equip,pendingWeapon.form,0);number(holster);log("TOUCH EQUIP %08X hand=%d result=%.0f",pendingWeapon.id,pendingHand,result);
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
   if(b){heightMenu=false;saveHeight();tell("Seated height saved",true);}
   else if(y){showSelectorPopups=!showSelectorPopups.load();WritePrivateProfileStringA("UI","ShowSelectorPopups",showSelectorPopups?"1":"0",settingsPath);heightText();}
   else if(a&&physicalHeight>.2f){heightOffset=std::clamp(desiredHeight-physicalHeight.load(),-.5f,1.2f);heightText();}
   else if(x){heightOffset=0;heightText();}
   else if(std::abs(r.y)>.65f&&(std::abs(oldR.y)<=.65f||now-heightStepAt>300)){heightOffset=std::clamp(heightOffset+(r.y>0?.05f:-.05f),-.5f,1.2f);heightStepAt=now;heightText();}
   previous=current;return;
  }
  if(b&&selectorOpen){close();tell("Selector closed");previous=current;return;}
  if(selectorOpen){
   if(wheelXRValid&&current.headValid&&scene::distance(current.head,wheelXRHead)>1.2f){close();tell("Selector dismissed");previous=current;return;}
   if(l.x*l.x+l.y*l.y>.16f){close();tell("Selector dismissed");previous=current;return;}
   if(scene::cell(player)!=wheelCell||scene::distance(scene::world(root).p,wheelRoot)>settings.walkAway){close();tell("Selector dismissed");previous=current;return;}
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
   else if(r.grip>.65f&&oldR.grip<=.65f){aidGripLatch=false;stickyAidButtons=false;mouthGate.reset();log("Aid grip: native control resumed");}
   else if(aidGripLatch&&heldAid.id!=0x15169){
    stickyAidButtons=true;
    if(a||x){if(build_adapter::stowSelectedGrab(player,selectedRef)){forgetHeldAid();tell("Item returned to inventory");}else mouthGate.reset();}
    else {
     auto head=scene::find(root,"Bip01 Head");auto itemRoot=scene::root(selectedRef);
     mouthDistance=-1;
     if(heldFood&&head&&itemRoot){
      Vec mouth=scene::world(head).p;mouth.z-=7.f;foodMouthPoint=mouth;
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
  float trigger=std::clamp(input.h[hand].trigger,0.f,1.f);
  float grip=std::clamp(input.h[hand].grip,0.f,1.f);
  freeGrip=(selectorOpen.load()||heightMenu.load()||controlDrain[hand][3])?grip:-1.f;
  // Thumb contact closes from relaxed to the full authored fist as grip rises.
  // A lifted thumb stays extended, independently of the finger grip.
  float targetThumb=g.thumb?(.20f+.80f*grip):.12f;
  float targetIndex=g.index?std::max(grip,.20f+.80f*trigger):trigger;
  float targetLift=g.thumb?0.f:grip;
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
    if(ok)ok=reinterpret_cast<bool(__cdecl*)(void*,Vec*,float*)>(b+0x1929B0)(e.value.form,&centre,&extent);
    if(ok){auto manager=reinterpret_cast<void*(__cdecl*)()>(b+0x101E0)();
     reinterpret_cast<Vec*(__thiscall*)(void*,Vec*,const Vec*)>(b+0x572320)(manager,&tracking,&centre);}
    std::ofstream probe(std::string(dir)+"\\target-probe.json");
    probe<<"{\"ok\":"<<(ok?"true":"false")<<",\"tracking\":["<<tracking.x<<","<<tracking.y<<","<<tracking.z<<"],\"extent\":"<<extent<<"}";
    release(e);continue;
   }
   if(cmd=="sc_scope_guard"||cmd=="sc_scope_native"){scope_surface_adapter::enabled=(cmd=="sc_scope_guard");continue;}
   if(cmd=="sc_scope_inline"||cmd=="sc_scope_separate"){presentation_adapter::testScopeSchedule(cmd=="sc_scope_inline");continue;}
   if(cmd=="sc_state"){
    std::ofstream state(std::string(dir)+"\\addon-state.json");auto rig=scene::root(playerForm());
    state<<"{\"pid\":"<<GetCurrentProcessId()<<",\"openHand\":"<<openHand<<",\"aidHand\":"<<aidHand.load()<<",\"weaponHand\":"<<weaponHand.load()<<",\"heightMenu\":"<<(heightMenu?"true":"false")<<",\"heightOffset\":"<<heightOffset.load()<<",\"physicalHeight\":"<<physicalHeight.load()<<",\"menu\":"<<number(getMenu)<<",\"uiEpoch\":"<<uiEpoch.load();
    state<<",\"dialoguePlacements\":"<<presentation_adapter::placements.load()<<",\"dialogueTargetDistance\":"<<presentation_adapter::lastDistance.load()<<",\"dialogueSideDegrees\":"<<presentation_adapter::lastAngle.load();
    presentation_adapter::diagnostics(state);
    {DialogueAnchor anchor{};bool valid=presentation_adapter::readSpeaker(anchor);state<<",\"speakerSourceId\":"<<presentation_adapter::speakerId<<",\"speakerFrameUpdates\":"<<presentation_adapter::speakerFrameUpdates.load()<<",\"speakerNameActive\":"<<(valid?"true":"false")<<",\"speakerNameTracking\":["<<anchor.position[0]<<","<<anchor.position[1]<<","<<anchor.position[2]<<"]";
     using Debug=void(__cdecl*)(char*,unsigned);auto layer=GetModuleHandleW(L"SeatedInput.dll");auto debug=layer?reinterpret_cast<Debug>(GetProcAddress(layer,"SC_LabelDebug")):nullptr;
     if(debug){char json[512]{};debug(json,sizeof(json));state<<",\"speakerNameCrop\":"<<json;}}
    auto grab=static_cast<Form*>(build_adapter::grabbedItem());auto grabRoot=scene::root(grab);
    state<<",\"nativeStimHand\":"<<build_adapter::stimHand()<<",\"nativeGrabHand\":"<<build_adapter::grabbedHand()<<",\"nativeGrabRef\":"<<(grab?grab->id:0)<<",\"nativeGrabScale\":"<<(grabRoot?scene::world(grabRoot).scale:0)<<",\"aidGripLatch\":"<<(aidGripLatch?"true":"false");
    state<<",\"heldFood\":"<<(heldFood?"true":"false")<<",\"mouthDistance\":"<<mouthDistance<<",\"mouthArmed\":"<<(mouthGate.armed?"true":"false")<<",\"freePoseWeight\":["<<finger_adapter::freePoseWeight[0].load()<<","<<finger_adapter::freePoseWeight[1].load()<<"]";
    bool manual=false;bool reloadKnown=build_adapter::readManualReload(manual);state<<",\"manualReloadKnown\":"<<(reloadKnown?"true":"false")<<",\"manualReload\":"<<(manual?"true":"false")<<",\"seatedPouchApplied\":"<<(pouchApplied?"true":"false");
    build_adapter::PouchDiagnostics pouch;
    if(build_adapter::readPouchDiagnostics(pouch)){
     auto node=[&](const build_adapter::PouchNodeDiagnostics& n){state<<"{\"readable\":"<<(n.readable?"true":"false")<<",\"culled\":"<<(n.culled?"true":"false")<<",\"position\":["<<n.world[0]<<","<<n.world[1]<<","<<n.world[2]<<"],\"scale\":"<<n.scale<<"}";};
     state<<",\"pouch\":{\"spareEnabled\":"<<(pouch.spareEnabled?"true":"false")<<",\"anchor\":"<<(pouch.torsoAnchor?"true":"false")<<",\"shown\":"<<(pouch.primaryShown?"true":"false")<<",\"mode\":"<<pouch.nativeMode<<",\"unitsPerMetre\":"<<pouch.metresToUnits<<",\"primary\":";node(pouch.primary);state<<",\"copy\":";node(pouch.copy);
     if(pouch.primary.readable){auto t=scene::world(pouch.primaryNode);auto center=t.p;if(pouch.spareCenterValid)center=center+scene::rotate(t,{pouch.spareLocalCenter[0],pouch.spareLocalCenter[1],pouch.spareLocalCenter[2]})*t.scale;state<<",\"center\":["<<center.x<<","<<center.y<<","<<center.z<<"]";}
     state<<"}";
    }
    auto vec=[&](Vec p){state<<"["<<p.x<<","<<p.y<<","<<p.z<<"]";};
    state<<",\"mouthPoint\":";vec(foodMouthPoint);state<<",\"heldCenter\":";
    {std::vector<scene::Box> mesh;if(grabRoot)scene::boxes(grabRoot,mesh);vec(mesh.empty()?Vec{}:scene::boxCenter(mesh));}
    state<<",\"wrist\":{\"attached\":"<<(wristNode?"true":"false")<<",\"valid\":"<<(wristFrame.valid?"true":"false")<<",\"facing\":"<<(wristFrame.facing?"true":"false")<<",\"tip\":";vec(wristFrame.fingertip);state<<",\"origin\":";vec(wristFrame.world.p);state<<",\"normal\":";vec(wrist_controls::axis(wristFrame.world,2));state<<",\"targets\":[";for(int i=0;i<4;i++){if(i)state<<",";vec(wrist_controls::buttonWorld(wristFrame,i));}state<<"]}";
    state<<",\"wristEye\":";vec(wristFrame.eye);
    state<<",\"leftFingertips\":[";const char* fingers[]={"Bip01 L Thumb12","Bip01 L Finger12","Bip01 L Finger22","Bip01 L Finger32","Bip01 L Finger42"};
    for(int i=0;i<5;i++){if(i)state<<",";auto n=scene::find(rig,fingers[i]);vec(n?scene::world(n).p:Vec{});}state<<"]";
    state<<",\"gestureGameplayAllowed\":"<<(gestureGameplayAllowed?"true":"false")<<",\"fingerGestures\":[";
    {std::lock_guard<std::mutex> lock(inputMutex);for(int h=0;h<2;h++){if(h)state<<",";auto& g=gestures[h];state<<"{\"active\":"<<(g.active?"true":"false")<<",\"thumb\":"<<g.thumbCurl<<",\"index\":"<<g.indexCurl<<",\"lift\":"<<g.lift<<",\"applied\":"<<finger_adapter::applied[h].load()<<",\"tips\":[";
     for(int f=0;f<5;f++){if(f)state<<",";char name[40];sprintf_s(name,"Bip01 %c %s",h?'R':'L',f==0?"Thumb12":f==1?"Finger12":f==2?"Finger22":f==3?"Finger32":"Finger42");auto n=scene::find(rig,name);auto w=scene::find(rig,h?"Bip01 R Hand":"Bip01 L Hand");vec(n&&w?scene::inverseRotate(scene::world(w),scene::world(n).p-scene::world(w).p):Vec{});}state<<"]}";}}state<<"]";
    state<<",\"wristPlungers\":[";for(int i=0;i<5;i++){if(i)state<<",";auto& p=wristPlungers[i];state<<"{\"found\":"<<(p.node?"true":"false")<<",\"depression\":"<<p.depression<<",\"localZ\":"<<(p.node?scene::local(p.node).p.z:0)<<"}";}state<<"]";
    state<<",\"hands\":[";for(int h=0;h<2;h++){if(h)state<<",";auto wrist=scene::find(rig,h?"Bip01 R Hand":"Bip01 L Hand");vec(wrist?scene::world(wrist).p:Vec{});}state<<"],\"palms\":[";vec(scene::palm(rig,0));state<<",";vec(scene::palm(rig,1));
    state<<"],\"previews\":[";bool first=true;for(int i=0;i<previewCount;i++){auto& p=previews[i];if(!p.pin.node)continue;if(!first)state<<",";first=false;state<<"{\"slot\":"<<i<<",\"form\":"<<(i==capSlot?15:items[page*slots+i].id)<<",\"center\":";Vec extent;vec(p.mesh.empty()?scene::world(p.pin.node).p:scene::boxCenter(p.mesh,&extent));state<<",\"extent\":";vec(extent);state<<"}";}state<<"]";
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
  if(m->type==DeferredInit){init();if(fingerGestures)log("FINGERS %s",finger_adapter::install(queryGesture));log("PRESENTATION %s",presentation_adapter::install(GetPrivateProfileIntA("Presentation","StableWorldPanels",0,settingsPath)!=0));return;}
  // xNVSE Serialization.cpp sends the boolean value cast to void*, not a pointer.
  if(m->type==PostLoadGame){loaded=m->data!=nullptr;log("PostLoadGame loaded=%d",loaded);return;}
  if(m->type==NewGame){clearWrist();loaded=true;forgetHeldAid();log("NewGame");return;}
  // Native TTWVR owns held-item save/load and teardown. Only discard addon
  // pointers here; never activate/stow a native object during lifecycle events.
  if(m->type==PreLoadGame||m->type==ExitMenu||m->type==ExitGame){clearWrist();enabled=false;gestureGameplayAllowed=false;weaponHand=-1;selectedWeaponID=0;weaponTransition=false;pendingWeapon={};if(selectorOpen)close();forgetHeldAid();heightMenu=false;loaded=false;return;}
  if(m->type==MainLoop){testMailbox();tick();}
 }
}
extern "C" __declspec(dllexport) u32 __cdecl SC_Input(const char* name,int hand,int kind,float x,float y){
 if(hand<0||hand>1||!name)return 0;
 static std::atomic<bool> announced{false};if(!announced.exchange(true))log("OpenXR input bridge connected");
 bool active=(kind&8)!=0;kind&=7;
 if(!strncmp(name,"fc_diag:",8)){log("GESTURE XR %s result=%d",name+8,int(x));return 0;}
 if(!strcmp(name,"fc_controls")){
  auto now=GetTickCount64();int action=holdState.update(unsigned(x),now,active,enabled&&!selectorOpen&&!heightMenu&&!weaponTransition);
  menuOwned=holdState.menuOwned;controlsAt=now;controlsActive=active;if(action>=0)pendingControl=action;return 0;
 }
 if(!strcmp(name,"fc_gestures")){
  std::lock_guard<std::mutex> lock(inputMutex);auto& g=gestures[hand];g.active=active;g.thumb=x>.5f;g.index=y>.5f;g.time=GetTickCount64();if(!active)g.blendTime=0;return 0;
 }
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
 bool modal=selectorOpen.load()||heightMenu.load()||weaponTransition.load();
 int control=stick?0:!strcmp(name,"primary_click")?1:!strcmp(name,"secondary_click")?2:!strcmp(name,"squeeze")?3:!strcmp(name,"trigger")?4:!strcmp(name,"thumbstick")?5:-1;
 if(control<0)return 0;
 bool pressed=kind==2?(x*x+y*y>.09f):x>.15f;
 bool locomotion=selectorOpen.load()&&!heightMenu.load()&&hand==0&&control==5;
 bool owned=(modal&&!locomotion)||(stickyAidButtons.load()&&aidGripLatch.load()&&control==1);
 bool suppress=owned;
 {std::lock_guard<std::mutex> lock(inputMutex);auto& drain=controlDrain[hand][control];
  if(owned&&pressed)drain=true;if(drain){suppress=true;if(!pressed)drain=false;}}
 if(active&&weaponHand.load()==hand&&control==3)return 2;
 return suppress?1:lootHorizontal?3:0;
}
extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Query(const NVSE* nvse,PluginInfo* info){info->infoVersion=1;info->name="SeatedControls";info->version=2;return !nvse->isEditor&&nvse->runtime==0x040020D0&&nvse->version>=6;}
extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Load(const NVSE* nvse){
 std::string path=std::string(nvse->runtimeDirectory())+"SeatedControls.log";logfile=_fsopen(path.c_str(),"w",_SH_DENYNO);
 scripts=static_cast<Scripts*>(nvse->queryInterface(6));arrays=static_cast<Arrays*>(nvse->queryInterface(5));consoleApi=static_cast<Console*>(nvse->queryInterface(1));auto messaging=static_cast<Messaging*>(nvse->queryInterface(2));
 if(auto data=static_cast<DataInterface*>(nvse->queryInterface(7)))inventoryRef=reinterpret_cast<InventoryRef*(__cdecl*)(u32)>(data->function(2));
 log("SeatedControls 1.4.5; runtime %08X; nvse %u",nvse->runtime,nvse->version);
 if(!scripts||!arrays||!messaging||!inventoryRef||!build_adapter::verifiedTTWVR(GetModuleHandleW(L"TTWVR.dll")))return false;
 if(!enableInputLayer()){log("Input layer missing; reinstall the complete addon archive");return false;}
 log("OpenXR input layer enabled for this process");
 readSettings();
 if(!messaging->listen(nvse->getPluginHandle(),"NVSE",message))return false;
 log("Reload preference: %s (applied after native initialization)",automaticReload?"automatic":"manual");
 log("Renderer isolation: %s",build_adapter::testDisableUpscaler());
 return true;
}
extern "C" __declspec(dllexport) u32 __cdecl SC_UI(char* text,u32 capacity){
 if(!text||capacity<2||!enabled.load())return 0;
 std::lock_guard<std::mutex> lock(displayMutex);
 if(displayText.empty()||(!selectorOpen.load()&&!heightMenu.load()&&GetTickCount64()>displayUntil))return 0;
 strncpy_s(text,capacity,displayText.c_str(),_TRUNCATE);return 1;
}

extern "C" __declspec(dllexport) float __cdecl SC_Height(float rawHeadHeight){if(rawHeadHeight>0)physicalHeight=rawHeadHeight;return heightOffset.load();}
extern "C" __declspec(dllexport) bool __cdecl SC_DialogueAnchor(DialogueAnchor* out){return out&&presentation_adapter::readSpeaker(*out);}
extern "C" __declspec(dllexport) void __cdecl SC_Head(float x,float y,float z){std::lock_guard<std::mutex> lock(inputMutex);input.head={x,y,z};input.headValid=std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z);}
extern "C" __declspec(dllexport) unsigned __cdecl SC_UIState(){return (uiEpoch.load()<<2)|(selectorOpen.load()?1:0)|(heightMenu.load()?2:0);}
