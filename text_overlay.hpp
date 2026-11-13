#pragma once
// A small OpenXR composition layer for inventory names, counts, and controls.
// It owns only its own swapchain and VIEW space; original layers pass through.
#include "controls_runtime.hpp"
#include "robco_theme.hpp"
#include <d3d11.h>
#include <d3d10.h>
#define XR_USE_GRAPHICS_API_D3D11
#include "openxr_platform.h"
#include <vector>
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"user32.lib")
namespace overlay {
inline PFN_xrEndFrame controlsSubmit=nullptr;inline void(*controlsCleanup)()=nullptr;
inline PFN_xrCreateSession createSession=nullptr;
inline PFN_xrDestroySession destroySession=nullptr;
inline PFN_xrEndFrame endFrame=nullptr;
inline PFN_xrCreateReferenceSpace createSpace=nullptr;
inline PFN_xrDestroySpace destroySpace=nullptr;
inline PFN_xrEnumerateSwapchainFormats formats=nullptr;
inline PFN_xrCreateSwapchain createSwapchain=nullptr;
inline PFN_xrDestroySwapchain destroySwapchain=nullptr;
inline PFN_xrEnumerateSwapchainImages enumerateImages=nullptr;
inline PFN_xrAcquireSwapchainImage acquire=nullptr;
inline PFN_xrWaitSwapchainImage wait=nullptr;
inline PFN_xrReleaseSwapchainImage release=nullptr;
inline XrSession session=XR_NULL_HANDLE;
inline XrSpace view=XR_NULL_HANDLE;
inline XrSwapchain chain=XR_NULL_HANDLE;
inline ID3D11Device* device=nullptr;
inline ID3D11DeviceContext* context=nullptr;
inline ID3D10Multithread* contextLock=nullptr;
inline BOOL previousProtection=TRUE;
inline std::vector<XrSwapchainImageD3D11KHR> images;
inline std::vector<unsigned char> pixels;
inline std::string lastText;
inline bool failed=false;
inline unsigned anchorEpoch=~0u;inline XrSpace anchorSpace=XR_NULL_HANDLE;inline XrPosef anchorPose{};
constexpr int width=1400,height=500;
inline void init(PFN_xrGetInstanceProcAddr get,XrInstance instance){
 #define LOAD(n,v) get(instance,n,reinterpret_cast<PFN_xrVoidFunction*>(&v))
 LOAD("xrCreateSession",createSession);LOAD("xrDestroySession",destroySession);LOAD("xrEndFrame",endFrame);
 LOAD("xrCreateReferenceSpace",createSpace);LOAD("xrDestroySpace",destroySpace);
 LOAD("xrEnumerateSwapchainFormats",formats);LOAD("xrCreateSwapchain",createSwapchain);LOAD("xrDestroySwapchain",destroySwapchain);
 LOAD("xrEnumerateSwapchainImages",enumerateImages);LOAD("xrAcquireSwapchainImage",acquire);
 LOAD("xrWaitSwapchainImage",wait);LOAD("xrReleaseSwapchainImage",release);
 #undef LOAD
}
inline void cleanup(){
 if(controlsCleanup)controlsCleanup();
 if(chain&&destroySwapchain)destroySwapchain(chain);chain=XR_NULL_HANDLE;images.clear();
 if(view&&destroySpace)destroySpace(view);view=XR_NULL_HANDLE;
 if(contextLock){contextLock->SetMultithreadProtected(previousProtection);contextLock->Release();contextLock=nullptr;}
 if(context)context->Release();context=nullptr;if(device)device->Release();device=nullptr;
 session=XR_NULL_HANDLE;lastText.clear();pixels.clear();failed=false;
}
inline XrResult XRAPI_CALL onCreateSession(XrInstance instance,const XrSessionCreateInfo* info,XrSession* out){
 auto r=createSession(instance,info,out);if(XR_FAILED(r))return r;
 const auto* next=reinterpret_cast<const XrBaseInStructure*>(info->next);
 while(next&&next->type!=XR_TYPE_GRAPHICS_BINDING_D3D11_KHR)next=next->next;
 if(next&&!session){auto binding=reinterpret_cast<const XrGraphicsBindingD3D11KHR*>(next);device=binding->device;
  if(device){device->AddRef();device->GetImmediateContext(&context);session=*out;
   if(SUCCEEDED(context->QueryInterface(__uuidof(ID3D10Multithread),reinterpret_cast<void**>(&contextLock))))previousProtection=contextLock->SetMultithreadProtected(TRUE);
  }}
 return r;
}
inline XrResult XRAPI_CALL onDestroySession(XrSession s){if(s==session)cleanup();return destroySession(s);}
inline bool ensureResources(){
 if(failed||!device||!context)return false;if(chain&&view)return true;
 XrReferenceSpaceCreateInfo space{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};space.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_VIEW;space.poseInReferenceSpace.orientation.w=1;
 if(XR_FAILED(createSpace(session,&space,&view))){failed=true;return false;}
 uint32_t count=0;if(XR_FAILED(formats(session,0,&count,nullptr))||!count){failed=true;return false;}
 std::vector<int64_t> supported(count);if(XR_FAILED(formats(session,count,&count,supported.data()))){failed=true;return false;}
 int64_t format=0;for(auto f:supported)if(f==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)format=f;
 if(!format)for(auto f:supported)if(f==DXGI_FORMAT_B8G8R8A8_UNORM)format=f;
 if(!format){failed=true;return false;}
 XrSwapchainCreateInfo info{XR_TYPE_SWAPCHAIN_CREATE_INFO};info.usageFlags=XR_SWAPCHAIN_USAGE_SAMPLED_BIT|XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
 info.format=format;info.sampleCount=1;info.width=width;info.height=height;info.faceCount=1;info.arraySize=1;info.mipCount=1;
 if(XR_FAILED(createSwapchain(session,&info,&chain))){failed=true;return false;}
 if(XR_FAILED(enumerateImages(chain,0,&count,nullptr))||!count){failed=true;return false;}
 images.resize(count);for(auto& i:images)i={XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR};
 if(XR_FAILED(enumerateImages(chain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data())))){failed=true;return false;}
 return true;
}
inline bool renderText(const char* text){
 HDC dc=CreateCompatibleDC(nullptr);if(!dc)return false;
 BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=width;bi.bmiHeader.biHeight=-height;
 bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
 void* data=nullptr;HBITMAP bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&data,nullptr,0);
 if(!bitmap){DeleteDC(dc);return false;}auto old=SelectObject(dc,bitmap);
 RECT rect{0,0,width,height};HBRUSH bg=CreateSolidBrush(RGB(12,17,17));FillRect(dc,&rect,bg);DeleteObject(bg);
 robco_theme::background(data,width,height,true);
 SetBkMode(dc,TRANSPARENT);SetTextColor(dc,robco_theme::phosphor);
 HFONT font=CreateFontA(-43,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,robco_theme::font);
 auto oldFont=SelectObject(dc,font);rect={65,100,width-65,height-80};DrawTextA(dc,text,-1,&rect,DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_WORDBREAK);
 GdiFlush();pixels.assign(static_cast<unsigned char*>(data),static_cast<unsigned char*>(data)+width*height*4);
 for(size_t i=3;i<pixels.size();i+=4)pixels[i]=255;
 SelectObject(dc,oldFont);DeleteObject(font);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);return true;
}
inline XrResult XRAPI_CALL onEndFrame(XrSession s,const XrFrameEndInfo* info){
 if(controlsSubmit&&controls_runtime::snapshot().visible)return controlsSubmit(s,info);
 using UI=uint32_t(__cdecl*)(char*,uint32_t);auto module=GetModuleHandleW(L"SeatedControls.dll");
 auto ui=module?reinterpret_cast<UI>(GetProcAddress(module,"SC_UI")):nullptr;
 char text[1024]{};
 if(s!=session||!info||!ui||!ui(text,sizeof(text))||!info->layerCount||!ensureResources())return endFrame(s,info);
 if(lastText!=text||pixels.empty()){
  if(!renderText(text))return endFrame(s,info);
  uint32_t index=0;XrSwapchainImageAcquireInfo ai{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
  if(XR_FAILED(acquire(chain,&ai,&index)))return endFrame(s,info);
  XrSwapchainImageWaitInfo wi{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wi.timeout=XR_INFINITE_DURATION;
  auto waited=wait(chain,&wi);XrSwapchainImageReleaseInfo ri{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
  if(XR_FAILED(waited)||index>=images.size()){failed=true;return endFrame(s,info);}
  context->UpdateSubresource(images[index].texture,0,nullptr,pixels.data(),width*4,0);context->Flush();
  if(XR_FAILED(release(chain,&ri)))return endFrame(s,info);lastText=text;
 }
 XrCompositionLayerQuad panel{XR_TYPE_COMPOSITION_LAYER_QUAD};panel.layerFlags=XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT|XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;
 panel.space=view;panel.eyeVisibility=XR_EYE_VISIBILITY_BOTH;panel.subImage.swapchain=chain;panel.subImage.imageRect.extent={width,height};
 panel.pose.orientation.w=1;panel.pose.position={0,.17f,-.62f};panel.size={.74f,.74f*height/width};
 using UIState=unsigned(__cdecl*)();auto status=reinterpret_cast<UIState>(GetProcAddress(module,"SC_UIState"));unsigned flags=status?status():0;
 if((flags&3)&&height_layer::lastBase){
  if(anchorEpoch!=(flags>>2)){anchorEpoch=flags>>2;anchorSpace=height_layer::lastBase;anchorPose=height_layer::lastHead;
   auto q=anchorPose.orientation;float yaw=std::atan2(2*(q.w*q.y+q.x*q.z),1-2*(q.y*q.y+q.x*q.x));
   anchorPose.orientation={0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};
   anchorPose.position.x-=std::sin(yaw)*.62f;anchorPose.position.z-=std::cos(yaw)*.62f;anchorPose.position.y+=.17f;
  }panel.space=height_layer::resolve(anchorSpace);panel.pose=anchorPose;
 }
 std::vector<const XrCompositionLayerBaseHeader*> layers(info->layers,info->layers+info->layerCount);layers.push_back(reinterpret_cast<const XrCompositionLayerBaseHeader*>(&panel));
 auto submission=*info;submission.layerCount=static_cast<uint32_t>(layers.size());submission.layers=layers.data();return endFrame(s,&submission);
}
}
