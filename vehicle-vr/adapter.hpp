#pragma once
#include "../build_adapter.hpp"
namespace vehicle_adapter {
inline float unitsPerMetre(){auto b=build_adapter::heldModule();if(!b)return 0;auto settings=reinterpret_cast<unsigned char*(__cdecl*)()>(b+0xAF97)();if(!settings)return 0;float v=*reinterpret_cast<float*>(settings+0x28);return std::isfinite(v)&&v>=20&&v<=200?v:0;}
// Exact E14567BC native grip query; pose target before the arm solve.
inline bool gripPoint(void* manager,int hand,scene::Vec& point){
 auto b=build_adapter::heldModule();float rotation[9]{};
 return b&&manager&&reinterpret_cast<bool(__thiscall*)(void*,bool,bool,scene::Vec*,float*)>(b+0x6C19F0)(manager,hand!=0,false,&point,rotation);
}
inline bool rawGripPoint(void* manager,int hand,scene::Vec& point){
 auto b=build_adapter::heldModule();float rotation[9]{};
 return b&&manager&&reinterpret_cast<bool(__thiscall*)(void*,bool,bool,scene::Vec*,float*)>(b+0x6C1D00)(manager,hand!=0,false,&point,rotation);
}
inline scene::Vec handWorldCorrection(void* manager,int hand){return scene::field<scene::Vec>(manager,0x19FC+hand*12);}
inline scene::Vec rawStageGrip(void* manager,int hand){return scene::field<scene::Vec>(manager,0x60+hand*0x40);}
inline bool heldPolicyVerified(){auto b=build_adapter::heldModule();if(!b)return false;
 const unsigned char policy[]={0x80,0x7C,0x24,0x08,0x00,0x74,0x24,0x8B,0x4C,0x24,0x04,0x51,0xE8,0x5F,0xDD,0xFF,0xFF,0x83,0xC4,0x04,0x84,0xC0,0x75,0x10};
 const unsigned char held[]={0x8B,0x44,0x24,0x04,0x74,0x08,0x39,0x05};
 const unsigned char freeze[]={0xF6,0x06,0x03,0x74,0x04};
 return !memcmp(b+0x209890,policy,sizeof policy)&&b[0x1B2CA0]==0x80&&b[0x1B2CA1]==0x3D&&scene::field<unsigned char*>(b,0x1B2CA2)==b+0x14DA7FD&&!memcmp(b+0x1B2CA7,held,sizeof held)&&!memcmp(b+0x42922C,freeze,sizeof freeze);
}
inline bool occupied(int hand,bool weaponVisible){auto b=build_adapter::heldModule();return !b||(weaponVisible&&reinterpret_cast<bool(__cdecl*)(int,bool)>(b+0x209890)(hand,true))||reinterpret_cast<bool(__cdecl*)(int)>(b+0x1B2CA0)(hand)||build_adapter::stimHand()==hand;}
// Same native actor-state query consumed by TTR's publication at 3F58A8.
inline int actorState(void* actor){auto b=build_adapter::heldModule();return b&&actor?reinterpret_cast<int(__cdecl*)(void*)>(b+0x426A00)(actor):-1;}
// Same exact-build world-to-tracking conversion used by the core diagnostics.
inline scene::Vec tracking(scene::Vec point){
 auto b=build_adapter::heldModule();scene::Vec result{};if(!b)return result;
 auto manager=reinterpret_cast<void*(__cdecl*)()>(b+0x13C73)();
 if(manager)reinterpret_cast<scene::Vec*(__thiscall*)(void*,scene::Vec*,const scene::Vec*)>(b+0x6C7D50)(manager,&result,&point);
 return result;
}
// Retail Actor::lifeState 108, verified against JIP LN GameObjects.h. A
// temporary ride restriction is restored only while it still owns state 5.
inline bool restrained(void* actor){return actor&&scene::field<unsigned>(actor,0x108)==5;}

// Current-runtime TESForm map, same retail contract used by the core's physical
// item adapter. Never resolve a live ref through a previous save's mod mapping.
inline Form* lookup(unsigned id){
 if(!id||!build_adapter::heldModule())return nullptr;
 struct Entry{Entry* next;unsigned key;Form* data;};
 struct Map{void* vt;unsigned buckets;Entry** table;unsigned count;};
 auto map=*reinterpret_cast<Map**>(0x11C54C0);
 if(!map||!map->table||!map->buckets||map->buckets>1000000)return nullptr;
 unsigned budget=0;for(auto e=map->table[id%map->buckets];e&&budget++<100000;e=e->next)if(e->key==id)return e->data;
 return nullptr;
}

}
