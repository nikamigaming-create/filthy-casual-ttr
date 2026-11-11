#pragma once
#include "../scene_adapter.hpp"
// FalloutNV 1.4.0.525; read-only layouts from xNVSE/JIP GameObjects,
// GameBSExtraData, GameExtraData. Called on the game thread after build gating.
namespace pooladapter {
// ExtraPackage is the BACKUP package during interrupt procedures, not the
// current script package. The current stable package is queried via JIP.
inline Form* backupPackage(Form* actor){
 if(!actor)return nullptr;
 auto node=scene::field<void*>(actor,0x48);
 for(unsigned n=0;node&&n<256;++n,node=scene::field<void*>(node,8))
  if(scene::field<unsigned char>(node,4)==0x19)return scene::field<Form*>(node,0xC);
 return nullptr;
}
inline scene::Vec tracking(scene::Vec world){
 auto b=reinterpret_cast<unsigned char*>(GetModuleHandleW(L"TTWVR.dll"));
 auto manager=reinterpret_cast<void*(__cdecl*)()>(b+0x12BC5)();
 scene::Vec out{};
 reinterpret_cast<scene::Vec*(__thiscall*)(void*,scene::Vec*,const scene::Vec*)>(b+0x64B0B0)(manager,&out,&world);
 return out;
}
// Agreed xNVSE/JIP prefix; GetAnimTime in kNVSE compares +3C to text keys.
// Read only the selected owned idle on the native game thread.
struct AnimationClock {bool valid=false;float time=0;unsigned state=0;};
inline AnimationClock ownAnimation(Form* actor,bool ownIdle,float duration,int reach,int lateral){
 if(!actor||!ownIdle)return {};
 auto vt=scene::field<void**>(actor,0);
 auto data=reinterpret_cast<void*(__thiscall*)(void*)>(vt[0x1E4/4])(actor);
 if(!data)return {};
 AnimationClock result{};void* selected=nullptr;char expected[96]{};sprintf_s(expected,"specialidle_fcpool_d%d_x%d.kf",reach,lateral);
 for(unsigned i=0;i<8;i++){
  auto seq=scene::field<void*>(data,0xE0+4*i);if(!seq)continue;
  auto name=scene::field<const char*>(seq,8);if(!name)continue;
  size_t n=strnlen_s(name,512);if(n==512)continue;
  char lower[512]{};memcpy(lower,name,n);_strlwr_s(lower);
  if(strcmp(lower,"specialidle_fcpool")&& !strstr(lower,expected))continue;
  auto state=scene::field<unsigned>(seq,0x44);if(state!=1&&state!=2)continue;
  float begin=scene::field<float>(seq,0x2C),end=scene::field<float>(seq,0x30),time=scene::field<float>(seq,0x3C);
  if(!std::isfinite(begin)||!std::isfinite(time)||!std::isfinite(end)||std::abs(begin)>.001f||std::abs(end-duration)>.02f||time<0||time>end+.02f)continue;
  if(selected&&selected!=seq)return {};
  selected=seq;result={true,time,state};
 }
 return result;
}

}

