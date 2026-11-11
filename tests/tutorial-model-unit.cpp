#include "../reload_tutorial.hpp"
#include <cassert>
#include <cstdio>
using namespace reload_tutorial;
int main(){
 tutorial_data::Weapon magazine;magazine.mag.present=true;magazine.handle.present=true;
 State s;s.visible=true;s.weapon=true;s.manual=true;s.installed=4;
 assert(choose(&magazine,s)==Step::FetchAmmo);s.held=1;assert(choose(&magazine,s)==Step::InsertMagazine);
 s.held=0;s.installed=2;assert(choose(&magazine,s)==Step::InsertMagazine);
 s.installed=1;assert(choose(&magazine,s)==Step::MovingPart);
 s.installed=0;s.installedRounds=12;s.chamberKnown=true;assert(choose(&magazine,s)==Step::CycleAction);
 s.chambered=true;assert(choose(&magazine,s)==Step::Ready);
 s.installedRounds=0;assert(choose(&magazine,s)==Step::RemoveMagazine);
 s.manual=false;assert(choose(&magazine,s)==Step::Automatic);
 s.weapon=false;assert(choose(&magazine,s)==Step::ChooseWeapon);
 s.visible=false;assert(choose(&magazine,s)==Step::Hidden);
 s.visible=s.weapon=s.manual=true;assert(choose(nullptr,s)==Step::Unmapped);
 tutorial_data::Weapon tube;tube.hasRounds=true;tube.handle.present=true;
 s.rounds=3;s.capacity=8;assert(choose(&tube,s)==Step::LoadRounds);
 s.chambered=false;assert(choose(&tube,s)==Step::CycleAction);
 s.rounds=0;assert(choose(&tube,s)==Step::LoadRounds);
 s.rounds=8;s.chambered=false;assert(choose(&tube,s)==Step::CycleAction);
 s.chambered=true;assert(choose(&tube,s)==Step::Ready);
 tutorial_data::Weapon melee;assert(choose(&melee,s)==Step::Unmapped);
 tutorial_data::Weapon cylinder;cylinder.loader.present=true;s.rounds=0;s.capacity=6;
 assert(choose(&cylinder,s)==Step::LoadRounds);
 assert(mechanism(&cylinder,s)==Mechanism::Cylinder);
 assert(std::string(card(Step::LoadRounds,Mechanism::Cylinder))=="cylinder-guide");
 s.weaponType=5;s.reloadAnimation=21;
 assert(mechanism(&tube,s)==Mechanism::Pump);
 assert(std::string(card(Step::CycleAction,Mechanism::Pump))=="pump-guide");
 assert(std::string(card(Step::LoadRounds,Mechanism::Pump))=="load-tube");
 // A chambered, full tube is ready; do not ask for another ejecting pump cycle.
 s.rounds=s.capacity;s.chambered=true;
 assert(std::string(card(choose(&tube,s),Mechanism::Pump))=="pump-ready");
 s.weaponType=3;assert(mechanism(&tube,s)!=Mechanism::Pump);
 tutorial_data::Weapon gate;gate.hasRounds=true;s.reloadAnimation=20;
 assert(mechanism(&gate,s)==Mechanism::Gate);
 s.weaponType=5;s.reloadAnimation=8;assert(mechanism(&gate,s)==Mechanism::BreakAction);
 s.reloadAnimation=9;assert(mechanism(&gate,s)==Mechanism::BreakAction);
 s.weaponType=3;assert(mechanism(&gate,s)!=Mechanism::BreakAction);
 s.weaponType=5;
 magazine.mag.node="##MicroFusionCell";assert(mechanism(&magazine,s)==Mechanism::Cell);
 assert(std::string(card(Step::InsertMagazine,Mechanism::Cell))=="insert-cell");
 magazine.mag.node="##Clip";magazine.handle.node="##G3Lever";
 assert(mechanism(&magazine,s)!=Mechanism::Lever);
 magazine.handle.node="##TurnLever";assert(mechanism(&magazine,s)==Mechanism::Bolt);
 tutorial_data::Weapon repeater;repeater.handle.present=true;repeater.handle.node="##BreachSlide";
 assert(mechanism(&repeater,s)==Mechanism::Lever);
 assert(settle(Step::CycleAction,Step::Ready,true)==Step::CycleAction);
 assert(settle(Step::CycleAction,Step::LoadRounds,true)==Step::CycleAction);
 assert(settle(Step::CycleAction,Step::Ready,false)==Step::Ready);
 assert(settle(Step::CycleAction,Step::Automatic,true)==Step::Automatic);
 assert(settle(Step::CycleAction,Step::Hidden,true)==Step::Hidden);
 assert(settle(Step::CycleAction,Step::FetchAmmo,true)==Step::FetchAmmo);
 assert(settle(Step::Hidden,Step::Ready,true)==Step::Ready);
 tutorial_data::Weapon charger;charger.charger.present=true;charger.mag.present=true;charger.handle.present=true;
 s.visible=s.weapon=s.manual=true;s.installed=0;s.held=0;s.installedRounds=0;s.chamberKnown=true;s.chambered=true;
 assert(mechanism(&charger,s)==Mechanism::Charger);
 assert(choose(&charger,s)==Step::LoadRounds);
 assert(std::string(card(choose(&charger,s),Mechanism::Charger))=="charger-guide");
 s.held=1;s.installedRounds=10;s.chambered=false;assert(choose(&charger,s)==Step::InsertMagazine);
 assert(std::string(card(choose(&charger,s),Mechanism::Charger))=="insert-charger");
 s.held=0;assert(choose(&charger,s)==Step::CycleAction);
 assert(std::string(card(choose(&charger,s),Mechanism::Charger))=="cycle-action");
 s.chambered=true;assert(choose(&charger,s)==Step::Ready);
 assert(std::string(card(choose(&charger,s),Mechanism::Charger))=="charger-ready");
 puts("PASS: native-state tutorial steps, magazine vs loading-port paths, unavailable state and automatic-mode behavior");
}
