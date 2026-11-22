#pragma once
#include "world_shadow_scene_adapter.hpp"
#include "lamp_shadow_shaders.hpp"
#include "projected_lighting.hpp"
#include <atomic>
#include "shadow_visibility.hpp"

namespace lamp_shadow_renderer {
using scene::Vec;
inline std::atomic<unsigned> generation{0};
inline std::atomic<bool> gameplay{false};
inline unsigned builtGeneration=~0u;
inline bool valid=false,attempted=false;
inline unsigned shaderCompiles=0;
inline HRESULT result=S_OK;
inline char error[512]{};
inline world_shadow_scene::Snapshot snapshot;
inline IDirect3DVertexShader9 *casterVS=nullptr,*grassVS=nullptr,*opaqueVS=nullptr,*maskedVS=nullptr;
inline IDirect3DPixelShader9 *casterPS=nullptr;
inline IDirect3DVertexDeclaration9* declaration=nullptr;
inline IDirect3DStateBlock9* state=nullptr;
template<class T>inline void drop(T*& p){if(p)p->Release();p=nullptr;}
struct Target {
 IDirect3DTexture9* texture=nullptr;IDirect3DSurface9* surface=nullptr;IDirect3DSurface9* depth=nullptr;unsigned w=0,h=0;
 void release(){drop(depth);drop(surface);drop(texture);w=h=0;}
 bool create(IDirect3DDevice9* d,unsigned width,unsigned height,D3DFORMAT format,bool z){
  if(texture&&w==width&&h==height)return true;release();
  result=d->CreateTexture(width,height,1,D3DUSAGE_RENDERTARGET,format,D3DPOOL_DEFAULT,&texture,nullptr);
  if(SUCCEEDED(result))result=texture->GetSurfaceLevel(0,&surface);
  if(z&&SUCCEEDED(result))result=d->CreateDepthStencilSurface(width,height,D3DFMT_D24X8,D3DMULTISAMPLE_NONE,0,FALSE,&depth,nullptr);
  if(FAILED(result)){release();return false;}w=width;h=height;return true;
 }
};
inline unsigned maps=0,failures=0,casterDraws=0,culledDraws=0;
inline Target lampMap;
inline float lampMatrix[16]{};
inline void release(bool programs=true){
 lampMap.release();drop(state);valid=attempted=false;builtGeneration=~0u;
 if(programs){drop(casterVS);drop(grassVS);drop(opaqueVS);drop(maskedVS);drop(casterPS);drop(declaration);}
 snapshot.clear();world_shadow_scene::cache.clear();world_shadow_scene::cacheBytes=0;
}
inline Vec unit(Vec v){float n=scene::distance(v,{});return n>.0001f?v*(1/n):Vec{0,0,1};}
inline float dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline void matrix(Vec position,Vec direction,float angle,float reach){
 const Vec z=unit(direction),x=unit(scene::cross(std::abs(z.z)>.98f?Vec{0,1,0}:Vec{0,0,1},z)),y=scene::cross(z,x);
 const float scale=1/std::tan(angle*.5f),depth=reach/(reach-1.f),along=dot(z,position);
 const float rows[16]={x.x*scale,x.y*scale,x.z*scale,-dot(x,position)*scale,
     y.x*scale,y.y*scale,y.z*scale,-dot(y,position)*scale,
     z.x*depth,z.y*depth,z.z*depth,-(along+1.f)*depth,z.x,z.y,z.z,-along};
 memcpy(lampMatrix,rows,sizeof rows);
}
inline ID3DBlob* compile(const char* source,const char* entry,const char* profile){
 ID3DBlob *code=nullptr,*messages=nullptr;++shaderCompiles;result=D3DCompile(source,strlen(source),nullptr,nullptr,nullptr,entry,profile,D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&messages);
 if(messages){strncpy_s(error,static_cast<const char*>(messages->GetBufferPointer()),_TRUNCATE);messages->Release();}if(FAILED(result))drop(code);return code;
}
inline bool prepare(IDirect3DDevice9* d){
 if(state)return true;if(attempted)return false;release(false);attempted=true;
 if(!casterVS||!grassVS||!opaqueVS||!maskedVS||!casterPS||!declaration){
 drop(casterVS);drop(grassVS);drop(opaqueVS);drop(maskedVS);drop(casterPS);drop(declaration);
 auto code=compile(lamp_shadow_shaders::caster,"vs","vs_3_0");if(!code)return false;result=d->CreateVertexShader(static_cast<const DWORD*>(code->GetBufferPointer()),&casterVS);drop(code);if(FAILED(result))return false;
 code=compile(lamp_shadow_shaders::caster,"grass","vs_3_0");if(!code)return false;result=d->CreateVertexShader(static_cast<const DWORD*>(code->GetBufferPointer()),&grassVS);drop(code);if(FAILED(result))return false;
 code=compile(lamp_shadow_shaders::caster,"opaque","vs_3_0");if(!code)return false;result=d->CreateVertexShader(static_cast<const DWORD*>(code->GetBufferPointer()),&opaqueVS);drop(code);if(FAILED(result))return false;
 code=compile(lamp_shadow_shaders::caster,"masked","vs_3_0");if(!code)return false;result=d->CreateVertexShader(static_cast<const DWORD*>(code->GetBufferPointer()),&maskedVS);drop(code);if(FAILED(result))return false;
 auto pixel=[&](const char* src,const char* entry,IDirect3DPixelShader9** out){auto c=compile(src,entry,"ps_3_0");if(!c)return false;result=d->CreatePixelShader(static_cast<const DWORD*>(c->GetBufferPointer()),out);drop(c);return SUCCEEDED(result);};
 if(!pixel(lamp_shadow_shaders::caster,"ps",&casterPS))return false;
 D3DVERTEXELEMENT9 layout[]={{0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},{0,12,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},{0,20,D3DDECLTYPE_FLOAT1,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,1},{0,24,D3DDECLTYPE_FLOAT1,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,2},D3DDECL_END()};
 result=d->CreateVertexDeclaration(layout,&declaration);if(FAILED(result))return false;
 }
 return SUCCEEDED(d->CreateStateBlock(D3DSBT_ALL,&state));
}
inline void sampler(IDirect3DDevice9* d,int slot,IDirect3DTexture9* tex){
 d->SetTexture(slot,tex);d->SetSamplerState(slot,D3DSAMP_MINFILTER,D3DTEXF_POINT);d->SetSamplerState(slot,D3DSAMP_MAGFILTER,D3DTEXF_POINT);d->SetSamplerState(slot,D3DSAMP_MIPFILTER,D3DTEXF_NONE);d->SetSamplerState(slot,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);d->SetSamplerState(slot,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);d->SetSamplerState(slot,D3DSAMP_SRGBTEXTURE,FALSE);
}
inline void baseline(IDirect3DDevice9* d){
 d->SetRenderState(D3DRS_CLIPPING,TRUE);
 for(unsigned stream=0;stream<16;++stream)d->SetStreamSourceFreq(stream,1);
 d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_STENCILENABLE,FALSE);d->SetRenderState(D3DRS_COLORWRITEENABLE,15);d->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);d->SetRenderState(D3DRS_CLIPPLANEENABLE,0);d->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);d->SetRenderState(D3DRS_DEPTHBIAS,0);d->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS,0);
}
inline bool map(IDirect3DDevice9* d,Target& target,const float* transform){
 if(!target.depth)return false;
 for(int i=0;i<9;i++)d->SetTexture(i,nullptr);
 d->SetDepthStencilSurface(nullptr);
 if(FAILED(d->SetRenderTarget(0,target.surface))||FAILED(d->SetDepthStencilSurface(target.depth)))return false;
 d->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS,TRUE);d->SetRenderState(D3DRS_MULTISAMPLEMASK,0xFFFFFFFF);
 D3DVIEWPORT9 vp=D3DVIEWPORT9{0,0,target.w,target.h,0,1};d->SetViewport(&vp);
 D3DRECT rect{LONG(vp.X),LONG(vp.Y),LONG(vp.X+vp.Width),LONG(vp.Y+vp.Height)};
 d->Clear(1,&rect,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xFFFFFFFF,1,0);
 d->SetVertexShader(casterVS);d->SetPixelShader(casterPS);d->SetVertexDeclaration(declaration);d->SetVertexShaderConstantF(0,transform,4);
 // D3D9 pixel centres are integer-valued; texture centres are half-integers.
 // Align rasterized depth with the receiver's standard [0,1] shadow UVs.
 float rasterOffset[4]={-1.f/vp.Width,1.f/vp.Height,0,0};d->SetVertexShaderConstantF(4,rasterOffset,1);
 d->SetRenderState(D3DRS_ZENABLE,TRUE);d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
 shadow_visibility::Frustum lightFrustum(transform);
 for(auto& b:snapshot.batches){if(!lightFrustum.intersects(b.centre.x,b.centre.y,b.centre.z,b.radius)){++culledDraws;continue;}++casterDraws;float alpha[4]={b.translucent?2.f:(b.alpha?1.f:0.f),b.threshold,b.opacity,0};d->SetPixelShaderConstantF(4,alpha,1);d->SetTexture(0,b.alpha);
  float pose[16]{};for(int r=0;r<3;r++){for(int c=0;c<3;c++)pose[r*4+c]=b.pose.r[r][c]*b.pose.scale;pose[r*4+3]=(&b.pose.p.x)[r];}pose[15]=1;d->SetVertexShaderConstantF(5,pose,4);
  d->SetVertexShader(b.gpu>=0?(b.alpha?maskedVS:opaqueVS):(b.grass>=0?grassVS:casterVS));
  d->SetVertexDeclaration(b.gpu>=0?snapshot.gpuMeshes[b.gpu].declaration:declaration);
  if(b.grass>=0){const auto& grass=snapshot.grasses[b.grass];d->SetVertexShaderConstantF(10,grass.wind,1);d->SetVertexShaderConstantF(11,grass.scale,1);d->SetVertexShaderConstantF(20,grass.positions.data(),228);}
  d->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP);d->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP);d->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
  if(b.gpu>=0){if(b.alpha){float material[4]={b.vertexAlpha?1.f:0.f,0,0,0};d->SetVertexShaderConstantF(9,material,1);}const auto& gpu=snapshot.gpuMeshes[b.gpu];
   d->SetStreamSource(0,gpu.vb,0,gpu.stride);d->SetIndices(gpu.ib);
   result=d->DrawIndexedPrimitive(gpu.topology,gpu.base,0,gpu.vertices,0,gpu.triangles);
   if(FAILED(result))return false;continue;
  }
  auto& mesh=*b.mesh;
  if(!b.skinned){if(!mesh.upload(d))return false;d->SetStreamSource(0,mesh.vb,0,sizeof(world_shadow_scene::Vertex));d->SetIndices(mesh.ib);result=d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,UINT(mesh.vertices.size()),0,b.indexCount/3);}
  else result=d->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,0,UINT(mesh.vertices.size()),b.indexCount/3,mesh.indices.data(),D3DFMT_INDEX16,mesh.vertices.data(),sizeof(world_shadow_scene::Vertex));
  if(FAILED(result))return false;
 }
 ++maps;return true;
}
inline void draw(IDirect3DDevice9* d){
 projected_lighting::Parameters p;
 {std::lock_guard<std::mutex> lock(projected_lighting::parameterMutex);p=projected_lighting::parameters;}
 if(!gameplay||!p.active){if(state||attempted)release(false);return;}
 const auto frame=generation.load();if(frame==builtGeneration)return;
 IDirect3DSurface9* ds=nullptr;d->GetDepthStencilSurface(&ds);
 if(!projected_depth_adapter::sampleTexture(ds)||!projected_lighting::worldTargetMatches(d,ds)){drop(ds);return;}
 builtGeneration=frame;valid=false;
 float camera[64]{},eye[4]{};
 if(!projected_lighting::camera(camera,eye)||!prepare(d)){drop(ds);++failures;return;}
 IDirect3DSurface9* rt[4]{};for(unsigned i=0;i<4;++i)d->GetRenderTarget(i,&rt[i]);
 if(!rt[0]){drop(ds);for(auto& t:rt)drop(t);return;}
 D3DVIEWPORT9 vp{};d->GetViewport(&vp);
 state->Capture();const bool began=SUCCEEDED(d->BeginScene());baseline(d);
 for(unsigned i=1;i<4;++i)d->SetRenderTarget(i,nullptr);
 Vec origin{p.origin[0],p.origin[1],p.origin[2]},viewer{eye[0],eye[1],eye[2]};
 if(world_shadow_scene::collect(snapshot,viewer,true,false,p.origin[3]+scene::distance(viewer,origin))){
  const Vec direction{p.direction[0],p.direction[1],p.direction[2]};
  matrix(origin-snapshot.origin,direction,std::acos(p.direction[3])*2.15f,p.origin[3]);
  valid=lampMap.create(d,512,512,D3DFMT_R32F,true)&&map(d,lampMap,lampMatrix);
 }
 d->SetDepthStencilSurface(nullptr);d->SetRenderTarget(0,rt[0]);for(unsigned i=1;i<4;++i)d->SetRenderTarget(i,rt[i]);
 d->SetDepthStencilSurface(ds);state->Apply();d->SetViewport(&vp);if(began)d->EndScene();
 drop(ds);for(auto& t:rt)drop(t);if(!valid)++failures;
}
// Called inside the existing flashlight's saved D3D state. Relative-eye
// projection keeps the shadow map's scene origin identical for either eye.
inline bool bindLamp(IDirect3DDevice9* d,const float* eye,float* transform){
 if(!gameplay||!valid||builtGeneration!=generation.load())return false;
 memcpy(transform,lampMatrix,64);float delta[4]={eye[0]-snapshot.origin.x,eye[1]-snapshot.origin.y,eye[2]-snapshot.origin.z,0};
 for(int r=0;r<4;r++)transform[r*4+3]+=lampMatrix[r*4]*delta[0]+lampMatrix[r*4+1]*delta[1]+lampMatrix[r*4+2]*delta[2];
 sampler(d,2,lampMap.texture);return true;
}
}
