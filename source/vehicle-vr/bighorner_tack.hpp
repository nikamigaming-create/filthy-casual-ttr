#pragma once
#include "bighorner_tack_assets.hpp"
// Our five inventory forms and bone attachments. Native animal AI, inventory,
// merchant prices and save serialization remain engine-owned.
namespace bighorner_tack {
Script *protection{},*markOwned{},*restrictAnimal{},*dead{},*stock{},*fit{},*registry{},*attach{},*detach{},*restore{},*sync{},*readGear{},*openBags{};
Script* erase[tack_assets::count]{};Script* legacyErase[3]{};
ULONGLONG nextStock=0,nextFit=0;bool available=false;
bool bagWas[2]{};
void initFit(){
 openBags=compile("begin function {}\nOpenTeammateContainer 1\nend");
 protection=compile(R"FC(
int enabled
int armor
float desired
float delta
begin function {enabled}
let desired := 0
if enabled
 let armor := AuxVarGetFlt "_FC_Tack_v1" 1
 if armor == 1
  let desired := 3
 elseif armor == 2
  let desired := 6
 elseif armor == 3
  let desired := 10
 endif
endif
let delta := desired - (AuxVarGetFlt "_FC_Tack_v1" 3)
if delta != 0
 ModAV DamageThreshold delta
 AuxVarSetFlt "_FC_Tack_v1" desired 3
endif
end
)FC");
 markOwned=compile("begin function {}\nAuxVarSetFlt \"_FC_Tack_v1\" 1 2\nend");
 restrictAnimal=compile("begin function {}\nSetRestrained 1\nend");
 dead=compile("GetDead",true);
 registry=compile("int slot\nbegin function {slot}\nSetFunctionValue (player.AuxVarGetRef \"_FC_TackAnimals_v1\" slot)\nend");
 attach=compile("ref animal\nstring_var path\nbegin function {animal,path}\nSetFunctionValue (AttachModel animal 3 \"%z\" path)\nend");
 detach=compile("ref animal\nstring_var path\nbegin function {animal,path}\nAttachModel animal 2 \"%z\" path\nend");
 for(int i=0;i<tack_assets::count;++i){std::string s="begin function {}\nRemoveNifBlock \"";s+=tack_assets::parts[i].name;s+="\"\nend";erase[i]=compile(s.c_str());}
 const char* legacy[]={"light","reinforced","heavy"};
 for(int i=0;i<3;++i){std::string s="begin function {}\nRemoveNifBlock \"FCTack_";s+=legacy[i];s+="\"\nend";legacyErase[i]=compile(s.c_str());}
 restore=compile("begin function {}\nif AuxVarGetFlt \"_FC_Tack_v1\" 2\nSetRestrained 0\nAuxVarSetFlt \"_FC_Tack_v1\" 0 2\nendif\nend");
 // Native inventory is the source of truth. Migrate the earlier flag-only
 // prototype once, retaining purchased gear and never duplicating it on load.
 sync=compile(R"FC(
int enabled
ref saddle
ref leather
ref metal
ref power
ref bags
int level
begin function {enabled}
let saddle := GetFormFromMod "Filthy Casual Bighorner Tack.esp" "000800"
let leather := GetFormFromMod "Filthy Casual Bighorner Tack.esp" "000801"
let metal := GetFormFromMod "Filthy Casual Bighorner Tack.esp" "000802"
let power := GetFormFromMod "Filthy Casual Bighorner Tack.esp" "000803"
let bags := GetFormFromMod "Filthy Casual Bighorner Tack.esp" "000804"
if eval !saddle || !leather || !metal || !power || !bags
 return
endif
if AuxVarGetFlt "_FC_Tack_v1" 6 == 0
 if AuxVarGetFlt "_FC_Tack_v1" 0
  if GetItemCount saddle == 0
   AddItem saddle 1 1
  endif
 endif
 let level := AuxVarGetFlt "_FC_Tack_v1" 1
 if level == 1 && GetItemCount leather == 0
  AddItem leather 1 1
 elseif level == 2 && GetItemCount metal == 0
  AddItem metal 1 1
 elseif level == 3 && GetItemCount power == 0
  AddItem power 1 1
 endif
 AuxVarSetFlt "_FC_Tack_v1" 1 6
endif
AuxVarSetFlt "_FC_Tack_v1" (GetItemCount saddle > 0) 0
let level := 0
if GetItemCount power
 let level := 3
elseif GetItemCount metal
 let level := 2
elseif GetItemCount leather
 let level := 1
endif
AuxVarSetFlt "_FC_Tack_v1" level 1
AuxVarSetFlt "_FC_Tack_v1" (GetItemCount bags > 0) 4
AuxVarSetFlt "_FC_Tack_v1" enabled 5
end
)FC");
 readGear=compile("(AuxVarGetFlt \"_FC_Tack_v1\" 0) + 2 * (AuxVarGetFlt \"_FC_Tack_v1\" 1) + 8 * (AuxVarGetFlt \"_FC_Tack_v1\" 4)",true);
 fit=compile(R"FC(
ref animal
ref saddle
ref piece
ref bags
ref slotRef
int slot
int ownSlot
int level
int previous
begin function {animal}
if eval !animal
 return
endif
let saddle := GetFormFromMod "Filthy Casual Bighorner Tack.esp" "000800"
let bags := GetFormFromMod "Filthy Casual Bighorner Tack.esp" "000804"
if eval !saddle || !bags
 return
endif
let ownSlot := -1
let slot := 0
while slot < 32
 let slotRef := player.AuxVarGetRef "_FC_TackAnimals_v1" slot
 if eval slotRef == animal
  let ownSlot := slot
  break
 elseif eval !slotRef && ownSlot < 0
  let ownSlot := slot
 endif
 let slot += 1
loop
if ownSlot < 0
 MessageExAlt 1 "Bighorner equipment limit reached. Equipment kept."
 return
endif
let previous := animal.AuxVarGetFlt "_FC_Tack_v1" 1
let level := 3
while level > previous
 if level == 3
  let piece := GetFormFromMod "Filthy Casual Bighorner Tack.esp" "000803"
 elseif level == 2
  let piece := GetFormFromMod "Filthy Casual Bighorner Tack.esp" "000802"
 else
  let piece := GetFormFromMod "Filthy Casual Bighorner Tack.esp" "000801"
 endif
 if player.GetItemCount piece > 0
  player.RemoveItem piece 1 1
  animal.AddItem piece 1 1
  break
 endif
 let level -= 1
loop
if animal.GetItemCount saddle == 0 && player.GetItemCount saddle > 0
 player.RemoveItem saddle 1 1
 animal.AddItem saddle 1 1
endif
if animal.GetItemCount bags == 0 && player.GetItemCount bags > 0
 player.RemoveItem bags 1 1
 animal.AddItem bags 1 1
endif
if animal.GetItemCount saddle || animal.GetItemCount bags || level > previous || previous > 0
 player.AuxVarSetRef "_FC_TackAnimals_v1" animal ownSlot
endif
SetFunctionValue (animal.GetItemCount saddle > 0)
end
)FC");

}
void ensurePark(Form* animal){if(!vehicle_adapter::restrained(animal)){callOn(markOwned,animal);callOn(restrictAnimal,animal);}}
bool equipForRide(Form* animal){
 if(!available||!animal)return false;
 callOn(sync,animal,1);auto result=call(fit,animal);callOn(sync,animal,1);
 bool saddled=result.type==1&&result.value.number==1;
 if(saddled)ensurePark(animal);callOn(protection,animal,1);return saddled;
}
void removePart(Form* animal,int i){
 const auto& p=tack_assets::parts[i];
 if((animal->id>>24)==0xFF)callOn(erase[i],animal);
 else {std::string path=p.bone;path+='|';path+=p.path;call(detach,animal,path.c_str());}
}
void snapshot(Form* animal){
 if(testDir.empty()||!animal)return;auto root=scene::root(animal);int total[5]{},present[5]{};
 for(const auto& p:tack_assets::parts){++total[p.kind];if(scene::find(root,p.name))++present[p.kind];}
 std::ofstream o(testDir+"\\tack-state.json");o<<"{\"pid\":"<<GetCurrentProcessId()<<",\"ref\":"<<animal->id<<",\"restrained\":"<<(vehicle_adapter::restrained(animal)?"true":"false")<<",\"models\":[";
 for(int i=0;i<5;++i){if(i)o<<",";o<<(present[i]==total[i]?"true":"false");}o<<"],\"components\":[";
 for(int i=0;i<5;++i){if(i)o<<",";o<<present[i];}o<<"],\"expected\":[";
 for(int i=0;i<5;++i){if(i)o<<",";o<<total[i];}auto frameRoot=scene::world(root);auto previewCenter=vehicle_adapter::tracking(frameRoot.p+scene::rotate(frameRoot,{0,15,65})*frameRoot.scale);
 o<<"],\"center\":["<<previewCenter.x<<","<<previewCenter.y<<","<<previewCenter.z<<"],\"bagPoints\":[";
 auto bag=scene::find(root,"FCTack_bags");
 if(bag){auto frame=scene::world(bag);for(int side=0;side<2;++side){if(side)o<<",";const auto& p=tack_assets::bagSockets[side];auto q=vehicle_adapter::tracking(frame.p+scene::rotate(frame,{p[0],p[1],p[2]})*frame.scale);o<<"["<<q.x<<","<<q.y<<","<<q.z<<"]";}}
 o<<"]}";
}
unsigned bagPress(const Hand (&h)[2]){unsigned press=0;for(int i=0;i<2;++i){if(h[i].x&&!bagWas[i])press|=1u<<i;bagWas[i]=h[i].x;}return press;}
bool tryBags(Form* animal,void* manager,unsigned pressed){
 if(!available||!bighornerEnabled||!fc_feature_client::enabled("Riding")||!pressed||!animal)return false;
 auto gear=callOn(readGear,animal);if(gear.type!=1||!(int(gear.value.number)&8))return false;
 auto node=scene::find(scene::root(animal),"FCTack_bags");if(!node)return false;
 auto frame=scene::world(node);
 for(int h=0;h<2;++h){scene::Vec palm{};if(!(pressed&(1u<<h))||vehicle_adapter::occupied(h,weaponVisible)||!vehicle_adapter::gripPoint(manager,h,palm))continue;
  for(int side=0;side<2;++side){const auto& p=tack_assets::bagSockets[side];auto world=frame.p+scene::rotate(frame,{p[0],p[1],p[2]})*frame.scale;
   if(scene::distance(palm,world)<20*frame.scale){callOn(openBags,animal);log("Native pannier inventory opened with contextual A/X");return true;}
  }
 }return false;
}
void tickFit(){
 if(!available||!player)return;auto now=GetTickCount64();if(now<nextFit)return;nextFit=now+500;
 bool enabled=bighornerEnabled&&fc_feature_client::enabled("Riding");
 for(int slot=0;slot<32;++slot){
  auto e=call(registry,slot);auto animal=e.type==2?e.value.form:nullptr;if(!animal)continue;
  auto root=scene::root(animal);if(!root)continue;
  auto death=callOn(dead,animal);bool live=enabled&&death.type==1&&!death.value.number;
  callOn(sync,animal,live?1:0);auto g=callOn(readGear,animal);int bits=g.type==1?int(g.value.number):0;
  bool saddle=bits&1,bags=bits&8;int armor=(bits>>1)&3;
  if(live&&saddle)ensurePark(animal);else callOn(restore,animal);
  callOn(protection,animal,live?1:0);
  // Retire old combined models on existing prototype saves.
  const char* old[]={"light","reinforced","heavy"};
  for(int i=0;i<3;++i){std::string name="FCTack_";name+=old[i];if(scene::find(root,name.c_str())){
   if((animal->id>>24)==0xFF)callOn(legacyErase[i],animal);
   else {std::string path="Bip01 Spine1|FilthyCasual\\Bighorner\\";path+=old[i];path+=".nif";call(detach,animal,path.c_str());}
  }}
  for(int i=0;i<tack_assets::count;++i){const auto& p=tack_assets::parts[i];bool wanted=live&&(p.kind==0?saddle:p.kind==4?bags:armor==p.kind);bool exists=scene::find(root,p.name)!=nullptr;
   if(wanted&&!exists){std::string path=p.bone;path+='|';path+=p.path;call(attach,animal,path.c_str());}
   else if(!wanted&&exists)removePart(animal,i);
  }
  snapshot(animal);
 }
}
void init(){
 initFit();
 stock=compile(R"FC(
ref vendor
ref piece
int index
float day
float previous
begin function {}
let vendor := StringToRef "00104C7A"
let piece := GetFormFromMod "Filthy Casual Bighorner Tack.esp" "000804"
if eval !vendor || !piece
 return
endif
let day := GameDaysPassed + 1
let previous := vendor.AuxVarGetFlt "_FC_BighornerStock_v2"
if previous > 0 && day < previous + 3
 return
endif
let index := 0
while index < 5
 if index == 0
  let piece := GetFormFromMod "Filthy Casual Bighorner Tack.esp" "000800"
 elseif index == 1
  let piece := GetFormFromMod "Filthy Casual Bighorner Tack.esp" "000801"
 elseif index == 2
  let piece := GetFormFromMod "Filthy Casual Bighorner Tack.esp" "000802"
 elseif index == 3
  let piece := GetFormFromMod "Filthy Casual Bighorner Tack.esp" "000803"
 else
  let piece := GetFormFromMod "Filthy Casual Bighorner Tack.esp" "000804"
 endif
 if vendor.GetItemCount piece < 1
  vendor.AddItem piece 1 1
 endif
 let index += 1
loop
vendor.AuxVarSetFlt "_FC_BighornerStock_v2" day
SetFunctionValue 1
end
)FC");
 available=protection&&stock&&fit&&registry&&attach&&detach&&restore&&dead&&markOwned&&restrictAnimal&&sync&&readGear&&openBags;
 for(auto script:erase)available=available&&script;for(auto script:legacyErase)available=available&&script;
 auto content=call(compile("GetFormFromMod \"Filthy Casual Bighorner Tack.esp\" \"000804\"",true));available=available&&content.type==2&&content.value.form;
 if(!available)log("Bighorner equipment unavailable; no fitting or merchant changes");
}
void tick(){
 tickFit();if(!available||!player||!bighornerEnabled||!fc_feature_client::enabled("Riding"))return;
 auto now=GetTickCount64();if(now<nextStock)return;nextStock=now+60000;
 auto menu=call(getMenu);if(menu.type!=1||menu.value.number){nextStock=now+1000;return;}
 auto result=call(stock);if(result.type==1&&result.value.number==1)log("Chet stocked separate saddle, leather/metal/power armor and saddlebags");
}
}
