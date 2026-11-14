#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <atomic>
#include <fstream>
#include <share.h>
#include "../nvse_api.hpp"
#include "../build_adapter.hpp"
#include "../scene_adapter.hpp"
#include "../layer_discovery.hpp"
#include "../pool-vr/adapter.hpp"
#include "adapter.hpp"
#include "../controls_provider.hpp"
#include "publication_adapter.hpp"
#include "performance.hpp"
#include "mounted_frame.hpp"
#include "contact_frame.hpp"
#include "grip_attachment.hpp"
#include "drive_input.hpp"
#include "knee_steering.hpp"
#include "animation_probe.hpp"
#include "seated_animation.hpp"
#include "ride_challenge.hpp"
namespace {
std::atomic<unsigned> controlsAvailable{0};std::atomic<bool> nativeMenu{true};
float bighornerChallengeSeconds=creature_ride::bighornerDefaultSeconds,deathclawChallengeSeconds=creature_ride::deathclawDefaultSeconds;
Arrays* rideArrays{};bool rideEnabled=false,bighornerEnabled=false,deathclawEnabled=false;std::atomic<bool> ridingDeathclaw{false};std::atomic<bool> riding{false},rideMountReady{false};
Scripts* scripts{};FILE* file{};std::mutex lock;
struct Hand {float grip=0,trigger=0,sx=0,sy=0;bool b=false,x=false,stick=false,valid=false,gripClaim=false;ULONGLONG time=0,poseAt=0;};Hand hands[2];
std::atomic<bool> cameraAnchorReady{false};float cameraAnchorZ=0,mountedYawOffset=0;bool mountedYawReady=false;scene::Vec mountedEyeOffset{};mounted_frame::Mat mountedStageHeading=mounted_frame::identity();
std::atomic<bool> active{false};bool loaded=false,ready=false,exitWas=false,reverseWas=false;Form* car{};Form* player{};Form* supported{};Form* flightBase{};Form* flightScript{};float flightHeading=0;std::atomic<bool> flying{false};float flightX=0,flightY=0;unsigned flightButtons=0;
scene::Vec earlyRaw[2]{};
bool kneeDriving=false;std::atomic<bool> weaponVisible{false};Script* getWeaponOut{};
Script *getCar{},*getPlayer{},*getDriving{},*getMenu{},*setVar{},*getVar{},*getFlight{},*getPiloting{},*getX{},*getY{},*getZ{},*yawFlight{},*showPilot{},*getAngleZ{},*getDelta{},*refreshSeat{},*sitPose{},*stopPose{},*allowDriveCombat{},*flightControlLocks{},*getPlayedIdle{};bool seatedPose=false;
float steering=0,throttle=0,brake=0,wheelAngle=0,grabAngle[2]{},grabWheel[2]{};bool grabbed[2]{};
std::atomic<bool> controlNear[2]{},controlHeld[2]{},handOccupied[2]{};
scene::Transform gripLocal[2]{};float grabInputAngle[2]{},stickX=0,stickY=0;
void updatePhysicalControls(void* manager);
void stopRide(bool restore);void updateRideNodes();bool tickRide();void rideContacts(void*);void initRide();
const char* seatName(){return riding?"FCRideSeat":flying?"PlayerPos":"Seat";}
const char* carrierName(){return riding?"FCRideCarrier":flying?"FCCockpitCarrier":"FCWheelCarrier";}
const char* eyeName(){return riding?"FCRideEye":flying?"FCPilotEye":"FCDriverEye";}
void resetGrips(){vehicle_grip::reset();kneeDriving=false;knee_steering::reset();for(int h=0;h<2;h++){grabbed[h]=false;controlNear[h]=false;controlHeld[h]=false;handOccupied[h]=false;}stickX=stickY=0;}

std::string testDir;scene::Vec center{},right{},up{},axis{};
void log(const char* s){if(file){fprintf(file,"%s\n",s);fflush(file);}}
template<class... A> Element call(Script* fn,A... a){Element e;if(fn)scripts->call(fn,nullptr,nullptr,&e,sizeof...(a),a...);return e;}
template<class... A> Element callOn(Script* fn,Form* object,A... a){Element e;if(fn)scripts->call(fn,object,nullptr,&e,sizeof...(a),a...);return e;}
Script* compile(const char* s,bool expr=false){auto f=expr?scripts->expression(s):scripts->compile(s);if(!f)log(s);return f;}
void variable(const char* key,float value){unsigned n;memcpy(&n,&value,4);callOn(setVar,car,key,n);}
float dot(scene::Vec a,scene::Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
scene::Vec unit(scene::Vec a){auto n=scene::distance(a,{});return n>.00001f?a*(1/n):scene::Vec{};}
float wrap(float a){while(a>3.14159265f)a-=6.2831853f;while(a< -3.14159265f)a+=6.2831853f;return a;}
// xNVSE public CommandInfo/CommandTable prefix. Preserve original execution and
// bytecode advancement; override only queries made by the currently driven car.
#define ARGS void* params,void* data,Form* object,Form* container,Script* script,void* events,double* result,unsigned* offset
#define PASS params,data,object,container,script,events,result,offset
using Exec=bool(__cdecl*)(ARGS);
#define EARGS Form* object,void* arg1,void* arg2,double* result
#define EPASS object,arg1,arg2,result
using Eval=bool(__cdecl*)(EARGS);
struct Command {const char* name;const char* shortName;unsigned opcode;const char* help;unsigned short parent,count;void* params;Exec execute;void* parse;Eval eval;unsigned flags;};
static_assert(offsetof(Command,execute)==0x18 && sizeof(Command)==0x28);
struct Table {unsigned version;void* start;void* end;void* opcode;const Command*(__cdecl* byName)(const char*);};
Table* commands{};Exec originals[7]{};Eval originalEval[5]{};
bool __cdecl evalSteer(EARGS){bool ok=originalEval[0](EPASS);if(active&&object==car)*result=flying?flightX:steering;return ok;}
bool __cdecl evalGas(EARGS){bool ok=originalEval[1](EPASS);if(active&&object==car)*result=throttle;return ok;}
bool __cdecl evalStop(EARGS){bool ok=originalEval[2](EPASS);if(active&&object==car)*result=brake;return ok;}
bool __cdecl evalFore(EARGS){bool ok=originalEval[3](EPASS);if(active&&flying&&object==car)*result=flightY;return ok;}
bool __cdecl evalButtons(EARGS){bool ok=originalEval[4](EPASS);if(active&&flying&&object==car)*result=(flightButtons&reinterpret_cast<unsigned>(arg1))?1:0;return ok;}
bool __cdecl steer(ARGS){bool ok=originals[0](PASS);if(active&&object==car)*result=flying?flightX:steering;return ok;}
bool __cdecl gas(ARGS){bool ok=originals[1](PASS);if(active&&object==car)*result=throttle;return ok;}
bool __cdecl stop(ARGS){bool ok=originals[2](PASS);if(active&&object==car)*result=brake;return ok;}
bool __cdecl fore(ARGS){bool ok=originals[3](PASS);if(active&&flying&&object==car)*result=flightY;return ok;}
bool __cdecl buttons(ARGS){auto start=*offset;bool ok=originals[4](PASS);if(active&&flying&&object==car){
 unsigned mask=0,flags=0;using Extract=bool(__cdecl*)(void*,void*,unsigned*,Script*,void*,...);
 if(reinterpret_cast<Extract>(scripts->extract)(params,data,&start,script,events,&mask,&flags))*result=(flightButtons&mask)?1:0;
 }return ok;}
// The desktop flight script aims the entire aircraft with the player's head.
// In VR, physical head motion is free look. Only this specific script receives
// the commanded aircraft heading and neutral pitch; other GetAngle users pass.
bool __cdecl flightAngle(ARGS){auto start=*offset;bool ok=originals[5](PASS);
 if(active&&flying&&object==player&&reinterpret_cast<Form*>(script)==flightScript){
  unsigned axisCode=0;using Extract=bool(__cdecl*)(void*,void*,unsigned*,Script*,void*,...);
  if(reinterpret_cast<Extract>(scripts->extract)(params,data,&start,script,events,&axisCode)){
   if(axisCode=='X')*result=0;else if(axisCode=='Z')*result=flightHeading;
  }
 }return ok;
}
// The upstream flight script disables fighting every frame, which forcibly
// holsters a native weapon. Preserve its other default seat locks without
// touching any other script or explicitly customized DisablePlayerControls call.
bool __cdecl flightLocks(ARGS){
 if(active&&flying&&reinterpret_cast<Form*>(script)==flightScript&&flightControlLocks){
  auto start=*offset;unsigned move=1,pip=1,fight=1,pov=1,look=0,text=0,sneak=0;
  using Extract=bool(__cdecl*)(void*,void*,unsigned*,Script*,void*,...);
  if(reinterpret_cast<Extract>(scripts->extract)(params,data,offset,script,events,&move,&pip,&fight,&pov,&look,&text,&sneak)&&move==1&&pip==1&&fight==1&&pov==1&&!look&&!text&&!sneak){
   call(flightControlLocks);*result=0;return true;
  }
  *offset=start;
 }
 return originals[6](PASS);
}
bool hooks(){const char* names[]={"GetLeftStickX","GetRightTrigger","GetLeftTrigger","GetLeftStickY","IsButtonPressed","GetAngle","DisablePlayerControls"};Exec f[]={steer,gas,stop,fore,buttons,flightAngle,flightLocks};Eval ef[]={evalSteer,evalGas,evalStop,evalFore,evalButtons};Command* c[7]{};DWORD protection[7]{};
 for(int i=0;i<7;i++){c[i]=const_cast<Command*>(commands->byName(names[i]));if(!c[i]||!c[i]->execute||(i<5&&!c[i]->eval))return false;}
 // Prepare every writable slot before changing any function pointer.
 int prepared=0;for(;prepared<7;prepared++)if(!VirtualProtect(c[prepared],sizeof(Command),PAGE_READWRITE,&protection[prepared]))break;
 if(prepared!=7){for(int i=prepared-1;i>=0;i--){DWORD old;VirtualProtect(c[i],sizeof(Command),protection[i],&old);}return false;}
 for(int i=0;i<7;i++){originals[i]=c[i]->execute;c[i]->execute=f[i];if(i<5){originalEval[i]=c[i]->eval;c[i]->eval=ef[i];}}
 for(int i=6;i>=0;i--){DWORD old;VirtualProtect(c[i],sizeof(Command),protection[i],&old);}return true;}

void* mountedRoot=nullptr;
void invalidateMountedFrame(){resetGrips();vehicle_contact::reset();mounted_frame::reset();cameraAnchorReady=false;mountedYawReady=false;mountedRoot=nullptr;}
void suspendMounted(){invalidateMountedFrame();active=false;steering=throttle=brake=0;flightX=flightY=0;flightButtons=0;grabbed[0]=grabbed[1]=false;}
void clear(){if(riding)stopRide(loaded);if(seatedPose&&player&&loaded&&seated_animation::ownsActive(player))call(stopPose);seated_animation::restore();seatedPose=false;invalidateMountedFrame();if(active)vehicle_perf::write(testDir);active=false;flying=false;flightX=flightY=0;flightButtons=0;exitWas=reverseWas=false;car=nullptr;steering=throttle=brake=wheelAngle=0;grabbed[0]=grabbed[1]=false;}
void ensureSeatedPose(){
 // Once the real idle sequence is retained, sample only its lower-body tracks.
 // Release the full-body idle once acquired so upper-body weapon animation
 // stays native; do not replay the idle on each weapon/root rebuild.
 if(seated_animation::ownedSequence&&seated_animation::ownedActor==player){
  if(seated_animation::ownsActive(player))call(stopPose);seatedPose=false;return;
 }
 auto root=player?scene::root(player):nullptr;if(!seated_animation::needsRequest(player,root))return;
 if(seatedPose&&seated_animation::ownsActive(player))call(stopPose);
 seated_animation::restore();auto result=call(sitPose);seatedPose=result.type==1&&result.value.number==1;
 seated_animation::noteRequest(player,root,seatedPose);
}
#include "bighorner_ride.hpp"
std::atomic<unsigned> publicationCalls{0},publicationApplied{0};DWORD mainThread=0;thread_local unsigned char* guardedPlayer=nullptr;thread_local unsigned char savedMountedControls=0;thread_local bool publicationCommitted=false;
bool anchorPublication(void* actor,scene::Vec& origin){
 if(GetCurrentThreadId()!=mainThread||actor!=player||!guardedPlayer||!car)return false;
 auto root=scene::root(car);auto seat=scene::find(root,seatName());if(!seat){invalidateMountedFrame();return false;}
 auto point=scene::world(seat).p;auto actorPoint=scene::field<scene::Vec>(player,0x30);
 if(scene::distance(point,actorPoint)>4){invalidateMountedFrame();return false;}
 if(!cameraAnchorReady){
  auto offset=origin.z-point.z;if(!std::isfinite(offset)||std::abs(offset)>180)return false;
  cameraAnchorZ=offset;cameraAnchorReady=true;
  if(file){fprintf(file,"Native mounted camera anchor calibrated z=%.4f\n",offset);fflush(file);}
 }
 origin={point.x,point.y,point.z+cameraAnchorZ};return true;
}
// Native TTR has later seated/camera corrections after the initial origin.
// Publish the mounted origin after those corrections, before cameras and hands
// consume it. Do not overwrite the physical tracked poses or the on-foot path.
void updateSteering();
void finalizePublication(void* manager){
 vehicle_perf::Scope timing(2,!testDir.empty()&&active);
 if(GetCurrentThreadId()!=mainThread)return;
 if(!guardedPlayer||!active||!cameraAnchorReady||!car||!manager){invalidateMountedFrame();return;}
 auto root=scene::root(car);
 if(root!=mountedRoot){invalidateMountedFrame();return;}
 auto seat=scene::find(root,seatName());if(!seat){invalidateMountedFrame();return;}
 auto point=scene::world(seat).p;
 scene::field<scene::Vec>(manager,0xB04)={point.x,point.y,point.z+cameraAnchorZ};
 // TTR resets this each publication. Its native attached-camera branch sets
 // the same flag: both coordinate conversions then use this origin directly,
 // instead of mixing the walking-height reference with the moving seat.
 scene::field<unsigned char>(manager,0xB21)=1;
 // Carry the tracking-space heading with the rendered chassis, preserving
 // the rider's initial relative heading and all physical HMD free look.
 auto body=scene::find(root,carrierName());
 if(!body){invalidateMountedFrame();return;}
 if(body){auto t=scene::world(body);float yaw=std::atan2(t.r[0][1],t.r[1][1]);auto& nativeYaw=scene::field<float>(manager,0xB3C);
  auto nativeBasis=mounted_frame::nativeBasis(manager);
  bool calibrating=!mountedYawReady;
  if(calibrating){
   // The native HMD rotation's second column is forward (exact-build VATS
   // consumer copies matrix offsets 4,16,28). Remove the native world basis
   // and latch physical boarding yaw; pitch and subsequent head turns stay free.
   mounted_frame::Mat head;mounted_frame::originalWorldRotation(manager,&head,static_cast<unsigned char*>(manager)+0x28);
   auto forward=mounted_frame::apply(mounted_frame::transpose(nativeBasis),{head.a[0][1],head.a[1][1],head.a[2][1]});
   if(!mounted_frame::headingFromForward(forward,mountedStageHeading)){invalidateMountedFrame();return;}
   mountedYawOffset=wrap(nativeYaw-yaw);mountedYawReady=true;
  }
  // Authored cabin +X right,+Y forward,+Z up. The boarding heading maps to
  // its forward axis; the rider keeps physical free look and chassis pitch/roll.
  mounted_frame::Mat bodyNow;memcpy(bodyNow.a,t.r,sizeof bodyNow.a);
  const mounted_frame::Mat stageToCabin={{{1,0,0},{0,0,-1},{0,1,0}}};
  auto desired=mounted_frame::mul(mounted_frame::mul(bodyNow,stageToCabin),mounted_frame::transpose(mountedStageHeading));
  auto correction=mounted_frame::mul(desired,mounted_frame::transpose(nativeBasis));
  auto eye=scene::find(body,eyeName());
  if(!eye){invalidateMountedFrame();return;}
  if(calibrating){
   scene::Vec nativeHead;mounted_frame::originalToWorld(manager,&nativeHead,&scene::field<scene::Vec>(manager,0x1C));
   auto delta=mounted_frame::apply(correction,nativeHead-scene::field<scene::Vec>(manager,0xB04));
   mountedEyeOffset=mounted_frame::apply(mounted_frame::transpose(bodyNow),delta);
  }
  // Seat/root stays on the real floor. The eye anchor absorbs the user's
  // standing-height calibration at boarding without sinking the actor model.
  using SeatOffset=float(__cdecl*)(int);auto core=GetModuleHandleW(L"SeatedControls.dll");auto offset=core?reinterpret_cast<SeatOffset>(GetProcAddress(core,"SC_SeatOffset")):nullptr;
  int mode=riding?(ridingDeathclaw?4:3):flying?2:1;float seatOffset=offset?offset(mode):0;
  if(!std::isfinite(seatOffset))seatOffset=0;
  auto origin=scene::world(eye).p-mounted_frame::apply(bodyNow,mountedEyeOffset)+mounted_frame::apply(bodyNow,{0,0,seatOffset*70.f});
  scene::field<scene::Vec>(manager,0xB04)=origin;
  mounted_frame::write({manager,correction,origin});
  if(riding&&!testDir.empty()){static std::ofstream o(testDir+"\\ride-frame.jsonl",std::ios::trunc);static unsigned count=0;if(o&&count++<12000){LARGE_INTEGER q;QueryPerformanceCounter(&q);o<<"{\"pid\":"<<GetCurrentProcessId()<<",\"frame\":"<<publicationCalls.load()<<",\"qpc\":"<<q.QuadPart<<",\"carrier\":["<<t.p.x<<","<<t.p.y<<","<<t.p.z<<"],\"up\":["<<t.r[0][2]<<","<<t.r[1][2]<<","<<t.r[2][2]<<"],\"forward\":["<<t.r[0][1]<<","<<t.r[1][1]<<","<<t.r[2][1]<<"]}\n";if(count%60==0)o.flush();}}

  publicationCommitted=true;mounted_frame::nativeSetup=false;
  if(!testDir.empty())for(int h=0;h<2;h++)earlyRaw[h]=vehicle_adapter::rawStageGrip(manager,h);
 }
}
void beginPublication(){
 mounted_frame::nativeSetup=true;publicationCommitted=false;
 vehicle_perf::Scope timing(1,!testDir.empty()&&active);
 ++publicationCalls;
 if(GetCurrentThreadId()!=mainThread)return;
 if(!loaded||!active||!car||!player){invalidateMountedFrame();return;}
 if(riding&&vehicle_adapter::lookup(rideCarrierID)!=car){clear();return;}
 auto root=scene::root(car);
 if(!root){invalidateMountedFrame();return;}
 if(root!=mountedRoot){invalidateMountedFrame();mountedRoot=root;}
 // SyncPos is the dependency's existing native seat attachment. Refresh its
 // normal SetPosEx path here, before TTR samples the player/camera origin.
 ++publicationApplied;if(riding){updateRideNodes();call(rideRefresh);}else call(refreshSeat,car);
 guardedPlayer=reinterpret_cast<unsigned char*>(player);savedMountedControls=guardedPlayer[0x680]&3;
 // Native TTR treats either movement OR look being disabled as a frozen
 // interaction frame (verified 3FEBE0). Our mounted publication supplies both;
 // preserve combat/menu flags and restore these two bits immediately after.
 guardedPlayer[0x680]&=~3u;
}
void observeBody(void* actor,void* root,bool after){
 if(!active||!publicationCommitted||actor!=player||GetCurrentThreadId()!=mainThread||!root)return;
 if(!after){vehicle_perf::Scope timing(4,!testDir.empty());seated_animation::sample(actor,root);}
 // Apply only after TTR has propagated its modified parent transforms.
 if(testDir.empty())return;
 static unsigned sampled=0;static bool capture=false;
 if(!after)capture=(publicationCalls.load()%30==0&&sampled++<50);
 if(!capture)return;
 animation_probe::capture(testDir,actor,root);
 static std::ofstream trace(testDir+"\\mounted-body-solve.jsonl",std::ios::trunc);
 auto idle=call(getPlayedIdle);trace<<"{\"publication\":"<<publicationCalls.load()<<",\"after\":"<<after<<",\"idle\":"<<(idle.type==2&&idle.value.form?idle.value.form->id:0)<<",\"bones\":{";bool first=true;
 for(auto name:{"Bip01 Pelvis","Bip01 Spine","Bip01 Spine1","Bip01 Spine2","Bip01 L Thigh","Bip01 L Calf","Bip01 L Foot","Bip01 R Thigh","Bip01 R Calf","Bip01 R Foot"})if(auto bone=scene::find(root,name)){
  auto t=scene::local(bone);if(!first)trace<<",";first=false;trace<<"\""<<name<<"\":["<<t.p.x<<","<<t.p.y<<","<<t.p.z;for(int i=0;i<9;i++)trace<<","<<t.r[i/3][i%3];trace<<"]";
 }trace<<"}}\n";trace.flush();
}
void recordMountedPose(){
 if(testDir.empty()||!active||!player||!car)return;
 static ULONGLONG last=0;auto now=GetTickCount64();if(now-last<250)return;last=now;
 auto root=scene::root(player),body=scene::find(scene::root(car),carrierName());if(!root||!body)return;
 auto t=scene::world(body);std::ofstream out(testDir+"\\mounted-pose.json");out<<"{\"earlyRaw\":[["<<earlyRaw[0].x<<","<<earlyRaw[0].y<<","<<earlyRaw[0].z<<"],["<<earlyRaw[1].x<<","<<earlyRaw[1].y<<","<<earlyRaw[1].z<<"]],\"pid\":"<<GetCurrentProcessId()<<",\"samples\":"<<seated_animation::samples<<",\"applies\":"<<seated_animation::applies<<",\"sourceTime\":"<<seated_animation::sourceTime<<",\"rootScale\":"<<seated_animation::lastRootScale<<",\"pelvisScale\":"<<seated_animation::lastPelvisScale<<",\"pelvisApplyError\":"<<scene::distance(seated_animation::lastTarget,seated_animation::lastImmediate)<<",\"localPredictionError\":"<<seated_animation::localPredictionError<<",\"localRewriteError\":"<<seated_animation::localRewriteError<<",\"pelvisFlags\":"<<seated_animation::lastFlags<<",\"bones\":{";bool first=true;
 for(auto name:{"Bip01 Head","Bip01 Spine","Bip01 Pelvis","Bip01 L Thigh","Bip01 L Calf","Bip01 L Foot","Bip01 R Thigh","Bip01 R Calf","Bip01 R Foot"})if(auto bone=scene::find(root,name)){
  auto p=scene::inverseRotate(t,scene::world(bone).p-t.p)*(1/t.scale);if(!first)out<<",";first=false;out<<"\""<<name<<"\":["<<p.x<<","<<p.y<<","<<p.z<<"]";
 }out<<"}}";
}
void beforeContacts(void* actor){
 if(GetCurrentThreadId()!=mainThread||!active||!publicationCommitted||actor!=player||!car)return;
 auto frame=mounted_frame::read();auto body=scene::find(scene::root(car),carrierName());
 vehicle_contact::before(frame.manager,scene::root(player),body,publicationCalls.load());
 updatePhysicalControls(frame.manager);
}
void recordHandFrame(){
 if(testDir.empty()||!active||!publicationCommitted||GetCurrentThreadId()!=mainThread||!player||!car)return;
 auto root=scene::root(player),body=scene::find(scene::root(car),carrierName());
 auto b=build_adapter::heldModule();auto manager=b?reinterpret_cast<void*(__cdecl*)()>(b+0x13903)():nullptr;
 if(!root||!body||!manager)return;
 static std::ofstream out(testDir+"\\mounted-hands.jsonl",std::ios::trunc);static unsigned count=0;if(!out||count++>=16000)return;
 auto c=scene::world(body);LARGE_INTEGER qpc;QueryPerformanceCounter(&qpc);
 out.precision(9);out<<"{\"earlyRaw\":[["<<earlyRaw[0].x<<","<<earlyRaw[0].y<<","<<earlyRaw[0].z<<"],["<<earlyRaw[1].x<<","<<earlyRaw[1].y<<","<<earlyRaw[1].z<<"]],\"pid\":"<<GetCurrentProcessId()<<",\"publication\":"<<publicationCalls.load()<<",\"qpc\":"<<qpc.QuadPart<<",\"contactCarries\":"<<vehicle_contact::carries<<",\"contactMisses\":"<<vehicle_contact::misses<<",\"contactReseeds\":"<<vehicle_contact::reseeds<<",\"carrier\":["<<c.p.x<<","<<c.p.y<<","<<c.p.z<<"],\"hands\":[";
 for(int h=0;h<2;h++){
  if(h)out<<",";scene::Vec target{};bool valid=vehicle_adapter::gripPoint(manager,h,target);auto targetLocal=scene::inverseRotate(c,target-c.p)*(1/c.scale);
  auto correction=vehicle_adapter::handWorldCorrection(manager,h),raw=vehicle_adapter::rawStageGrip(manager,h);scene::Vec rawWorld{};vehicle_adapter::rawGripPoint(manager,h,rawWorld);rawWorld=scene::inverseRotate(c,rawWorld-c.p)*(1/c.scale);
  out<<"{\"valid\":"<<valid<<",\"correction\":["<<correction.x<<","<<correction.y<<","<<correction.z<<"],\"raw\":["<<raw.x<<","<<raw.y<<","<<raw.z<<"],\"rawWorld\":["<<rawWorld.x<<","<<rawWorld.y<<","<<rawWorld.z<<"],\"target\":["<<targetLocal.x<<","<<targetLocal.y<<","<<targetLocal.z<<"],\"bones\":[";
  const char* names[2][3]={{"Bip01 L Hand","Bip01 L Forearm","Bip01 L UpperArm"},{"Bip01 R Hand","Bip01 R Forearm","Bip01 R UpperArm"}};
  for(int j=0;j<3;j++){if(j)out<<",";auto node=scene::find(root,names[h][j]);auto p=node?scene::inverseRotate(c,scene::world(node).p-c.p)*(1/c.scale):scene::Vec{};out<<"["<<p.x<<","<<p.y<<","<<p.z<<"]";}out<<"]}";
 }out<<"]}\n";if(count%60==0)out.flush();
}
scene::Transform publishedUICarrier{};void* publishedUINode=nullptr;
void endPublication(){
 publishedUINode=nullptr;
 if(GetCurrentThreadId()==mainThread&&publicationCommitted&&active&&car&&player){
  vehicle_perf::Scope timing(5,!testDir.empty());
  auto vehicleRoot=scene::root(car);auto body=scene::find(vehicleRoot,carrierName()),seat=scene::find(vehicleRoot,seatName());
  if(body){publishedUICarrier=scene::world(body);publishedUINode=body;}
  if(seated_animation::apply(scene::root(player),body,seat,riding)){if(riding)spreadRideLegs();else if(!flying)knee_steering::apply(scene::root(player),body,scene::find(vehicleRoot,"FCSteeringWheel"),kneeDriving,steering);}
 }else seated_animation::pending={};
 if(GetCurrentThreadId()==mainThread&&!publicationCommitted)mounted_frame::reset();
 vehicle_perf::Scope timing(3,!testDir.empty()&&active);
 auto birdRoot=(!testDir.empty()&&GetCurrentThreadId()==mainThread&&car&&flying)?scene::root(car):nullptr;
 auto cockpit=birdRoot?scene::find(birdRoot,"FCCockpitCarrier"):nullptr;
  if(!testDir.empty()&&cockpit){
   static std::ofstream trace(testDir+"\\flight-rig.jsonl",std::ios::trunc);static unsigned frames=0;
   if(trace&&frames++<12000){
    auto c=scene::world(cockpit);auto eyeNode=scene::find(cockpit,"FCPilotEye");auto eye=eyeNode?scene::world(eyeNode).p:c.p+scene::rotate(c,{26,170,566})*c.scale;
    auto t=vehicle_adapter::tracking(eye),forward=vehicle_adapter::tracking(eye+scene::rotate(c,{0,70,0})*c.scale);
    auto seat=scene::find(cockpit,"PlayerPos");auto sp=seat?vehicle_adapter::tracking(scene::world(seat).p):scene::Vec{};
    auto body=scene::find(birdRoot,"BodyMain");
    trace<<"{\"tickMs\":"<<GetTickCount64()<<",\"publication\":"<<publicationCalls.load()<<",\"applied\":"<<publicationApplied.load()<<",\"thread\":"<<GetCurrentThreadId()<<",\"mainThread\":"<<mainThread<<",\"modelEye\":["<<t.x<<","<<t.y<<","<<t.z<<"],\"modelForward\":["<<forward.x<<","<<forward.y<<","<<forward.z<<"],\"seat\":["<<sp.x<<","<<sp.y<<","<<sp.z<<"],\"bodyFlags\":"<<(body?scene::field<unsigned>(body,0x30):0)<<",\"carrier\":["<<c.p.x<<","<<c.p.y<<","<<c.p.z<<"],\"rotation\":[";
    for(int i=0;i<9;i++){if(i)trace<<",";trace<<c.r[i/3][i%3];}trace<<"]}\n";if(frames%30==0)trace.flush();
   }
  }
 if(guardedPlayer&&!flying&&publicationCommitted){
  // Steering and grip constraints publish before the native contact/arm solve.
  if(!testDir.empty()&&car){static std::ofstream trace(testDir+"\\car-rig.jsonl",std::ios::trunc);static unsigned count=0;
   auto b=build_adapter::heldModule();auto m=reinterpret_cast<void*(__cdecl*)()>(b+0x13903)();auto rt=scene::root(car);auto w=scene::find(rt,"FCSteeringWheel"),st=scene::find(rt,"Seat");
   if(m&&w&&st&&count++<12000){auto origin=scene::field<scene::Vec>(m,0xB04),anchor=scene::field<scene::Vec>(m,0xB10),seat=scene::world(st).p,wheel=scene::world(w).p;
    auto carrier=scene::find(rt,"FCWheelCarrier");auto ct=scene::world(carrier);auto relative=scene::inverseRotate(ct,wheel-seat)*(1/ct.scale);float bodyYaw=std::atan2(ct.r[0][1],ct.r[1][1]);
    trace<<"{\"bodyYaw\":"<<bodyYaw<<",\"yawOffset\":"<<mountedYawOffset<<",\"relative\":["<<relative.x<<","<<relative.y<<","<<relative.z<<"],\"sequence\":"<<publicationCalls.load()<<",\"epochPid\":"<<GetCurrentProcessId()<<",\"origin\":["<<origin.x<<","<<origin.y<<","<<origin.z<<"],\"anchor\":["<<anchor.x<<","<<anchor.y<<","<<anchor.z<<"],\"seat\":["<<seat.x<<","<<seat.y<<","<<seat.z<<"],\"wheel\":["<<wheel.x<<","<<wheel.y<<","<<wheel.z<<"],\"yaw\":"<<scene::field<float>(m,0xB3C)<<",\"mode\":"<<unsigned(scene::field<unsigned char>(m,0xB21))<<"}\n";if(count%30==0)trace.flush();
   }
  }
 }
 recordMountedPose();recordHandFrame();
 if(guardedPlayer){guardedPlayer[0x680]=(guardedPlayer[0x680]&~3u)|savedMountedControls;guardedPlayer=nullptr;}
 mounted_frame::nativeSetup=false;
}
void tick(){
 vehicle_perf::Scope timing(0,!testDir.empty()&&active);
 mainThread=GetCurrentThreadId();auto weapon=call(getWeaponOut);weaponVisible=weapon.type==1&&weapon.value.number!=0;
 if(active){auto delta=call(getDelta);if(delta.type==1)seated_animation::advance(float(delta.value.number));}
 if(rideEnabled&&tickRide())return;
 auto flight=call(getFlight),piloting=call(getPiloting);
 auto bird=flight.type==2?flight.value.form:nullptr;
 if(bird&&flightBase&&scene::field<Form*>(bird,0x20)==flightBase&&piloting.type==1&&piloting.value.number==1&&scene::root(bird)){
  if(car!=bird){clear();car=bird;auto heading=callOn(getAngleZ,player);flightHeading=heading.type==1?float(heading.value.number):0;call(showPilot);call(allowDriveCombat);}flying=true;active=true;call(allowDriveCombat);ensureSeatedPose();
  auto birdRoot=scene::root(bird);auto cockpit=scene::find(birdRoot,"FCCockpitCarrier");
  if(cockpit){
   for(auto name:{"BodyMain","BodyTail"})if(auto body=scene::find(birdRoot,name))scene::field<unsigned>(body,0x30)|=1u;
   if(auto caps=scene::find(cockpit,"VertibirdHull01:2"))scene::field<unsigned>(caps,0x30)|=1u;
  }


  Hand h[2];{std::lock_guard<std::mutex> g(lock);h[0]=hands[0];h[1]=hands[1];}
  auto now=GetTickCount64();auto m=call(getMenu);nativeMenu=m.type!=1||m.value.number!=0;bool live=!nativeMenu;
  for(auto& v:h)live=live&&v.valid&&now-v.poseAt<200&&now-v.time<200;
  // Thumbstick always wins over physical-control contact, including while
  // gripping. The other hand must remain native for slides/bolts and magazines.
  const bool thumb=std::abs(h[1].sx)>.15f||std::abs(h[1].sy)>.15f;
  flightX=live?(thumb?h[1].sx:stickX):0;flightY=live?(thumb?h[1].sy:stickY):0;
  throttle=live&&!weaponVisible&&!handOccupied[1]?h[1].trigger:0;brake=live&&!weaponVisible&&!handOccupied[0]?h[0].trigger:0;
  flightButtons=0;if(live){if(h[1].x&&!weaponVisible&&!handOccupied[1])flightButtons|=32768;if(h[0].x&&!weaponVisible&&!handOccupied[0])flightButtons|=64;if(h[1].b)flightButtons|=4096|64;if(h[0].b)flightButtons|=32;if(h[1].stick)flightButtons|=16384;if(h[0].stick)flightButtons|=8192;
   float yaw=std::abs(h[0].sx)>.2f?h[0].sx:0;auto delta=call(getDelta);float dt=delta.type==1?std::clamp(float(delta.value.number),0.f,.05f):0;flightHeading=std::fmod(flightHeading+yaw*60*dt+360.f,360.f);unsigned bits;memcpy(&bits,&yaw,4);call(yawFlight,bits);
  }
  if(!testDir.empty()){static ULONGLONG last=0;if(now-last>100){last=now;auto px=callOn(getX,car),py=callOn(getY,car),pz=callOn(getZ,car),control=callOn(getVar,car,"Controlling"),land=callOn(getVar,car,"Landing");std::ofstream o(testDir+"\\flight-state.json");o<<"{\"active\":true,\"live\":"<<live<<",\"car\":"<<car->id<<",\"scale\":"<<scene::world(birdRoot).scale<<",\"x\":"<<px.value.number<<",\"y\":"<<py.value.number<<",\"z\":"<<pz.value.number<<",\"sx\":"<<flightX<<",\"sy\":"<<flightY<<",\"buttons\":"<<flightButtons<<",\"controlling\":"<<control.value.number<<",\"landing\":"<<land.value.number<<"}";}}
  return;
 }
 if(flying)clear();
 auto current=call(getCar);Form* target=current.type==2?current.value.form:nullptr;
 auto driving=target?callOn(getDriving,target):Element{};auto menu=call(getMenu);
 if(!testDir.empty()){static ULONGLONG lastProbe=0;if(GetTickCount64()-lastProbe>250){lastProbe=GetTickCount64();std::ofstream o(testDir+"\\vehicle-probe.json");o<<"{\"target\":"<<(target?target->id:0)<<",\"driving\":"<<driving.value.number<<",\"type\":"<<int(driving.type)<<"}";}}
 if(!target||!supported||scene::field<Form*>(target,0x20)!=supported||driving.type!=1||driving.value.number!=3){clear();return;}
 if(car!=target){clear();car=target;
  // Roadster's native boarding script disables combat (third argument).
  // Restore only combat for independent gun/item triggers; retain its normal
  // locomotion, POV, rollover and sneak locks and native seat attachment.
  call(allowDriveCombat);
 }
 active=true;ensureSeatedPose();
}
void updateSteering(){
 if(!active||flying||!car||!player)return;
 auto menu=call(getMenu);nativeMenu=menu.type!=1||menu.value.number!=0;
 auto root=scene::root(car);auto rig=scene::root(player);if(!root||!rig){suspendMounted();return;}
 // First supported cockpit: original MkT Roadster. Fail closed for other rigs.
 auto wheel=scene::find(root,"FCSteeringWheel");if(!wheel){static Form* reported=nullptr;if(reported!=car){reported=car;log("Missing imported wheel node");}suspendMounted();return;}
 auto carrier=scene::find(root,"FCWheelCarrier");if(!carrier){suspendMounted();return;}
 active=true;auto t=scene::world(carrier);
 // Measured from the dependency's authored wheel mesh (game units).
 center=scene::world(wheel).p;
 right=unit(scene::rotate(t,{1,0,0}));up=unit(scene::rotate(t,{0,.34394f,.93899f}));axis=unit(scene::cross(right,up));
 Hand h[2];{std::lock_guard<std::mutex> g(lock);h[0]=hands[0];h[1]=hands[1];}
 auto now=GetTickCount64();float sum=0;int count=0;bool fresh[2]{};
 auto manager=mounted_frame::read().manager;
 auto wt=scene::world(wheel);
 for(int i=0;i<2;i++){
  fresh[i]=h[i].valid&&now-h[i].poseAt<200&&now-h[i].time<200;
  scene::Transform raw{};bool valid=vehicle_grip::raw(manager,i,raw);
  auto d=raw.p-center;float x=dot(d,right),y=dot(d,up),z=dot(d,axis),rad=std::sqrt(x*x+y*y);
  float angle=std::atan2(y,x);bool nearRim=valid&&std::abs(z)<12*t.scale&&std::abs(rad-10.f*t.scale)<10*t.scale;
  controlNear[i]=nearRim&&!handOccupied[i]&&!nativeMenu;
  if(!fresh[i]||!valid||handOccupied[i]||h[i].grip<.35f||nativeMenu){grabbed[i]=false;continue;}
  if(!grabbed[i]&&nearRim&&h[i].gripClaim&&h[i].grip>.65f){grabbed[i]=true;grabAngle[i]=angle;grabWheel[i]=wheelAngle;gripLocal[i]=vehicle_grip::relative(wt,raw);}
  if(grabbed[i]){
   auto target=vehicle_grip::world(wt,gripLocal[i]);
   // A deliberate reach-away or tracking jump releases; ordinary rim motion
   // keeps the original attachment rather than repeatedly snapping/reacquiring.
   if(scene::distance(raw.p,target.p)>38*t.scale){grabbed[i]=false;continue;}
   sum+=grabWheel[i]+wrap(angle-grabAngle[i]);count++;
  }
 }
 if(count&&(!fresh[1]||std::abs(h[1].sx)<=.15f))wheelAngle=std::clamp(sum/count,-1.5707963f,1.5707963f);
 else {float sx=fresh[1]?h[1].sx:0;steering=std::abs(sx)>.15f?std::clamp(sx,-1.f,1.f):0;wheelAngle=-steering*1.5707963f;}
 // Rotate the locally imported wheel about its measured steering-column axis.
 auto& local=scene::local(wheel);scene::Vec n=unit({0,-.93899f,.34394f});float co=std::cos(wheelAngle),si=std::sin(wheelAngle),v=1-co;
 float q[3]={n.x,n.y,n.z};for(int row=0;row<3;row++)for(int col=0;col<3;col++)local.r[row][col]=(row==col?co:0)+q[row]*q[col]*v;
 local.r[0][1]-=n.z*si;local.r[0][2]+=n.y*si;local.r[1][0]+=n.z*si;local.r[1][2]-=n.x*si;local.r[2][0]-=n.y*si;local.r[2][1]+=n.x*si;scene::update(wheel);
 steering=std::clamp(-wheelAngle/1.5707963f,-1.f,1.f);
 wt=scene::world(wheel);for(int i=0;i<2;i++){controlHeld[i]=grabbed[i];vehicle_grip::targets[i]={grabbed[i],manager,vehicle_grip::world(wt,gripLocal[i])};}
 bool live=menu.type==1&&!menu.value.number;
 auto gear=callOn(getVar,car,"Reverse"),momentum=callOn(getVar,car,"Momentum");
 auto pedals=vehicle_drive::pedals(h[0].sy,live&&fresh[0]&&gear.type==1&&momentum.type==1,gear.value.number!=0,float(momentum.value.number));
 throttle=pedals.gas;brake=pedals.brake;
 kneeDriving=live&&momentum.type==1&&momentum.value.number>.5&&!grabbed[0]&&!grabbed[1];
 if(pedals.changeGear)variable("Reverse",pedals.reverse?1.f:0.f);
 // The desktop script leaves the opposite pedal latched when one is active.
 // Clear that obsolete value while keeping all acceleration/physics native.
 if(!throttle)variable("Throttle",0);if(!brake)variable("BrakeForce",0);
 if(live&&fresh[1]&&h[1].b&&!exitWas)variable("Dismount",1);exitWas=h[1].b;
 if(!testDir.empty()){
  static ULONGLONG last=0;if(now-last>100){last=now;auto c=pooladapter::tracking(center),r=pooladapter::tracking(center+right*(10*t.scale)),u=pooladapter::tracking(center+up*(10*t.scale));
   auto pa=pooladapter::tracking(scene::palm(rig,0)),pb=pooladapter::tracking(scene::palm(rig,1));
   scene::Vec shoulder[2]{};float armLength[2]{};
   for(int h=0;h<2;h++){auto a=scene::find(rig,h?"Bip01 R UpperArm":"Bip01 L UpperArm"),e=scene::find(rig,h?"Bip01 R Forearm":"Bip01 L Forearm"),w=scene::find(rig,h?"Bip01 R Hand":"Bip01 L Hand");if(a&&e&&w){shoulder[h]=pooladapter::tracking(scene::world(a).p);armLength[h]=(scene::distance(scene::world(a).p,scene::world(e).p)+scene::distance(scene::world(e).p,scene::world(w).p))/vehicle_adapter::unitsPerMetre();}}
   auto px=callOn(getX,car),py=callOn(getY,car);std::ofstream o(testDir+"\\vehicle-state.json");o<<"{\"shoulders\":[["<<shoulder[0].x<<","<<shoulder[0].y<<","<<shoulder[0].z<<"],["<<shoulder[1].x<<","<<shoulder[1].y<<","<<shoulder[1].z<<"]],\"armLength\":["<<armLength[0]<<","<<armLength[1]<<"],\"active\":true,\"car\":"<<car->id<<",\"angle\":"<<wheelAngle<<",\"steer\":"<<steering<<",\"gas\":"<<throttle<<",\"actorState\":"<<vehicle_adapter::actorState(player)<<",\"controlFlags\":"<<unsigned(scene::field<unsigned char>(player,0x680))<<",\"idleOwned\":"<<seated_animation::ownsActive(player)<<",\"knees\":"<<knee_steering::weight<<",\"momentum\":"<<momentum.value.number<<",\"reverse\":"<<pedals.reverse<<",\"brake\":"<<brake<<",\"grabbed\":["<<grabbed[0]<<","<<grabbed[1]<<"],\"center\":["<<c.x<<","<<c.y<<","<<c.z<<"],\"right\":["<<r.x<<","<<r.y<<","<<r.z<<"],\"up\":["<<u.x<<","<<u.y<<","<<u.z<<"],\"palms\":[["<<pa.x<<","<<pa.y<<","<<pa.z<<"],["<<pb.x<<","<<pb.y<<","<<pb.z<<"]],\"position\":["<<px.value.number<<","<<py.value.number<<"]}";
  }
 }
}
void updatePhysicalControls(void* manager){
 vehicle_grip::owner=GetCurrentThreadId();auto b=build_adapter::heldModule();if(!b||!manager){resetGrips();return;}
 for(int h=0;h<2;h++)handOccupied[h]=vehicle_adapter::occupied(h,weaponVisible);
 if(riding){rideContacts(manager);return;}
 if(!flying){updateSteering();return;}
 auto carrier=scene::find(scene::root(car),"FCCockpitCarrier"),stick=scene::find(carrier,"FCFlightControl");
 if(!stick){resetGrips();return;}
 auto c=scene::world(carrier);auto t=scene::world(stick);auto baseLocal=scene::local(stick);for(int i=0;i<3;i++)for(int j=0;j<3;j++)baseLocal.r[i][j]=i==j?1.f:0.f;auto neutral=vehicle_grip::world(c,baseLocal);Hand h[2];{std::lock_guard<std::mutex> g(lock);h[0]=hands[0];h[1]=hands[1];}
 auto now=GetTickCount64();scene::Vec delta{};int count=0;
 for(int i=0;i<2;i++){
  scene::Transform raw{};auto socket=scene::find(stick,i?"FCFlightGripR":"FCFlightGripL");
  bool valid=socket&&vehicle_grip::raw(manager,i,raw)&&h[i].valid&&now-h[i].poseAt<200&&now-h[i].time<200;
  bool nearControl=valid&&scene::distance(raw.p,scene::world(socket).p)<12*c.scale;
  controlNear[i]=nearControl&&!nativeMenu&&!handOccupied[i];
  if(!valid||h[i].grip<.35f||nativeMenu||handOccupied[i])grabbed[i]=false;
  else if(!grabbed[i]&&nearControl&&h[i].gripClaim&&h[i].grip>.65f){grabbed[i]=true;gripLocal[i]=vehicle_grip::relative(t,raw);}
  if(grabbed[i]){
   auto target=vehicle_grip::world(t,gripLocal[i]);
   if(scene::distance(raw.p,target.p)>32*c.scale)grabbed[i]=false;
   else {auto rest=vehicle_grip::world(neutral,gripLocal[i]);auto travel=scene::inverseRotate(c,raw.p-rest.p)*(1/c.scale);delta=delta+travel;++count;}
  }
 }
 // One hand owns direction; two hands share it. A moving controller drives the
 // control, never an independently animated wrist or the aircraft's world travel.
 stickX=count?std::clamp(delta.x/(count*10.f),-1.f,1.f):0;
 stickY=count?std::clamp(delta.y/(count*10.f),-1.f,1.f):0;
 float pitch=stickY*.25f,roll=-stickX*.25f;auto& local=scene::local(stick);
 float cp=std::cos(pitch),sp=std::sin(pitch),cr=std::cos(roll),sr=std::sin(roll);
 mounted_frame::Mat rx={{{1,0,0},{0,cp,-sp},{0,sp,cp}}},ry={{{cr,0,sr},{0,1,0},{-sr,0,cr}}};auto rot=mounted_frame::mul(rx,ry);memcpy(local.r,rot.a,sizeof local.r);scene::update(stick);t=scene::world(stick);
 for(int i=0;i<2;i++){controlHeld[i]=grabbed[i];vehicle_grip::targets[i]={grabbed[i],manager,vehicle_grip::world(t,gripLocal[i])};}
 if(!testDir.empty()){static ULONGLONG last=0;if(now-last>100){last=now;std::ofstream o(testDir+"\\flight-control.json");o<<"{\"gripped\":["<<grabbed[0]<<","<<grabbed[1]<<"],\"sockets\":[";for(int i=0;i<2;i++){if(i)o<<",";auto n=scene::find(stick,i?"FCFlightGripR":"FCFlightGripL");auto p=vehicle_adapter::tracking(scene::world(n).p);o<<"["<<p.x<<","<<p.y<<","<<p.z<<"]";}o<<"],\"x\":"<<stickX<<",\"y\":"<<stickY<<"}";}}
}
void __cdecl message(Message* m){
 if(m->type==DeferredInit){
  getCar=compile("GetRefVariable \"CurrentCarRef\" (GetFormFromMod \"UmDrivingVIIIb.esp\" \"000ADE\")",true);
  getPlayer=compile("player",true);getMenu=compile("MenuMode",true);getWeaponOut=compile("player.IsWeaponOut",true);
  getDriving=compile("GetVariable \"Driving\"",true);
  getVar=compile("string_var key\nbegin function {key}\nSetFunctionValue (GetVariable \"$key\")\nend");
  setVar=compile("string_var key\nfloat val\nbegin function {key,val}\nSetVariable \"$key\" val\nend");
  auto base=call(compile("GetFormFromMod \"UmDrivingVIIIb.esp\" \"00E6BC\"",true));supported=base.type==2?base.value.form:nullptr;
  auto fb=call(compile("GetFormFromMod \"Player Vertibird.esm\" \"0045B4\"",true));flightBase=fb.type==2?fb.value.form:nullptr;
  if(flightBase){getFlight=compile("GetRefVariable \"PilotingBird\" (GetFormFromMod \"Player Vertibird.esm\" \"000AE7\")",true);getPiloting=compile("GetVariable \"Piloting\" (GetFormFromMod \"Player Vertibird.esm\" \"000AE7\")",true);}
  getAngleZ=compile("GetAngle Z",true);getDelta=compile("GetSecondsPassed",true);
  auto fs=call(compile("GetFormFromMod \"Player Vertibird.esm\" \"0045B6\"",true));flightScript=fs.type==2?fs.value.form:nullptr;
  getX=compile("GetPos X",true);getY=compile("GetPos Y",true);getZ=compile("GetPos Z",true);
  yawFlight=compile("float axis\nbegin function {axis}\nplayer.SetAngle Z (player.GetAngle Z + axis * 60 * GetSecondsPassed)\nend");
  refreshSeat=compile("ref aircraft\nbegin function {aircraft}\nif aircraft.GetVariable \"Controlling\" == 1 || aircraft.GetVariable \"Driving\" == 3\nplayer.SetPosEx (player.GetPos X) (player.GetPos Y) (player.GetPos Z)\nendif\nend");
  showPilot=compile("begin function {}\nplayer.SetActorAlpha 1\nend");
  allowDriveCombat=compile("begin function {}\nEnablePlayerControls 0 0 1 0 0 0 0\nend");
  getPlayedIdle=compile("player.GetPlayedIdle",true);
  sitPose=compile("begin function {}\nSetFunctionValue (player.ForcePlayIdle SitChairRelaxIdleC 1)\nend");
  stopPose=compile("begin function {}\nplayer.ForceStopIdle 1\nend");
  flightControlLocks=compile("begin function {}\nDisablePlayerControls 1 1 0 1 0 0 0\nend");
  initRide();
  ready=flightControlLocks&&vehicle_adapter::heldPolicyVerified()&&vehicle_grip::install()&&sitPose&&stopPose&&allowDriveCombat&&refreshSeat&&showPilot&&vehicle_contact::verified()&&mounted_frame::install()&&vehicle_publication::install(beginPublication,endPublication,anchorPublication,finalizePublication,observeBody,beforeContacts)&&getAngleZ&&getDelta&&(!flightBase||flightScript)&&(supported||flightBase||rideEnabled)&&getCar&&getPlayer&&getMenu&&getDriving&&getVar&&setVar&&hooks();log(ready?"Vehicle bridge ready":"Vehicle bridge failed");controlsAvailable=ready?((supported?1u:0u)|(flightBase?2u:0u)|(rideEnabled&&bighornerEnabled?4u:0u)|(rideEnabled&&deathclawEnabled?8u:0u)):0u;
 }
 if(m->type==PostLoadGame||m->type==NewGame){clear();rideRestorePending=true;loaded=m->type==NewGame||m->data!=nullptr;auto p=call(getPlayer);player=p.type==2?p.value.form:nullptr;}
 if(m->type==PreLoadGame||m->type==ExitMenu||m->type==ExitGame){rideRestorePending=false;rideHostileRef=0;if(riding)stopRide(false);seatedPose=false;clear();loaded=false;player=nullptr;}
 if(m->type==MainLoop&&loaded){restoreRideLease();tickRideHostility();}
 if(m->type==MainLoop&&ready&&loaded){tick();if(!testDir.empty()){static ULONGLONG last=0;auto now=GetTickCount64();if(now-last>3000){last=now;vehicle_perf::write(testDir);}}}
}
}
extern "C" __declspec(dllexport) bool __cdecl FC_VehicleActive(){return active;}
extern "C" __declspec(dllexport) unsigned __cdecl FC_VehicleInput(const char* name,int hand,int kind,float x,float y){
 if(!name||hand<0||hand>1)return 0;bool valid=(kind&8)!=0;std::lock_guard<std::mutex> g(lock);auto& h=hands[hand];
 if(!strcmp(name,"vehicle_pose")){h.valid=valid;h.poseAt=GetTickCount64();return 0;}
 int control=!strcmp(name,"squeeze")?0:!strcmp(name,"trigger")?1:!strcmp(name,"primary_click")?2:!strcmp(name,"secondary_click")?3:!strcmp(name,"thumbstick")?4:!strcmp(name,"thumbstick_click")?5:-1;
 if(control<0)return 0;
 static bool drain[2][6]{},menuHeld[2][6]{};
 bool pressed=valid&&(std::abs(x)>.15f||std::abs(y)>.15f),mounted=active&&!nativeMenu;
 if(nativeMenu&&pressed)menuHeld[hand][control]=true;
 if(!pressed)menuHeld[hand][control]=false;
 bool logicalValid=valid&&!(active&&menuHeld[hand][control]);
 if(control==0){
  // A squeeze belongs to the interaction where it STARTED. Carrying a native
  // magazine past the wheel/flight handle must not turn that held squeeze into
  // a vehicle grab (and send a false release to TTR).
  if(!mounted||!logicalValid||x<.35f)h.gripClaim=false;
  else if(h.grip<=.65f&&x>.65f)h.gripClaim=controlNear[hand]&&!handOccupied[hand];
  h.grip=logicalValid?x:0;h.time=GetTickCount64();
 }if(control==1)h.trigger=logicalValid?x:0;if(control==2)h.x=logicalValid&&x>.5f;if(control==3)h.b=logicalValid&&x>.5f;if(control==4){h.sx=logicalValid?x:0;h.sy=logicalValid?y:0;}if(control==5)h.stick=logicalValid&&x>.5f;
 bool vehicleOwns=mounted;
 if(control==0)vehicleOwns=(mounted&&(controlHeld[hand]||h.gripClaim))||(!active&&rideMountReady);
 if(control==1||control==2)vehicleOwns=mounted&&flying&&!weaponVisible&&!handOccupied[hand];
 if(control==3&&hand==0&&!flying)vehicleOwns=false; // deliberate native Y pause
 if(riding&&(control==1||control==2||control==5||(control==3&&hand==0)))vehicleOwns=false;
 if(vehicleOwns&&pressed&&!menuHeld[hand][control])drain[hand][control]=true;
 bool consume=vehicleOwns||drain[hand][control];if(!pressed)drain[hand][control]=false;return consume?1:0;
}
extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Query(const NVSE* n,PluginInfo* i){i->infoVersion=1;i->name="FilthyCasualVehicles";i->version=1;return !n->isEditor&&n->runtime==0x040020D0;}
extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Load(const NVSE* n){
 file=_fsopen((std::string(n->runtimeDirectory())+"FilthyCasualVehicles.log").c_str(),"w",_SH_DENYNO);
 rideArrays=static_cast<Arrays*>(n->queryInterface(5));scripts=static_cast<Scripts*>(n->queryInterface(6));commands=static_cast<Table*>(n->queryInterface(3));auto msgs=static_cast<Messaging*>(n->queryInterface(2));
 if(!scripts||!commands||!msgs||!build_adapter::verifiedTTWVR(GetModuleHandleW(L"TTWVR.dll")))return false;
 wchar_t path[MAX_PATH]{};GetModuleFileNameW(GetModuleHandleW(L"FilthyCasualVehicles.dll"),path,MAX_PATH);auto slash=wcsrchr(path,L'\\');if(!slash)return false;*slash=0;
 bighornerEnabled=GetPrivateProfileIntW(L"Bighorner",L"Enabled",0,(std::wstring(path)+L"\\FilthyCasualVehicles.ini").c_str())!=0;
 deathclawEnabled=GetPrivateProfileIntW(L"Deathclaw",L"Enabled",0,(std::wstring(path)+L"\\FilthyCasualVehicles.ini").c_str())!=0;rideEnabled=bighornerEnabled||deathclawEnabled;
 auto readChallengeSeconds=[&](const wchar_t* section,float fallback){wchar_t value[128]{};GetPrivateProfileStringW(section,L"ChallengeSeconds",L"",value,128,(std::wstring(path)+L"\\FilthyCasualVehicles.ini").c_str());return creature_ride::configuredSeconds(value,fallback);};
 bighornerChallengeSeconds=readChallengeSeconds(L"Bighorner",creature_ride::bighornerDefaultSeconds);
 deathclawChallengeSeconds=readChallengeSeconds(L"Deathclaw",creature_ride::deathclawDefaultSeconds);
 std::wstring dir=std::wstring(path)+L"\\FilthyCasualVehicles";if(!layer_discovery::append(L"XR_API_LAYER_PATH",dir)||!layer_discovery::append(L"XR_ENABLE_API_LAYERS",L"XR_APILAYER_NIKAMI_vehicles"))return false;
 char dirTest[MAX_PATH]{};GetEnvironmentVariableA("SEATED_TEST_DIR",dirTest,MAX_PATH);testDir=dirTest;
 return msgs->listen(n->getPluginHandle(),"NVSE",message);
}

extern "C" __declspec(dllexport) bool __cdecl FC_NativePoseOwner(){return native_pose_dispatch::original!=nullptr;}
extern "C" __declspec(dllexport) bool __cdecl FC_NativePoseSubscribe(native_pose_dispatch::Callback before,native_pose_dispatch::Completed after){return native_pose_dispatch::local(before,after);}

extern "C" __declspec(dllexport) const fc_provider::Provider* __cdecl FC_ControlsProviderV1(){
 static const fc_provider::Mode modes[]={
 {"vehicles.driving","DRIVING","Left stick drives forward/reverse; right stick steers; opposite direction brakes before reversing. Neutral slows to a stop. Both triggers stay native for weapons. Optional independent rim grips.",
 {"Native left trigger","Left wheel grip / native grab","Native X","Pause",nullptr,"Native right trigger","Right wheel grip / native grab","Native A","Exit vehicle",nullptr,"Left Menu",nullptr,nullptr,"Brake / reverse","Forward","Steer left","Steer right",nullptr,nullptr,"Left thumb-rest touch","Right thumb-rest touch","Weapon wheel","Consumable wheel","Reload mode","Height / settings","Weapon tutorial"}},
 {"vehicles.flight","FLIGHT","Right stick flies (also while gripping); left stick yaws. Grip the visible control with either/both hands for physical steering; release frees that hand.",
 {"Left aircraft weapon / held-item trigger","Left flight-control grip / native grab","Descend","Aircraft HUD","Left weapon selector","Right aircraft weapon / held-item trigger","Right flight-control grip / native grab","Climb","Land","Right weapon selector",nullptr,"Yaw left","Yaw right",nullptr,nullptr,"Fly left","Fly right","Fly backward","Fly forward","Left thumb-rest touch","Right thumb-rest touch","Weapon wheel","Consumable wheel","Reload mode","Height / settings","Weapon tutorial"}}
 ,{"vehicles.bighorner","BIGHORNER RIDE","Both grips near a Bighorner to mount. Release your hands, or grip near the withers to hold on. B dismounts.", {"Native trigger","Hold on / native grab","Native X","Pause",nullptr,"Native trigger","Hold on / native grab","Native A","Dismount"}}
,{"vehicles.deathclaw","DEATHCLAW RIDE","Sneak behind an unaware Deathclaw and grip its body with both hands. Hold on through two minutes of bucks to earn steering. B dismounts; it immediately becomes hostile.", {"Native trigger","Hold on / native grab","Native X","Pause",nullptr,"Native trigger","Hold on / native grab","Native A","Dismount"}}
 };
 static const fc_provider::Provider p={sizeof(fc_provider::Provider),1,4,modes,
 +[]()->uint32_t{return controlsAvailable.load();},+[]()->int{return !active?-1:riding?(ridingDeathclaw?3:2):flying?1:0;}};
 return &p;
}

extern "C" __declspec(dllexport) bool __cdecl FC_VehicleHandContact(int hand){return hand>=0&&hand<2&&controlHeld[hand];}

// Returns the last completed native carrier frame; MainLoop may snapshot it
// to open UI, and post-publication consumers receive the current transaction.
extern "C" __declspec(dllexport) void* __cdecl FC_VehicleTutorialFrame(scene::Transform* out){
 if(!out||GetCurrentThreadId()!=mainThread||!active||!car||!publicationCommitted)return nullptr;
 if(!publishedUINode||!std::isfinite(publishedUICarrier.scale)||publishedUICarrier.scale<.001f)return nullptr;
 *out=publishedUICarrier;return publishedUINode;
}

extern "C" __declspec(dllexport) unsigned __cdecl FC_RidePulse(){return ridePulse.load();}

extern "C" __declspec(dllexport) int __cdecl FC_SeatContext(){return !active?0:riding?(ridingDeathclaw?4:3):flying?2:1;}
