#pragma once
#include <cstdint>
namespace settlement_ui {
constexpr unsigned version=1,actions=12;
// Fixed-copy ABI; no game object/string pointers cross the render/game boundary.
struct State {
 unsigned size=sizeof(State),abi=version,epoch=0; int mode=0,stage=0,selected=0;
 uint64_t updated=0;
 char category[96]{},item[160]{},cost[256]{},supplies[256]{},page[64]{},items[9][128]{};
 unsigned bindings[actions]{};
};
using Snapshot=bool(__cdecl*)(State*,unsigned);
using Pointer=void(__cdecl*)(int,int,bool);
constexpr const char* labels[]={"PLACE / CONFIRM","BACK (HOLD TO EXIT)","< CATEGORY","CATEGORY >","< ITEM","ITEM >","SNAP","GROUND","DISTANCE","RESET","MOVE","SCRAP (HOLD)"};
constexpr const char* actionsNames[]={"Place","Back","TabPrev","TabNext","ItemPrev","ItemNext","Snap","Ground","Zoom","Reset","Modify","Delete"};
}
