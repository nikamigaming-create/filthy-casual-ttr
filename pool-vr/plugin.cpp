#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstddef>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <atomic>
#include <mutex>
#include <string>
#include <fstream>
#include <share.h>
#include "../nvse_api.hpp"
#include "../build_adapter.hpp"
#include "../scene_adapter.hpp"
#include "cue.hpp"
#include "adapter.hpp"
#include "stroke_contract.hpp"
#include "../layer_discovery.hpp"
namespace {
// Public xNVSE 6 event interface, prefix through SetNativeEventHandler.
struct Events {
 bool(__cdecl* reg)(const char*,uint8_t,uint8_t*,uint32_t);
 void* dispatch;void* dispatchAlt;
 bool(__cdecl* listen)(const char*,void(__cdecl*)(Form*,void*));
};
Scripts* scripts{};Arrays* arrays{};FILE* file{};std::mutex mutex;
std::atomic<ULONGLONG> liveUntil{0};bool loaded=false,ready=false;
struct Hand {float grip=0,trigger=0;bool b=false;bool active=false,poseValid=false;ULONGLONG time=0,poseTime=0;unsigned poseEpoch=0;};
Hand hands[2];bool drain[2][6]{};bool priorTrigger=false;
unsigned poseEpochs[2]{};
poolcue::Stroke stroke;scene::Transform cuePose{};bool cueVisible=false;
Script *getPlayer{},*request{},*start{},*init{},*clearRequest{},*closeMenus{};
Form* idleBank[351]{};
Script *menuMode{},*holster{},*dialogueTarget{};int pendingRequest=0;ULONGLONG pendingAt=0,pendingClearAt=0;
Form* player{};unsigned frame=0;std::string testDir;
void log(const char* f,...){if(!file)return;va_list a;va_start(a,f);vfprintf(file,f,a);va_end(a);fputc('\n',file);fflush(file);}
bool enablePoolInput(){
 wchar_t path[MAX_PATH]{};if(!GetModuleFileNameW(GetModuleHandleW(L"FilthyCasualPool.dll"),path,MAX_PATH))return false;
 auto slash=wcsrchr(path,L'\\');if(!slash)return false;*slash=0;
 std::wstring dir=std::wstring(path)+L"\\FilthyCasualPool";
 if(GetFileAttributesW((dir+L"\\FilthyCasualPoolInput.json").c_str())==INVALID_FILE_ATTRIBUTES)return false;
 return layer_discovery::append(L"XR_API_LAYER_PATH",dir)&&layer_discovery::append(L"XR_ENABLE_API_LAYERS",L"XR_APILAYER_NIKAMI_pool");
}
void release(Element& e){if(e.type==3&&e.value.string)build_adapter::freeGameString(e.value.string);}
template<class... A> Element call(Script* fn,A... a){Element out;if(fn)scripts->call(fn,nullptr,nullptr,&out,sizeof...(a),a...);return out;}
Script* compile(const char* text,bool expression=false){auto s=expression?scripts->expression(text):scripts->compile(text);if(!s)log("FAILED compile: %s",text);return s;}
Element key(const char* s){Element e;e.type=3;e.value.string=const_cast<char*>(s);return e;}
Element idx(int i){Element e;e.type=1;e.value.number=i;return e;}
Element get(void* a,Element k){Element e;if(a)arrays->get(a,k,e);return e;}
double num(void* a,const char* k){auto e=get(a,key(k));double n=e.type==1?e.value.number:0;release(e);return n;}
double at(void* a,int i){auto e=get(a,idx(i));double n=e.type==1?e.value.number:0;release(e);return n;}
void* arr(void* a,const char* k){auto e=get(a,key(k));return e.type==4?e.value.array:nullptr;}
Form* form(void* a,const char* k){auto e=get(a,key(k));return e.type==2?e.value.form:nullptr;}
void set(void* a,const char* k,double n){Element v;v.type=1;v.value.number=n;reinterpret_cast<void(__cdecl*)(void*,const Element&,const Element&)>(arrays->set)(a,key(k),v);}
void setForm(void* a,const char* k,Form* f){Element v;v.type=2;v.value.form=f;reinterpret_cast<void(__cdecl*)(void*,const Element&,const Element&)>(arrays->set)(a,key(k),v);}
void* state(void* p){if(!p)return nullptr;auto a=arrays->lookup(reinterpret_cast<u32>(*static_cast<void**>(p)));return a&&form(a,"table")&&form(a,"table")->id==0x107D7B?a:nullptr;}
poolcue::V v(scene::Vec a){return {a.x,a.y,a.z};}
scene::Vec s(poolcue::V a){return {a.x,a.y,a.z};}
// Only our cue node is reparented. Retain/restore its original scene parent;
// no actor data, save data or native hand animation is replaced.
struct CueAttachment {
 void *node=nullptr,*home=nullptr,*hand=nullptr;scene::Transform original{};
 void restore(){if(!node)return;scene::moveParent(node,home);scene::local(node)=original;scene::update(node);scene::release(hand);scene::release(home);scene::release(node);node=home=hand=nullptr;}
 void bind(void* source,void* target,const scene::Transform& pose){
  if(!source||!target)return;
  if(node!=source||hand!=target){restore();if(!scene::parent(source))return;node=source;home=scene::parent(source);hand=target;original=scene::local(node);scene::retain(node);scene::retain(home);scene::retain(hand);scene::moveParent(node,hand);}
  scene::local(node)=pose;scene::update(node);
 }
} cueAttachment;
bool npcCue=false;scene::Transform npcCueLocal{};
void clear(){cueAttachment.restore();npcCue=false;liveUntil=0;stroke.reset();cueVisible=false;priorTrigger=false;}
void __cdecl suspend(Form*,void* params){if(auto a=state(params)){set(a,"actorGapValid",0);set(a,"actorClockStall",0);}clear();}
void __cdecl check(Form*,void* params){
 auto a=state(params);if(!a)return;
 if(GetModuleHandleW(L"SeatedControls.dll")){
  auto layer=GetModuleHandleW(L"SeatedInput.dll");auto compatible=layer?reinterpret_cast<unsigned(__cdecl*)(uint64_t)>(GetProcAddress(layer,"SC_PoolBridgeVersion")):nullptr;
  if(!compatible||!compatible(0)){set(a,"canStart",0);log("Pool requires Filthy Casual TTR 1.4.5 or newer when the optional seated addon is installed");return;}
 }
 auto actor=form(a,"opponentRef");auto package=pooladapter::backupPackage(actor);
 auto prior=form(a,"priorPackage");
 // GetCurrentAIPackage uses script IDs: Wander=13, Sandbox=36. Sunny's
 // ordinary idle routine is the owned master's DefaultStayAtEditorLocation.
 set(a,"canStart",!actor||(!package&&((prior&&(prior->id==0x200D5||prior->id==0x105CC7))||num(a,"priorAIType")==36||num(a,"priorAIType")==13)));
 log("Start ownership: actor=%08X backupPackage=%08X priorAIType=%g priorPackage=%08X",actor?actor->id:0,package?package->id:0,num(a,"priorAIType"),prior?prior->id:0);
}
void __cdecl releaseSunny(Form*,void* params){
 auto a=state(params);if(!a)return;
 auto own=form(a,"actorPackage"),backup=pooladapter::backupPackage(form(a,"opponentRef"));
 set(a,"removePackage",own&&(backup==own||form(a,"stablePackage")==own));
}
scene::Vec tableToWorld(const scene::Transform& t,float yaw,float x,float y){
 float sn=std::sin(yaw),cs=std::cos(yaw),lx=-109.777778f+x/4.5f,ly=55.111111f-y/4.5f;
 return {t.p.x+(lx*cs+ly*sn)*t.scale,t.p.y+(-lx*sn+ly*cs)*t.scale,t.p.z+27.566667f*t.scale};
}
scene::Transform calibratedCue(scene::Vec tip,scene::Vec axis,float scale){
 auto side=scene::cross({0,0,1},axis);float n=scene::distance(side,{});scene::Transform out{};out.scale=scale;if(n<.001f)return out;
 side=side*(1/n);auto up=scene::cross(axis,side);float basis[3][3]{};
 for(int i=0;i<3;i++){basis[i][0]=(&axis.x)[i];basis[i][1]=(&side.x)[i];basis[i][2]=(&up.x)[i];}
 for(int i=0;i<3;i++)for(int j=0;j<3;j++)for(int k=0;k<3;k++)out.r[i][j]+=basis[i][k]*poolmotion::SourceToAxis[k][j];
 out.p=tip-scene::rotate(out,poolmotion::SourceTip)*scale;return out;
}
void orientCue(scene::Vec tip,scene::Vec axis,float scale){cuePose=calibratedCue(tip,axis,scale);}
void __cdecl draw(Form*,void* params){
 auto a=state(params);if(!a)return;
 if(!cueVisible){cueAttachment.restore();return;}
 auto root=scene::root(form(a,"cueRef"));if(!root){cueAttachment.restore();return;}
 if(npcCue){
  auto hand=scene::find(scene::root(form(a,"opponentRef")),"Bip01 R Hand");
  if(hand)cueAttachment.bind(root,hand,npcCueLocal);else cueAttachment.restore();
 }else{cueAttachment.restore();scene::setWorld(root,cuePose);}
}
void __cdecl candidate(Form*,void* params){
 auto a=state(params);if(!a)return;auto balls=arr(a,"balls");auto e=get(balls,idx(0));auto ball=e.type==4?e.value.array:nullptr;
 set(a,"candidateReachable",ball&&poolcue::stanceDistance(float(at(ball,0)),float(at(ball,1)),float(num(a,"candidateNX")),float(num(a,"candidateNY")))<=poolcue::MaxStanceReach);
}
void __cdecl update(Form*,void* params){
 auto a=state(params);if(!a||!player)return;
 Hand h[2];{std::lock_guard<std::mutex> lock(mutex);h[0]=hands[0];h[1]=hands[1];}
 auto now=GetTickCount64();liveUntil=now+200;
 set(a,"vrShot",0);set(a,"vrPlace",0);set(a,"vrShowCue",0);set(a,"actorContact",0);cueVisible=false;npcCue=false;
 if(h[1].b){set(a,"vrEnd",1);clear();return;}
 auto table=form(a,"table");auto root=scene::root(table);auto rig=scene::root(player);
 if(!root||!rig){stroke.reset();return;}
 auto t=scene::world(root);float yaw=float(num(a,"yaw"))*.01745329252f;
 auto balls=arr(a,"balls");auto e=get(balls,idx(0));auto ball=e.type==4?e.value.array:nullptr;if(!ball)return;
 auto ballWorld=tableToWorld(t,yaw,float(at(ball,0)),float(at(ball,1)));
 bool ownTurn=num(a,"turn")==0&&!num(a,"finished")&&!num(a,"moving");
 for(int i=0;i<2;i++)if(poseEpochs[i]!=h[i].poseEpoch){stroke.reset();poseEpochs[i]=h[i].poseEpoch;}
 bool fresh=h[0].active&&h[1].active&&h[0].poseValid&&h[1].poseValid&&now-h[0].time<200&&now-h[1].time<200&&now-h[0].poseTime<200&&now-h[1].poseTime<200;
 auto left=scene::palm(rig,0),right=scene::palm(rig,1);
 if(fresh&&ownTurn&&num(a,"inhand")){
  auto d=(right-t.p)*(1/t.scale);float cs=std::cos(yaw),sn=std::sin(yaw);
  set(a,"vrBallX",std::clamp((d.x*cs-d.y*sn+109.777778f)*4.5f,14.f,986.f));
  set(a,"vrBallY",std::clamp((55.111111f-d.x*sn-d.y*cs)*4.5f,14.f,486.f));
  bool trigger=h[1].trigger>.6f;set(a,"vrPlace",trigger&&!priorTrigger);priorTrigger=trigger;
  stroke.reset();return;
 }
 priorTrigger=h[1].trigger>.6f;
 auto delta=left-right;float separation=scene::distance(left,right);
 bool usable=fresh&&ownTurn&&h[0].grip>.6f&&h[1].grip>.6f&&separation>12&&separation<85;
 if(usable){
  // The supplied cue spans X=-102.8307..0. Rear grip at -92 leaves a
  // 10.8-unit butt behind the palm; the forward hand slides on the shaft.
  auto axis=delta*(1/separation);auto tip=right+axis*92.f;
  orientCue(tip,axis,1);cueVisible=true;
  float speed=stroke.update(v(tip)*(1/70.f),v(axis),v(ballWorld)*(1/70.f),float(num(a,"dt")),!num(a,"inhand"));
  float cs=std::cos(yaw),sn=std::sin(yaw);
  float nx=axis.x*cs-axis.y*sn,ny=-axis.x*sn-axis.y*cs,n=std::sqrt(nx*nx+ny*ny);
  if(speed>0&&n>.25f){set(a,"vrNX",nx/n);set(a,"vrNY",ny/n);set(a,"vrShot",std::clamp(speed*70.f*4.5f/t.scale,30.f,1600.f));log("Stroke speed=%.3f m/s",speed);}
 }else stroke.reset();
 if(num(a,"turn")&&num(a,"aiPlanned")&&(!num(a,"moving")||num(a,"actorFollow")>0)){
  float nx=float(num(a,"nx")),ny=float(num(a,"ny"));float cs=std::cos(yaw),sn=std::sin(yaw);
  if(!num(a,"actorPositioned")&&poolcue::stanceDistance(float(at(ball,0)),float(at(ball,1)),nx,ny)>poolcue::MaxStanceReach){
   // A legal defensive shot from the nearest rail when no reachable pot exists.
   float x=float(at(ball,0)),y=float(at(ball,1));float sides[]{x,1000.f-x,y,500.f-y};
   int side=int(std::min_element(sides,sides+4)-sides);nx=side==0?1.f:side==1?-1.f:0.f;ny=side==2?1.f:side==3?-1.f:0.f;
   set(a,"nx",nx);set(a,"ny",ny);set(a,"aiSpeed",350);set(a,"aiTargetBall",0);log("Reachable defensive fallback");
  }
  scene::Vec axis{nx*cs-ny*sn,-nx*sn-ny*cs,0};
  if(!num(a,"actorPositioned")){
   float bx=-109.777778f+float(at(ball,0))/4.5f,by=55.111111f-float(at(ball,1))/4.5f;
   float dx=-nx,dy=ny,exit=10000.f;
   if(std::abs(dx)>.0001f)exit=std::min(exit,((dx>0?115.f:-115.f)-bx)/dx);
   if(std::abs(dy)>.0001f)exit=std::min(exit,((dy>0?62.f:-62.f)-by)/dy);
   auto position=ballWorld-axis*((std::max(0.f,exit)+42.f)*t.scale);
   set(a,"actorX",position.x);set(a,"actorY",position.y);set(a,"actorZ",t.p.z-24.f*t.scale);
   float angle=std::atan2(axis.x,axis.y)*57.2957795f;if(angle<0)angle+=360;
   set(a,"actorYaw",angle);set(a,"actorMove",1);set(a,"actorPositioned",1);
   set(a,"actorCueX",ballWorld.x);set(a,"actorCueY",ballWorld.y);set(a,"actorCueZ",ballWorld.z);
  }
  if(num(a,"actorSettled")&&!num(a,"actorIdleChosen")){
   auto actorRoot=scene::root(form(a,"opponentRef"));
   if(actorRoot){auto actorFrame=scene::world(actorRoot);if(std::abs(actorFrame.scale-.95f)>.005f){set(a,"vrEnd",1);log("Unsupported opponent rig scale %.5f",actorFrame.scale);return;}auto offset=ballWorld-actorFrame.p;
    float lateral=offset.x*axis.y-offset.y*axis.x,reach=offset.x*axis.x+offset.y*axis.y;
    if(reach<50||reach>180||lateral<-30||lateral>30){set(a,"vrEnd",1);log("Pose outside authored reach");return;}
    int xi=int(std::lround((lateral+30.f)/5.f)),di=int(std::lround((reach-50.f)/5.f));
    auto idle=idleBank[di*13+xi];
    if(idle){
     float rd=reach-(50+5*di),rx=lateral-(-30+5*xi);auto p=scene::world(actorRoot).p;
     auto aligned=p+axis*rd+scene::Vec{axis.y,-axis.x,0}*rx;
     set(a,"actorGridD",50+5*di);set(a,"actorGridX",-30+5*xi);
     set(a,"actorAlignX",aligned.x);set(a,"actorAlignY",aligned.y);set(a,"actorAlignZ",p.z);
     setForm(a,"actorIdle",idle);set(a,"actorIdleChosen",1);set(a,"actorPoseReach",reach);set(a,"actorPoseLateral",lateral);
     log("Pose: reach=%.2f lateral=%.2f alignment=%.3f idle=%08X",reach,lateral,scene::distance(p,aligned),idle->id);
    }

   }
  }
  if(num(a,"actorAligned")&&!num(a,"actorStarted")){
   auto actorRoot=scene::root(form(a,"opponentRef"));set(a,"actorAlignmentOK",0);
   if(actorRoot){auto p=scene::world(actorRoot).p;float dx=p.x-float(num(a,"actorAlignX")),dy=p.y-float(num(a,"actorAlignY"));
    float height=ballWorld.z-p.z-56.f;set(a,"actorHeightResidual",height);
    set(a,"actorAlignmentOK",dx*dx+dy*dy<.0625f&&std::abs(height)<1.f);
   }
  }
  if(num(a,"actorStarted")){
   auto actor=form(a,"opponentRef");auto clock=pooladapter::ownAnimation(actor,num(a,"actorOwnIdle")!=0,poolmotion::Duration,int(num(a,"actorGridD")),int(num(a,"actorGridX")));
   float previous=float(num(a,"actorAimTime"));
   if(clock.valid&&clock.time+.001f>=previous){
    set(a,"actorClockStall",clock.time>previous+.00001f?0:num(a,"actorClockStall")+num(a,"dt"));
    if((clock.time>5.1f&&!num(a,"actorHitDone"))||num(a,"actorClockStall")>1.5){set(a,"vrEnd",1);set(a,"actorFollow",0);log("Owned stroke missed or stalled; cancelled without a synthetic hit");return;}
    set(a,"actorAimTime",clock.time);set(a,"actorReady",1);set(a,"actorClockWait",0);set(a,"actorFollow",std::max(0.f,poolmotion::Duration-clock.time));
    auto hand=scene::find(scene::root(actor),"Bip01 R Hand");
    if(hand&&clock.time<poolmotion::Duration-.03f){
     auto h=scene::world(hand);auto tip=h.p+scene::rotate(h,poolmotion::HandTip)*h.scale;
     auto direction=scene::rotate(h,{poolmotion::Slope,0,1});direction=direction*(1/scene::distance(direction,{}));
     orientCue(tip,direction,h.scale);cueVisible=true;npcCue=true;
     // Convert the calibrated world pose into the actual hand's local frame.
     npcCueLocal={};npcCueLocal.scale=cuePose.scale/h.scale;npcCueLocal.p=scene::inverseRotate(h,cuePose.p-h.p)*(1/h.scale);
     for(int r=0;r<3;r++)for(int c=0;c<3;c++)for(int k=0;k<3;k++)npcCueLocal.r[r][c]+=h.r[k][r]*cuePose.r[k][c];
     scene::Vec origin{float(num(a,"actorCueX")),float(num(a,"actorCueY")),float(num(a,"actorCueZ"))};
     auto delta=origin-tip;float gap=delta.x*direction.x+delta.y*direction.y+delta.z*direction.z;
     float lateralGap=scene::distance(delta-direction*gap,{}),radius=poolmotion::Radius*t.scale;
     if(int(clock.time*10)!=int(previous*10)&&clock.time>=4.3f&&clock.time<=5.1f){
      auto ar=scene::world(scene::root(actor));
      log("Stroke geometry: t=%.4f root=(%.3f %.3f %.3f) scales=(%.5f %.5f %.5f) hand=(%.3f %.3f %.3f) tip=(%.3f %.3f %.3f) axis=(%.5f %.5f %.5f) gap=%.4f side=%.4f",clock.time,ar.p.x,ar.p.y,ar.p.z,ar.scale,h.scale,t.scale,h.p.x,h.p.y,h.p.z,tip.x,tip.y,tip.z,direction.x,direction.y,direction.z,gap,lateralGap);
     }
     if(!num(a,"actorHitDone")&&clock.time>=4.3f&&clock.time<5.1f&&lateralGap<radius){
      float surface=std::sqrt(radius*radius-lateralGap*lateralGap);
      if(num(a,"actorGapValid")&&num(a,"actorLastGap")>surface&&gap<=surface){set(a,"actorContact",1);log("Native cue contact: time=%.4f gap=%.3f lateral=%.3f",clock.time,gap,lateralGap);}
     }
     set(a,"actorLastGap",gap);set(a,"actorGapValid",1);
    }
   }else{
    set(a,"actorGapValid",0);
    if(clock.valid&&clock.time+.05f<previous){set(a,"vrEnd",1);log("Owned animation clock regressed; cancelled");}
    set(a,"actorReady",0);set(a,"actorClockWait",num(a,"actorClockWait")+num(a,"dt"));
    if(previous>=poolmotion::Duration-.1f){set(a,"actorFollow",0);}
    else if(num(a,"actorClockWait")>1.5){set(a,"vrEnd",1);log("Owned animation clock unavailable; cancelled without shooting");}
   }
  }
 }
 set(a,"vrShowCue",cueVisible?1:0);
 if(++frame%30==0&&!testDir.empty()){
  std::ofstream o(testDir+"\\pool-state.json");
  auto bt=pooladapter::tracking(ballWorld);
  auto lt=pooladapter::tracking(left),rt=pooladapter::tracking(right);
  auto ahead=pooladapter::tracking(tableToWorld(t,yaw,float(at(ball,0))+100.f,float(at(ball,1))));
  auto direction=ahead-bt;direction=direction*(1/scene::distance(direction,{}));
  std::ofstream tracking(testDir+"\\pool-tracking.json");
  std::ofstream opponent(testDir+"\\pool-opponent.json");auto actor=form(a,"opponentRef");
  auto actorRoot=scene::root(actor);auto p=actorRoot?scene::world(actorRoot).p:scene::Vec{};
  auto package=pooladapter::backupPackage(actor);
  float ay=float(num(a,"actorYaw"))*.01745329252f;auto look=pooladapter::tracking(p+scene::Vec{std::sin(ay)*40,std::cos(ay)*40,70});
  opponent<<"{\"ready\":"<<num(a,"actorReady")<<",\"aligned\":"<<num(a,"actorAligned")<<",\"started\":"<<num(a,"actorStarted")<<",\"aimTime\":"<<num(a,"actorAimTime")<<",\"look\":["<<look.x<<","<<look.y<<","<<look.z<<"],\"planned\":"<<num(a,"aiPlanned")<<",\"wait\":"<<num(a,"aiWait")<<",\"actor\":["<<p.x<<","<<p.y<<","<<p.z<<"],\"target\":["<<num(a,"actorX")<<","<<num(a,"actorY")<<","<<num(a,"actorZ")<<"],\"package\":"<<(package?package->id:0)<<"}";
  tracking<<"{\"ball\":["<<bt.x<<","<<bt.y<<","<<bt.z<<"],\"left\":["<<lt.x<<","<<lt.y<<","<<lt.z<<"],\"right\":["<<rt.x<<","<<rt.y<<","<<rt.z<<"],\"forward\":["<<direction.x<<","<<direction.y<<","<<direction.z<<"]}";
  o<<"{\"frames\":"<<frame<<",\"turn\":"<<num(a,"turn")<<",\"shots\":"<<num(a,"shots")<<",\"moving\":"<<num(a,"moving")<<",\"finished\":"<<num(a,"finished")<<",\"cueVisible\":"<<(cueVisible?"true":"false")<<",\"tip\":["<<cuePose.p.x<<","<<cuePose.p.y<<","<<cuePose.p.z<<"],\"ball\":["<<ballWorld.x<<","<<ballWorld.y<<","<<ballWorld.z<<"],\"left\":["<<left.x<<","<<left.y<<","<<left.z<<"],\"right\":["<<right.x<<","<<right.y<<","<<right.z<<"]}";
 }
}
void __cdecl message(Message* m){
 if(m->type==DeferredInit){
  getPlayer=compile("player",true);request=compile("PlayerRef.AuxVarGetFlt \"*_FC_PoolRequest\"",true);
  clearRequest=compile("begin function {}\nPlayerRef.AuxVarSetFlt \"*_FC_PoolRequest\" 0\nend");
  closeMenus=compile("begin function {}\nCloseAllMenus\nend");
  menuMode=compile("MenuMode",true);dialogueTarget=compile("SunnyRef.GetDialogueTarget",true);
  unsigned idleCount=0;
  for(unsigned i=0;i<351;i++){char expr[128];sprintf_s(expr,"GetFormFromMod \"Filthy Casual Pool.esp\" \"%06X\"",0x900+i);auto e=call(compile(expr,true));if(e.type==2&&e.value.form){idleBank[i]=e.value.form;++idleCount;}}
  log("Native pool pose forms: %u/351",idleCount);
  holster=compile("begin function {}\nplayer.SetWeaponOut 0\nend");
  auto load=compile("string_var p\nbegin function {p}\nSetFunctionValue (CompileScript p)\nend");
  auto i=call(load,"FilthyCasualPool/Init.txt");init=i.type==2?reinterpret_cast<Script*>(i.value.form):nullptr;
  auto st=call(load,"FilthyCasualPool/Start.txt");start=st.type==2?reinterpret_cast<Script*>(st.value.form):nullptr;
  ready=getPlayer&&request&&clearRequest&&init&&start&&menuMode&&holster&&dialogueTarget&&idleCount==351;log("Initialize: %s",ready?"ready":"FAILED");return;
 }
 if(m->type==PostLoadGame||m->type==NewGame){clear();loaded=m->type==NewGame||m->data!=nullptr;auto e=call(getPlayer);player=e.type==2?e.value.form:nullptr;if(loaded&&ready)call(init);return;}
 if(m->type==PreLoadGame||m->type==ExitMenu||m->type==ExitGame){clear();pendingRequest=0;pendingClearAt=0;loaded=false;player=nullptr;return;}
 if(m->type==MainLoop&&loaded&&ready){
  static ULONGLONG last=0;auto now=GetTickCount64();if(now-last<100)return;last=now;
  auto e=call(request);int r=e.type==1?int(e.value.number):0;
  if(r){call(clearRequest);pendingRequest=r;pendingAt=now;pendingClearAt=0;call(closeMenus);log("Requested %s",r==2?"Sunny match":"practice");}
  if(pendingRequest&&now-pendingAt>=350){
   auto menu=call(menuMode);auto target=pendingRequest==2?call(dialogueTarget):Element{};
   bool targetClear=pendingRequest!=2||(target.type==2&&!target.value.form)||(target.type==1&&target.value.number==0);
   bool nativeClear=menu.type==1&&!menu.value.number&&targetClear;
   if(!nativeClear)pendingClearAt=0;
   else if(!pendingClearAt)pendingClearAt=now;
   // Let native dialogue finish before assigning our travel package. MenuMode
   // alone can clear while the actor still owns its dialogue animation.
   if(now-pendingAt>8000){log("Start cancelled: native dialogue did not settle");pendingRequest=0;pendingClearAt=0;}
   else if(nativeClear&&(pendingRequest!=2||now-pendingClearAt>=1500)){
    int wanted=pendingRequest;pendingRequest=0;pendingClearAt=0;call(holster);call(start,wanted==2?1:0);
   }
  }
 }
}
}
extern "C" __declspec(dllexport) bool __cdecl FC_PoolActive(){return GetTickCount64()<liveUntil.load();}
extern "C" __declspec(dllexport) unsigned __cdecl FC_PoolInput(const char* name,int hand,int kind,float x,float){
 if(!name||hand<0||hand>1)return 0;bool active=(kind&8)!=0;
 if(!strcmp(name,"pool_pose")){std::lock_guard<std::mutex> lock(mutex);auto& h=hands[hand];if(h.poseValid&&!active)++h.poseEpoch;h.poseValid=active;h.poseTime=GetTickCount64();return 0;}
 int control=!strcmp(name,"squeeze")?0:!strcmp(name,"trigger")?1:!strcmp(name,"primary_click")?2:!strcmp(name,"secondary_click")?3:!strcmp(name,"thumbstick_click")?4:!strcmp(name,"menu_click")?5:-1;
 if(control<0)return 0;
 std::lock_guard<std::mutex> lock(mutex);auto& h=hands[hand];
 if(control==0){h.active=active;h.time=GetTickCount64();}
 if(control==0)h.grip=active?x:0;if(control==1)h.trigger=active?x:0;if(control==3)h.b=active&&x>.5f;
 bool own=FC_PoolActive()&&control!=5;
 bool pressed=active&&x>.15f;auto& d=drain[hand][control];if(own&&pressed)d=true;
 bool block=own||d;if(!pressed)d=false;return block?1:0;
}
extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Query(const NVSE* n,PluginInfo* i){i->infoVersion=1;i->name="FilthyCasualPool";i->version=1;return !n->isEditor&&n->runtime==0x040020D0;}
extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Load(const NVSE* n){
 file=_fsopen((std::string(n->runtimeDirectory())+"FilthyCasualPool.log").c_str(),"w",_SH_DENYNO);
 scripts=static_cast<Scripts*>(n->queryInterface(6));arrays=static_cast<Arrays*>(n->queryInterface(5));auto events=static_cast<Events*>(n->queryInterface(8));auto msgs=static_cast<Messaging*>(n->queryInterface(2));
 if(!scripts||!arrays||!events||!msgs||!build_adapter::verifiedTTWVR(GetModuleHandleW(L"TTWVR.dll")))return false;
 if(!enablePoolInput()){log("Standalone pool input discovery failed");return false;}
 char dir[MAX_PATH]{};GetEnvironmentVariableA("SEATED_TEST_DIR",dir,MAX_PATH);testDir=dir;
 for(auto name:{"FC:PoolFrame","FC:PoolDraw","FC:PoolClose","FC:PoolSuspend","FC:PoolCheck","FC:PoolRelease","FC:PoolCandidate"}){
  if(!events->reg(name,0,nullptr,6))return false;
  auto fn=!strcmp(name,"FC:PoolFrame")?update:!strcmp(name,"FC:PoolDraw")?draw:!strcmp(name,"FC:PoolCheck")?check:!strcmp(name,"FC:PoolRelease")?releaseSunny:!strcmp(name,"FC:PoolCandidate")?candidate:suspend;
  if(!events->listen(name,fn))return false;
 }
 log("Pool bridge loaded; native camera untouched");return msgs->listen(n->getPluginHandle(),"NVSE",message);
}
