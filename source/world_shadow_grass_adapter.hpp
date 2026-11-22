#pragma once
#include "scene_adapter.hpp"
#include <array>
#include <cmath>

// FNV 1.4.0.525 only, behind the existing exact host guard.
// Live PID32964: TallGrassTriStrips instances use property+7C, texture+84.
// Native BAAF40 copies count * float4 from batch+14; active count is +12.
// GRASS23x000 equations were inspected privately from shaderpackage019.sdp.
// Never distribute those shader bytes. This is independently written code.
namespace world_shadow_grass {
struct InstanceData {
 std::array<float,228*4> positions{};
 float wind[4]{};float scale[4]{};
};
inline bool read(void* node,void* property,void* data,scene::Vec origin,InstanceData& out,const float*& ids,void*& texture){
 using scene::field;
 if(field<unsigned>(property,0)!=0x10ADEA8||field<unsigned>(node,0)!=0x10BCC90)return false;
 auto group=field<void*>(property,0x7C);if(!group)return false;
 unsigned capacity=field<unsigned short>(group,0x10),active=field<unsigned short>(group,0x12);
 auto source=field<float*>(group,0x14);
 if(!capacity||capacity>228||!active||active>capacity||!source)return false;
 auto extra=field<void*>(data,0x30);unsigned n=field<unsigned short>(data,8);
 if(!extra||field<unsigned>(extra,0x10)!=1||field<unsigned short>(extra,0xC)!=n)return false;
 auto format=field<unsigned*>(extra,0x14);auto blocks=field<void**>(extra,0x20);
 if(!format||format[2]!=4||format[4]!=4||!blocks||!blocks[0]||field<unsigned>(blocks[0],4)<n*4)return false;
 ids=field<float*>(blocks[0],8);texture=field<void*>(property,0x84);if(!ids||!texture)return false;
 for(unsigned i=0;i<active;++i){
  for(unsigned k=0;k<4;++k){float v=source[i*4+k];if(!std::isfinite(v))return false;out.positions[i*4+k]=v-(k<3?(&origin.x)[k]:0);}
 }
 // Native XYZ instance coordinates are world positions. Per-vertex extra
 // scalar selects an instance; base positions already contain blade rotation.
 bool uniform=(field<unsigned>(property,0x24)&0x800)!=0;
 out.scale[0]=out.scale[1]=uniform?1.f:0.f;out.scale[2]=1;out.scale[3]=float(active);
 for(unsigned k=0;k<3;++k){out.wind[k]=reinterpret_cast<float*>(0x1200550)[k];if(!std::isfinite(out.wind[k]))return false;}
 constexpr double tau=6.283180236816406;
 double phase=double(*reinterpret_cast<float*>(0x11F940C))/3600*tau*field<float>(group,0x34);
 if(!std::isfinite(phase))return false;out.wind[3]=float(std::fmod(phase,tau));
 // World-origin rebasing must not alter the native spatial wind phase.
 out.wind[3]+=std::fmod((origin.x+origin.y)/128.f,float(tau));
 return true;
}
}
