#pragma once
#include "glazing_hit.hpp"
#include "../glass_effects.hpp"
#include "glazing_scatter.hpp"
// xNVSE EventManager's public prefix, checked against PluginAPI.h. Classic
// onhit supplies {victim, attacker}; thisObj is null. No shooter filtering.
namespace vehicle_glazing {
struct Events {
 void* registerEvent;void* dispatch;void* dispatchAlt;
 bool(__cdecl* subscribe)(const char*,void(__cdecl*)(Form*,void*));
 void* unsubscribe;
};
struct Hit {unsigned victim=0,attacker=0;scene::Vec point{},direction{},start{},end{};float damage=0;bool projectile=false,traced=false;char paneName[32]{};scene::Vec localImpact{};};
struct Shot {unsigned id=0,source=0;scene::Vec previous{};ULONGLONG created=0;};
inline std::vector<Shot> shots;inline std::vector<unsigned> nearbyVehicles;
inline bool projectilesRegistered=false;
struct LoadedPanes {unsigned id;void* root;};
inline std::vector<LoadedPanes> restoredPanes;
inline std::mutex pendingLock;inline std::vector<Hit> pending;
inline Script *hitData=nullptr,*hitDamage=nullptr,*hitProjectile=nullptr,*savePane=nullptr,*readPane=nullptr;
inline Script* texture[3]{};inline bool enabled=false;
inline Script* meleeType=nullptr;
inline Script* breakSound=nullptr;inline unsigned soundRequests=0;
inline void* meleeRoot=nullptr;inline scene::Vec meleeTip{},lastMeleeTip{},lastMeleeGrip{},lastMeleeStage{};inline ULONGLONG meleeAt=0;
inline bool meleeValid=false;
inline unsigned received=0,broken=0;
inline Events* events=nullptr;
struct Fist {scene::Vec stage{},world{};ULONGLONG at=0,cooldown=0;bool valid=false;};
inline Fist fists[2];
inline bool candidate=false;inline Script* probeTarget=nullptr;
inline bool supported(Form* ref){return ref&&((drivingFeature&&findCarModel(ref))||(flightFeature&&flightBase&&scene::field<Form*>(ref,0x20)==flightBase));}
inline void segment(Shot& shot,scene::Vec next){
 if(!std::isfinite(next.x)||!std::isfinite(next.y)||!std::isfinite(next.z))return;
 if(scene::distance(shot.previous,next)<.01f)return;
 Hit h;h.attacker=shot.source;h.projectile=h.traced=true;h.damage=8;h.start=shot.previous;h.end=next;h.point=next;h.direction=next-shot.previous;
 {std::lock_guard<std::mutex> g(pendingLock);if(pending.size()<256)pending.push_back(h);}shot.previous=next;
}
// ShowOff's documented events provide the real projectile and its owner.
// Copy positions during callbacks. Never hold the projectile pointer after it
// is destroyed; native collision supplies the final endpoint for hitscan too.
inline void __cdecl projectileCreated(Form* ref,void* args){
 if(!enabled||!loaded||!ref||!args||!fc_feature_client::enabled("GlassImpacts")||shots.size()>=128)return;
 auto source=static_cast<Form**>(args)[0];
 shots.push_back({ref->id,source?source->id:0,scene::field<scene::Vec>(ref,0x30),GetTickCount64()});
}
inline void __cdecl projectileImpact(Form* ref,void*){
 if(!ref)return;for(auto& s:shots)if(s.id==ref->id){segment(s,scene::field<scene::Vec>(ref,0x30));break;}
}
inline void __cdecl projectileDestroyed(Form* ref,void* args){
 if(!ref)return;projectileImpact(ref,args);
 shots.erase(std::remove_if(shots.begin(),shots.end(),[&](const Shot& s){return s.id==ref->id;}),shots.end());
}
inline void subscribeProjectiles(){
 if(!enabled||!events)return;
 bool a=events->subscribe("ShowOff:OnProjectileCreate",projectileCreated);
 bool b=events->subscribe("ShowOff:OnProjectileImpact",projectileImpact);
 bool c=events->subscribe("ShowOff:OnProjectileDestroy",projectileDestroyed);
 projectilesRegistered=a&&b&&c;
 log(projectilesRegistered?"Vehicle glazing native projectile events ready":"Vehicle glazing projectile events unavailable");
}
inline void __cdecl onHit(Form*,void* args){
 if(!enabled||!loaded||!args||!fc_feature_client::enabled("GlassImpacts"))return;
 auto victim=static_cast<Form**>(args)[0],attacker=static_cast<Form**>(args)[1];
 if(!supported(victim))return;
 auto data=callOn(hitData,victim);if(data.type!=4||!data.value.array||!rideArrays||rideArrays->size(data.value.array)!=6)return;
 Element v[6];if(!rideArrays->elements(data.value.array,v,nullptr))return;
 for(auto& value:v)if(value.type!=1||!std::isfinite(value.value.number))return;
 auto damage=callOn(hitDamage,victim),projectile=callOn(hitProjectile,victim);
 Hit h;h.victim=victim->id;h.attacker=attacker?attacker->id:0;
 h.point={float(v[0].value.number),float(v[1].value.number),float(v[2].value.number)};
 h.direction={float(v[3].value.number),float(v[4].value.number),float(v[5].value.number)};
 h.damage=damage.type==1?float(damage.value.number):0;h.projectile=projectile.type==2&&projectile.value.form;
 // Gentle touch has no native damage event. Small melee damage is ignored.
 if(!std::isfinite(h.damage)||(!h.projectile&&h.damage<3))return;
 std::lock_guard<std::mutex> g(pendingLock);if(pending.size()<64)pending.push_back(h);
}
inline bool init(){
 auto ev=events;if(!candidate||!ev||!ev->subscribe)return false;
 if(!testDir.empty())probeTarget=compile("player.AuxiliaryVariableGetRef \"*FCGlassProbe\"",true);
 hitData=compile("GetHit3DData 2",true);hitDamage=compile("GetHitBaseWeaponDamage",true);hitProjectile=compile("GetHitProjectile",true);
 meleeType=compile("GetWeaponType (player.GetEquippedObject 5)",true);
 breakSound=compile("begin function {}\nPlaySound3D FXMeleeGenericGlass\nSetFunctionValue 1\nend");
 savePane=compile("string_var pane\nbegin function {pane}\nAuxiliaryVariableSetFloat \"$pane\" 1\nend");
 readPane=compile("string_var pane\nbegin function {pane}\nSetFunctionValue (AuxiliaryVariableGetFloat \"$pane\")\nend");
 for(unsigned i=0;i<3;i++){char s[128];sprintf_s(s,"GetFormFromMod \"Filthy Casual Glass.esp\" \"%06X\"",0x800+i);texture[i]=compile(s,true);}
 enabled=hitData&&hitDamage&&hitProjectile&&savePane&&readPane&&texture[0]&&texture[1]&&texture[2];
 if(enabled)enabled=ev->subscribe("onhit",onHit);subscribeProjectiles();return enabled;
}
inline bool contact(void* node,const Hit& h,scene::Vec& point){
 // Only the deliberately imported rigid triangle panes are eligible.
 if(strncmp(scene::name(node),"FCGlassPane",11))return false;
 auto data=scene::field<void*>(node,0xB8);if(!bottle_impact_adapter::readable(data,0x50))return false;
 auto count=scene::field<unsigned short>(data,8);auto vertices=scene::field<scene::Vec*>(data,0x20);
 auto triangles=scene::field<unsigned short>(data,0x40);auto ix=scene::field<unsigned short*>(data,0x48);
 if(!count||!triangles||!bottle_impact_adapter::readable(vertices,count*sizeof(scene::Vec))||!bottle_impact_adapter::readable(ix,triangles*6))return false;
 auto t=scene::world(node);if(!std::isfinite(t.scale)||std::abs(t.scale)<.001f)return false;
 auto length=scene::distance(h.direction,{});if(!std::isfinite(length)||length<.001f)return false;
 auto direction=h.direction*(1/length);
 // Native impact position, not actor centre or an arbitrary nearest window.
 auto a=scene::inverseRotate(t,(h.traced?h.start:h.point-direction*8)-t.p)*(1/t.scale);
 auto b=scene::inverseRotate(t,(h.traced?h.end:h.point+direction*8)-t.p)*(1/t.scale);
 auto cv=[](scene::Vec v){return glazing_hit::Vec{v.x,v.y,v.z};};
 float nearest=2;
 for(unsigned i=0;i<triangles;i++){
  auto x=ix[i*3],y=ix[i*3+1],z=ix[i*3+2];if(x>=count||y>=count||z>=count)return false;
  float fraction;if(glazing_hit::segment(cv(a),cv(b),cv(vertices[x]),cv(vertices[y]),cv(vertices[z]),fraction))nearest=std::min(nearest,fraction);
 }
 if(nearest>1)return false;point=t.p+scene::rotate(t,a+(b-a)*nearest)*t.scale;return true;
}
inline void restorePanes(Form* ref){
 auto root=scene::root(ref);if(!root)return;
 auto found=std::find_if(restoredPanes.begin(),restoredPanes.end(),[&](const LoadedPanes& p){return p.id==ref->id;});
 if(found!=restoredPanes.end()&&found->root==root)return;
 std::vector<scene::Box> shapes;scene::boxes(root,shapes);unsigned matched=0;
 for(auto& shape:shapes){
  auto node=shape.node;if(strncmp(scene::name(node),"FCGlassPane",11))continue;++matched;
  std::string key=std::string("FCVehicleGlass:")+scene::name(node);
  auto value=callOn(readPane,ref,key.c_str());if(value.type==1&&value.value.number>0)scene::field<unsigned>(node,0x30)|=1;
 }
 // An assembling actor may have a root before its geometry is available.
 if(!matched)return;
 if(found!=restoredPanes.end())found->root=root;
 else if(restoredPanes.size()<128)restoredPanes.push_back({ref->id,root});
}
inline void sampleFists(ULONGLONG now){
 auto manager=vehicle_adapter::manager();if(!manager)return;
 Hand input[2];{std::lock_guard<std::mutex> g(lock);input[0]=hands[0];input[1]=hands[1];}
 for(int hand=0;hand<2;hand++){
  auto& f=fists[hand];auto& in=input[hand];scene::Vec world{};
  bool valid=in.valid&&now-in.time<150&&now-in.poseAt<150&&!nativeMenu&&in.grip>.65f&&!controlHeld[hand]&&
    !vehicle_adapter::occupied(hand,weaponVisible)&&vehicle_adapter::gripPoint(manager,hand,world);
  auto stage=vehicle_adapter::rawStageGrip(manager,hand);
  auto dt=float(now-f.at)*.001f;float travel=scene::distance(stage,f.stage);
  // Real tracking-space speed, not vehicle/locomotion speed. Gaps and large
  // tracking jumps reseed, never punch. Ordinary gentle touches remain intact.
  if(valid&&f.valid&&dt>.001f&&dt<.1f&&travel<.35f&&travel/dt>=1.5f&&now>=f.cooldown){
   auto delta=world-f.world;float length=scene::distance(delta,{});
   if(length>.01f){Hit h;h.attacker=player?player->id:0;h.traced=true;h.damage=8;h.start=f.world;h.end=world+delta*(4/length);h.point=world;h.direction=delta;
    std::lock_guard<std::mutex> g(pendingLock);if(pending.size()<256)pending.push_back(h);
   }
  }
  f.stage=stage;f.world=world;f.at=now;f.valid=valid;
 }
}
inline void sampleMelee(ULONGLONG now){
 auto type=call(meleeType);auto hand=build_adapter::drawnWeaponHand();
 if(!weaponVisible||nativeMenu||type.type!=1||(type.value.number!=1&&type.value.number!=2)||hand<0||hand>1){meleeValid=false;meleeRoot=nullptr;return;}
 Hand in;{std::lock_guard<std::mutex> g(lock);in=hands[hand];}
 if(!in.valid||now-in.time>=150||now-in.poseAt>=150||controlHeld[hand]){meleeValid=false;return;}
 auto manager=vehicle_adapter::manager();scene::Vec grip{};
 if(!manager||!vehicle_adapter::gripPoint(manager,hand,grip)){meleeValid=false;return;}
 build_adapter::PouchDiagnostics facts{};build_adapter::readPouchDiagnostics(facts);
 auto root=facts.weaponRoot?facts.weaponRoot:scene::find(scene::root(player),"Weapon");
 if(!root){meleeValid=false;return;}
 std::vector<scene::Box> shapes;scene::boxes(root,shapes);
 float furthest=0;scene::Vec tip{};
 // Follow the displayed melee mesh, not a trigger or bare-hand hit volume.
 for(auto& shape:shapes){auto data=scene::field<void*>(shape.node,0xB8);if(!bottle_impact_adapter::readable(data,0x24))continue;auto n=scene::field<unsigned short>(data,8);auto v=scene::field<scene::Vec*>(data,0x20);if(n>12000||!bottle_impact_adapter::readable(v,n*sizeof(scene::Vec)))continue;auto t=scene::world(shape.node);
  for(unsigned i=0;i<n;i++){auto p=t.p+scene::rotate(t,v[i])*t.scale;auto d=scene::distance(p,grip);if(d>furthest&&d<160){furthest=d;tip=p;}}
 }
 auto stage=vehicle_adapter::tracking(tip);
 auto dt=float(now-meleeAt)*.001f;float distance=scene::distance(stage,lastMeleeStage);
 if(meleeValid&&root==meleeRoot&&furthest>10&&dt>.001f&&dt<.1f&&distance<.5f&&distance/dt>2.f){
  Hit h;h.attacker=player->id;h.traced=true;h.damage=12;h.point=tip;h.direction=tip-lastMeleeTip;
  std::lock_guard<std::mutex> g(pendingLock);
  // A bat strikes with its barrel too. Sweep the actual grip-to-tip shaft,
  // rather than requiring its single farthest vertex to pierce the window.
  for(int i=1;i<=8&&pending.size()<256;i++){
   float f=float(i)/8;h.start=lastMeleeGrip+(lastMeleeTip-lastMeleeGrip)*f;h.end=grip+(tip-grip)*f;pending.push_back(h);
  }
  if(pending.size()<256){h.start=grip+(tip-grip)*.125f;h.end=tip;pending.push_back(h);}
 }
 meleeRoot=root;meleeTip=tip;lastMeleeTip=tip;lastMeleeGrip=grip;lastMeleeStage=stage;meleeAt=now;meleeValid=furthest>10;
}
// A chassis collision has no projectile trajectory. Select the intact pane
// closest to the impact on the struck side and feed a real triangle hit
// through the same persistence, sound, shard and decal path as other impacts.
inline void crash(Form* vehicle,scene::Vec impact,scene::Vec direction,float severity){
 if(!enabled||!loaded||!fc_feature_client::enabled("GlassImpacts")||severity<8)return;
 auto root=scene::root(vehicle);if(!root)return;
 const auto origin=scene::world(root).p;
 std::vector<scene::Box> panes;scene::boxes(root,panes);
 float nearest=210;Hit hit;
 for(const auto& pane:panes){
  auto node=pane.node;
  if(strncmp(scene::name(node),"FCGlassPane",11)||(scene::field<unsigned>(node,0x30)&1))continue;
  const auto transform=scene::world(node);
  auto center=transform.p+scene::rotate(transform,(pane.lo+pane.hi)*.5f)*transform.scale;
  if((center.x-origin.x)*direction.x+(center.y-origin.y)*direction.y<0)continue;
  Hit h;h.victim=vehicle->id;h.traced=true;h.direction=direction;
  h.start=center-direction*45;h.end=center+direction*45;
  scene::Vec crossing;if(!contact(node,h,crossing))continue;
  const float gap=scene::distance(crossing,impact);
  if(gap>=nearest)continue;
  if(strlen(scene::name(node))>=sizeof(h.paneName))continue;
  strcpy_s(h.paneName,scene::name(node));
  // Preserve the triangle contact in pane space across the queued physics tick.
  h.localImpact=scene::inverseRotate(transform,crossing-transform.p)*(1/transform.scale);
  nearest=gap;h.traced=false;h.point=crossing;h.damage=severity*2;hit=h;
 }
 if(hit.victim){std::lock_guard<std::mutex> g(pendingLock);if(pending.size()<64)pending.push_back(hit);}
}
inline void tick(){
 if(!enabled)return;
 if(!loaded||!fc_feature_client::enabled("GlassImpacts")){
  std::lock_guard<std::mutex> g(pendingLock);pending.clear();shots.clear();fists[0].valid=fists[1].valid=false;glazing_scatter::clear();return;
 }
 auto now=GetTickCount64();
 glazing_scatter::tick(now);
 sampleFists(now);
 sampleMelee(now);
 for(auto it=shots.begin();it!=shots.end();){auto ref=vehicle_adapter::lookup(it->id);if(!ref||now-it->created>10000){it=shots.erase(it);continue;}segment(*it,scene::field<scene::Vec>(ref,0x30));++it;}
 static ULONGLONG scanAt=0;
 if(now-scanAt>500&&parkedActors&&player){
  scanAt=now;nearbyVehicles.clear();auto list=call(parkedActors);
  if(list.type==4&&list.value.array&&rideArrays){auto n=rideArrays->size(list.value.array);if(n<=1024){std::vector<Element> actors(n);if(n&&rideArrays->elements(list.value.array,actors.data(),nullptr))for(auto& a:actors)if(a.type==2&&supported(a.value.form)&&scene::distance(scene::field<scene::Vec>(a.value.form,0x30),scene::field<scene::Vec>(player,0x30))<5000){nearbyVehicles.push_back(a.value.form->id);if(nearbyVehicles.size()>=24)break;}}}
  restoredPanes.erase(std::remove_if(restoredPanes.begin(),restoredPanes.end(),[](const LoadedPanes& p){return std::find(nearbyVehicles.begin(),nearbyVehicles.end(),p.id)==nearbyVehicles.end();}),restoredPanes.end());
  for(auto id:nearbyVehicles)if(auto ref=vehicle_adapter::lookup(id))restorePanes(ref);
 }
 std::vector<Hit> hits;{std::lock_guard<std::mutex> g(pendingLock);hits.swap(pending);}
 std::vector<Hit> expanded;for(auto h:hits){if(h.traced){for(auto id:nearbyVehicles){h.victim=id;expanded.push_back(h);}}else expanded.push_back(h);}hits.swap(expanded);
 for(auto& h:hits){
  ++received;auto ref=vehicle_adapter::lookup(h.victim);if(!supported(ref))continue;
  if(!testDir.empty()){char s[256];sprintf_s(s,"GLAZING hit victim=%08X attacker=%08X point=%.2f,%.2f,%.2f direction=%.3f,%.3f,%.3f damage=%.1f projectile=%u",h.victim,h.attacker,h.point.x,h.point.y,h.point.z,h.direction.x,h.direction.y,h.direction.z,h.damage,h.projectile);log(s);}
  std::vector<scene::Box> shapes;scene::boxes(scene::root(ref),shapes);
  for(auto& shape:shapes){
   auto node=shape.node;if(strncmp(scene::name(node),"FCGlassPane",11))continue;
   std::string key=std::string("FCVehicleGlass:")+scene::name(node);
   auto previous=callOn(readPane,ref,key.c_str());if(previous.type==1&&previous.value.number>0){scene::field<unsigned>(node,0x30)|=1;continue;}
   scene::Vec point;
   if(h.paneName[0]){
    if(strcmp(scene::name(node),h.paneName))continue;
    const auto pose=scene::world(node);
    point=pose.p+scene::rotate(pose,h.localImpact)*pose.scale;
   }else if(!contact(node,h,point))continue;
   // Imported panes have no rigid body. Never disable the chassis collision.
   if(scene::field<void*>(node,0x1C))continue;
   auto style=glass_variation::choose(h.victim^received,h.damage);
   auto tx=call(texture[style.pattern]);if(tx.type!=2||!tx.value.form)continue;
   auto normal=h.direction*(-1/std::max(.001f,scene::distance(h.direction,{})));
   auto result=glass_effects::emit(point,normal,nullptr,node,tx.value.form,h.victim^received,h.damage);
   // Preserve the native guarded point-sound path. If another sound hook
   // owns it, dispatch the game's glass sound through its public script API.
   if(!result.sound&&breakSound){auto audio=callOn(breakSound,ref);result.sound=audio.type==1&&audio.value.number==1;}
   if(result.sound)++soundRequests;
   if(!result.particles&&!result.sound)continue;
   scene::field<unsigned>(node,0x30)|=1;callOn(savePane,ref,key.c_str());++broken;
   glazing_scatter::burst(point,normal,tx.value.form->id,h.victim^received);
   if(h.traced&&!h.projectile)for(auto& fist:fists)fist.cooldown=now+250;
   char line[256];sprintf_s(line,"GLAZING break victim=%08X attacker=%08X pane=%s projectile=%u decal=%u sound=%u particles=%u",h.victim,h.attacker,scene::name(node),h.projectile,result.decal,result.sound,result.particles);log(line);
   if(!h.traced)break;
  }
 }
 if(!testDir.empty()){
  static ULONGLONG last=0;auto now=GetTickCount64();if(now-last<300)return;last=now;
  std::ofstream o(testDir+"\\vehicle-glass.json");o<<"{\"enabled\":"<<enabled<<",\"projectileEvents\":"<<projectilesRegistered<<",\"received\":"<<received<<",\"broken\":"<<broken<<",\"panes\":[";
  auto target=call(probeTarget);std::vector<scene::Box> panes;
  if(target.type==2&&supported(target.value.form))scene::boxes(scene::root(target.value.form),panes);
  static unsigned reported=0;
  if(target.type==2&&target.value.form&&reported!=target.value.form->id){
   reported=target.value.form->id;char line[256];
   sprintf_s(line,"GLAZING probe ref=%08X supported=%u root=%p shapes=%u",reported,supported(target.value.form),scene::root(target.value.form),unsigned(panes.size()));log(line);
   for(auto& p:panes){sprintf_s(line,"GLAZING shape %.180s",scene::name(p.node));log(line);}
  }
  bool comma=false;for(auto& p:panes){if(strncmp(scene::name(p.node),"FCGlassPane",11))continue;if(comma)o<<",";comma=true;
   auto t=scene::world(p.node);auto point=t.p+scene::rotate(t,(p.lo+p.hi)*.5f)*t.scale;auto q=vehicle_adapter::tracking(point);
   auto data=scene::field<void*>(p.node,0xB8);auto verts=scene::field<scene::Vec*>(data,0x20);auto ix=scene::field<unsigned short*>(data,0x48);
   auto normal=scene::rotate(t,scene::cross(verts[ix[1]]-verts[ix[0]],verts[ix[2]]-verts[ix[0]]));float size=scene::distance(normal,{});if(size>.001f)normal=normal*(1/size);
   auto radial=point-scene::world(scene::root(target.value.form)).p;
   if(normal.x*radial.x+normal.y*radial.y+normal.z*radial.z<0)normal=normal*(-1);
   auto normalTracking=vehicle_adapter::tracking(point+normal*70)-q;
   o<<"{\"name\":\""<<scene::name(p.node)<<"\",\"hidden\":"<<(scene::field<unsigned>(p.node,0x30)&1)<<",\"world\":["<<point.x<<","<<point.y<<","<<point.z<<"],\"tracking\":["<<q.x<<","<<q.y<<","<<q.z<<"],\"normal\":["<<normalTracking.x<<","<<normalTracking.y<<","<<normalTracking.z<<"]}";
  }auto tip=vehicle_adapter::tracking(meleeTip);o<<"],\"scatterLanded\":"<<glazing_scatter::landed<<",\"soundRequests\":"<<soundRequests<<",\"meleeValid\":"<<meleeValid<<",\"meleeTip\":["<<tip.x<<","<<tip.y<<","<<tip.z<<"]}";
 }
}
inline void clear(){std::lock_guard<std::mutex> g(pendingLock);pending.clear();shots.clear();nearbyVehicles.clear();restoredPanes.clear();fists[0]={};fists[1]={};meleeValid=false;meleeRoot=nullptr;glazing_scatter::clear();received=broken=soundRequests=0;}
}
