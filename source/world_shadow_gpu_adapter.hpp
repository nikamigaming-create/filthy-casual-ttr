#pragma once
#include "scene_adapter.hpp"
#include <d3d9.h>
// FNV 1.4.0.525, exact host-build guard required by the caller.
// NiGeometryData +34 -> NiGeometryBufferData (VT 10F017C).
// Read-only PID35644 observation: GPU-only LOD has null CPU positions,
// one stream, native declaration, chip+08 VB, +34 IB, triangle-list topology.
// Public GameNi layout cross-checked against the live buffer sizes/counts.
// Own COM references only for the render snapshot; never retain native nodes.
namespace world_shadow_gpu {
using scene::field;
struct Geometry {
 IDirect3DVertexBuffer9* vb=nullptr;IDirect3DIndexBuffer9* ib=nullptr;
 IDirect3DVertexDeclaration9* declaration=nullptr;
 unsigned stride=0,vertices=0,triangles=0;int base=0;D3DPRIMITIVETYPE topology=D3DPT_TRIANGLELIST;
 Geometry()=default;Geometry(const Geometry&)=delete;Geometry& operator=(const Geometry&)=delete;
 Geometry(Geometry&& o) noexcept:vb(o.vb),ib(o.ib),declaration(o.declaration),stride(o.stride),vertices(o.vertices),triangles(o.triangles),base(o.base),topology(o.topology){o.vb=nullptr;o.ib=nullptr;o.declaration=nullptr;}
 ~Geometry(){if(vb)vb->Release();if(ib)ib->Release();if(declaration)declaration->Release();}
};
inline bool read(void* data,Geometry& out,bool requireUV=false,bool requireColour=false){
 if(field<unsigned char>(data,0x39)&0x10)return false; // unverified compressed positions
 auto buffer=field<void*>(data,0x34);
 if(!buffer||field<unsigned>(buffer,0)!=0x10F017C||field<unsigned>(buffer,0x20)!=1||field<unsigned>(buffer,0x48)!=1)return false;
 auto strides=field<unsigned*>(buffer,0x24);auto chips=field<void**>(buffer,0x28);
 if(!strides||!chips||!chips[0]||field<unsigned>(chips[0],0xC)!=0)return false;
 auto vb=field<IDirect3DVertexBuffer9*>(chips[0],8);auto ib=field<IDirect3DIndexBuffer9*>(buffer,0x34);
 auto declaration=field<IDirect3DVertexDeclaration9*>(buffer,0x10);
 unsigned vertices=field<unsigned>(buffer,0x18),triangles=field<unsigned>(buffer,0x40),stride=strides[0];
 int base=field<int>(buffer,0x38);auto topology=field<D3DPRIMITIVETYPE>(buffer,0x3C);
 if(!vb||!ib||!declaration||base<0||!vertices||vertices>65535||!triangles||triangles>200000||stride<12||stride>256)return false;
 if(topology!=D3DPT_TRIANGLELIST&&topology!=D3DPT_TRIANGLESTRIP)return false;
 D3DVERTEXELEMENT9 elements[MAXD3DDECLLENGTH+1]{};UINT count=MAXD3DDECLLENGTH+1;
 if(FAILED(declaration->GetDeclaration(elements,&count))||count>MAXD3DDECLLENGTH+1)return false;
 bool position=false,uv=false,colour=false;
 for(UINT i=0;i<count&&elements[i].Stream!=0xFF;++i){
  const auto& e=elements[i];
  if(e.Usage==D3DDECLUSAGE_POSITION&&e.UsageIndex==0){
   if(e.Stream!=0||e.Method!=D3DDECLMETHOD_DEFAULT||(e.Type!=D3DDECLTYPE_FLOAT3&&e.Type!=D3DDECLTYPE_FLOAT4)||unsigned(e.Offset)+(e.Type==D3DDECLTYPE_FLOAT3?12u:16u)>stride)return false;
   position=true;
  }
  if(e.Stream==0&&e.Method==D3DDECLMETHOD_DEFAULT&&e.UsageIndex==0){
   if(e.Usage==D3DDECLUSAGE_TEXCOORD&&e.Type==D3DDECLTYPE_FLOAT2&&unsigned(e.Offset)+8<=stride)uv=true;
   if(e.Usage==D3DDECLUSAGE_COLOR&&((e.Type==D3DDECLTYPE_FLOAT4&&unsigned(e.Offset)+16<=stride)||(e.Type==D3DDECLTYPE_D3DCOLOR&&unsigned(e.Offset)+4<=stride)))colour=true;
  }
 }
 D3DVERTEXBUFFER_DESC vd{};D3DINDEXBUFFER_DESC id{};
 if(!position||(requireUV&&!uv)||(requireColour&&!colour)||FAILED(vb->GetDesc(&vd))||FAILED(ib->GetDesc(&id))||id.Format!=D3DFMT_INDEX16)return false;
 unsigned indices=topology==D3DPT_TRIANGLELIST?triangles*3:triangles+2;
 if((static_cast<unsigned long long>(base)+vertices)*stride>vd.Size||indices*2>id.Size)return false;
 vb->AddRef();ib->AddRef();declaration->AddRef();out.vb=vb;out.ib=ib;out.declaration=declaration;
 out.stride=stride;out.vertices=vertices;out.triangles=triangles;out.base=base;out.topology=topology;return true;
}
}
