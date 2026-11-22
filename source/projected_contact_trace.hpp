#pragma once
// Opt-in readback of a 3x3 patch at the native wrist-ray hit. Called within
// the projector's saved D3D state, after lighting the original scene.
namespace projected_lighting::contact_trace {
inline void (*log)(const char*,...)=nullptr;
inline IDirect3DPixelShader9* shader=nullptr;
inline IDirect3DSurface9 *target=nullptr,*readback=nullptr;
inline bool attempted=false;
inline ULONGLONG next=0;
inline unsigned samples=0;
inline IDirect3DQuery9* coverage=nullptr;
inline D3DVIEWPORT9 coverageViewport{};
inline bool coveragePending=false,coverageUnavailable=false;
inline ULONGLONG nextCoverage=0;
inline unsigned coverageReports=0;

inline void release(){
 if(shader)shader->Release();if(target)target->Release();if(readback)readback->Release();
 shader=nullptr;target=readback=nullptr;attempted=false;
 if(coverage)coverage->Release();
 coverage=nullptr;coveragePending=coverageUnavailable=false;nextCoverage=0;
}

// Count the actual samples covered by the projector. Its non-MSAA pass has
// no depth test or discards, so this measures the raster behind TTR's getter.
inline bool beginCoverage(IDirect3DDevice9* d,const D3DVIEWPORT9& vp){
 if(!log||coverageUnavailable||coverageReports>=12)return false;
 if(coveragePending){
  DWORD pixels=0;
  HRESULT result=coverage->GetData(&pixels,sizeof(pixels),0);
  if(result==S_FALSE)return false;
  coveragePending=false;
  if(FAILED(result)){coverageUnavailable=true;return false;}
  log("WRIST RASTER logical=%ux%u coveredPixels=%lu linearScale=%.4f",
      coverageViewport.Width,coverageViewport.Height,pixels,
      std::sqrt(double(pixels)/(double(coverageViewport.Width)*coverageViewport.Height)));
  ++coverageReports;
 }
 const auto now=GetTickCount64();
 if(now<nextCoverage||!vp.Width||!vp.Height||coverageReports>=12)return false;
 if(!coverage&&FAILED(d->CreateQuery(D3DQUERYTYPE_OCCLUSION,&coverage))){coverageUnavailable=true;return false;}
 coverageViewport=vp;nextCoverage=now+5000;
 return SUCCEEDED(coverage->Issue(D3DISSUE_BEGIN));
}
inline void endCoverage(){
 coveragePending=SUCCEEDED(coverage->Issue(D3DISSUE_END));
}

inline bool prepare(IDirect3DDevice9* d){
 if(attempted)return shader&&target&&readback;
 attempted=true;
 const char source[]="sampler2D Depth:register(s0); float4 main(float2 uv:TEXCOORD0):COLOR0{return tex2D(Depth,uv).rrrr;}";
 ID3DBlob *code=nullptr,*errors=nullptr;
 HRESULT hr=D3DCompile(source,sizeof(source)-1,nullptr,nullptr,nullptr,"main","ps_3_0",0,0,&code,&errors);
 if(FAILED(hr)&&errors)log("WRIST PROJECTION trace shader: %.200s",static_cast<const char*>(errors->GetBufferPointer()));
 if(errors)errors->Release();
 const char* stage="compile";
 if(SUCCEEDED(hr)){stage="shader";hr=d->CreatePixelShader(static_cast<DWORD*>(code->GetBufferPointer()),&shader);}
 if(code)code->Release();
 if(SUCCEEDED(hr)){stage="target";hr=d->CreateRenderTarget(4,4,D3DFMT_R32F,D3DMULTISAMPLE_NONE,0,FALSE,&target,nullptr);}
 if(SUCCEEDED(hr)){stage="readback";hr=d->CreateOffscreenPlainSurface(4,4,D3DFMT_R32F,D3DPOOL_SYSTEMMEM,&readback,nullptr);}
 if(FAILED(hr))log("WRIST PROJECTION trace %s failed hr=%08lx",stage,hr);
 return SUCCEEDED(hr);
}

inline void sample(IDirect3DDevice9* d,const Parameters& p,const float* inv,const float* eye,
                   const D3DVIEWPORT9& vp,const D3DSURFACE_DESC& td){
 if(!log||!p.hit||samples>=48)return;
 const auto now=GetTickCount64();if(now<next)return;
 float clip[16]{},h[4]{};
 if(!inverse(inv,clip))return;
 for(int r=0;r<4;r++){
  h[r]=clip[r*4+3];
  for(int c=0;c<3;c++)h[r]+=clip[r*4+c]*(p.contact[c]-eye[c]);
 }
 if(!(h[3]>0&&vp.Width>=3&&vp.Height>=3))return;
 const float nx=h[0]/h[3],ny=h[1]/h[3],nz=h[2]/h[3];
 if(!(std::abs(nx)<1&&std::abs(ny)<1&&nz>0&&nz<1))return;
 const int x=int(vp.X+(nx+1)*.5f*vp.Width),y=int(vp.Y+(1-ny)*.5f*vp.Height);
 if(x<=int(vp.X)||y<=int(vp.Y)||x+1>=int(vp.X+vp.Width)||y+1>=int(vp.Y+vp.Height))return;
 next=now+1809;++samples;if(!prepare(d))return;

 // The projector has already set up point sampling on the live eye depth.
 // Keep its scene and auxiliary outputs alive while the tiny readback runs.
 IDirect3DSurface9* bound[4]{};
 for(unsigned i=0;i<4;i++)d->GetRenderTarget(i,&bound[i]);
 for(unsigned i=1;i<4;i++)d->SetRenderTarget(i,nullptr);
 HRESULT hr=d->SetRenderTarget(0,target);
 float depth[9]{};
 if(SUCCEEDED(hr)){
  const D3DVIEWPORT9 tiny{0,0,3,3,0,1};d->SetViewport(&tiny);
  d->SetVertexShader(nullptr);d->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
  d->SetPixelShader(shader);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
  d->SetRenderState(D3DRS_COLORWRITEENABLE,15);
  const float l=float(x-1)/td.Width,r=float(x+2)/td.Width;
  const float t=float(y-1)/td.Height,b=float(y+2)/td.Height;
  const float quad[4][6]={{-.5f,-.5f,0,1,l,t},{2.5f,-.5f,0,1,r,t},{-.5f,2.5f,0,1,l,b},{2.5f,2.5f,0,1,r,b}};
  hr=d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,quad,sizeof(quad[0]));
  if(SUCCEEDED(hr))hr=d->GetRenderTargetData(target,readback);
  D3DLOCKED_RECT lock{};
  if(SUCCEEDED(hr))hr=readback->LockRect(&lock,nullptr,D3DLOCK_READONLY);
  if(SUCCEEDED(hr)){
   for(int row=0;row<3;row++)memcpy(depth+row*3,static_cast<char*>(lock.pBits)+row*lock.Pitch,3*sizeof(float));
   readback->UnlockRect();
  }
 }
 for(unsigned i=0;i<4;i++){d->SetRenderTarget(i,bound[i]);if(bound[i])bound[i]->Release();}
 if(FAILED(hr)){log("WRIST PROJECTION trace sample failed hr=%08lx",hr);return;}

 char settings[MAX_PATH]{},resolution[16]{},upscaler[16]{};
 if(GetFullPathNameA("Data\\NVSE\\Plugins\\TTWVR\\TTWVR.ini",MAX_PATH,settings,nullptr)<MAX_PATH){
  GetPrivateProfileStringA("VR","Resolution","?",resolution,sizeof(resolution),settings);
  GetPrivateProfileStringA("VR","Upscaler","?",upscaler,sizeof(upscaler),settings);
 }
 const float range=vp.MaxZ-vp.MinZ;
 const float centre=(depth[4]-vp.MinZ)/range;
 const float invW=inv[12]*nx+inv[13]*ny+inv[14]*centre+inv[15];
 const auto bounds=std::minmax_element(depth,depth+9);
 log("WRIST PROJECTION preset=%s upscaler=%s target=%ux%u viewport=%u,%u,%u,%u depthSurfaces=%u pixel=%d,%d "
     "hit=(%.2f,%.2f,%.2f) aim=(%.3f,%.3f,%.3f) eye=(%.2f,%.2f,%.2f) "
     "eyeDepth=%.2f sampledEyeDepth=%.2f expectedD24=%.8f sampledD24=%.8f range=[%.8f,%.8f]",
     resolution,upscaler,td.Width,td.Height,vp.X,vp.Y,vp.Width,vp.Height,projected_depth_adapter::activeSurfaces(),x,y,
     p.contact[0],p.contact[1],p.contact[2],p.direction[0],p.direction[1],p.direction[2],eye[0],eye[1],eye[2],
     h[3],invW>0?1.f/invW:0.f,vp.MinZ+nz*range,depth[4],*bounds.first,*bounds.second);
}
}
