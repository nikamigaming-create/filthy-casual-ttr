#pragma once
#include "arcade_drive.hpp"
namespace vehicle_fleet {
struct Model {const char* form;const char* name;const char* directory;float speed,acceleration,braking;};
inline const Model models[]={
 {"00E6BC","Roadster","Creatures\\MkTRoadster",27,10,19},
 {"0010A8","Beetle","Uhmatt\\Creatures\\UmBeetle",12,6,20},
 {"014CF3","Bradley","Uhmatt\\Creatures\\UmBradleyGenII",10,4,16},
 {"014CF5","BTR-80","Uhmatt\\Creatures\\UmBTR",12,5,18},
 {"01008A","Elderville","Creatures\\UmElderville",16,8,23},
 {"007B0B","Deathcar","Creatures\\UmDeathcar",16,8,23},
 {"012308","Carriage","Creatures\\UmMkTCarriage",9,4,16},
 {"00EF58","Corvega Coupe","Creatures\\UmCorvegaCoupeB",16,8,23},
 {"00D802","Corvega Sedan","Creatures\\UmCorvegaSedan",15,7,22},
 {"00EC79","Firetruck","Creatures\\UmFiretruck",11,4,18},
 {"00A347","Floor Buffer","Creatures\\UmFloorBuffer",4,3,12},
 {"00A064","Frontloader","Creatures\\UmFrontloader",7,3,14},
 {"00CEC0","Fusion Flea","Creatures\\UmFlea",13,7,22},
 {"010C06","Golf Cart","Creatures\\UmGolfCart",8,5,16},
 {"00E0FC","Humvee","Creatures\\UmFunvee",14,6,21},
 {"008966","Jeep","Uhmatt\\Creatures\\UmJeep",14,8,23},
 {"014CF2","LAV-25","Uhmatt\\Creatures\\UmLAVGenII",12,5,18},
 {"006CB3","Pickup","Creatures\\UmPickup",14,7,22},
 {"011784","Rocket Cart","Creatures\\UmJokerineCart",13,8,22},
 {"014CF4","Sherman","Uhmatt\\Creatures\\UmShermanGenII",9,3,15},
 {"00C8AA","Sport Coupe","Creatures\\NVHPCar",17,10,25},
 {"00A345","Street Cleaner","Creatures\\UmStreetCleaner",6,3,14},
 {"011D4A","Tractor","Creatures\\UmMkTTractor",8,3,15},
 {"00ABE5","Motorcycle","Creatures\\UmMotorcycleA",12,6,20},
 {"00B482","Classic Motorcycle","Creatures\\UmMotorcycleClassic",12,6,20},
 {"00D4D8","Chopper","Creatures\\UmMotorcycleChopper",12,6,20},
 {"00E3DC","Truck","Creatures\\UmArmyTruckNEW",11,4,18},
 {"00D80D","Van","Creatures\\UmVan",13,6,20}
};
}
