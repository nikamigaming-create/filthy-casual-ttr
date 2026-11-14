#pragma once
#include "ride_challenge.hpp"
// Optional Bighorner ride. Uses retail skeleton/animation and the existing
// mounted publication transaction; no redistributed meshes or camera-only bob.
creature_ride::Challenge rideChallenge;std::atomic<unsigned> ridePulse{0};bool rideOwnedRestraint=false;
bool rideCandidateDeathclaw=false;
bool rideRestorePending=false;unsigned rideHostileRef=0;ULONGLONG rideHostileAt=0,rideHostileUntil=0;
Script *rideLeaseStore{},*rideLeaseRef{},*rideLeaseFlags{},*rideLeaseVersion{},*rideLeaseErase{},*rideResolve{};
Script *rideSneak{},*rideDetected{},*rideCombat{},*rideHostile{};
Script *rideRestrict{},*rideUnrestrict{},*rideNotice{};
Script *rideActors{},*rideModel{},*rideDead{},*rideInsert{},*rideRemove{},*rideMount{},*rideRefresh{},*rideExit{},*rideMove{},*rideBuck{},*rideWalk{},*rideRun{},*rideIdle{};
unsigned rideCandidateID=0,rideCarrierID=0;ULONGLONG rideScan=0,rideHold=0,rideStarted=0,rideLast=0,rideAnimAt=0;
scene::Vec rideHome{};unsigned char rideMoveFlag=0;float rideHeading=0;bool rideBuckPhase=false;float rideSpeed=0;int rideGait=-1;
void initRide(){

 rideLeaseStore=compile("ref animal\nint flags\nbegin function {animal,flags}\nplayer.AuxVarSetRef \"_FC_CreatureRide\" animal 0\nplayer.AuxVarSetFlt \"_FC_CreatureRide\" flags 1\nplayer.AuxVarSetFlt \"_FC_CreatureRide\" 1 2\nend");
 rideLeaseRef=compile("player.AuxVarGetRef \"_FC_CreatureRide\" 0",true);
 rideLeaseFlags=compile("player.AuxVarGetFlt \"_FC_CreatureRide\" 1",true);
 rideLeaseVersion=compile("player.AuxVarGetFlt \"_FC_CreatureRide\" 2",true);
 rideLeaseErase=compile("begin function {}\nplayer.AuxVarErase \"_FC_CreatureRide\"\nend");
 rideResolve=compile("string_var id\nbegin function {id}\nSetFunctionValue (StringToRef \"$id\")\nend");
 rideSneak=compile("player.IsSneaking",true);
 rideDetected=compile("GetDetected player",true);
 rideCombat=compile("IsInCombat",true);
 rideHostile=compile("begin function {}\nStartCombat player\nend");
 rideRestrict=compile("begin function {}\nSetRestrained 1\nend");
 rideUnrestrict=compile("begin function {}\nSetRestrained 0\nend");
 rideNotice=compile("string_var text\nbegin function {text}\nMessageExAlt 1 \"%z\" text\nend");
 rideActors=compile("GetActorsByProcessingLevel 0",true);
 rideModel=compile("GetSkeletonModel (GetBaseObject)",true);
 rideDead=compile("GetDead",true);
 rideInsert=compile("ref animal\nstring_var name\nbegin function {animal,name}\nSetFunctionValue (InsertNode animal 3 \"%z\" name)\nend");
 rideRemove=compile("ref animal\nstring_var name\nbegin function {animal,name}\nInsertNode animal 2 \"%z\" name\nend");
 rideMount=compile("ref animal\nbegin function {animal}\nplayer.SyncPos animal 0 0 \"FCRideSeat\"\nDisablePlayerControls 1 0 0 0 0 0 0\nend");
 rideRefresh=compile("begin function {}\nplayer.SetPosEx (player.GetPos X) (player.GetPos Y) (player.GetPos Z)\nend");
 rideExit=compile("float x\nfloat y\nfloat z\nfloat bottom\nfloat range\nbegin function {x,y,z}\nplayer.SyncPos\nlet bottom := z - 300\nlet z += 200\nlet range := GetP2PRayCastRange x y z x y bottom 6\nif range > 0\nlet z := bottom + range + 3\nplayer.SetPosEx x y z\nendif\nend");
 rideMove=compile("float vx\nfloat vy\nfloat heading\nbegin function {vx,vy,heading}\nSetActorVelocity X vx\nSetActorVelocity Y vy\nSetAngle Z heading\nend");
 rideBuck=compile("begin function {}\nPlayGroup AttackPower 1\nend");
 rideWalk=compile("begin function {}\nPlayGroup Forward 1\nend");
 rideRun=compile("begin function {}\nPlayGroup FastForward 1\nend");
 rideIdle=compile("begin function {}\nPlayGroup Idle 1\nend");
 rideEnabled=rideEnabled&&rideLeaseStore&&rideLeaseRef&&rideLeaseFlags&&rideLeaseVersion&&rideLeaseErase&&rideResolve&&rideSneak&&rideDetected&&rideCombat&&rideHostile&&rideRestrict&&rideUnrestrict&&rideNotice&&rideArrays&&rideActors&&rideModel&&rideDead&&rideInsert&&rideRemove&&rideMount&&rideRefresh&&rideExit&&rideMove&&rideBuck&&rideWalk&&rideRun&&rideIdle;
 log(rideEnabled?"Optional creature ride modes ready":"Creature ride modes unavailable");
}
unsigned floatBits(float v){unsigned n;memcpy(&n,&v,4);return n;}
void restoreRideLease(){
 if(!rideRestorePending||!player)return;rideRestorePending=false;
 auto version=call(rideLeaseVersion);if(version.type!=1||version.value.number!=1)return;
 auto stored=call(rideLeaseRef),f=call(rideLeaseFlags);unsigned flags=f.type==1?unsigned(f.value.number):0;
 auto animal=stored.type==2?stored.value.form:nullptr;
 if(animal&&(flags&2)&&vehicle_adapter::restrained(animal))callOn(rideUnrestrict,animal);
 auto p=scene::field<scene::Vec>(player,0x30);
 if(animal&&scene::root(animal)){auto t=scene::world(scene::root(animal));p=t.p+scene::rotate(t,{115,0,0});}
 call(rideExit,floatBits(p.x),floatBits(p.y),floatBits(p.z));
 auto& move=scene::field<unsigned char>(player,0x680);move=(move&~1u)|(flags&1u);
 if(animal&&(flags&4)){rideHostileRef=animal->id;rideHostileAt=GetTickCount64()+100;rideHostileUntil=rideHostileAt+3000;}
 call(rideLeaseErase);log("Saved ride temporary AI/movement lease restored; challenge reset");
}
void tickRideHostility(){
 if(!rideHostileRef||!player)return;auto now=GetTickCount64();if(now<rideHostileAt)return;
 if(now>rideHostileUntil){rideHostileRef=0;return;}rideHostileAt=now+250;
 auto animal=vehicle_adapter::lookup(rideHostileRef);
 if(!animal||!scene::root(animal)){rideHostileRef=0;return;}
 auto dead=callOn(rideDead,animal);if(dead.type!=1||dead.value.number){rideHostileRef=0;return;}
 auto combat=callOn(rideCombat,animal);if(combat.type==1&&combat.value.number){rideHostileRef=0;log("Dismounted Deathclaw native combat confirmed");return;}
 callOn(rideHostile,animal);
}
void updateRideNodes(){
 if(!riding||!car)return;auto root=scene::root(car);if(!root)return;
 auto spine=scene::find(root,"Bip01 Spine1"),front=scene::find(root,ridingDeathclaw?"Bip01 Spine2":"Bip01 Spine3"),carrier=scene::find(root,"FCRideCarrier");
 auto seat=scene::find(root,"FCRideSeat"),eye=scene::find(root,"FCRideEye");if(!spine||!front||!carrier||!seat||!eye)return;
 // The creature's root is the authoritative movement frame. Do not make
 // the HMD inherit spine animation (headbutts/idle breathing are not tracking).
 // Hands, seated body and UI use this same frame, with no camera-only filter.
 auto base=scene::world(root);auto t=base;float yaw=std::atan2(base.r[0][1],base.r[1][1]),co=std::cos(yaw),si=std::sin(yaw);
 t.r[0][0]=co;t.r[0][1]=si;t.r[0][2]=0;t.r[1][0]=-si;t.r[1][1]=co;t.r[1][2]=0;t.r[2][0]=t.r[2][1]=0;t.r[2][2]=1;
 t.p=base.p+scene::rotate(t,{0,ridingDeathclaw?-20.f:0,(ridingDeathclaw?155.f:79.5f)})*base.scale;scene::setWorld(carrier,t);
 scene::Transform s{};s.r[0][0]=s.r[1][1]=s.r[2][2]=1;s.scale=1;s.p={0,0,-22};scene::local(seat)=s;scene::update(seat);
 s.p={0,0,48};scene::local(eye)=s;scene::update(eye);
}
void stopRide(bool restore){
 if(!riding)return;
 auto animal=vehicle_adapter::lookup(rideCarrierID);rideCarrierID=0;bool hostile=ridingDeathclaw;riding=false;ridingDeathclaw=false;rideHold=rideStarted=0;
 if(restore&&player&&!animal){call(rideExit,floatBits(rideHome.x),floatBits(rideHome.y),floatBits(rideHome.z));auto& flags=scene::field<unsigned char>(player,0x680);flags=(flags&~1u)|rideMoveFlag;call(rideLeaseErase);}
 if(restore&&player&&animal){
  if(rideOwnedRestraint&&vehicle_adapter::restrained(animal))callOn(rideUnrestrict,animal);
  auto root=scene::root(animal);auto t=root?scene::world(root):scene::Transform{};auto p=root?t.p+scene::rotate(t,{115,0,0}):rideHome;
  callOn(rideMove,animal,floatBits(0),floatBits(0),floatBits(rideHeading));
  call(rideExit,floatBits(p.x),floatBits(p.y),floatBits(p.z));
  auto& flags=scene::field<unsigned char>(player,0x680);flags=(flags&~1u)|rideMoveFlag;
  for(auto n:{"FCRideEye","FCRideSeat","FCRideCarrier"})call(rideRemove,animal,n);
  auto dead=callOn(rideDead,animal);if(hostile&&dead.type==1&&!dead.value.number){rideHostileRef=animal->id;rideHostileAt=GetTickCount64()+100;rideHostileUntil=rideHostileAt+3000;}
  call(rideLeaseErase);
 }
 if(!testDir.empty()){std::ofstream o(testDir+"\\bighorner-state.json");o<<"{\"active\":false,\"dismounted\":true}";}
 rideChallenge.reset(ridingDeathclaw?deathclawChallengeSeconds:bighornerChallengeSeconds);rideOwnedRestraint=false;rideCandidateID=0;rideScan=GetTickCount64()+700;log(hostile?"Deathclaw dismounted - native combat resumed":"Bighorner dismounted");
}
void spreadRideLegs(){
 if(!player||!car)return;auto root=scene::root(player),carrier=scene::find(scene::root(car),"FCRideCarrier");if(!root||!carrier)return;auto frame=scene::world(carrier);
 const char* names[2][3]={{"Bip01 L Thigh","Bip01 L Calf","Bip01 L Foot"},{"Bip01 R Thigh","Bip01 R Calf","Bip01 R Foot"}};
 for(int h=0;h<2;h++){
  auto thigh=scene::find(root,names[h][0]),calf=scene::find(root,names[h][1]),foot=scene::find(root,names[h][2]);if(!thigh||!calf||!foot)continue;
  auto hip=scene::world(thigh),knee=scene::world(calf),ankle=scene::world(foot);float sign=h?1.f:-1.f;
  auto desired=frame.p+scene::rotate(frame,{sign*28,16,-18})*frame.scale;
  knee_steering::orient(thigh,mounted_frame::mul(knee_steering::align(knee.p-hip.p,desired-hip.p),knee_steering::rotation(hip)));
  auto newKnee=scene::world(calf),newAnkle=scene::world(foot);auto down=scene::rotate(frame,{sign*2,-8,-24});
  knee_steering::orient(calf,mounted_frame::mul(knee_steering::align(newAnkle.p-newKnee.p,down),knee_steering::rotation(newKnee)));
  knee_steering::orient(foot,knee_steering::rotation(ankle));
 }
}
// Deathclaw shoulders animate substantially relative to the locomotion root.
// Attach a holding hand to that current bone publication, while keeping the
// HMD on the shared level carrier. No new pose or camera history is substituted.
scene::Transform rideContactFrame(void* root){
 auto c=scene::world(scene::find(root,"FCRideCarrier"));
 if(ridingDeathclaw){auto bone=scene::find(root,"Bip01 Spine2");if(bone)c.p=scene::world(bone).p;}
 return c;
}
scene::Vec rideSocket(const scene::Transform& c,int hand){
 auto local=ridingDeathclaw?scene::Vec{hand?22.f:-22.f,-34,0}:scene::Vec{hand?13.f:-13.f,20,4};
 return c.p+scene::rotate(c,local)*c.scale;
}
bool rideSurface(void* root,scene::Vec worldPoint,float tolerance=8,bool deathclaw=false){
 if(!root)return false;auto t=scene::world(root);if(t.scale<.1f)return false;
 auto shellDistance=[](scene::Vec p,scene::Vec radii){
  float n=std::sqrt(p.x*p.x/(radii.x*radii.x)+p.y*p.y/(radii.y*radii.y)+p.z*p.z/(radii.z*radii.z));
  return n>.1f?scene::distance(p,p*(1/n)):10000.f;
 };
 if(deathclaw){
  for(auto name:{"Bip01 Spine1","Bip01 Spine2"}){auto bone=scene::find(root,name);if(!bone)continue;
   auto p=scene::inverseRotate(t,worldPoint-scene::world(bone).p)*(1/t.scale);
   if(shellDistance(p,{40,36,30})<=tolerance)return true;
  }return false;
 }
 auto p=scene::inverseRotate(t,worldPoint-t.p)*(1/t.scale)-scene::Vec{0,4,68};
 return shellDistance(p,{26,42,24})<=tolerance;
}
bool tickRide(){
 rideMountReady=false;
 auto now=GetTickCount64();auto rideCandidate=vehicle_adapter::lookup(rideCandidateID);Hand h[2];{std::lock_guard<std::mutex> g(lock);h[0]=hands[0];h[1]=hands[1];}
 auto menu=call(getMenu);nativeMenu=menu.type!=1||menu.value.number!=0;
 bool live=!nativeMenu;for(auto& v:h)live=live&&v.valid&&now-v.poseAt<200&&now-v.time<200;
 if(riding){
  if(!car||vehicle_adapter::lookup(rideCarrierID)!=car){clear();return true;}auto playerDead=callOn(rideDead,player);if(playerDead.type==1&&playerDead.value.number){clear();return true;}
  auto dead=callOn(rideDead,car);
  if(!car||!scene::root(car)||scene::distance(scene::field<scene::Vec>(car,0x30),scene::field<scene::Vec>(player,0x30))>600||dead.type!=1||dead.value.number){log("Bighorner attachment invalid/dead/distant");clear();return true;}
  if(live&&h[1].b&&!exitWas){clear();return true;}exitWas=h[1].b;
  ensureSeatedPose();
  if(!live)return true;
  auto delta=call(getDelta);float dt=delta.type==1?std::clamp(float(delta.value.number),0.f,.1f):0;
  auto cue=rideChallenge.cue;rideChallenge.update(dt,live,grabbed[0]||grabbed[1]);
  if(cue!=rideChallenge.cue){++ridePulse;call(rideNotice,rideChallenge.controlled?"YOUR RIDE - STICKS TO STEER":"HOLD ON");}
  if(rideChallenge.thrown){call(rideNotice,"BUCKED OFF");clear();return true;}
  float elapsed=rideChallenge.elapsed;bool buck=rideChallenge.bucking;
  float heading=rideHeading+20.f*std::sin(elapsed*.3f),speed=buck?2.f:7.f;
  if(rideChallenge.controlled){
   float y=h[0].sy;float demand=std::abs(y)>.15f?std::copysign((std::min(std::abs(y),1.f)-.15f)/.85f,y):0;
   float target=demand*(demand<0?5.f:18.f);rideSpeed+=std::clamp(target-rideSpeed,-22.f*dt,12.f*dt);speed=rideSpeed;
   float turn=std::abs(h[1].sx)>.15f?h[1].sx:0;rideHeading+=turn*60.f*dt;heading=rideHeading;
  }else rideSpeed=speed;
  float radians=heading*.01745329252f;
  if(now!=rideLast){callOn(rideMove,car,floatBits(std::sin(radians)*speed),floatBits(std::cos(radians)*speed),floatBits(heading));rideLast=now;}
  int gait=buck?3:std::abs(speed)<.2f?0:speed>7.f?2:1;
  if(gait!=rideGait||now-rideAnimAt>6000){callOn(gait==3?rideBuck:gait==2?rideRun:gait==1?rideWalk:rideIdle,car);rideAnimAt=now;rideGait=gait;}rideBuckPhase=buck;
  updateRideNodes();
 }else{
  if(active||!player||!live||weaponVisible){rideHold=0;return false;}
  if(now>=rideScan){
   rideScan=now+250;rideCandidate=nullptr;rideCandidateID=0;float nearest=150;auto list=call(rideActors);
   if(list.type==4&&list.value.array){auto n=rideArrays->size(list.value.array);if(n<=1024){std::vector<Element> values(n);if(n&&rideArrays->elements(list.value.array,values.data(),nullptr))for(auto& e:values){
    auto ref=e.type==2?e.value.form:nullptr;if(!ref||ref==player||!scene::root(ref))continue;
    auto distance=scene::distance(scene::field<scene::Vec>(ref,0x30),scene::field<scene::Vec>(player,0x30));if(distance>nearest)continue;
    auto model=callOn(rideModel,ref);bool dc=model.type==3&&model.value.string&&_stricmp(model.value.string,"creatures\\DeathClaw\\skeleton.nif")==0;
    bool match=(dc&&deathclawEnabled)||(bighornerEnabled&&model.type==3&&model.value.string&&_stricmp(model.value.string,"creatures\\NVBigHorner\\skeleton.nif")==0);
    if(model.type==3&&model.value.string)build_adapter::freeGameString(model.value.string);
    if(match){auto dead=callOn(rideDead,ref);if(dead.type==1&&!dead.value.number){rideCandidate=ref;rideCandidateID=ref->id;rideCandidateDeathclaw=dc;nearest=distance;}}
   }}}
  }
  if(!rideCandidate||!scene::root(rideCandidate)){rideHold=0;return false;}
  auto root=scene::root(rideCandidate);auto spine=scene::find(root,"Bip01 Spine1"),front=scene::find(root,rideCandidateDeathclaw?"Bip01 Spine2":"Bip01 Spine3");if(!spine||!front)return false;
  auto a=scene::world(spine).p,b=scene::world(front).p;auto base=build_adapter::heldModule();auto manager=base?reinterpret_cast<void*(__cdecl*)()>(base+0x13C73)():nullptr;
  bool closeEnough=true;for(int i=0;i<2;i++){scene::Vec p{};bool ok=vehicle_adapter::gripPoint(manager,i,p);closeEnough=closeEnough&&ok&&!vehicle_adapter::occupied(i,weaponVisible)&&rideSurface(root,p,30,rideCandidateDeathclaw);}
  if(!testDir.empty()){static ULONGLONG traceAt=0;if(now-traceAt>250){traceAt=now;std::ofstream o(testDir+"\\bighorner-contact.json");o<<"{\"hands\":[";for(int i=0;i<2;i++){scene::Vec p{};auto ok=vehicle_adapter::gripPoint(manager,i,p);if(i)o<<",";o<<"{\"grip\":"<<h[i].grip<<",\"valid\":"<<ok<<",\"distance\":"<<std::min(scene::distance(p,a),scene::distance(p,b))<<",\"occupied\":"<<vehicle_adapter::occupied(i,weaponVisible)<<",\"surface\":"<<rideSurface(root,p)<<"}";}o<<"]}";}}
  if(rideCandidateDeathclaw){
   auto sneak=call(rideSneak),detected=callOn(rideDetected,rideCandidate),combat=callOn(rideCombat,rideCandidate);
   auto t=scene::world(root);auto local=scene::inverseRotate(t,scene::field<scene::Vec>(player,0x30)-t.p);
   float horizontal=std::sqrt(local.x*local.x+local.y*local.y);
   bool eligible=sneak.type==1&&sneak.value.number!=0&&detected.type==1&&!detected.value.number&&combat.type==1&&!combat.value.number&&horizontal>1&&local.y/horizontal<-.45f;
   if(!testDir.empty()){std::ofstream o(testDir+"\\deathclaw-eligibility.json");o<<"{\"sneaking\":"<<(sneak.type==1?sneak.value.number:-1)<<",\"detected\":"<<(detected.type==1?detected.value.number:-1)<<",\"combat\":"<<(combat.type==1?combat.value.number:-1)<<",\"behind\":"<<(horizontal>1?local.y/horizontal:0)<<",\"eligible\":"<<(eligible?"true":"false")<<"}";}
   closeEnough=closeEnough&&eligible;
  }
  rideMountReady=closeEnough;
  closeEnough=closeEnough&&h[0].grip>.75f&&h[1].grip>.75f;
  if(!closeEnough){rideHold=0;}else if(!rideHold){rideHold=now;}else if(now-rideHold>100){
   Form* target=rideCandidate;bool dc=rideCandidateDeathclaw;clear();car=target;rideCarrierID=target->id;flying=false;ridingDeathclaw=dc;
   for(auto n:{"FCRideCarrier","FCRideSeat","FCRideEye"})call(rideInsert,car,n);
   auto carrier=scene::find(root,"FCRideCarrier"),seat=scene::find(root,"FCRideSeat"),eye=scene::find(root,"FCRideEye");
   if(!carrier||!seat||!eye){car=nullptr;return false;}scene::moveParent(seat,carrier);scene::moveParent(eye,carrier);
   rideHome=scene::field<scene::Vec>(player,0x30);rideMoveFlag=scene::field<unsigned char>(player,0x680)&1u;
   auto angle=callOn(getAngleZ,car);rideHeading=angle.type==1?float(angle.value.number):0;riding=true;active=true;
   rideChallenge.reset(ridingDeathclaw?deathclawChallengeSeconds:bighornerChallengeSeconds);rideOwnedRestraint=!vehicle_adapter::restrained(car);
   unsigned leaseFlags=rideMoveFlag|(rideOwnedRestraint?2u:0u)|(ridingDeathclaw?4u:0u);call(rideLeaseStore,car,leaseFlags);
   auto lease=call(rideLeaseRef);if(lease.type!=2||lease.value.form!=car){log("Ride refused: temporary state lease unavailable");clear();return false;}
   callOn(rideRestrict,car);
   updateRideNodes();call(rideMount,car);ensureSeatedPose();rideStarted=now;rideLast=rideAnimAt=0;rideBuckPhase=true;rideSpeed=0;rideGait=-1;exitWas=h[1].b;
   log(ridingDeathclaw?"Deathclaw mounted from hidden rear approach":"Bighorner mounted with both grips");
  }
 }
 if(!testDir.empty()&&(riding||rideCandidate)){
  static ULONGLONG at=0;if(now-at>100){at=now;auto target=riding?car:rideCandidate;auto root=scene::root(target);auto back=scene::find(root,"Bip01 Spine1"),front=scene::find(root,(riding?ridingDeathclaw.load():rideCandidateDeathclaw)?"Bip01 Spine2":"Bip01 Spine3");auto p=back?vehicle_adapter::tracking(scene::world(back).p):scene::Vec{},q=front?vehicle_adapter::tracking(scene::world(front).p):scene::Vec{};
   std::ofstream o(testDir+"\\bighorner-state.json");o<<"{\"active\":"<<(riding?"true":"false")<<",\"deathclaw\":"<<((riding?ridingDeathclaw.load():rideCandidateDeathclaw)?"true":"false")<<",\"ref\":"<<target->id<<",\"back\":["<<p.x<<","<<p.y<<","<<p.z<<"],\"front\":["<<q.x<<","<<q.y<<","<<q.z<<"],\"held\":["<<grabbed[0]<<","<<grabbed[1]<<"],\"buck\":"<<rideBuckPhase<<",\"speed\":"<<rideSpeed<<",\"gait\":"<<rideGait<<",\"seconds\":"<<rideChallenge.elapsed<<",\"controlled\":"<<(rideChallenge.controlled?"true":"false")<<",\"warning\":"<<rideChallenge.warning;
   auto ap=scene::field<scene::Vec>(target,0x30),pp=scene::field<scene::Vec>(player,0x30);o<<",\"animalPosition\":["<<ap.x<<","<<ap.y<<","<<ap.z<<"],\"playerPosition\":["<<pp.x<<","<<pp.y<<","<<pp.z<<"],\"restrained\":"<<vehicle_adapter::restrained(target);
   if(riding){auto c=rideContactFrame(root);auto position=scene::field<scene::Vec>(car,0x30);o<<",\"position\":["<<position.x<<","<<position.y<<","<<position.z<<"],\"sockets\":[";for(int h=0;h<2;h++){if(h)o<<",";auto pt=vehicle_adapter::tracking(grabbed[h]?vehicle_grip::world(c,gripLocal[h]).p:rideSocket(c,h));o<<"["<<pt.x<<","<<pt.y<<","<<pt.z<<"]";}o<<"]";}o<<"}";
  }
 }
 return riding;
}
void rideContacts(void* manager){
 auto carrier=scene::find(scene::root(car),"FCRideCarrier");if(!carrier){resetGrips();return;}auto c=rideContactFrame(scene::root(car));
 Hand h[2];{std::lock_guard<std::mutex> g(lock);h[0]=hands[0];h[1]=hands[1];}auto now=GetTickCount64();
 for(int i=0;i<2;i++){
  scene::Transform raw{};bool valid=vehicle_grip::raw(manager,i,raw)&&h[i].valid&&now-h[i].poseAt<200&&now-h[i].time<200;
  auto p=scene::inverseRotate(c,raw.p-c.p)*(1/c.scale);bool closeEnough=valid&&rideSurface(scene::root(car),raw.p,8,ridingDeathclaw);
  controlNear[i]=closeEnough&&!nativeMenu&&!handOccupied[i];
  bool wasHeld=grabbed[i];
  if(!valid||h[i].grip<.35f||nativeMenu||handOccupied[i])grabbed[i]=false;
  else if(!grabbed[i]&&closeEnough&&h[i].gripClaim&&h[i].grip>.65f){grabbed[i]=true;gripLocal[i]=vehicle_grip::relative(c,raw);}
  auto target=vehicle_grip::world(c,gripLocal[i]);if(grabbed[i]&&scene::distance(raw.p,target.p)>18)grabbed[i]=false;
  if(wasHeld&&!grabbed[i]&&file){fprintf(file,"RIDE GRIP LOST hand=%d valid=%d age=%llu inputAge=%llu grip=%.3f menu=%d occupied=%d distance=%.2f near=%d\n",i,valid,now-h[i].poseAt,now-h[i].time,h[i].grip,nativeMenu.load(),handOccupied[i].load(),scene::distance(raw.p,target.p),closeEnough);fflush(file);}
  controlHeld[i]=grabbed[i];vehicle_grip::targets[i]={grabbed[i],manager,target};
 }
}
