#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../feature_client.hpp"
#include <cstdio>
#include <cstring>
#include <mutex>
#include <atomic>
#include <fstream>
#include <share.h>
#include "../nvse_function.hpp"
#include "../build_adapter.hpp"
#include "../scene_adapter.hpp"
#include "../reload_adapter.hpp"
#include "../layer_discovery.hpp"
#include "adapter.hpp"
#include "../controls_provider.hpp"
#include "publication_adapter.hpp"
#include "performance.hpp"
#include "mounted_frame.hpp"
#include "contact_frame.hpp"
#include "grip_attachment.hpp"
#include "drive_input.hpp"
#include "arcade_drive.hpp"
#include "flight_gimbal.hpp"
#include "drive_trace.hpp"
#include "motion_adapter.hpp"
#include "fleet_catalog.hpp"
#include "stock_roadster_wheel.hpp"
#include "stock_vertibird_cabin.hpp"
#include "road_contacts.hpp"
#include "road_fit_native.hpp"
#include "knee_steering.hpp"
#include "animation_probe.hpp"
#include "seated_animation.hpp"
#include "ride_challenge.hpp"
#include "../glass_effects.hpp"
namespace {
constexpr unsigned DrivingMode=1,FlightMode=2,BighornerMode=4,DeathclawMode=8;
std::atomic<unsigned> controlsAvailable{0},requestedModes{DrivingMode};
unsigned installedModes=0;
bool inputDrain[2][6]{},menuHeld[2][6]{};
std::atomic<bool> nativeMenu{true};
float bighornerChallengeSeconds=creature_ride::bighornerDefaultSeconds,deathclawChallengeSeconds=creature_ride::deathclawDefaultSeconds;
Arrays* rideArrays{};bool drivingFeature=true,flightFeature=false;bool rideEnabled=false,bighornerEnabled=false,deathclawEnabled=false;std::atomic<bool> ridingDeathclaw{false};std::atomic<bool> riding{false},rideMountReady{false};
Scripts* scripts{};FILE* file{};std::mutex lock;
struct Hand {float grip=0,trigger=0,sx=0,sy=0;bool b=false,x=false,stick=false,valid=false,gripClaim=false,triggerClaim=false;ULONGLONG time=0,poseAt=0,rimPressAt=0;};Hand hands[2];
std::atomic<bool> cameraAnchorReady{false};float cameraAnchorZ=0,mountedYawOffset=0;bool mountedYawReady=false;scene::Vec mountedEyeOffset{};mounted_frame::Mat mountedStageHeading=mounted_frame::identity();
std::atomic<bool> active{false};bool loaded=false,ready=false,exitWas=false,reverseWas=false;Form* car{};Form* player{};Form* supported{};Form* flightBase{};Form* flightScript{};float flightHeading=0;std::atomic<bool> flying{false};float flightX=0,flightY=0;unsigned flightButtons=0;
scene::Vec earlyRaw[2]{};
std::atomic<bool> fittedFlightControls{false};
float flightPitch=0,flightBank=0,physicalLift=0;Script* setFlightBank{};
bool kneeDriving=false;std::atomic<bool> weaponVisible{false};Script* getWeaponOut{};
Form* drivingScript{};
Script *getAttachedScript{},*getCar{},*getPlayer{},*getDriving{},*getMenu{},*setVar{},*getVar{},*getFlight{},*getPiloting{},*getX{},*getY{},*getZ{},*yawFlight{},*showPilot{},*getAngleZ{},*getDelta{},*refreshSeat{},*sitPose{},*stopPose{},*allowDriveCombat{},*flightControlLocks{},*getPlayedIdle{};bool seatedPose=false;
Script *engineIdle{},*engineDrive{},*engineStop{},*getEnginePitch{},*setEnginePitch{};int engineMode=0;float savedEnginePitch=0,enginePitch=0;bool enginePitchOwned=false;
void restoreEnginePitch(){if(enginePitchOwned&&loaded){unsigned bits;memcpy(&bits,&savedEnginePitch,4);Element e;scripts->call(setEnginePitch,nullptr,nullptr,&e,1,bits);}enginePitchOwned=false;}
Script *readFrequency{},*setFrequency{};arcade_drive::Config driveConfig;arcade_drive::State driveState;
arcade_drive::Crest crest;
Script* setVerticalVelocity{};
Script *vehicleContacts{}, *pushParkedCar{};
struct ImpactSample { bool ready=false; scene::Vec position{}; float elapsed=0,previousTravelSpeed=0; };
ImpactSample lastImpact;
bool arcadeEnabled=false,arcadeOwns=false,roadFitEnabled=true;float savedFrequency[4]{},savedCruise=0,lastDriveDelta=0;unsigned wheelHits=0;
const char* driveSequences[]={"Creatures\\MkTRoadster\\mtforward.kf","Creatures\\MkTRoadster\\mtfastforward.kf","Creatures\\MkTRoadster\\mtleft.kf","Creatures\\MkTRoadster\\mtbackward.kf"};
Form* fleetBases[std::size(vehicle_fleet::models)]{};const vehicle_fleet::Model* carModel=nullptr;
stock_roadster_wheel::Rig stockWheel;
void* stockWheelAttempt=nullptr;
Script *insertStockWheel{},*removeStockWheel{};
stock_vertibird_cabin::Rig stockCabin;
void* stockCabinAttempt=nullptr;
Script *insertStockCabin{},*removeStockCabin{};
bool stockCabinEnabled=false;
std::string modelSequences[4];float savedTurn=0;
const vehicle_fleet::Model* findCarModel(Form* ref){if(!ref)return nullptr;auto base=scene::field<Form*>(ref,0x20);for(unsigned i=0;i<std::size(fleetBases);i++)if(base&&base==fleetBases[i])return &vehicle_fleet::models[i];return nullptr;}
struct ParkedFit {unsigned id=0;road_fit_native::State fit;};
std::vector<ParkedFit> parkedFits;ULONGLONG parkedScan=0;unsigned parkedFrame=0;
Script* parkedActors=nullptr;
void restoreParked(){for(auto& p:parkedFits)p.fit.restore(vehicle_adapter::lookup(p.id));}
void scanParked(){
 restoreParked();if(!player||!parkedActors||!rideArrays||!arcadeEnabled||!roadFitEnabled)return;
 auto now=GetTickCount64();if(now<parkedScan)return;parkedScan=now+1000;
 Element list;scripts->call(parkedActors,nullptr,nullptr,&list,0);if(list.type!=4||!list.value.array)return;
 auto count=rideArrays->size(list.value.array);if(count>1024)return;
 std::vector<Element> values(count);if(!count||!rideArrays->elements(list.value.array,values.data(),nullptr))return;
 std::vector<std::pair<float,unsigned>> candidates;
 for(auto& e:values){auto ref=e.type==2?e.value.form:nullptr;if(!ref||!findCarModel(ref)||!scene::root(ref))continue;
  float distance=scene::distance(scene::field<scene::Vec>(ref,0x30),scene::field<scene::Vec>(player,0x30));
  if(distance<2400)candidates.push_back({distance,ref->id});}
 std::sort(candidates.begin(),candidates.end());if(candidates.size()>12)candidates.resize(12);
 std::vector<ParkedFit> next;for(auto& n:candidates){auto found=std::find_if(parkedFits.begin(),parkedFits.end(),[&](const ParkedFit& p){return p.id==n.second;});next.push_back(found!=parkedFits.end()?*found:ParkedFit{n.second,{}});}
 parkedFits=std::move(next);
}
void presentParked(){
 if(!loaded||!player||!arcadeEnabled||!roadFitEnabled)return;++parkedFrame;
 for(unsigned i=0;i<parkedFits.size();i++){auto& p=parkedFits[i];auto ref=vehicle_adapter::lookup(p.id);
  if(!ref||!scene::root(ref)||ref==car)continue;
  // Four casts for only three nearby cars per frame. Other cars reuse their
  // relative support correction; no world-position teleport or physics write.
  if(i%4==parkedFrame%4)p.fit.apply(ref,player,.05f);else p.fit.presentCached(ref);
 }
}
void chooseCarModel(const vehicle_fleet::Model* model){carModel=model;driveConfig={};if(!model)return;driveConfig.topSpeed=model->speed;driveConfig.acceleration=model->acceleration;driveConfig.braking=model->braking;const char* suffix[]={"\\mtforward.kf","\\mtfastforward.kf","\\mtleft.kf","\\mtbackward.kf"};for(int i=0;i<4;i++){modelSequences[i]=std::string(model->directory)+suffix[i];driveSequences[i]=modelSequences[i].c_str();}}
void restoreDrive();void updateDrive();
float steering=0,throttle=0,brake=0,wheelAngle=0,grabAngle[2]{},grabWheel[2]{};bool grabbed[2]{};
std::atomic<bool> controlNear[2]{},controlHeld[2]{},handOccupied[2]{};
scene::Transform gripLocal[2]{};ULONGLONG flightGrabAt[2]{};float grabInputAngle[2]{},stickX=0,stickY=0;
void updatePhysicalControls(void* manager);
void stopRide(bool restore);void updateRideNodes();bool tickRide();void rideContacts(void*);void initRide();
const char* seatName(){return riding?"FCRideSeat":flying?"PlayerPos":"Seat";}
const char* carrierName(){return riding?"FCRideCarrier":flying?"FCCockpitCarrier":"FCWheelCarrier";}
const char* eyeName(){return riding?"FCRideEye":flying?"FCPilotEye":"FCDriverEye";}
// Original car meshes have a driver Seat but no FC nodes. Basic driving must
// work without the optional private wheel import. Use one native root frame
// for camera, body, hands and UI; never borrow a passenger anchor.
void* mountedCarrier(void* root){
 auto fitted=scene::find(root,carrierName());
 // The stock aircraft's PlayerPos inherits the animated spine's rotated bind
 // axes. Its root, like the stock car, supplies the forward/up camera frame.
 return fitted?fitted:(riding?nullptr:root);
}
bool mountedEye(void* root,void* body,void* seat,scene::Vec& point){
 if(auto eye=scene::find(body,eyeName())){point=scene::world(eye).p;return true;}
 if(riding||!root||!body||!seat)return false;
 auto frame=scene::world(body);
 point=scene::world(seat).p+scene::rotate(frame,{0,0,cameraAnchorReady?cameraAnchorZ:0});
 return true;
}
unsigned mountedMode(){
 if(riding)return ridingDeathclaw?DeathclawMode:BighornerMode;
 return flying?FlightMode:DrivingMode;
}
void resetGrips(){vehicle_grip::reset();kneeDriving=false;knee_steering::reset();{std::lock_guard<std::mutex> guard(lock);for(auto& h:hands){h.rimPressAt=0;h.gripClaim=false;}}for(int h=0;h<2;h++){grabbed[h]=false;controlNear[h]=false;controlHeld[h]=false;handOccupied[h]=false;}stickX=stickY=physicalLift=0;}

std::string testDir;scene::Vec center{},right{},up{},axis{};
void log(const char* s){if(file){fprintf(file,"%s\n",s);fflush(file);}}
template<class... A> Element call(Script* fn,A... a){Element e;if(fn)scripts->call(fn,nullptr,nullptr,&e,sizeof...(a),a...);return e;}
void clearStockWheel(){
 const bool attached=stockWheel.root!=nullptr;
 stockWheel.restore();
 if(attached&&car&&loaded)call(removeStockWheel,car);
 stockWheelAttempt=nullptr;
}
void prepareStockWheel(void* root){
 if(!carModel||strcmp(carModel->form,"00E6BC")||!root||root==stockWheelAttempt)return;
 clearStockWheel();stockWheelAttempt=root;
 scene::Transform bind{};
 if(!insertStockWheel||!removeStockWheel||scene::find(root,"FCWheelCarrier")||!stockWheel.stockRim(root,bind))return;
 call(insertStockWheel,car);
 if(stockWheel.attach(root))log("Roadster wheel bound to the original steering skin");
 else{
  call(removeStockWheel,car);
  if(auto basis=scene::find(root,"FCStockWheelBasis"))scene::moveParent(basis,nullptr);
  stockWheel.restore();
 }
}
void clearStockCabin(){
 const bool attached=stockCabin.root!=nullptr;
 stockCabin.restore();
 if(attached&&car&&loaded)call(removeStockCabin,car);
 stockCabinAttempt=nullptr;
}
void prepareStockCabin(void* root){
 if(!stockCabinEnabled||!root||root==stockCabinAttempt)return;
 clearStockCabin();stockCabinAttempt=root;
 if(!insertStockCabin||!removeStockCabin||scene::find(root,"FCCockpitCarrier")||!scene::find(root,"Bip01 Spine1"))return;
 call(insertStockCabin,car);
 if(stockCabin.mount(root))log("Attached installed Vertibird interior to native airframe");
 else{
  call(removeStockCabin,car);
  if(auto carrier=scene::find(root,"FCCockpitCarrier"))scene::moveParent(carrier,nullptr);
 }
}
template<class... A> Element callOn(Script* fn,Form* object,A... a){Element e;if(fn)scripts->call(fn,object,nullptr,&e,sizeof...(a),a...);return e;}
Script* compile(const char* s,bool expr=false){auto f=expr?scripts->expression(s):scripts->compile(s);if(!f)log(s);return f;}
#include "vehicle_radio.hpp"
#include "glazing_events.hpp"
void variable(const char* key,float value){unsigned n;memcpy(&n,&value,4);callOn(setVar,car,key,n);}
float dot(scene::Vec a,scene::Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
scene::Vec unit(scene::Vec a){auto n=scene::distance(a,{});return n>.00001f?a*(1/n):scene::Vec{};}
float wrap(float a){while(a>3.14159265f)a-=6.2831853f;while(a< -3.14159265f)a+=6.2831853f;return a;}
#include "vehicle_commands.hpp"

void* mountedRoot=nullptr;
void invalidateMountedFrame(){resetGrips();vehicle_contact::reset();mounted_frame::reset();cameraAnchorReady=false;mountedYawReady=false;mountedRoot=nullptr;}
void suspendMounted(){vehicle_motion::clear();invalidateMountedFrame();active=false;steering=throttle=brake=0;flightX=flightY=0;flightButtons=0;grabbed[0]=grabbed[1]=false;}
void clear(){vehicle_radio::reset();clearStockWheel();clearStockCabin();restoreDrive();if(car&&engineMode&&loaded)callOn(engineStop,car);restoreEnginePitch();engineMode=0;if(riding)stopRide(loaded);if(seatedPose&&player&&loaded&&seated_animation::ownsActive(player))call(stopPose);seated_animation::restore();seatedPose=false;invalidateMountedFrame();if(active)vehicle_perf::write(testDir);active=false;fittedFlightControls=false;flying=false;flightX=flightY=0;flightButtons=0;exitWas=reverseWas=false;car=nullptr;drivingScript=nullptr;steering=throttle=brake=wheelAngle=0;grabbed[0]=grabbed[1]=false;}
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
 scene::Vec checkedEye{};auto carrier=mountedCarrier(root);
 if(!carrier||!mountedEye(root,carrier,seat,checkedEye)){invalidateMountedFrame();return false;}
 auto point=scene::world(seat).p;auto actorPoint=scene::field<scene::Vec>(player,0x30);
 auto nativePoint=arcadeOwns&&roadFitEnabled?road_fit_native::current.nativePoint(point):point;
 if(scene::distance(nativePoint,actorPoint)>4){invalidateMountedFrame();return false;}
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
 auto body=mountedCarrier(root);scene::Vec eyePoint{};
 // Do not leave a partially overwritten camera origin/attached flag when a
 // fitted flight or creature rig is absent. Validate the complete frame first.
 if(!body||!mountedEye(root,body,seat,eyePoint)){invalidateMountedFrame();return;}
 auto point=scene::world(seat).p;
 scene::field<scene::Vec>(manager,0xB04)={point.x,point.y,point.z+cameraAnchorZ};
 // TTR resets this each publication. Its native attached-camera branch sets
 // the same flag: both coordinate conversions then use this origin directly,
 // instead of mixing the walking-height reference with the moving seat.
 scene::field<unsigned char>(manager,0xB21)=1;
 // Carry the tracking-space heading with the rendered chassis, preserving
 // the rider's initial relative heading and all physical HMD free look.
 if(body){auto t=scene::world(body);float yaw=std::atan2(t.r[0][1],t.r[1][1]);auto& nativeYaw=scene::field<float>(manager,0xB3C);
  auto nativeBasis=mounted_frame::nativeBasis(manager);
  bool calibrating=!mountedYawReady;
  if(calibrating){
   // The native HMD rotation's second column is forward (exact-build VATS
   // consumer copies matrix offsets 4,16,28). Remove the native world basis
   // and latch physical boarding yaw; pitch and subsequent head turns stay free.
   mounted_frame::Mat head;mounted_frame::originalWorldRotation(manager,&head,static_cast<unsigned char*>(manager)+0x2C);
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
  if(calibrating){
   scene::Vec nativeHead;mounted_frame::originalToWorld(manager,&nativeHead,&scene::field<scene::Vec>(manager,0x20));
   auto delta=mounted_frame::apply(correction,nativeHead-scene::field<scene::Vec>(manager,0xB04));
   mountedEyeOffset=mounted_frame::apply(mounted_frame::transpose(bodyNow),delta);
  }
  // Seat/root stays on the real floor. The eye anchor absorbs the user's
  // standing-height calibration at boarding without sinking the actor model.
  using SeatOffset=float(__cdecl*)(int);auto core=GetModuleHandleW(L"SeatedControls.dll");auto offset=core?reinterpret_cast<SeatOffset>(GetProcAddress(core,"SC_SeatOffset")):nullptr;
  int mode=riding?(ridingDeathclaw?4:3):flying?2:1;float seatOffset=offset?offset(mode):0;
  if(!std::isfinite(seatOffset))seatOffset=0;
  auto origin=eyePoint-mounted_frame::apply(bodyNow,mountedEyeOffset)+mounted_frame::apply(bodyNow,{0,0,seatOffset*70.f});
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
 presentParked();
 if(!loaded||!active||!car||!player){invalidateMountedFrame();return;}
 if(riding&&vehicle_adapter::lookup(rideCarrierID)!=car){clear();return;}
 auto root=scene::root(car);
 if(!root){invalidateMountedFrame();return;}
 if(root!=mountedRoot){invalidateMountedFrame();mountedRoot=root;}
 // Refresh the dependency's real seat before applying visual suspension.
 // SyncPos must never feed the visual offset into player collision. The
 // camera/hand frame below is allowed to use the fitted render seat instead.
 if(arcadeOwns&&roadFitEnabled)road_fit_native::restore(car);
 ++publicationApplied;if(riding){updateRideNodes();call(rideRefresh);}else call(refreshSeat,car);
 if(arcadeOwns&&roadFitEnabled&&!flying&&!riding)road_fit_native::apply(car,player,lastDriveDelta);
 if(arcadeOwns&&!testDir.empty()&&publicationCalls.load()%20==0){
  std::ofstream out(testDir+"\\road-fit.json");out<<"{\"pid\":"<<GetCurrentProcessId()<<",\"car\":"<<car->id<<",\"applied\":"<<road_fit_native::applied<<",\"showing\":"<<road_fit_native::showing<<",\"offset\":"<<road_fit_native::offset<<",\"gapBefore\":"<<road_fit_native::maxGapBefore<<",\"gapAfter\":"<<road_fit_native::maxGapAfter<<"}";
 }
 // SyncPos is the dependency's existing native seat attachment. Refresh its
 // normal SetPosEx path here, before TTR samples the player/camera origin.
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
 auto root=scene::root(player),body=mountedCarrier(scene::root(car));if(!root||!body)return;
 auto t=scene::world(body);std::ofstream out(testDir+"\\mounted-pose.json");out<<"{\"earlyRaw\":[["<<earlyRaw[0].x<<","<<earlyRaw[0].y<<","<<earlyRaw[0].z<<"],["<<earlyRaw[1].x<<","<<earlyRaw[1].y<<","<<earlyRaw[1].z<<"]],\"pid\":"<<GetCurrentProcessId()<<",\"samples\":"<<seated_animation::samples<<",\"applies\":"<<seated_animation::applies<<",\"sourceTime\":"<<seated_animation::sourceTime<<",\"rootScale\":"<<seated_animation::lastRootScale<<",\"pelvisScale\":"<<seated_animation::lastPelvisScale<<",\"pelvisApplyError\":"<<scene::distance(seated_animation::lastTarget,seated_animation::lastImmediate)<<",\"localPredictionError\":"<<seated_animation::localPredictionError<<",\"localRewriteError\":"<<seated_animation::localRewriteError<<",\"pelvisFlags\":"<<seated_animation::lastFlags<<",\"bones\":{";bool first=true;
 for(auto name:{"Bip01 Head","Bip01 Spine","Bip01 Pelvis","Bip01 L Thigh","Bip01 L Calf","Bip01 L Foot","Bip01 R Thigh","Bip01 R Calf","Bip01 R Foot"})if(auto bone=scene::find(root,name)){
  auto p=scene::inverseRotate(t,scene::world(bone).p-t.p)*(1/t.scale);if(!first)out<<",";first=false;out<<"\""<<name<<"\":["<<p.x<<","<<p.y<<","<<p.z<<"]";
 }out<<"}}";
}
void beforeContacts(void* actor){
 if(GetCurrentThreadId()!=mainThread||!active||!publicationCommitted||actor!=player||!car)return;
 auto frame=mounted_frame::read();auto body=mountedCarrier(scene::root(car));
 vehicle_contact::before(frame.manager,scene::root(player),body,publicationCalls.load());
 updatePhysicalControls(frame.manager);
}
void recordHandFrame(){
 if(testDir.empty()||!active||!publicationCommitted||GetCurrentThreadId()!=mainThread||!player||!car)return;
 auto root=scene::root(player),body=mountedCarrier(scene::root(car));
 auto b=build_adapter::heldModule();auto manager=b?reinterpret_cast<void*(__cdecl*)()>(b+0x141CD)():nullptr;
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
 if(GetCurrentThreadId()==mainThread&&roadFitEnabled){
  for(auto& p:parkedFits)if(auto ref=vehicle_adapter::lookup(p.id);ref&&ref!=car)p.fit.finishPublication(ref);
  if(arcadeOwns&&car)road_fit_native::current.finishPublication(car);
 }
 publishedUINode=nullptr;
 if(GetCurrentThreadId()==mainThread&&publicationCommitted&&active&&car&&player){
  vehicle_perf::Scope timing(5,!testDir.empty());
  auto vehicleRoot=scene::root(car);auto body=mountedCarrier(vehicleRoot),seat=scene::find(vehicleRoot,seatName());
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
   auto b=build_adapter::heldModule();auto m=reinterpret_cast<void*(__cdecl*)()>(b+0x141CD)();auto rt=scene::root(car);auto w=scene::find(rt,"FCSteeringWheel"),st=scene::find(rt,"Seat");
   if(m&&st&&count++<12000){auto origin=scene::field<scene::Vec>(m,0xB04),anchor=scene::field<scene::Vec>(m,0xB10),seat=scene::world(st).p,wheel=w?scene::world(w).p:seat;
    auto carrier=mountedCarrier(rt);auto ct=scene::world(carrier);auto relative=scene::inverseRotate(ct,wheel-seat)*(1/ct.scale);float bodyYaw=std::atan2(ct.r[0][1],ct.r[1][1]);
    trace<<"{\"bodyYaw\":"<<bodyYaw<<",\"yawOffset\":"<<mountedYawOffset<<",\"relative\":["<<relative.x<<","<<relative.y<<","<<relative.z<<"],\"sequence\":"<<publicationCalls.load()<<",\"epochPid\":"<<GetCurrentProcessId()<<",\"origin\":["<<origin.x<<","<<origin.y<<","<<origin.z<<"],\"anchor\":["<<anchor.x<<","<<anchor.y<<","<<anchor.z<<"],\"seat\":["<<seat.x<<","<<seat.y<<","<<seat.z<<"],\"wheel\":["<<wheel.x<<","<<wheel.y<<","<<wheel.z<<"],\"yaw\":"<<scene::field<float>(m,0xB3C)<<",\"mode\":"<<unsigned(scene::field<unsigned char>(m,0xB21))<<"}\n";if(count%30==0)trace.flush();
   }
  }
 }
 recordMountedPose();recordHandFrame();
 if(arcadeOwns&&publicationCommitted&&car)drive_trace::record(testDir,car,mountedCarrier(scene::root(car)),publicationCalls.load(),driveState.speed,lastDriveDelta);
 if(guardedPlayer){guardedPlayer[0x680]=(guardedPlayer[0x680]&~3u)|savedMountedControls;guardedPlayer=nullptr;}
 mounted_frame::nativeSetup=false;
}
void tick(){
 if(drivingFeature)scanParked();
 if(car)road_fit_native::restore(car);
 vehicle_perf::Scope timing(0,!testDir.empty()&&active);
 mainThread=GetCurrentThreadId();auto weapon=call(getWeaponOut);weaponVisible=weapon.type==1&&weapon.value.number!=0;
 if(active){auto delta=call(getDelta);if(delta.type==1)seated_animation::advance(float(delta.value.number));}
 if(rideEnabled&&tickRide())return;
 auto flight=call(getFlight),piloting=call(getPiloting);
 auto bird=flight.type==2?flight.value.form:nullptr;
 auto birdRoot=bird?scene::root(bird):nullptr;
 if(flightFeature&&bird&&flightBase&&scene::field<Form*>(bird,0x20)==flightBase&&piloting.type==1&&piloting.value.number==1&&scene::find(birdRoot,"PlayerPos")){
  if(car!=bird){clear();car=bird;flightPitch=flightBank=0;auto heading=callOn(getAngleZ,player);flightHeading=heading.type==1?float(heading.value.number):0;call(showPilot);call(allowDriveCombat);}flying=true;active=true;call(allowDriveCombat);ensureSeatedPose();
  vehicle_radio::tick();prepareStockCabin(birdRoot);
  auto cockpit=scene::find(birdRoot,"FCCockpitCarrier");
  const bool hasHandles=scene::find(cockpit,"FCFlightGripR")&&scene::find(cockpit,"FCFlightGripL");
  fittedFlightControls=hasHandles;
  if(cockpit){
   for(auto name:{"BodyMain","BodyTail"})if(auto body=scene::find(birdRoot,name))scene::field<unsigned>(body,0x30)|=1u;
   if(auto caps=scene::find(cockpit,"VertibirdHull01:2"))scene::field<unsigned>(caps,0x30)|=1u;
  }


  Hand h[2];{std::lock_guard<std::mutex> g(lock);h[0]=hands[0];h[1]=hands[1];}
  auto now=GetTickCount64();auto m=call(getMenu);nativeMenu=m.type!=1||m.value.number!=0;bool live=!nativeMenu;
  for(auto& v:h)live=live&&v.valid&&now-v.poseAt<200&&now-v.time<200;
  // A thumbstick is a seated control assist, never a remote flight control.
  // Each hand must actually hold its matching handle. Releasing that handle
  // removes all flight intent from that side, while head tracking stays free.
  const bool seated=seated_animation::ownedActor==player&&seated_animation::ownedSequence;
  const bool cyclic=live&&seated&&(!hasHandles||(controlHeld[1]&&h[1].grip>.35f));
  const bool collective=live&&seated&&(!hasHandles||(controlHeld[0]&&h[0].grip>.35f));
  const bool thumb=std::abs(h[1].sx)>.15f||std::abs(h[1].sy)>.15f;
  flightX=cyclic?(thumb?h[1].sx:stickX):0;flightY=cyclic?(thumb?h[1].sy:stickY):0;
  auto flightDelta=call(getDelta);float flightDt=flightDelta.type==1?std::clamp(float(flightDelta.value.number),0.f,.05f):0;
  flightPitch=arcade_drive::approach(flightPitch,-arcade_drive::axis(flightY)*14.f,flightDt*35.f);
  auto landing=callOn(getVar,car,"Landing");float bankTarget=live&&landing.type==1&&landing.value.number==0?-arcade_drive::axis(flightX)*12.f:0;
  flightBank=arcade_drive::approach(flightBank,bankTarget,flightDt*30.f);unsigned bankBits;memcpy(&bankBits,&flightBank,4);callOn(setFlightBank,car,bankBits);
  throttle=cyclic&&h[1].triggerClaim?h[1].trigger:0;brake=collective&&h[0].triggerClaim?h[0].trigger:0;
  flightButtons=0;if(live){if(cyclic&&h[1].x&&!weaponVisible&&!handOccupied[1])flightButtons|=32768;if(collective&&h[0].x&&!weaponVisible&&!handOccupied[0])flightButtons|=64;if(h[1].b)flightButtons|=4096|64;if(h[0].b)flightButtons|=32;if(cyclic&&h[1].stick)flightButtons|=16384;if(collective&&h[0].stick)flightButtons|=8192;
   float lift=collective?(std::abs(h[0].sy)>.15f?h[0].sy:physicalLift):0;
   if(lift>.25f)flightButtons|=32768;else if(lift<-.25f)flightButtons|=64;
   float yaw=collective&&std::abs(h[0].sx)>.2f?h[0].sx:0;auto delta=call(getDelta);float dt=delta.type==1?std::clamp(float(delta.value.number),0.f,.05f):0;flightHeading=std::fmod(flightHeading+yaw*60*dt+360.f,360.f);unsigned bits;memcpy(&bits,&yaw,4);call(yawFlight,bits);
  }
  if(!testDir.empty()){static ULONGLONG last=0;if(now-last>100){last=now;auto px=callOn(getX,car),py=callOn(getY,car),pz=callOn(getZ,car),control=callOn(getVar,car,"Controlling"),land=callOn(getVar,car,"Landing");std::ofstream o(testDir+"\\flight-state.json");o<<"{\"active\":true,\"live\":"<<live<<",\"car\":"<<car->id<<",\"scale\":"<<scene::world(birdRoot).scale<<",\"x\":"<<px.value.number<<",\"y\":"<<py.value.number<<",\"z\":"<<pz.value.number<<",\"sx\":"<<flightX<<",\"sy\":"<<flightY<<",\"buttons\":"<<flightButtons<<",\"controlling\":"<<control.value.number<<",\"landing\":"<<land.value.number<<"}";}}
  return;
 }
 if(flying)clear();
 if(!drivingFeature){clear();return;}
 auto current=call(getCar);Form* target=current.type==2?current.value.form:nullptr;
 auto driving=target?callOn(getDriving,target):Element{};auto menu=call(getMenu);
 if(!testDir.empty()){static ULONGLONG lastProbe=0;if(GetTickCount64()-lastProbe>250){lastProbe=GetTickCount64();std::ofstream o(testDir+"\\vehicle-probe.json");o<<"{\"target\":"<<(target?target->id:0)<<",\"driving\":"<<driving.value.number<<",\"type\":"<<int(driving.type)<<"}";}}
 auto model=findCarModel(target);
 if(!target||!model||driving.type!=1||driving.value.number!=3){clear();return;}
 if(car!=target){clear();
  const auto attached=callOn(getAttachedScript,target);
  if(attached.type!=2||!attached.value.form)return;
  drivingScript=attached.value.form;
  car=target;
  chooseCarModel(model);
  const auto wheel=scene::find(scene::root(car),"FCSteeringWheel");
  char message[160];sprintf_s(message,"Driver entered %s; wheel grip %s",model->name,wheel?"available":"unavailable on this mesh");log(message);
  // Roadster's native boarding script disables combat (third argument).
  // Restore only combat for independent gun/item triggers; retain its normal
  // locomotion, POV, rollover and sneak locks and native seat attachment.
  call(allowDriveCombat);
 }
 active=true;prepareStockWheel(scene::root(car));ensureSeatedPose();vehicle_radio::tick();updateDrive();
 // Original engine assets, one state transition at a time. Also restores audio
 // after loading a save already seated (native Driving==1 never runs then).
 auto speed=callOn(getVar,car,"Momentum");int sound=(throttle>.15f||(speed.type==1&&speed.value.number>8))?2:1;
 if(sound!=engineMode){callOn(engineStop,car);callOn(sound==2?engineDrive:engineIdle,car);engineMode=sound;}
 if(getEnginePitch&&setEnginePitch){
  if(!enginePitchOwned){auto p=call(getEnginePitch);if(p.type==1&&std::isfinite(p.value.number)){savedEnginePitch=enginePitch=float(p.value.number);enginePitchOwned=true;}}
  if(enginePitchOwned){auto d=call(getDelta);float dt=d.type==1?std::clamp(float(d.value.number),0.f,.05f):0;float speedFraction=arcadeOwns?std::abs(driveState.speed)/std::max(1.f,driveConfig.topSpeed):float(speed.value.number)/120.f;
   float target=savedEnginePitch-8+42*std::clamp(speedFraction,0.f,1.f)+10*throttle;float next=arcade_drive::approach(enginePitch,target,dt*40);
   if(int(next)!=int(enginePitch)){unsigned bits;memcpy(&bits,&next,4);call(setEnginePitch,bits);}enginePitch=next;
  }
 }

}
// Read-only support casts at original wheel-bone centres. Actor collision and
// gravity remain native; this first slice uses support for traction, not a
// four-wheel rigid-body replacement. No collision bypass or SetPos motion.
void restoreDrive(){
 road_fit_native::clear(loaded?car:nullptr);
 vehicle_motion::clear();
 if(arcadeOwns&&car&&loaded){
  for(int i=0;i<4;i++)if(savedFrequency[i]>0){unsigned bits;memcpy(&bits,&savedFrequency[i],4);callOn(setFrequency,car,driveSequences[i],bits);}
  variable("CruiseControl",savedCruise);variable("MaxTurnSpeed",savedTurn);
 }
 arcadeOwns=false;driveState={};crest={};lastImpact={};wheelHits=0;
}
// Cars are creature actors. JIP applies horizontal knockback through their
// collision controller and lets it settle without switching to a ragdoll.
void transferImpact(Form* target,float closingLoss){
 if(!pushParkedCar || !findCarModel(target) || closingLoss<4.f)return;
 // JIP's initial push velocity scales with force squared.
 using Level=int(__cdecl*)();
 auto core=GetModuleHandleW(L"SeatedControls.dll");
 auto level=core?reinterpret_cast<Level>(GetProcAddress(core,"FC_CrashStrength")):nullptr;
 const float transfer[]={.42f,.80f,1.4f};
 const int setting=level?std::clamp(level(),0,2):0;
 const float force=std::sqrt(std::min(closingLoss,24.f)*transfer[setting]*9.99875f/.00744f);
 callOn(pushParkedCar,target,floatBits(force),car);
}
void updateDrive(){
 if(!arcadeEnabled||!active||flying||riding||!car)return;
 auto root=scene::root(car);if(!root)return;
 bool importedContacts=scene::find(root,"FCRoadContact0")!=nullptr;
 bool measuredTyres=scene::find(root,"FCRoadWheel0")!=nullptr;
 // The stock Roadster's four suspension pivots coincide with its tyre centres.
 // Other stock rigs have not established that mapping. Keep native handling.
 const bool stockRoadster=carModel&&!strcmp(carModel->form,"00E6BC")&&!importedContacts&&!measuredTyres;
 if((!importedContacts||!measuredTyres)&&!stockRoadster){if(arcadeOwns)restoreDrive();return;}
 const char* wheelNames[]={"Bip01 HandLeft","Bip01 HandRight","Bip01 FootLeft","Bip01 FootRight"};
 const char* tyreNames[]={"FCRoadWheel0","FCRoadWheel1","FCRoadWheel2","FCRoadWheel3"};
 const char* radiusNames[]={"FCRoadRadius0","FCRoadRadius1","FCRoadRadius2","FCRoadRadius3"};
 const char* contactNames[]={"FCRoadContact0","FCRoadContact1","FCRoadContact2","FCRoadContact3"};
 void* wheels[4]{};for(int i=0;i<4;i++){wheels[i]=scene::find(root,measuredTyres?tyreNames[i]:importedContacts?contactNames[i]:wheelNames[i]);if(!wheels[i])return;}
 auto delta=call(getDelta);float dt=delta.type==1?float(delta.value.number):0;
 lastDriveDelta=dt;
 auto menu=call(getMenu);bool live=menu.type==1&&menu.value.number==0;
 Hand h[2];{std::lock_guard<std::mutex> guard(lock);h[0]=hands[0];h[1]=hands[1];}
 auto now=GetTickCount64();for(auto& v:h)live=live&&v.valid&&now-v.time<200&&now-v.poseAt<200;
 wheelHits=0;float normalZ=0;scene::Vec roadNormal{},wheelPoints[4]{},roadPoints[4]{};
 for(int i=0;i<4;i++){
  auto p=scene::world(wheels[i]).p;float radius=importedContacts?0:(i<2?27.11194f:45.11193f)*scene::world(root).scale;
  if(measuredTyres){auto radial=scene::find(wheels[i],radiusNames[i]);if(!radial)return;radius=scene::distance(p,scene::world(radial).p);}
  wheelPoints[i]=p-scene::Vec{0,0,radius};scene::Vec end=p+scene::Vec{0,0,-radius-60},normal{};Form* hit{};
  bool found=road_contacts::ray(p+scene::Vec{0,0,8},end,normal,hit,car,player);
  if(!found)found=road_contacts::landscape(wheelPoints[i],60,end,normal);
  if(found&&normal.z>.35f){++wheelHits;normalZ+=normal.z;roadNormal=roadNormal+normal;roadPoints[i]=end;}
 }
 if(!arcadeOwns){
  if(stockRoadster&&wheelHits<2)return;
  vehicle_motion::beforeOwnedMotion=road_fit_native::restore;
  if(!vehicle_motion::install()){log("Native vehicle motion guard refused; original driving retained");arcadeEnabled=false;return;}
  for(int i=0;i<4;i++){auto e=callOn(readFrequency,car,driveSequences[i]);savedFrequency[i]=(e.type==1&&std::isfinite(e.value.number)&&e.value.number>0&&e.value.number<=10)?float(e.value.number):0;}
  auto e=callOn(getVar,car,"CruiseControl");savedCruise=e.type==1?float(e.value.number):0;
  e=callOn(getVar,car,"MaxTurnSpeed");savedTurn=e.type==1?float(e.value.number):.6f;
  driveConfig.wheelbase=std::max(1.f,scene::distance(scene::world(wheels[0]).p,scene::world(wheels[2]).p)/69.99125f);
  arcadeOwns=true;driveState={};log("Continuous car handling acquired; native collision retained");
 }
 auto freqBefore=callOn(readFrequency,car,driveSequences[2]);
 float traction=wheelHits>=2?std::clamp(normalZ/4.f,0.f,1.f):0;
 auto fuel=callOn(getVar,car,"Fuel");
 float pedal=(h[1].triggerClaim?h[1].trigger:0)-(h[0].triggerClaim?h[0].trigger:0);
 // Fuel loss removes engine thrust, not braking. Keep the opposite pedal
 // available while coasting downhill or carrying existing momentum.
 if((fuel.type!=1||fuel.value.number<=0)&&pedal*driveState.speed>=0)pedal=0;
 auto result=arcade_drive::step(driveState,driveConfig,pedal,steering,traction,dt,live);
 const auto forward=scene::rotate(scene::world(root),{0,1,0});
 // Require both measured loss of travel and a wall at the leading axle.
 // Braking, changing seats, cresting a ramp or going through a menu cannot
 // shatter a window from apparent deceleration alone.
 if(live&&dt>0&&dt<=.1f&&std::abs(driveState.speed)>6){
  const auto position=scene::field<scene::Vec>(car,0x30);
  if(!lastImpact.ready){lastImpact.position=position;lastImpact.ready=true;}
  lastImpact.elapsed+=dt;
  if(lastImpact.elapsed>=.1f){
   const bool reverse=driveState.speed<0;
   const auto direction=forward*(reverse?-1.f:1.f);
   const auto travel=position-lastImpact.position;
   const float actual=(travel.x*direction.x+travel.y*direction.y)/(69.99125f*lastImpact.elapsed);
   const float lost=std::abs(driveState.speed)-std::max(0.f,actual);
   const float impactLoss=lastImpact.previousTravelSpeed-std::max(0.f,actual);
   lastImpact.previousTravelSpeed=std::max(0.f,actual);
   // Native actor capsules can hit before either rendered bumper reaches a wall ray.
   // Ask the original controller which vehicle it touched during the measured stop.
   bool vehicleHit=false;
   if(impactLoss>=8 && vehicleContacts && rideArrays){
    auto contacts=callOn(vehicleContacts,car);
    if(contacts.type==4 && contacts.value.array){
     const auto count=rideArrays->size(contacts.value.array);
     Element refs[64]{};
     if(count && count<=64 && rideArrays->elements(contacts.value.array,refs,nullptr)){
      for(unsigned j=0;j<count;++j){
       if(refs[j].type!=2)continue;
       auto other=refs[j].value.form;
       if(other==car || !vehicle_glazing::supported(other))continue;
       const auto offset=scene::field<scene::Vec>(other,0x30)-position;
       const auto outward=unit({offset.x,offset.y,0});
       const float closing=dot(outward,direction);
       if(closing<.4f)continue;
       const auto point=position+offset*.65f+scene::Vec{0,0,35};
       vehicle_glazing::crash(car,point,outward,impactLoss*closing);
       vehicle_glazing::crash(other,point,outward*(-1.f),impactLoss*closing);
       transferImpact(other,impactLoss*closing);
       driveState.speed=std::copysign(std::max(0.f,actual),driveState.speed);
       vehicleHit=true;
       break;
      }
     }
    }
   }
   if(lost>=5 && !vehicleHit){
    for(int i=reverse?2:0,last=i+2;i<last;++i){
     auto start=wheelPoints[i]+scene::Vec{0,0,30};auto end=start+direction*55;
     scene::Vec normal{};Form* object{};
     if(!road_contacts::ray(start,end,normal,object,car,player)||std::abs(normal.z)>.55f)continue;
     const float closing=-(normal.x*direction.x+normal.y*direction.y);
     if(closing<.4f)continue;
     vehicle_glazing::crash(car,end,direction,impactLoss*closing);
     if(object && vehicle_glazing::supported(object)){
      vehicle_glazing::crash(object,end,direction*(-1.f),impactLoss*closing);
      transferImpact(object,impactLoss*closing);
     }
     driveState.speed=std::copysign(std::max(0.f,std::abs(driveState.speed)-lost*closing),driveState.speed);
     break;
    }
   }
   lastImpact.position=position;lastImpact.elapsed=0;
  }
 }else lastImpact={};
 const float grade=normalZ>0?-(roadNormal.x*forward.x+roadNormal.y*forward.y)/normalZ:0;
 const float launch=crest.sample(dt,driveState.speed,wheelHits,grade,live);
 if(launch>0&&setVerticalVelocity){
  // JIP takes Havok units/s: 69.99125 game units/m, seven game units/Havok unit.
  const float velocity=launch*69.99125f/7.f;
  unsigned raw;memcpy(&raw,&velocity,sizeof(raw));callOn(setVerticalVelocity,car,raw);
 }
 vehicle_motion::command(car,driveState.speed);
 variable("CruiseControl",1);variable("Reverse",result.reverse?1:0);
 float momentum=std::abs(driveState.speed)<.03f?0:result.reverse?25.f:120.f;
 variable("Momentum",momentum);variable("Throttle",momentum);variable("BrakeForce",0);
 // Original script multiplies StickX * TurnSpeed once per update, and flips
 // steering in reverse. Normalize the native turn rate by measured delta time.
 float divisor=result.reverse?.33f:1.f;
 float perUpdate=std::abs(steering)>.1f?std::abs(result.yawRate/steering)*std::clamp(dt,0.f,.05f)/divisor:0;
 variable("MaxTurnSpeed",perUpdate);
 // Wheel animation may follow speed, but desired travel is supplied separately
 // at native movement extraction, before collision. Clip wrap cannot add travel.
 float frequency=result.frequency;
 unsigned bits;memcpy(&bits,&frequency,4);if(savedFrequency[result.reverse?3:2]>0)callOn(setFrequency,car,driveSequences[result.reverse?3:2],bits);
 if(!testDir.empty()){
  auto freqAfter=callOn(readFrequency,car,driveSequences[2]);std::ofstream out(testDir+"\\arcade-drive.json");out<<"{\"active\":true,\"pid\":"<<GetCurrentProcessId()<<",\"car\":"<<car->id<<",\"fuel\":"<<(fuel.type==1?fuel.value.number:-1)<<",\"live\":"<<live<<",\"pedal\":"<<pedal<<",\"wheelHits\":"<<wheelHits<<",\"traction\":"<<traction<<",\"speedMps\":"<<driveState.speed<<",\"yawRate\":"<<result.yawRate<<",\"frequency\":"<<result.frequency<<",\"frequencyBefore\":"<<freqBefore.value.number<<",\"frequencyAfter\":"<<freqAfter.value.number<<",\"rootZ\":"<<scene::world(root).p.z<<",\"wheelsZ\":[";for(int i=0;i<4;i++){if(i)out<<",";out<<wheelPoints[i].z;}out<<"],\"roadZ\":[";for(int i=0;i<4;i++){if(i)out<<",";out<<roadPoints[i].z;}out<<"]}";
 }
}
void updateSteering(){
 if(!active||flying||!car||!player)return;
 auto menu=call(getMenu);nativeMenu=menu.type!=1||menu.value.number!=0;
 auto root=scene::root(car);auto rig=scene::root(player);if(!root||!rig){suspendMounted();return;}
 auto wheel=scene::find(root,"FCSteeringWheel");
 auto carrier=mountedCarrier(root);if(!carrier){suspendMounted();return;}
 active=true;auto t=scene::world(carrier);
 // Measured from the dependency's authored wheel mesh (game units).
 center=wheel?scene::world(wheel).p:scene::world(carrier).p;
 right=unit(scene::rotate(t,{1,0,0}));up=unit(scene::rotate(t,{0,.34394f,.93899f}));axis=unit(scene::cross(right,up));
 float rimRadius=10*t.scale;
 if(wheel&&wheel==stockWheel.pivot){
  t=scene::world(stockWheel.basis);
  right=unit(scene::rotate(t,{1,0,0}));up=unit(scene::rotate(t,stock_roadster_wheel::up));axis=unit(scene::cross(right,up));
  rimRadius=stock_roadster_wheel::radius*t.scale;
 }
 if(wheel){auto r=scene::find(carrier,"FCWheelRight"),u=scene::find(carrier,"FCWheelUp");if(r&&u){auto rv=scene::world(r).p-center;rimRadius=scene::distance(rv,{});right=unit(rv);up=unit(scene::world(u).p-center);axis=unit(scene::cross(right,up));}}
 Hand h[2];{std::lock_guard<std::mutex> g(lock);h[0]=hands[0];h[1]=hands[1];}
 auto now=GetTickCount64();float sum=0;int count=0;bool fresh[2]{};
 auto manager=mounted_frame::read().manager;
 for(int i=0;i<2;i++)fresh[i]=h[i].valid&&now-h[i].poseAt<200&&now-h[i].time<200;
 if(wheel){auto wt=scene::world(wheel);
 for(int i=0;i<2;i++){
  fresh[i]=h[i].valid&&now-h[i].poseAt<200&&now-h[i].time<200;
  scene::Transform raw{};bool valid=vehicle_grip::raw(manager,i,raw);
  auto d=raw.p-center;float x=dot(d,right),y=dot(d,up),z=dot(d,axis),rad=std::sqrt(x*x+y*y);
  float angle=std::atan2(y,x);bool nearRim=valid&&std::abs(z)<12*t.scale&&std::abs(rad-rimRadius)<10*t.scale;
  controlNear[i]=nearRim&&!handOccupied[i]&&!nativeMenu;
  if(!fresh[i]||!valid||handOccupied[i]||h[i].grip<.35f||nativeMenu){grabbed[i]=false;continue;}
  // Reaching and pressing can span different controller/rig updates.
  // An old held squeeze cannot pick up the wheel later.
  const bool recent=h[i].rimPressAt&&now>=h[i].rimPressAt&&now-h[i].rimPressAt<=220;
  if(!grabbed[i]&&nearRim&&(h[i].gripClaim||recent)&&h[i].grip>.65f){
   grabbed[i]=true;grabAngle[i]=angle;grabWheel[i]=wheelAngle;gripLocal[i]=vehicle_grip::relative(wt,raw);
   {std::lock_guard<std::mutex> guard(lock);hands[i].rimPressAt=0;hands[i].gripClaim=false;}
   char message[120];sprintf_s(message,"Driver wheel grip hand=%d buffered=%d",i,int(!h[i].gripClaim));log(message);
  }
  if(grabbed[i]){
   auto target=vehicle_grip::world(wt,gripLocal[i]);
   // A deliberate reach-away or tracking jump releases; ordinary rim motion
   // keeps the original attachment rather than repeatedly snapping/reacquiring.
   if(scene::distance(raw.p,target.p)>38*t.scale){grabbed[i]=false;continue;}
   sum+=grabWheel[i]+wrap(angle-grabAngle[i]);count++;
  }
 }
 if(count&&(!fresh[0]||std::abs(h[0].sx)<=.15f))wheelAngle=std::clamp(sum/count,-1.5707963f,1.5707963f);
 else {float sx=fresh[0]?h[0].sx:0;steering=std::abs(sx)>.15f?std::clamp(sx,-1.f,1.f):0;wheelAngle=-steering*1.5707963f;}
 // Rotate the locally imported wheel about its measured steering-column axis.
 auto& local=scene::local(wheel);scene::Vec n=unit(scene::inverseRotate(t,axis));float co=std::cos(wheelAngle),si=std::sin(wheelAngle),v=1-co;
 float q[3]={n.x,n.y,n.z};for(int row=0;row<3;row++)for(int col=0;col<3;col++)local.r[row][col]=(row==col?co:0)+q[row]*q[col]*v;
 local.r[0][1]-=n.z*si;local.r[0][2]+=n.y*si;local.r[1][0]+=n.z*si;local.r[1][2]-=n.x*si;local.r[2][0]-=n.y*si;local.r[2][1]+=n.x*si;scene::update(wheel);
 steering=std::clamp(-wheelAngle/1.5707963f,-1.f,1.f);
 wt=scene::world(wheel);for(int i=0;i<2;i++){controlHeld[i]=grabbed[i];vehicle_grip::targets[i]={grabbed[i],manager,vehicle_grip::world(wt,gripLocal[i])};}
 }else{
  steering=fresh[0]?arcade_drive::axis(h[0].sx):0;wheelAngle=0;
  for(int i=0;i<2;i++){controlNear[i]=controlHeld[i]=grabbed[i]=false;vehicle_grip::targets[i]={};}
 }
 bool live=menu.type==1&&!menu.value.number;
 auto gear=callOn(getVar,car,"Reverse"),momentum=callOn(getVar,car,"Momentum");
 float demand=(h[1].triggerClaim?h[1].trigger:0)-(h[0].triggerClaim?h[0].trigger:0);
 auto pedals=vehicle_drive::pedals(demand,live&&fresh[0]&&fresh[1]&&gear.type==1&&momentum.type==1,gear.value.number!=0,float(momentum.value.number));
 throttle=pedals.gas;brake=pedals.brake;
 kneeDriving=live&&momentum.type==1&&momentum.value.number>.5&&!grabbed[0]&&!grabbed[1];
 if(!arcadeOwns&&pedals.changeGear)variable("Reverse",pedals.reverse?1.f:0.f);
 // The desktop script leaves the opposite pedal latched when one is active.
 // Clear that obsolete value while keeping all acceleration/physics native.
 if(!arcadeOwns){if(!throttle)variable("Throttle",0);if(!brake)variable("BrakeForce",0);}
 if(live&&fresh[1]&&h[1].b&&!exitWas)variable("Dismount",1);exitWas=h[1].b;
 if(!testDir.empty()){
  static ULONGLONG last=0;if(now-last>100){last=now;auto c=vehicle_adapter::tracking(center),r=vehicle_adapter::tracking(center+right*(10*t.scale)),u=vehicle_adapter::tracking(center+up*(10*t.scale));
   auto pa=vehicle_adapter::tracking(scene::palm(rig,0)),pb=vehicle_adapter::tracking(scene::palm(rig,1));
   scene::Vec shoulder[2]{};float armLength[2]{};
   for(int h=0;h<2;h++){auto a=scene::find(rig,h?"Bip01 R UpperArm":"Bip01 L UpperArm"),e=scene::find(rig,h?"Bip01 R Forearm":"Bip01 L Forearm"),w=scene::find(rig,h?"Bip01 R Hand":"Bip01 L Hand");if(a&&e&&w){shoulder[h]=vehicle_adapter::tracking(scene::world(a).p);armLength[h]=(scene::distance(scene::world(a).p,scene::world(e).p)+scene::distance(scene::world(e).p,scene::world(w).p))/vehicle_adapter::unitsPerMetre();}}
   auto px=callOn(getX,car),py=callOn(getY,car);std::ofstream o(testDir+"\\vehicle-state.json");o<<"{\"shoulders\":[["<<shoulder[0].x<<","<<shoulder[0].y<<","<<shoulder[0].z<<"],["<<shoulder[1].x<<","<<shoulder[1].y<<","<<shoulder[1].z<<"]],\"armLength\":["<<armLength[0]<<","<<armLength[1]<<"],\"active\":true,\"arcadeHandling\":"<<arcadeOwns<<",\"car\":"<<car->id<<",\"angle\":"<<wheelAngle<<",\"steer\":"<<steering<<",\"gas\":"<<throttle<<",\"actorState\":"<<vehicle_adapter::actorState(player)<<",\"controlFlags\":"<<unsigned(scene::field<unsigned char>(player,0x680))<<",\"idleOwned\":"<<seated_animation::ownsActive(player)<<",\"knees\":"<<knee_steering::weight<<",\"momentum\":"<<momentum.value.number<<",\"reverse\":"<<pedals.reverse<<",\"brake\":"<<brake<<",\"grabbed\":["<<grabbed[0]<<","<<grabbed[1]<<"],\"center\":["<<c.x<<","<<c.y<<","<<c.z<<"],\"right\":["<<r.x<<","<<r.y<<","<<r.z<<"],\"up\":["<<u.x<<","<<u.y<<","<<u.z<<"],\"palms\":[["<<pa.x<<","<<pa.y<<","<<pa.z<<"],["<<pb.x<<","<<pb.y<<","<<pb.z<<"]],\"position\":["<<px.value.number<<","<<py.value.number<<"]}";
  }
 }
}
void updatePhysicalControls(void* manager){
 vehicle_grip::owner=GetCurrentThreadId();auto b=build_adapter::heldModule();if(!b||!manager){resetGrips();return;}
 for(int h=0;h<2;h++)handOccupied[h]=vehicle_adapter::occupied(h,weaponVisible);
 vehicle_radio::contacts(manager,car?mountedCarrier(scene::root(car)):nullptr);
 if(riding){rideContacts(manager);return;}
 if(!flying){updateSteering();return;}
 auto carrier=scene::find(scene::root(car),"FCCockpitCarrier"),stick=scene::find(carrier,"FCFlightControl");
 if(!stick){resetGrips();return;}
 auto collective=scene::find(carrier,"FCFlightCollective");
 auto c=scene::world(carrier);Hand h[2];{std::lock_guard<std::mutex> g(lock);h[0]=hands[0];h[1]=hands[1];}
 auto now=GetTickCount64();flight_gimbal::Axis input{};int count=0;physicalLift=0;
 scene::Transform frames[2]{};void* controls[2]={collective?collective:stick,stick};
 for(int i=0;i<2;i++){
  auto control=controls[i];auto t=scene::world(control);auto local=scene::local(control);
  for(int r=0;r<3;r++)for(int k=0;k<3;k++)local.r[r][k]=r==k?1.f:0.f;
  auto neutral=vehicle_grip::world(c,local);scene::Transform raw{};
  auto socket=scene::find(control,i?"FCFlightGripR":"FCFlightGripL");
  bool valid=socket&&vehicle_grip::raw(manager,i,raw)&&h[i].valid&&now-h[i].poseAt<200&&now-h[i].time<200;
  bool nearControl=valid&&scene::distance(raw.p,scene::world(socket).p)<10*c.scale;
  controlNear[i]=nearControl&&!nativeMenu&&!handOccupied[i];
  if(!valid||h[i].grip<.35f||nativeMenu||handOccupied[i])grabbed[i]=false;
  else if(!grabbed[i]&&nearControl&&h[i].gripClaim&&h[i].grip>.65f){grabbed[i]=true;gripLocal[i]=vehicle_grip::relative(t,raw);flightGrabAt[i]=now;}
  if(grabbed[i]){
   auto target=vehicle_grip::world(t,gripLocal[i]);
   if(scene::distance(raw.p,target.p)>25*c.scale)grabbed[i]=false;
   else {
    auto lever=scene::inverseRotate(neutral,raw.p-neutral.p)*(1/c.scale);
    if(collective&&i==0){
     // Collective is a separate fore/aft hinge, never averaged with cyclic.
     float neutralAngle=std::atan2(gripLocal[i].p.z,gripLocal[i].p.y);
     physicalLift=flight_gimbal::response((std::atan2(lever.z,lever.y)-neutralAngle)/.34906585f,0).x;
    }else{
     lever.x-=gripLocal[i].p.x;lever.y-=gripLocal[i].p.y;
     auto value=scene::find(stick,"FCGimbalV2")?flight_gimbal::fromLever(lever.x,lever.y,lever.z):flight_gimbal::response(lever.x/10,lever.y/10);
     input.x+=value.x;input.y+=value.y;++count;
    }
   }
  }
 }
 stickX=count?input.x/count:0;stickY=count?input.y/count:0;
 auto angles=flight_gimbal::angles({stickX,stickY});
 float cp=std::cos(angles.x),sp=std::sin(angles.x),cr=std::cos(angles.y),sr=std::sin(angles.y);
 mounted_frame::Mat rx={{{1,0,0},{0,cp,-sp},{0,sp,cp}}},ry={{{cr,0,sr},{0,1,0},{-sr,0,cr}}};auto rot=mounted_frame::mul(rx,ry);
 memcpy(scene::local(stick).r,rot.a,sizeof rot.a);scene::update(stick);
 if(collective){
  float angle=physicalLift*.34906585f,co=std::cos(angle),si=std::sin(angle);
  mounted_frame::Mat r={{{1,0,0},{0,co,-si},{0,si,co}}};memcpy(scene::local(collective).r,r.a,sizeof r.a);scene::update(collective);
 }
 for(int i=0;i<2;i++){
  frames[i]=scene::world(controls[i]);controlHeld[i]=grabbed[i];auto contact=gripLocal[i];
  if(auto paddle=scene::find(controls[i],i?"FCFlightTriggerR":"FCFlightTriggerL")){scene::local(paddle).p.y=grabbed[i]?-.4f*std::clamp(h[i].trigger,0.f,1.f):0;scene::update(paddle);}
  // Capture input relative to where the real grip began, but settle the drawn
  // palm onto the handle. Retaining the acquisition offset for rendering left
  // hands visibly floating beside a control whenever the grab was off-centre.
  auto socket=scene::find(controls[i],i?"FCFlightGripR":"FCFlightGripL");
  if(grabbed[i]&&socket){float blend=std::clamp(float(now-flightGrabAt[i])/100.f,0.f,1.f);blend=blend*blend*(3-2*blend);contact.p=contact.p*(1-blend)+scene::local(socket).p*blend;}
  vehicle_grip::targets[i]={grabbed[i],manager,vehicle_grip::world(frames[i],contact)};
 }
 if(!testDir.empty()){static ULONGLONG last=0;if(now-last>100){last=now;std::ofstream o(testDir+"\\flight-control.json");o<<"{\"gripped\":["<<grabbed[0]<<","<<grabbed[1]<<"],\"sockets\":[";for(int i=0;i<2;i++){if(i)o<<",";auto n=scene::find(controls[i],i?"FCFlightGripR":"FCFlightGripL");auto p=n?vehicle_adapter::tracking(scene::world(n).p):scene::Vec{};o<<"["<<p.x<<","<<p.y<<","<<p.z<<"]";}o<<"],\"x\":"<<stickX<<",\"y\":"<<stickY<<",\"lift\":"<<physicalLift<<",\"separateCollective\":"<<(collective!=nullptr)<<"}";}}

}
void syncVehicleFeatures(){
 const unsigned modes=installedModes&requestedModes.load();
 const unsigned previous=controlsAvailable.exchange(modes);
 if(modes==previous)return;
 if(car&&!(modes&mountedMode())){
  if(loaded&&vehicle_radio::host==car&&vehicle_radio::on)vehicle_radio::tune(nullptr,false,false);
  clear();
 }
 drivingFeature=(modes&DrivingMode)!=0;
 flightFeature=(modes&FlightMode)!=0;
 rideEnabled=(modes&(BighornerMode|DeathclawMode))!=0;
 if(!rideEnabled){rideMountReady=false;rideHold=0;rideCandidateID=0;}
 if((previous&DrivingMode)&&!drivingFeature){restoreParked();parkedFits.clear();}
 if(!active){
  std::lock_guard<std::mutex> guard(lock);
  for(auto& hand:hands)hand={};
  memset(inputDrain,0,sizeof(inputDrain));
  memset(menuHeld,0,sizeof(menuHeld));
 }
}
void __cdecl message(Message* m){
 if(m->type==DeferredInit){
  drivingFeature=fc_feature_client::enabled("Driving");
  getPlayer=compile("player",true);getMenu=compile("MenuMode",true);
  if(!drivingFeature){log("Vehicle features off; leaving native controls in place");return;}
  vehicle_glazing::candidate=vehicle_glazing::candidate&&(drivingFeature||flightFeature);
  getCar=compile("GetRefVariable \"CurrentCarRef\" (GetFormFromMod \"UmDrivingVIIIb.esp\" \"000ADE\")",true);
  getWeaponOut=compile("player.IsWeaponOut",true);
  if(!vehicle_radio::init())log("Vehicle radio scripts unavailable; native radio retained");
  getAttachedScript=compile("GetScript",true);
  getDriving=compile("GetVariable \"Driving\"",true);
  insertStockWheel=compile("ref vehicle\nbegin function {vehicle}\nInsertNode vehicle 3 \"Bip01 Spine|FCStockWheelBasis\"\nInsertNode vehicle 3 \"FCStockWheelBasis|FCSteeringWheel\"\nInsertNode vehicle 3 \"FCSteeringWheel|FCStockWheelSkin\"\nend");
  removeStockWheel=compile("ref vehicle\nbegin function {vehicle}\nInsertNode vehicle 2 \"FCSteeringWheel|FCStockWheelSkin\"\nInsertNode vehicle 2 \"FCStockWheelBasis|FCSteeringWheel\"\nInsertNode vehicle 2 \"Bip01 Spine|FCStockWheelBasis\"\nend");
  getVar=fc::nvse::loadFunction(*scripts,"Vehicles/ReadVariable");
  vehicleContacts=compile("GetContactRefs",true);
  pushParkedCar=compile("float force\nref source\nbegin function {force,source}\nPushActorNoRagdoll force 0 source 0\nend");
  setVerticalVelocity=compile("float velocity\nbegin function {velocity}\nSetActorVelocity Z velocity\nend");
  setVar=fc::nvse::loadFunction(*scripts,"Vehicles/WriteVariable");
  auto base=call(compile("GetFormFromMod \"UmDrivingVIIIb.esp\" \"00E6BC\"",true));supported=base.type==2?base.value.form:nullptr;
  for(unsigned i=0;i<std::size(fleetBases);i++){auto text=std::string("GetFormFromMod \"UmDrivingVIIIb.esp\" \"")+vehicle_fleet::models[i].form+"\"";auto value=call(compile(text.c_str(),true));fleetBases[i]=value.type==2?value.value.form:nullptr;}
  readFrequency=fc::nvse::loadFunction(*scripts,"Vehicles/ReadFrequency");
  setFrequency=fc::nvse::loadFunction(*scripts,"Vehicles/SetFrequency");
  engineIdle=fc::nvse::loadFunction(*scripts,"Vehicles/StartEngineIdle");
  engineDrive=fc::nvse::loadFunction(*scripts,"Vehicles/StartEngineDrive");
  engineStop=fc::nvse::loadFunction(*scripts,"Vehicles/StopEngine");
  getEnginePitch=compile("GetSoundTraitNumeric UmEngineDriveB 2",true);
  setEnginePitch=fc::nvse::loadFunction(*scripts,"Vehicles/SetEnginePitch");
  getAngleZ=compile("GetAngle Z",true);getDelta=compile("GetSecondsPassed",true);
  getX=compile("GetPos X",true);getY=compile("GetPos Y",true);getZ=compile("GetPos Z",true);
  refreshSeat=fc::nvse::loadFunction(*scripts,"Vehicles/RefreshSeat");
  allowDriveCombat=fc::nvse::loadFunction(*scripts,"Vehicles/AllowCombat");
  getPlayedIdle=compile("player.GetPlayedIdle",true);
  sitPose=fc::nvse::loadFunction(*scripts,"Vehicles/StartSeatIdle");
  stopPose=fc::nvse::loadFunction(*scripts,"Vehicles/StopSeatIdle");
  if(!vehicle_glazing::init())log("Vehicle glazing events unavailable; glass remains unchanged");
  parkedActors=compile("GetActorsByProcessingLevel 0",true);
  auto check=[](bool ok,const char* stage){if(!ok){char text[160];sprintf_s(text,"Vehicle initialization failed: %s",stage);log(text);}return ok;};
  const bool cars=drivingFeature&&supported;
  const bool aircraft=flightFeature&&flightBase;
  const bool commonScripts=getPlayer&&getMenu&&getVar&&setVar&&getAngleZ&&getDelta&&sitPose&&stopPose;
  const bool carScripts=getAttachedScript&&getCar&&getDriving&&engineIdle&&engineDrive&&engineStop&&allowDriveCombat;
  const bool flightScripts=flightScript&&getFlight&&getPiloting&&flightControlLocks&&allowDriveCombat&&refreshSeat&&showPilot;
  ready=check(cars||aircraft||rideEnabled,"dependencies")&&check(commonScripts,"seat scripts")&&
        check(!cars||carScripts,"driving scripts")&&check(!aircraft||flightScripts,"flight scripts")&&
        check(vehicle_adapter::heldPolicyVerified(),"held policy")&&check(vehicle_contact::verified(),"contact contract")&&
        check(vehicle_grip::install(),"grip hook")&&check(mounted_frame::install(),"coordinate conversions")&&
        check(vehicle_publication::install(beginPublication,endPublication,anchorPublication,finalizePublication,observeBody,beforeContacts),"shared pose publication")&&
        check(installVehicleCommands(cars,aircraft),"native input commands");
  installedModes=ready?((cars?1u:0u)|(aircraft?2u:0u)|(rideEnabled&&bighornerEnabled?4u:0u)|(rideEnabled&&deathclawEnabled?8u:0u)):0u;
  controlsAvailable=installedModes&requestedModes.load();
  log(ready?"Vehicle bridge ready":"Vehicle bridge failed");
 }
 if(m->type==PostLoadGame||m->type==NewGame){clear();bighorner_tack::nextStock=0;rideRestorePending=false;loaded=m->type==NewGame||m->data!=nullptr;auto p=call(getPlayer);player=p.type==2?p.value.form:nullptr;vehicle_glazing::subscribeProjectiles();}
 if(m->type==PreLoadGame||m->type==ExitMenu||m->type==ExitGame){restoreParked();parkedFits.clear();parkedScan=0;rideRestorePending=false;rideHostileRef=0;if(riding)stopRide(false);seatedPose=false;clear();loaded=false;player=nullptr;}
 if(m->type==PreLoadGame||m->type==ExitMenu||m->type==ExitGame)vehicle_glazing::clear();
 if(m->type==MainLoop&&ready)syncVehicleFeatures();
 if(m->type==MainLoop&&loaded){vehicle_glazing::tick();}
 if(m->type==MainLoop&&controlsAvailable&&loaded){tick();if(!testDir.empty()){static ULONGLONG last=0;auto now=GetTickCount64();if(now-last>3000){last=now;vehicle_perf::write(testDir);}}}
}
}
extern "C" __declspec(dllexport) void __cdecl FC_VehicleFeaturesChanged(bool driving,bool,bool){
 requestedModes=driving?DrivingMode:0u;
}
extern "C" __declspec(dllexport) bool __cdecl FC_VehicleActive(){return active&&(requestedModes.load()&mountedMode());}
extern "C" __declspec(dllexport) unsigned __cdecl FC_VehicleInput(const char* name,int hand,int kind,float x,float y){
 const unsigned modes=controlsAvailable.load()&requestedModes.load();
 if(!modes||(active&&!(modes&mountedMode()))||!name||hand<0||hand>1)return 0;bool valid=(kind&8)!=0;std::lock_guard<std::mutex> g(lock);auto& h=hands[hand];
 if(!strcmp(name,"vehicle_pose")){h.valid=valid;h.poseAt=GetTickCount64();if(!valid){h.gripClaim=false;h.triggerClaim=false;h.rimPressAt=0;}return 0;}
 int control=!strcmp(name,"squeeze")?0:!strcmp(name,"trigger")?1:!strcmp(name,"primary_click")?2:!strcmp(name,"secondary_click")?3:!strcmp(name,"thumbstick")?4:!strcmp(name,"thumbstick_click")?5:-1;
 if(control<0)return 0;
 bool pressed=valid&&(std::abs(x)>.15f||std::abs(y)>.15f),mounted=active&&!nativeMenu;
 if(nativeMenu&&pressed)menuHeld[hand][control]=true;
 if(!pressed)menuHeld[hand][control]=false;
 bool logicalValid=valid&&!(active&&menuHeld[hand][control]);
 bool radioOwns=control==2&&vehicle_radio::input(hand,logicalValid&&x>.5f,mounted&&!riding&&!handOccupied[hand]);
 if(control==0){
  // A squeeze belongs to the interaction where it STARTED. Carrying a native
  // magazine past the wheel/flight handle must not turn that held squeeze into
  // a vehicle grab (and send a false release to TTR).
  const auto now=GetTickCount64();
  if(!mounted||!logicalValid||x<.35f||handOccupied[hand]){h.gripClaim=false;h.rimPressAt=0;}
  else if(h.grip<=.65f&&x>.65f){
   h.gripClaim=controlNear[hand];
   h.rimPressAt=!flying&&!riding?now:0;
  }
  h.grip=logicalValid?x:0;h.time=now;
 }if(control==1){
  // A held weapon trigger cannot become an accelerator halfway through the
  // squeeze, including drawing/holstering while the trigger is still down.
  if(!mounted||!logicalValid||x<=.15f||weaponVisible||handOccupied[hand]||(flying&&fittedFlightControls&&!controlHeld[hand]))h.triggerClaim=false;
  else if(h.trigger<=.15f&&x>.15f)h.triggerClaim=true;
  h.trigger=logicalValid?x:0;
 }if(control==2)h.x=logicalValid&&x>.5f&&!radioOwns;if(control==3)h.b=logicalValid&&x>.5f;if(control==4){h.sx=logicalValid?x:0;h.sy=logicalValid?y:0;}if(control==5)h.stick=logicalValid&&x>.5f;
 bool vehicleOwns=mounted;
 if(control==0)vehicleOwns=(mounted&&(controlHeld[hand]||h.gripClaim))||(!active&&rideMountReady);
 if(control==1)vehicleOwns=mounted&&!riding&&h.triggerClaim;
 if(control==2)vehicleOwns=mounted&&flying&&(!fittedFlightControls||controlHeld[hand])&&!weaponVisible&&!handOccupied[hand];
 if(control==3&&hand==0&&!flying)vehicleOwns=false; // deliberate native Y pause
 if(riding&&(control==1||control==2||control==5||(control==3&&hand==0)))vehicleOwns=false;
 vehicleOwns=vehicleOwns||radioOwns;
 if(vehicleOwns&&pressed&&!menuHeld[hand][control])inputDrain[hand][control]=true;
 bool consume=vehicleOwns||inputDrain[hand][control];if(!pressed)inputDrain[hand][control]=false;return consume?1:0;
}
extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Query(const NVSE* n,PluginInfo* i){i->infoVersion=1;i->name="FilthyCasualVehicles";i->version=1;return !n->isEditor&&n->runtime==0x040020D0;}
extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Load(const NVSE* n){
 vehicle_glazing::events=static_cast<vehicle_glazing::Events*>(n->queryInterface(8));
 file=_fsopen((std::string(n->runtimeDirectory())+"FilthyCasualVehicles.log").c_str(),"w",_SH_DENYNO);
 rideArrays=static_cast<Arrays*>(n->queryInterface(5));scripts=static_cast<Scripts*>(n->queryInterface(6));commands=static_cast<Table*>(n->queryInterface(3));auto msgs=static_cast<Messaging*>(n->queryInterface(2));
 if(!scripts||!commands||!msgs||!build_adapter::verifiedTTWVR(GetModuleHandleW(L"TTWVR.dll")))return false;
 wchar_t path[MAX_PATH]{};GetModuleFileNameW(GetModuleHandleW(L"FilthyCasualVehicles.dll"),path,MAX_PATH);auto slash=wcsrchr(path,L'\\');if(!slash)return false;*slash=0;
 arcadeEnabled=GetPrivateProfileIntW(L"Driving",L"ExperimentalArcadeHandling",0,(std::wstring(path)+L"\\FilthyCasualVehicles.ini").c_str())!=0;
 vehicle_glazing::candidate=GetPrivateProfileIntW(L"Glass",L"Experimental",0,(std::wstring(path)+L"\\FilthyCasualVehicles.ini").c_str())!=0;
 // Suspension presentation is withheld after final-eye cabin flicker. Native
 // ground/collision remains active; arcade acceleration is independent.
 roadFitEnabled=false;
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
 {"vehicles.driving","DRIVING","Right trigger accelerates; left trigger brakes/reverses. Left stick or rim grips steer. Release coasts. Drawing a weapon frees both triggers; release before accelerating again.",
 {"Brake / reverse or held-item trigger","Left wheel grip / native grab","Native X","Pause",nullptr,"Accelerate or held-item trigger","Right wheel grip / native grab","Native A","Exit vehicle",nullptr,"Filthy Casuals shortcuts","Steer left","Steer right",nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,"Left thumb-rest touch","Right thumb-rest touch","Weapon wheel","Consumable wheel","Reload mode","Height / settings","Weapon tutorial"}},
 };
 static const fc_provider::Provider p={sizeof(fc_provider::Provider),1,1,modes,
 +[]()->uint32_t{return controlsAvailable.load();},+[]()->int{return active?0:-1;}};
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

// A temporarily missing scene rig suspends presentation. The native mount
// still owns the seat until dismount or load clears its reference.
extern "C" __declspec(dllexport) int __cdecl FC_SeatContext(){return !loaded||!car?0:riding?(ridingDeathclaw?4:3):flying?2:1;}

extern "C" __declspec(dllexport) bool __cdecl FC_NativeTrackingHook(const unsigned char* entry){return mounted_frame::ownsTrackingHook(entry);}




extern "C" __declspec(dllexport) unsigned __cdecl FC_FeatureGateVersion(){return 1;}
