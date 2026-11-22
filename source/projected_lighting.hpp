#pragma once
#include <d3d9.h>
#include <d3dcompiler.h>
#include <mutex>
#include <algorithm>
#include <cmath>
#include <vector>
#include "projected_depth_adapter.hpp"
#pragma comment(lib,"d3dcompiler.lib")
namespace projected_lighting {
struct Parameters {float origin[4]{},direction[4]{},colour[4]{},contact[3]{};bool active=false,hit=false;};
inline Parameters parameters;inline std::mutex parameterMutex;
inline bool enabled=false;inline bool ready=false;inline bool measureGpu=false;inline IDirect3DPixelShader9* shader=nullptr;inline IDirect3DStateBlock9* state=nullptr;
inline IDirect3DVertexShader9* screenTriangle=nullptr;
inline IDirect3DTexture9* sceneColour=nullptr;inline IDirect3DSurface9* sceneSurface=nullptr;inline D3DSURFACE_DESC sceneDesc{};
inline double cpuTotalUs=0,cpuMaxUs=0;inline unsigned cpuSamples=0;
inline unsigned draws=0,failures=0;inline HRESULT lastResult=S_OK;
inline unsigned shaderCompiles=0;
inline bool (*lampShadow)(IDirect3DDevice9*,const float*,float*)=nullptr;
inline IDirect3DQuery9 *gpuStart=nullptr,*gpuEnd=nullptr,*gpuFrequency=nullptr;inline bool gpuPending=false,gpuAttempted=false;
inline double gpuTotalMs=0,gpuMaxMs=0;inline unsigned gpuSamples=0;
inline void pollGpu(IDirect3DDevice9* d){
 if(!gpuAttempted){gpuAttempted=true;d->CreateQuery(D3DQUERYTYPE_TIMESTAMP,&gpuStart);d->CreateQuery(D3DQUERYTYPE_TIMESTAMP,&gpuEnd);d->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ,&gpuFrequency);}
 if(gpuPending){UINT64 a{},b{},f{};if(gpuStart->GetData(&a,sizeof(a),0)==S_OK&&gpuEnd->GetData(&b,sizeof(b),0)==S_OK&&gpuFrequency->GetData(&f,sizeof(f),0)==S_OK){gpuPending=false;if(f&&b>=a){double ms=double(b-a)*1000/double(f);gpuTotalMs+=ms;gpuMaxMs=std::max(gpuMaxMs,ms);++gpuSamples;}}}
}
inline void publish(bool active,const float* pos,const float* dir,float range,float degrees,const float* rgb,float brightness,const float* contact=nullptr){
 std::lock_guard<std::mutex> lock(parameterMutex);parameters.active=active;
 if(!active)return;for(int i=0;i<3;i++){parameters.origin[i]=pos[i];parameters.direction[i]=dir[i];parameters.colour[i]=rgb[i]*brightness;}
 parameters.hit=contact!=nullptr;if(contact)memcpy(parameters.contact,contact,sizeof(parameters.contact));
 parameters.origin[3]=range;parameters.direction[3]=cosf(degrees*.5f*.0174532925f);parameters.colour[3]=cosf(degrees*.14f*.0174532925f);
}
inline bool inverse(const float* source,float* result){
 double a[4][8]{};for(int r=0;r<4;r++)for(int c=0;c<4;c++){a[r][c]=source[r*4+c];a[r][c+4]=r==c?1:0;}
 for(int c=0;c<4;c++){int pivot=c;for(int r=c+1;r<4;r++)if(fabs(a[r][c])>fabs(a[pivot][c]))pivot=r;if(fabs(a[pivot][c])<1e-10)return false;
  for(int j=0;j<8;j++)std::swap(a[c][j],a[pivot][j]);double divisor=a[c][c];for(int j=0;j<8;j++)a[c][j]/=divisor;
  for(int r=0;r<4;r++)if(r!=c){double f=a[r][c];for(int j=0;j<8;j++)a[r][j]-=f*a[c][j];}
 }for(int r=0;r<4;r++)for(int c=0;c<4;c++)result[r*4+c]=float(a[r][c+4]);return true;
}
}
#include "projected_camera_adapter.hpp"
#include "projected_contact_trace.hpp"
namespace projected_lighting {
inline void release(){ready=false;
 contact_trace::release();
 if(gpuStart)gpuStart->Release();if(gpuEnd)gpuEnd->Release();if(gpuFrequency)gpuFrequency->Release();gpuStart=gpuEnd=gpuFrequency=nullptr;gpuPending=gpuAttempted=false;
 if(sceneSurface)sceneSurface->Release();if(sceneColour)sceneColour->Release();sceneSurface=nullptr;sceneColour=nullptr;sceneDesc={};
 if(state)state->Release();if(shader)shader->Release();if(screenTriangle)screenTriangle->Release();state=nullptr;shader=nullptr;screenTriangle=nullptr;
}
inline bool prepare(IDirect3DDevice9* d){
 if(shader&&screenTriangle&&state)return true;
 if(!screenTriangle){
  static const char vertexSource[]=R"HLSL(
   float4 main(float3 corner:POSITION,out float2 eyeUV:TEXCOORD0):POSITION{
    eyeUV=corner.xy;
    return float4(corner.x*2-1,1-corner.y*2,0,1);
   }
  )HLSL";
  ID3DBlob* code=nullptr;
  lastResult=D3DCompile(vertexSource,sizeof(vertexSource)-1,nullptr,nullptr,nullptr,"main","vs_3_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,nullptr);
  if(SUCCEEDED(lastResult)){lastResult=d->CreateVertexShader(static_cast<DWORD*>(code->GetBufferPointer()),&screenTriangle);code->Release();}
  if(FAILED(lastResult))return false;
 }
 if(shader){shader->Release();shader=nullptr;}if(state){state->Release();state=nullptr;}
 static const char source[]=R"HLSL(
 sampler2D Depth:register(s0);sampler2D Scene:register(s1);sampler2D Shadow:register(s2);
 row_major float4x4 InverseClip:register(c0);
 float4 Lamp:register(c4);float4 Aim:register(c5);float4 Tint:register(c6);
 row_major float4x4 ShadowClip:register(c8);float4 ShadowMode:register(c12);
 // TTR can report a logical viewport and rasterize into a scaled eye.
 // Interpolation spans the actual raster. Its derivatives are one physical
 // pixel, including when the reported viewport stays at the game's size.
 float4 TextureRect:register(c13);float4 DepthRange:register(c15);
 float4 main(float2 eyeUV:TEXCOORD0):COLOR0{
  float2 uv=eyeUV+.5*float2(ddx(eyeUV.x),ddy(eyeUV.y));
  float2 sampleUV=TextureRect.xy+uv*TextureRect.zw;
  float z=(tex2D(Depth,sampleUV).r-DepthRange.x)*DepthRange.y;
  // The clear depth has no receiving surface.
  if(z<=0 || z>=.999999)return 0;
  float4 h=mul(InverseClip,float4(uv.x*2-1,1-uv.y*2,z,1));
  if(h.w<=.000001)return 0;
  float3 world=h.xyz/h.w;
  float3 offset=world-Lamp.xyz;float distance=max(length(offset),.001);float3 ray=offset/distance;
  float cone=smoothstep(Aim.w,Tint.w,dot(ray,Aim.xyz));
  // The source image already carries authored material normals and shading.
  // Depth derivatives would reveal coarse triangles and silhouette discontinuities.
  // Apply a smooth projected exposure only; never synthesize edge normals here.
  float attenuation=(1-smoothstep(.72,1,distance/Lamp.w))/(1+pow(distance/1000,2));
  float gain=cone*attenuation*.85;
  if(gain<=0)return 0;
  if(ShadowMode.x>0){float4 q=mul(ShadowClip,float4(world,1));float3 p=q.xyz/max(q.w,.00001);float2 uvShadow=p.xy*float2(.5,-.5)+.5;
   if(q.w>0&&p.z>0&&p.z<1&&min(uvShadow.x,uvShadow.y)>0&&max(uvShadow.x,uvShadow.y)<1){
    float visible=0;float bias=.0007;
    visible+=(p.z-bias<=tex2D(Shadow,uvShadow+float2(-.5,-.5)/512).r);
    visible+=(p.z-bias<=tex2D(Shadow,uvShadow+float2(.5,-.5)/512).r);
    visible+=(p.z-bias<=tex2D(Shadow,uvShadow+float2(-.5,.5)/512).r);
    visible+=(p.z-bias<=tex2D(Shadow,uvShadow+float2(.5,.5)/512).r);gain*=visible*.25;
   }
  }
  float3 base=max(tex2D(Scene,sampleUV).rgb,0);
  float peak=max(base.r,max(base.g,base.b));
  // Bounded relighting: brighten dark texture detail without whitening bright ground.
  float3 added=base/(.035+peak)*Tint.rgb*gain;
  // Preserve existing bright rims/specular highlights instead of amplifying
  // them into a traced white contour. A soft shoulder avoids a hard clamp.
  float headroom=max(.65-peak,0);
  added*=headroom/(headroom+max(added.r,max(added.g,added.b))+.00001);
  return float4(added,0);
 }
 )HLSL";
 ID3DBlob *code=nullptr,*error=nullptr;
 ++shaderCompiles;lastResult=D3DCompile(source,sizeof(source)-1,nullptr,nullptr,nullptr,"main","ps_3_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&error);
 if(error)error->Release();if(SUCCEEDED(lastResult))lastResult=d->CreatePixelShader(reinterpret_cast<DWORD*>(code->GetBufferPointer()),&shader);if(code)code->Release();
 if(SUCCEEDED(lastResult))lastResult=d->CreateStateBlock(D3DSBT_ALL,&state);
 return SUCCEEDED(lastResult);
}
inline bool copyScene(IDirect3DDevice9* d){
 IDirect3DSurface9* rt=nullptr;if(FAILED(d->GetRenderTarget(0,&rt))||!rt)return false;
 D3DSURFACE_DESC desc{};
 if(FAILED(rt->GetDesc(&desc))){rt->Release();return false;}
 if(!sceneColour||desc.Width!=sceneDesc.Width||desc.Height!=sceneDesc.Height||desc.Format!=sceneDesc.Format){
  if(sceneSurface)sceneSurface->Release();if(sceneColour)sceneColour->Release();sceneSurface=nullptr;sceneColour=nullptr;
  lastResult=d->CreateTexture(desc.Width,desc.Height,1,D3DUSAGE_RENDERTARGET,desc.Format,D3DPOOL_DEFAULT,&sceneColour,nullptr);
  if(SUCCEEDED(lastResult))lastResult=sceneColour->GetSurfaceLevel(0,&sceneSurface);
  sceneDesc=desc;
 }
 if(sceneSurface)lastResult=d->StretchRect(rt,nullptr,sceneSurface,nullptr,D3DTEXF_NONE);
 rt->Release();return sceneSurface&&SUCCEEDED(lastResult);
}
inline bool composite(IDirect3DDevice9* d,IDirect3DTexture9* depth,float* constants,const D3DVIEWPORT9& vp){
 if(!sceneDesc.Width||!sceneDesc.Height||!vp.Width||!vp.Height||vp.X>sceneDesc.Width||vp.Y>sceneDesc.Height||vp.Width>sceneDesc.Width-vp.X||vp.Height>sceneDesc.Height-vp.Y||vp.MaxZ<=vp.MinZ)return false;
 constants[52]=float(vp.X)/sceneDesc.Width;constants[53]=float(vp.Y)/sceneDesc.Height;
 constants[54]=float(vp.Width)/sceneDesc.Width;constants[55]=float(vp.Height)/sceneDesc.Height;
 constants[60]=vp.MinZ;constants[61]=1.f/(vp.MaxZ-vp.MinZ);
 d->SetDepthStencilSurface(nullptr);d->SetViewport(&vp);
 d->SetRenderState(D3DRS_STENCILENABLE,FALSE);d->SetRenderState(D3DRS_CLIPPLANEENABLE,0);d->SetRenderState(D3DRS_FOGENABLE,FALSE);d->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
 for(unsigned i=0;i<2;++i){d->SetTextureStageState(i,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);d->SetTextureStageState(i,D3DTSS_TEXCOORDINDEX,i);}
 d->SetVertexShader(screenTriangle);d->SetPixelShader(shader);d->SetPixelShaderConstantF(0,constants,16);
 d->SetFVF(D3DFVF_XYZ);d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
 d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);d->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
 // Preserve all auxiliary outputs (for example the temporal velocity buffer).
 d->SetRenderState(D3DRS_COLORWRITEENABLE1,0);
 d->SetRenderState(D3DRS_COLORWRITEENABLE2,0);
 d->SetRenderState(D3DRS_COLORWRITEENABLE3,0);
 d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_COLORWRITEENABLE,7);d->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);
 d->SetTexture(0,depth);d->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT);d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);d->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE);d->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE);d->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);d->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
 d->SetTexture(1,sceneColour);d->SetSamplerState(1,D3DSAMP_MINFILTER,D3DTEXF_POINT);d->SetSamplerState(1,D3DSAMP_MAGFILTER,D3DTEXF_POINT);d->SetSamplerState(1,D3DSAMP_MIPFILTER,D3DTEXF_NONE);d->SetSamplerState(1,D3DSAMP_SRGBTEXTURE,FALSE);d->SetSamplerState(1,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);d->SetSamplerState(1,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
 static const float corners[3][3]={{0,0,0},{2,0,0},{0,2,0}};
 lastResult=d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,1,corners,sizeof(corners[0]));ready=SUCCEEDED(lastResult);if(ready)++draws;else ++failures;
 return ready;
}
inline bool worldTargetMatches(IDirect3DDevice9* d,IDirect3DSurface9* depth){
 IDirect3DSurface9* rt=nullptr;
 if(FAILED(d->GetRenderTarget(0,&rt))||!rt)return false;
 D3DSURFACE_DESC colour{},stencil{};
 const bool matches=SUCCEEDED(rt->GetDesc(&colour))&&SUCCEEDED(depth->GetDesc(&stencil))&&
  colour.Format==D3DFMT_A16B16G16R16F&&colour.Width==stencil.Width&&colour.Height==stencil.Height&&
  colour.MultiSampleType==D3DMULTISAMPLE_NONE&&stencil.MultiSampleType==D3DMULTISAMPLE_NONE;
 rt->Release();
 return matches;
}
inline void draw(IDirect3DDevice9* d){
 if(!enabled)return;Parameters p;{std::lock_guard<std::mutex> lock(parameterMutex);p=parameters;}if(!p.active)return;
 IDirect3DSurface9* ds=nullptr;if(FAILED(d->GetDepthStencilSurface(&ds))||!ds)return;
 auto depth=projected_depth_adapter::sampleTexture(ds);
 if(!depth||!worldTargetMatches(d,ds)){ready=false;ds->Release();return;}
 LARGE_INTEGER begin{},finish{},frequency{};if(measureGpu){QueryPerformanceCounter(&begin);QueryPerformanceFrequency(&frequency);}
 float constants[64]{},worldEye[4]{};if(!camera(constants,worldEye)||!prepare(d)){ready=false;++failures;ds->Release();return;}
 memcpy(constants+16,p.origin,16);for(int i=0;i<3;i++)constants[16+i]-=worldEye[i];memcpy(constants+20,p.direction,16);memcpy(constants+24,p.colour,16);
 if(measureGpu)pollGpu(d);bool measure=measureGpu&&!gpuPending&&gpuStart&&gpuEnd&&gpuFrequency;if(measure){gpuFrequency->Issue(D3DISSUE_END);gpuStart->Issue(D3DISSUE_END);}
 if(!copyScene(d)){ready=false;++failures;ds->Release();return;}
 D3DVIEWPORT9 vp{};d->GetViewport(&vp);
 // Bounded private observation of the matrix actually present at world end.
 static unsigned observed=0;char debugDir[MAX_PATH]{};
 if(observed<8&&GetEnvironmentVariableA("SEATED_TEST_DIR",debugDir,MAX_PATH)){
  char mark[MAX_PATH];sprintf_s(mark,"%s\\light-camera.enable",debugDir);
  if(GetFileAttributesA(mark)!=INVALID_FILE_ATTRIBUTES){
   sprintf_s(mark,"%s\\light-camera.txt",debugDir);FILE* f=nullptr;fopen_s(&f,mark,observed?"a":"w");
   if(f){auto graph=*reinterpret_cast<unsigned char**>(0x11DEB7C);auto cam=graph?*reinterpret_cast<unsigned char**>(graph+0xAC):nullptr;
    fprintf(f,"frame %u vp %u %u %u %u\n",observed,vp.X,vp.Y,vp.Width,vp.Height);
    if(cam)for(unsigned off:{0x68u,0x9cu,0xdcu}){fprintf(f,"cam%X",off);for(int i=0;i<(off==0xDC?6:off==0x68?13:16);i++)fprintf(f," %.9g",reinterpret_cast<float*>(cam+off)[i]);fputc('\n',f);}
    for(auto which:{D3DTS_VIEW,D3DTS_PROJECTION}){D3DMATRIX m{};d->GetTransform(which,&m);fprintf(f,"d3d%d",which);for(int i=0;i<16;i++)fprintf(f," %.9g",reinterpret_cast<float*>(&m)[i]);fputc('\n',f);}float constantsVS[64*4]{};if(SUCCEEDED(d->GetVertexShaderConstantF(0,constantsVS,64)))for(unsigned reg=0;reg<64;++reg)fprintf(f,"vs%u %.9g %.9g %.9g %.9g\n",reg,constantsVS[reg*4],constantsVS[reg*4+1],constantsVS[reg*4+2],constantsVS[reg*4+3]);
    if(observed==0){IDirect3DVertexShader9* vs=nullptr;d->GetVertexShader(&vs);if(vs){UINT size=0;vs->GetFunction(nullptr,&size);if(size>0&&size<65536){std::vector<unsigned char> code(size);if(SUCCEEDED(vs->GetFunction(code.data(),&size))){ID3DBlob* assembly=nullptr;if(SUCCEEDED(D3DDisassemble(code.data(),size,0,nullptr,&assembly))){fprintf(f,"NATIVE VS\n%.*s\n",int(assembly->GetBufferSize()),static_cast<char*>(assembly->GetBufferPointer()));assembly->Release();}}}vs->Release();}}
    fclose(f);++observed;
   }
  }
 }
 state->Capture();bool began=SUCCEEDED(d->BeginScene());
 constants[48]=lampShadow&&lampShadow(d,worldEye,constants+32)?1.f:0;
 const bool countRaster=contact_trace::beginCoverage(d,vp);
 ready=composite(d,depth,constants,vp);
 if(countRaster)contact_trace::endCoverage();
 if(ready&&contact_trace::log)contact_trace::sample(d,p,constants,worldEye,vp,sceneDesc);
 d->SetTexture(0,nullptr);d->SetTexture(1,nullptr);d->SetTexture(2,nullptr);d->SetDepthStencilSurface(ds);state->Apply();d->SetViewport(&vp);if(began)d->EndScene();ds->Release();if(measure){gpuEnd->Issue(D3DISSUE_END);gpuPending=true;}
 if(measureGpu){QueryPerformanceCounter(&finish);double us=double(finish.QuadPart-begin.QuadPart)*1e6/double(frequency.QuadPart);cpuTotalUs+=us;cpuMaxUs=std::max(cpuMaxUs,us);++cpuSamples;}
}
}
