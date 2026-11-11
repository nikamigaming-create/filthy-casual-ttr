#pragma once
#include "reload_adapter.hpp"
#include "scene_adapter.hpp"
// TTR1.1-only read-only facts. ECCBA0 is the native 'chambered %d' argument
// at2E95CF and spent-case branch at2E92FE. Runtime transition checks required.
namespace tutorial_adapter {
struct Snapshot {bool valid=false,chamberKnown=false,chambered=false;build_adapter::PouchDiagnostics reload;};
inline Snapshot read(){
 Snapshot s;auto b=build_adapter::heldModule();if(!b||!build_adapter::readPouchDiagnostics(s.reload))return s;
 auto chamber=b[0xECCBA0];s.chamberKnown=chamber<=1;s.chambered=chamber==1;
 s.valid=s.reload.nativeInitialized&&s.reload.weaponRoot&&build_adapter::readPouchNode(s.reload.weaponRoot).readable;
 return s;
}
// Read-only FNV1.4.0.525 TESObjectWEAP facts, guarded by plugin runtime/type.
// Layout source: xNVSE/NVSE nvse/nvse/GameForms.h TESObjectWEAP.
struct WeaponFacts {int type=-1,reload=-1;};
inline WeaponFacts facts(void* weapon){
 WeaponFacts f;if(!weapon||scene::field<unsigned char>(weapon,4)!=0x28)return f;
 f.type=scene::field<unsigned char>(weapon,0xF4);f.reload=scene::field<unsigned char>(weapon,0x103);return f;
}
inline void namedParts(void* node,std::vector<void*>& out,int depth=0){
 if(!node||depth>32||out.size()>=160)return;
 if(strncmp(scene::name(node),"##",2)==0)out.push_back(node);
 auto vt=scene::field<void**>(node,0);if(!reinterpret_cast<void*(__thiscall*)(void*)>(vt[3])(node))return;
 auto count=scene::field<unsigned short>(node,0xA6);auto children=scene::field<void**>(node,0xA0);
 if(!children||count>4096)return;
 for(unsigned i=0;i<count;i++)namedParts(children[i],out,depth+1);
}
inline scene::Vec position(void* weapon,tutorial_data::Vec p){
 auto t=scene::world(weapon);return t.p+scene::rotate(t,{p.x,p.y,p.z})*t.scale;
}
// Generic loose rounds use TTR's live target, which may be adjusted from the
// packaged handling marker. Specialized loader/charger paths keep their own
// geometry. This is a read-only, exact-build-gated presentation lookup.
inline scene::Vec loadingPoint(const Snapshot& s,const tutorial_data::Weapon& w){
 const auto& r=s.reload;
 if(s.valid&&r.roundTargetValid&&!w.loader.present&&!w.charger.present&&
    r.roundFlags[0]==0&&r.roundFlags[1]==0&&r.roundFlags[2]!=0)
  return position(r.weaponRoot,{r.roundTargetLocal[0],r.roundTargetLocal[1],r.roundTargetLocal[2]});
 return position(r.weaponRoot,w.rounds);
}
}
