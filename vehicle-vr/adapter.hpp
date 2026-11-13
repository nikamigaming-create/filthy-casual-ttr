#pragma once
#include "../build_adapter.hpp"
namespace vehicle_adapter {
inline float unitsPerMetre(){auto b=build_adapter::heldModule();if(!b)return 0;auto settings=reinterpret_cast<unsigned char*(__cdecl*)()>(b+0xADC1)();if(!settings)return 0;float v=*reinterpret_cast<float*>(settings+0x28);return std::isfinite(v)&&v>=20&&v<=200?v:0;}
// Exact 7EB72860 native grip query; pose target before the arm solve.
inline bool gripPoint(void* manager,int hand,scene::Vec& point){
 auto b=build_adapter::heldModule();float rotation[9]{};
 return b&&manager&&reinterpret_cast<bool(__thiscall*)(void*,bool,bool,scene::Vec*,float*)>(b+0x6949C0)(manager,hand!=0,false,&point,rotation);
}
inline bool rawGripPoint(void* manager,int hand,scene::Vec& point){
 auto b=build_adapter::heldModule();float rotation[9]{};
 return b&&manager&&reinterpret_cast<bool(__thiscall*)(void*,bool,bool,scene::Vec*,float*)>(b+0x694CD0)(manager,hand!=0,false,&point,rotation);
}
inline scene::Vec handWorldCorrection(void* manager,int hand){return scene::field<scene::Vec>(manager,0x19F8+hand*12);}
inline scene::Vec rawStageGrip(void* manager,int hand){return scene::field<scene::Vec>(manager,0x3C+hand*0x40);}
// Same exact-build world-to-tracking conversion used by the core diagnostics.
inline scene::Vec tracking(scene::Vec point){
 auto b=build_adapter::heldModule();scene::Vec result{};if(!b)return result;
 auto manager=reinterpret_cast<void*(__cdecl*)()>(b+0x13903)();
 if(manager)reinterpret_cast<scene::Vec*(__thiscall*)(void*,scene::Vec*,const scene::Vec*)>(b+0x69AAC0)(manager,&result,&point);
 return result;
}
}
