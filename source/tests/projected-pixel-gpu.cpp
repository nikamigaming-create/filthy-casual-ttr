#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include "projected_lighting.hpp"
#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"user32.lib")
using namespace projected_lighting;
static constexpr unsigned W=128,H=96;
// Model the TTR eye-resolution wrapper. The game sees a fixed full viewport
// while the device rasterizes into the current input resolution.
using SetViewportFn=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,const D3DVIEWPORT9*);
static SetViewportFn nativeSetViewport=nullptr;
static D3DVIEWPORT9 physicalViewport{};
static HRESULT STDMETHODCALLTYPE remapViewport(IDirect3DDevice9* device,const D3DVIEWPORT9* logical){
 assert(logical->Width>=physicalViewport.Width && logical->Height>=physicalViewport.Height);
 return nativeSetViewport(device,&physicalViewport);
}
static bool rail(unsigned x,unsigned y){
 const int column=int(x),row=int(y);
 const bool frame=(column>=38&&column<43)||(column>=82&&column<87)||(row>=43&&row<48);
 const bool betweenPosts=column>=43&&column<82&&row>=48&&row<82;
 // Emulate the gaps in the bridge's alpha-tested diagonal railing.
 const int horizontal=column-62,vertical=row-65;
 const bool crossBrace=std::abs(horizontal-vertical)<2||std::abs(horizontal+vertical)<2;
 return frame||(betweenPosts&&crossBrace);
}
int main(int argc,char** argv){
 HWND win=CreateWindowExA(0,"STATIC","FC pixel regression",WS_POPUP,0,0,W,H,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);assert(win);
 auto api=Direct3DCreate9(D3D_SDK_VERSION);assert(api);D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=win;pp.BackBufferWidth=W;pp.BackBufferHeight=H;pp.BackBufferFormat=D3DFMT_A8R8G8B8;
 IDirect3DDevice9* d=nullptr;assert(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,win,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&d)));
 IDirect3DSurface9 *rt=nullptr,*upload=nullptr,*readback=nullptr;assert(SUCCEEDED(d->GetRenderTarget(0,&rt)));
 assert(SUCCEEDED(d->CreateOffscreenPlainSurface(W,H,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&upload,nullptr)));
 assert(SUCCEEDED(d->CreateOffscreenPlainSurface(W,H,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&readback,nullptr)));
 D3DLOCKED_RECT lock{};upload->LockRect(&lock,nullptr,0);for(unsigned y=0;y<H;y++)for(unsigned x=0;x<W;x++)reinterpret_cast<DWORD*>(static_cast<char*>(lock.pBits)+y*lock.Pitch)[x]=rail(x,y)?0xff202020:0xff000000;upload->UnlockRect();
 IDirect3DTexture9* depth=nullptr;
 assert(SUCCEEDED(d->CreateTexture(W,H,1,0,D3DFMT_R32F,D3DPOOL_MANAGED,&depth,nullptr)));
 depth->LockRect(0,&lock,nullptr,0);for(unsigned y=0;y<H;y++)for(unsigned x=0;x<W;x++)reinterpret_cast<float*>(static_cast<char*>(lock.pBits)+y*lock.Pitch)[x]=.5f;depth->UnlockRect(0);
 assert(prepare(d));
 if(argc>1){ID3DBlob* code=nullptr;ID3DBlob* errors=nullptr;wchar_t path[MAX_PATH];MultiByteToWideChar(CP_UTF8,0,argv[1],-1,path,MAX_PATH);auto hr=D3DCompileFromFile(path,nullptr,nullptr,"main","ps_3_0",0,0,&code,&errors);if(errors)errors->Release();assert(SUCCEEDED(hr));shader->Release();assert(SUCCEEDED(d->CreatePixelShader((DWORD*)code->GetBufferPointer(),&shader)));code->Release();}

 IDirect3DSurface9* auxiliary=nullptr;assert(SUCCEEDED(d->CreateRenderTarget(W,H,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,FALSE,&auxiliary,nullptr)));
 const D3DVIEWPORT9 cases[]={{0,0,W,H,0,1},{0,0,80,60,0,1},{16,12,80,60,0,1},{32,4,64,88,0,1}};
 for(auto vp:cases){
  assert(SUCCEEDED(d->UpdateSurface(upload,nullptr,rt,nullptr)));assert(copyScene(d));
  assert(SUCCEEDED(d->ColorFill(auxiliary,nullptr,0xff102030)));assert(SUCCEEDED(d->SetRenderTarget(1,auxiliary)));
  // Deliberately inherit unrelated material UV state, as the game can.
  D3DMATRIX uv{};uv._11=uv._22=uv._33=uv._44=1;uv._31=.25f;d->SetTransform(D3DTS_TEXTURE0,&uv);d->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
  float c[64]{};c[0]=c[5]=c[10]=c[15]=1;c[19]=1000;c[22]=1;c[23]=-1;c[24]=c[25]=c[26]=1;c[27]=-.9f;
  d->BeginScene();assert(composite(d,depth,c,vp));d->EndScene();assert(SUCCEEDED(d->GetRenderTargetData(rt,readback)));
  assert(SUCCEEDED(d->GetRenderTargetData(auxiliary,readback)));readback->LockRect(&lock,nullptr,D3DLOCK_READONLY);unsigned auxiliaryChanged=0;
  for(unsigned y=0;y<H;y++)for(unsigned x=0;x<W;x++)if(reinterpret_cast<DWORD*>(static_cast<char*>(lock.pBits)+y*lock.Pitch)[x]!=0xff102030)++auxiliaryChanged;
  readback->UnlockRect();printf("auxiliary target changed=%u\n",auxiliaryChanged);if(auxiliaryChanged){puts("FAIL flashlight corrupted another render target");return 3;}
  assert(SUCCEEDED(d->GetRenderTargetData(rt,readback)));
  readback->LockRect(&lock,nullptr,D3DLOCK_READONLY);unsigned misplaced=0,lit=0,outside=0;
  for(unsigned y=0;y<H;y++)for(unsigned x=0;x<W;x++){
   unsigned value=reinterpret_cast<DWORD*>(static_cast<char*>(lock.pBits)+y*lock.Pitch)[x]&255;bool inside=x>=vp.X&&x<vp.X+vp.Width&&y>=vp.Y&&y<vp.Y+vp.Height;
   if(!rail(x,y)&&value)++misplaced;if(inside&&rail(x,y)&&value>32)++lit;if(!inside&&value!=(rail(x,y)?32:0))++outside;
  }readback->UnlockRect();printf("viewport %u,%u %ux%u misplaced=%u lit=%u outside=%u\n",vp.X,vp.Y,vp.Width,vp.Height,misplaced,lit,outside);if(misplaced||!lit||outside){puts("FAIL displaced or missing illumination");return 2;}
 }
 // Check actual beam aim as well as matching color/depth texture addresses.
 // The same 80-unit surface point must be relit at its projected each-eye
 // position when the viewport shrinks or moves within the backing texture.
 constexpr float nearZ=2.f,farZ=200.f,wallZ=80.f;
 constexpr float zScale=farZ/(farZ-nearZ),zOffset=-nearZ*zScale;
 constexpr float wallDepth=zScale+zOffset/wallZ;
 const float clip[16]={1,0,0,0,0,1,0,0,0,0,zScale,zOffset,0,0,1,0};
 d->SetRenderTarget(1,nullptr);
 depth->LockRect(0,&lock,nullptr,0);
 for(unsigned y=0;y<H;y++)for(unsigned x=0;x<W;x++)
  reinterpret_cast<float*>(static_cast<char*>(lock.pBits)+y*lock.Pitch)[x]=wallDepth;
 depth->UnlockRect(0);
 for(const auto& vp:cases)for(float eye:{-1.2f,1.2f}){
  assert(SUCCEEDED(d->ColorFill(rt,nullptr,0xff202020)));assert(copyScene(d));
  float c[64]{};assert(inverse(clip,c));
  c[16]=-eye;c[18]=8.f;c[19]=250.f;
  const float aim[3]={32.f,16.f,wallZ-c[18]};
  const float length=std::sqrt(aim[0]*aim[0]+aim[1]*aim[1]+aim[2]*aim[2]);
  for(unsigned i=0;i<3;i++)c[20+i]=aim[i]/length;
  c[23]=std::cos(.075f);c[24]=c[25]=c[26]=1.f;c[27]=std::cos(.027f);
  d->BeginScene();assert(composite(d,depth,c,vp));d->EndScene();
  assert(SUCCEEDED(d->GetRenderTargetData(rt,readback)));
  readback->LockRect(&lock,nullptr,D3DLOCK_READONLY);
  double total=0,xTotal=0,yTotal=0;
  for(unsigned y=0;y<H;y++)for(unsigned x=0;x<W;x++){
   const DWORD pixel=reinterpret_cast<DWORD*>(static_cast<char*>(lock.pBits)+y*lock.Pitch)[x]&255u;
   const double gain=std::max(0,int(pixel)-32);
   total+=gain;xTotal+=(x+.5)*gain;yTotal+=(y+.5)*gain;
  }
  readback->UnlockRect();
  const double expectedX=vp.X+vp.Width*(1+(32.f-eye)/wallZ)/2;
  const double expectedY=vp.Y+vp.Height*(1-16.f/wallZ)/2;
  if(!total)return 5;
  printf("viewport %ux%u eye %.1f wall hit (%.2f, %.2f) expected (%.2f, %.2f)\n",
         vp.Width,vp.Height,eye,xTotal/total,yTotal/total,expectedX,expectedY);
  if(std::abs(xTotal/total-expectedX)>1.5||std::abs(yTotal/total-expectedY)>1.5)return 6;
 }
 d->SetRenderTarget(1,nullptr);
 // Keep the game's reported viewport fixed while resizing the real color
 // and depth resources in both directions without reloading the shader.
 // Triangulate the apparent endpoint from the two measured eye centroids.
 void** methods=*reinterpret_cast<void***>(d);
 DWORD protect{},ignored{};
 assert(VirtualProtect(methods+47,sizeof(void*),PAGE_READWRITE,&protect));
 nativeSetViewport=reinterpret_cast<SetViewportFn>(methods[47]);
 InterlockedExchangePointer(methods+47,reinterpret_cast<void*>(&remapViewport));
 const D3DVIEWPORT9 logical{0,0,512,384,0,1};
 bool scalePass=true;
 for(unsigned width:{256u,384u,512u,384u,256u}){
  const unsigned height=width*logical.Height/logical.Width;
  physicalViewport={0,0,width,height,0,1};
  IDirect3DSurface9 *eyeColor=nullptr,*eyeReadback=nullptr;
  IDirect3DTexture9* eyeDepth=nullptr;
  assert(SUCCEEDED(d->CreateRenderTarget(width,height,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,FALSE,&eyeColor,nullptr)));
  assert(SUCCEEDED(d->CreateOffscreenPlainSurface(width,height,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&eyeReadback,nullptr)));
  assert(SUCCEEDED(d->CreateTexture(width,height,1,0,D3DFMT_R32F,D3DPOOL_MANAGED,&eyeDepth,nullptr)));
  assert(SUCCEEDED(eyeDepth->LockRect(0,&lock,nullptr,0)));
  for(unsigned y=0;y<height;++y)
   std::fill_n(reinterpret_cast<float*>(static_cast<char*>(lock.pBits)+y*lock.Pitch),width,wallDepth);
  eyeDepth->UnlockRect(0);
  assert(SUCCEEDED(d->SetRenderTarget(0,eyeColor)));
  double centres[2]{};
  for(int i=0;i<2;++i){
   const float eye=i?3.f:-3.f;
   assert(SUCCEEDED(d->ColorFill(eyeColor,nullptr,0xff202020)));assert(copyScene(d));
   const auto physical=sceneDesc;
   sceneDesc.Width=logical.Width;sceneDesc.Height=logical.Height;
   float c[64]{};assert(inverse(clip,c));
   c[16]=-eye;c[18]=8;c[19]=250;
   constexpr float targetX=-24,targetY=16;
   const float direction[3]={targetX,targetY,wallZ-8};
   const float length=std::sqrt(targetX*targetX+targetY*targetY+(wallZ-8)*(wallZ-8));
   for(int j=0;j<3;++j)c[20+j]=direction[j]/length;
   c[23]=std::cos(.075f);c[24]=c[25]=c[26]=1;c[27]=std::cos(.027f);
   d->BeginScene();assert(composite(d,eyeDepth,c,logical));d->EndScene();
   sceneDesc=physical;
   assert(SUCCEEDED(d->GetRenderTargetData(eyeColor,eyeReadback)));
   assert(SUCCEEDED(eyeReadback->LockRect(&lock,nullptr,D3DLOCK_READONLY)));
   double total=0,sx=0,sy=0;
   for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
    const auto pixel=reinterpret_cast<DWORD*>(static_cast<char*>(lock.pBits)+y*lock.Pitch)[x]&255;
    const double gain=std::max(0,int(pixel)-32);
    total+=gain;sx+=(x+.5)*gain;sy+=(y+.5)*gain;
   }
   eyeReadback->UnlockRect();
   centres[i]=total?sx/total:0;
   // A perspective cone has a small asymmetric footprint on the wall.
   scalePass &= total>0 && std::abs(centres[i]-width*.5*(1+(targetX-eye)/wallZ))<1 &&
                std::abs((total?sy/total:0)-height*.5*(1-targetY/wallZ))<1;
  }
  const double disparity=centres[0]-centres[1];
  const double distance=disparity>0?width*.5*6/disparity:0;
  printf("logical %ux%u raster %ux%u spot L/R %.2f/%.2f stereo distance %.2f (wall %.0f)\n",
          logical.Width,logical.Height,width,height,centres[0],centres[1],distance,wallZ);
  scalePass &= std::abs(distance-wallZ)<.5;
  eyeReadback->Release();eyeDepth->Release();eyeColor->Release();
 }
 InterlockedExchangePointer(methods+47,reinterpret_cast<void*>(nativeSetViewport));
 assert(VirtualProtect(methods+47,sizeof(void*),protect,&ignored));
 if(!scalePass)return 7;
 assert(SUCCEEDED(d->SetRenderTarget(0,rt)));
 // Move a wrist light across both eye positions. Alternate clear sky and
 // a receiving wall. A near wrist hit must never illuminate an empty pixel.
 D3DVIEWPORT9 full{0,0,W,H,0,1};
 depth->LockRect(0,&lock,nullptr,0);
 for(unsigned y=0;y<H;++y)for(unsigned x=0;x<W;++x)
  reinterpret_cast<float*>(static_cast<char*>(lock.pBits)+y*lock.Pitch)[x]=x<W/2?1.f:.8f;
 depth->UnlockRect(0);
 for(float eyeOffset:{-.6f,.6f})for(float nearHit:{12.f,240.f}){
  assert(SUCCEEDED(d->ColorFill(rt,nullptr,0xff202020)));assert(copyScene(d));
  float c[64]{};c[0]=c[5]=4;c[10]=100;c[15]=1;c[16]=eyeOffset;c[18]=1;c[19]=300;
  c[22]=1;c[23]=.93f;c[24]=c[25]=c[26]=1;c[27]=.98f;
  c[31]=nearHit; // Former air-shaft length. Surface lighting is independent of it.
  d->BeginScene();assert(composite(d,depth,c,full));d->EndScene();
  assert(SUCCEEDED(d->GetRenderTargetData(rt,readback)));readback->LockRect(&lock,nullptr,D3DLOCK_READONLY);
  unsigned emptyChanged=0,wallLit=0;
  for(unsigned y=0;y<H;++y)for(unsigned x=0;x<W;++x){
   const DWORD v=reinterpret_cast<DWORD*>(static_cast<char*>(lock.pBits)+y*lock.Pitch)[x]&255;
   if(x<W/2)emptyChanged+=(v!=32);else wallLit+=(v>32);
  }
  readback->UnlockRect();printf("eye %.1f wrist hit %.0f empty changed %u wall lit %u\n",eyeOffset,nearHit,emptyChanged,wallLit);
  if(emptyChanged||wallLit!=W/2*H)return 4;
 }
 d->SetRenderTarget(1,nullptr);auxiliary->Release();release();depth->Release();readback->Release();upload->Release();rt->Release();d->Release();api->Release();DestroyWindow(win);
 puts("PASS actual flashlight shader preserves geometry pixel positions across full, reduced and offset viewports");
}
