#pragma once
#include <d3d9.h>
#include <atomic>
// Standard D3D9 interface hooks; select only the actual world HDR target's
// non-MSAA D24S8 surface. No resolution preset and no CPU depth readback.
namespace projected_depth_adapter {
inline IDirect3DDevice9* owner=nullptr;
inline std::atomic_bool requested{false};
struct EyeDepth {
 IDirect3DSurface9* original=nullptr;
 IDirect3DSurface9* surface=nullptr;
 IDirect3DTexture9* texture=nullptr;
 ULONGLONG lastUse=0;
 void release(){
  if(surface)surface->Release();
  if(texture)texture->Release();
  if(original)original->Release();
  *this={};
 }
};
// TTR can reuse one depth surface or alternate independent ones. Remember
// both active surfaces, including when one is replaced during a resize.
inline EyeDepth eyes[2]{};
inline ULONGLONG use=0;
inline IDirect3DSurface9* rejected=nullptr;
inline void (*beforeReset)()=nullptr;inline bool installed=false;inline unsigned swaps=0;inline HRESULT creation=E_FAIL;
using SetDepth=HRESULT(__stdcall*)(IDirect3DDevice9*,IDirect3DSurface9*);
using Reset=HRESULT(__stdcall*)(IDirect3DDevice9*,D3DPRESENT_PARAMETERS*);
inline SetDepth oldSet=nullptr;inline Reset oldReset=nullptr;
inline EyeDepth* byOriginal(IDirect3DSurface9* depth){
 for(auto& eye:eyes)if(eye.original==depth)return &eye;
 return nullptr;
}
inline IDirect3DTexture9* sampleTexture(IDirect3DSurface9* depth){
 if(depth)for(auto& eye:eyes)if(eye.surface==depth)return eye.texture;
 return nullptr;
}
inline unsigned activeSurfaces(){
 unsigned count=0;
 for(const auto& eye:eyes)if(eye.surface)++count;
 return count;
}
inline void release(){
 if(rejected){rejected->Release();rejected=nullptr;}
 for(auto& eye:eyes)eye.release();
 use=0;
}
inline HRESULT __stdcall set(IDirect3DDevice9* d,IDirect3DSurface9* depth){
 if(d==owner&&requested.load(std::memory_order_relaxed)&&depth){
  if(auto eye=byOriginal(depth)){
   auto hr=oldSet(d,eye->surface);
   if(SUCCEEDED(hr)){eye->lastUse=++use;++swaps;return hr;}
  }
 }
 return oldSet(d,depth);
}
inline HRESULT __stdcall reset(IDirect3DDevice9* d,D3DPRESENT_PARAMETERS* p){
 if(d==owner){oldSet(d,nullptr);if(beforeReset)beforeReset();release();}return oldReset(d,p);
}
inline void selectWorldDepth(IDirect3DDevice9* d){
 if(d!=owner)return;IDirect3DSurface9 *ds=nullptr,*rt=nullptr;
 if(FAILED(d->GetDepthStencilSurface(&ds))||!ds)return;
 if(sampleTexture(ds)||byOriginal(ds)||ds==rejected){ds->Release();return;}
 D3DSURFACE_DESC dd{},rd{};ds->GetDesc(&dd);d->GetRenderTarget(0,&rt);if(rt){rt->GetDesc(&rd);rt->Release();}
 if(rd.Format==D3DFMT_A16B16G16R16F&&dd.Width==rd.Width&&dd.Height==rd.Height&&dd.Width>=128&&dd.Height>=128&&dd.Format==D3DFMT_D24S8&&dd.MultiSampleType==D3DMULTISAMPLE_NONE){
  IDirect3DTexture9* next=nullptr;IDirect3DSurface9* level=nullptr;
  creation=d->CreateTexture(dd.Width,dd.Height,1,D3DUSAGE_DEPTHSTENCIL,(D3DFORMAT)MAKEFOURCC('I','N','T','Z'),D3DPOOL_DEFAULT,&next,nullptr);
  if(SUCCEEDED(creation))creation=next->GetSurfaceLevel(0,&level);
  if(SUCCEEDED(creation)){
   auto& slot=eyes[eyes[0].lastUse<=eyes[1].lastUse?0:1];
   slot.release();
   ds->AddRef();
   slot={ds,level,next,++use};
  }else{
   if(level)level->Release();if(next)next->Release();
   if(rejected)rejected->Release();rejected=ds;rejected->AddRef();
  }
 }
 ds->Release();
}
inline bool install(IDirect3DDevice9* d){
 if(installed)return d==owner;auto vt=*reinterpret_cast<void***>(d);DWORD a{},b{};
 if(!VirtualProtect(vt+16,sizeof(void*),PAGE_READWRITE,&a))return false;
 if(!VirtualProtect(vt+39,sizeof(void*),PAGE_READWRITE,&b)){DWORD ignored;VirtualProtect(vt+16,sizeof(void*),a,&ignored);return false;}
 oldSet=reinterpret_cast<SetDepth>(vt[39]);oldReset=reinterpret_cast<Reset>(vt[16]);owner=d;
 InterlockedExchangePointer(vt+39,reinterpret_cast<void*>(&set));InterlockedExchangePointer(vt+16,reinterpret_cast<void*>(&reset));
 DWORD ignored;VirtualProtect(vt+39,sizeof(void*),b,&ignored);VirtualProtect(vt+16,sizeof(void*),a,&ignored);installed=true;return true;
}
}
