#pragma once
#include "tutorial_data.hpp"
namespace reload_tutorial {
enum class Step {Hidden,ChooseWeapon,Unmapped,Automatic,Ready,RemoveMagazine,FetchAmmo,InsertMagazine,CycleAction,LoadRounds,MovingPart};
enum class Mechanism {Generic,Magazine,Cell,Charger,Slide,Bolt,ChargingHandle,Cylinder,Gate,BreakAction,Pump,Lever};
struct State {int weaponType=-1,reloadAnimation=-1;bool visible=false,weapon=false,manual=false,chamberKnown=false,chambered=false;int rounds=0,capacity=0,installed=-1,installedRounds=0,held=0;};
inline Mechanism mechanism(const tutorial_data::Weapon* w,const State& s){
 if(!w)return Mechanism::Generic;
 if(w->charger.present)return Mechanism::Charger;
 if(w->loader.present)return Mechanism::Cylinder;
 auto handle=tutorial_data::key(w->handle.node),mag=tutorial_data::key(w->mag.node);
 // Several ordinary charging handles are named "Lever" in TTR meshes.
 // Classify only the observed lever-action families, not a substring match.
 if(handle=="##bblever"||handle=="##leverbolt"||handle=="##lashlever"||handle=="##breachslide")return Mechanism::Lever;
 // These animation IDs are observed with their corresponding live TTR parts.
 if(w->hasRounds&&!w->mag.present){
  if(s.reloadAnimation==20&&s.weaponType==3)return Mechanism::Gate;
  if((s.reloadAnimation==8||s.reloadAnimation==9)&&s.weaponType==5)return Mechanism::BreakAction;
  if(s.reloadAnimation==21&&s.weaponType==5&&w->handle.present)return Mechanism::Pump;
 }
 if(w->mag.present){
  if(mag.find("cell")!=std::string::npos||mag.find("battery")!=std::string::npos)return Mechanism::Cell;
  if(handle.find("bolt")!=std::string::npos||handle=="##turnlever")return Mechanism::Bolt;
  if(handle.find("charging")!=std::string::npos)return Mechanism::ChargingHandle;
  if(handle.find("slide")!=std::string::npos)return Mechanism::Slide;
  return Mechanism::Magazine;
 }
 return Mechanism::Generic;
}
inline Step choose(const tutorial_data::Weapon* w,const State& s){
 if(!s.visible)return Step::Hidden;
 if(!s.weapon)return Step::ChooseWeapon;
 if(!w)return Step::Unmapped;
 if(!s.manual)return Step::Automatic;
 // Chargers feed an internal magazine. Their native installed mode remains0;
 // A/X does not remove a detachable magazine. Held clip takes precedence over
 // the internal round count, which can fill while the clip is still in hand.
 if(w->charger.present){
  if(s.held)return Step::InsertMagazine;
  if(s.installedRounds<=0)return Step::LoadRounds;
  if(s.chamberKnown&&!s.chambered&&w->handle.present)return Step::CycleAction;
  return Step::Ready;
 }
 if(w->mag.present){
  if(s.installed==4)return s.held?Step::InsertMagazine:Step::FetchAmmo;
  // Native pouch pickup enters the installed-part insertion state directly.
  if(s.installed==2)return Step::InsertMagazine;
  if(s.installed!=0)return Step::MovingPart;
  if(s.chamberKnown&&!s.chambered&&s.installedRounds>0&&w->handle.present)return Step::CycleAction;
  if(s.installedRounds<=0)return Step::RemoveMagazine;
  return Step::Ready;
 }
 if(w->hasRounds||w->loader.present){
  // A loaded but unchambered pump/bolt must be cycled before optional topping up.
  if(s.rounds>0&&s.chamberKnown&&!s.chambered&&w->handle.present)return Step::CycleAction;
  if(s.rounds<s.capacity)return Step::LoadRounds;
  if(s.chamberKnown&&!s.chambered&&w->handle.present)return Step::CycleAction;
  return Step::Ready;
 }
 if(mechanism(w,s)==Mechanism::Lever)return s.chamberKnown&&!s.chambered?Step::CycleAction:Step::Ready;
 return Step::Unmapped;
}
// Native chamber state can become true while a slide/bolt is still held.
// Keep the demonstrated cycle instruction until its controls are released.
// This presentation-only hold must never override native menus, mode or weapon.
inline Step settle(Step previous,Step observed,bool cyclingControls){
 if(previous==Step::CycleAction&&cyclingControls&&(observed==Step::Ready||observed==Step::LoadRounds))return Step::CycleAction;
 return observed;
}
inline const char* name(Step step){
 switch(step){
 case Step::Hidden:return "hidden";case Step::ChooseWeapon:return "choose-weapon";
 case Step::Unmapped:return "unmapped";case Step::Automatic:return "automatic";
 case Step::Ready:return "ready";case Step::RemoveMagazine:return "remove-magazine";
 case Step::FetchAmmo:return "fetch-ammo";case Step::InsertMagazine:return "insert-magazine";
 case Step::CycleAction:return "cycle-action";case Step::LoadRounds:return "load-rounds";
 case Step::MovingPart:return "moving-part";default:return "hidden";
 }
}
inline const char* card(Step step,Mechanism m){
 if(m==Mechanism::Charger){
  if(step==Step::LoadRounds)return "charger-guide";
  if(step==Step::InsertMagazine)return "insert-charger";
  if(step==Step::CycleAction)return "cycle-action";
  if(step==Step::Ready)return "charger-ready";
 }

 if(step==Step::Ready||step==Step::LoadRounds){
  if(m==Mechanism::Cylinder)return "cylinder-guide";
  if(m==Mechanism::Gate)return "gate-guide";
  if(m==Mechanism::BreakAction)return "break-guide";
  if(m==Mechanism::Pump)return step==Step::Ready?"pump-ready":"load-tube";
  if(m==Mechanism::Lever)return "lever-guide";
 }
 if(step==Step::CycleAction){
  if(m==Mechanism::Pump)return "pump-guide";
  if(m==Mechanism::Lever)return "lever-guide";
  if(m==Mechanism::Bolt)return "bolt-guide";
  if(m==Mechanism::ChargingHandle)return "charging-guide";
 }
 if(m==Mechanism::Cell){
  if(step==Step::RemoveMagazine)return "remove-cell";
  if(step==Step::InsertMagazine)return "insert-cell";
 }
 return name(step);
}
inline constexpr const char* cardNames[]={"pump-ready","charger-guide","insert-charger","charger-ready","choose-weapon","unmapped","automatic","ready","remove-magazine","fetch-ammo","insert-magazine","cycle-action","load-rounds","moving-part","cylinder-guide","gate-guide","break-guide","pump-guide","load-tube","lever-guide","bolt-guide","charging-guide","remove-cell","insert-cell"};

}
