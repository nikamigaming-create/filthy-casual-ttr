#pragma once
#include "dialogue_anchor.hpp"
#include <unordered_map>
#include <mutex>
// Split the EXISTING native name pixels from the native dialogue quad.
// No new text, font, swapchain, game UI edits, or duplicate name is generated.
namespace dialogue_label {
inline PFN_xrAcquireSwapchainImage acquire=nullptr;
inline PFN_xrReleaseSwapchainImage release=nullptr;
inline PFN_xrDestroySwapchain destroy=nullptr;
inline std::mutex mutex;
inline std::unordered_map<XrSwapchain,unsigned> acquired;
inline XrSwapchain requested=XR_NULL_HANDLE;
inline XrRect2Di requestRect{},nameRect{};
inline unsigned epoch=0,attempts=0;
inline int bodyTop=0;
inline bool ready=false;
inline ULONGLONG nextInspect=0;
inline bool anchor(DialogueAnchor& value){
 using Query=bool(__cdecl*)(DialogueAnchor*);
 auto module=GetModuleHandleW(L"SeatedControls.dll");
 auto fn=module?reinterpret_cast<Query>(GetProcAddress(module,"SC_DialogueAnchor")):nullptr;
 return fn&&fn(&value);
}
inline void reset(){std::lock_guard<std::mutex> lock(mutex);acquired.clear();requested=XR_NULL_HANDLE;ready=false;epoch=0;attempts=0;nextInspect=0;}
inline XrResult XRAPI_CALL onDestroy(XrSwapchain chain){
 auto result=destroy(chain);if(XR_SUCCEEDED(result)){std::lock_guard<std::mutex> lock(mutex);acquired.erase(chain);if(chain==requested){requested=XR_NULL_HANDLE;ready=false;attempts=0;nextInspect=0;}}
 return result;
}
inline XrResult XRAPI_CALL onDestroySession(XrSession session){reset();return overlay::onDestroySession(session);}
inline XrResult XRAPI_CALL onAcquire(XrSwapchain chain,const XrSwapchainImageAcquireInfo* info,unsigned* index){
 auto result=acquire(chain,info,index);
 if(XR_SUCCEEDED(result)){std::lock_guard<std::mutex> lock(mutex);acquired[chain]=*index;}
 return result;
}
inline void inspect(XrSwapchain chain,unsigned index){
 if(!overlay::device||!overlay::context||!overlay::enumerateImages)return;
 ++attempts;nextInspect=GetTickCount64()+250;
 uint32_t count=0;if(XR_FAILED(overlay::enumerateImages(chain,0,&count,nullptr))||index>=count)return;
 std::vector<XrSwapchainImageD3D11KHR> images(count,{XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
 if(XR_FAILED(overlay::enumerateImages(chain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data()))))return;
 auto texture=images[index].texture;D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);
 if(desc.SampleDesc.Count!=1||desc.ArraySize!=1||
  (desc.Format!=DXGI_FORMAT_B8G8R8A8_UNORM&&desc.Format!=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB&&desc.Format!=DXGI_FORMAT_R8G8B8A8_UNORM&&desc.Format!=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB))return;
 if(requestRect.offset.x<0||requestRect.offset.y<0||requestRect.extent.width<8||requestRect.extent.height<16||
  unsigned(requestRect.offset.x+requestRect.extent.width)>desc.Width||unsigned(requestRect.offset.y+requestRect.extent.height)>desc.Height)return;
 desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;
 ID3D11Texture2D* staging=nullptr;if(FAILED(overlay::device->CreateTexture2D(&desc,nullptr,&staging)))return;
 overlay::context->CopyResource(staging,texture);
 D3D11_MAPPED_SUBRESOURCE mapped{};
 if(FAILED(overlay::context->Map(staging,0,D3D11_MAP_READ,0,&mapped))){staging->Release();return;}
 int first=-1,last=-1,left=requestRect.extent.width,right=-1,gap=0;
 for(int y=0;y<requestRect.extent.height;++y){
  const auto row=static_cast<unsigned char*>(mapped.pData)+(requestRect.offset.y+y)*mapped.RowPitch+requestRect.offset.x*4;
  int lo=requestRect.extent.width,hi=-1;
  for(int x=0;x<requestRect.extent.width;++x)if(row[x*4+3]>16){lo=std::min(lo,x);hi=x;}
  if(hi>=0){if(first<0)first=y;last=y;left=std::min(left,lo);right=std::max(right,hi);gap=0;}
  else if(first>=0&&++gap>=6)break;
  if(first>=0&&y-first>requestRect.extent.height/4)break;
 }
 // The alpha band includes the native tile's transparent/dark padding. Fit
 // the visible native glyphs so the physical name size describes the text,
 // rather than shrinking that text inside its old padded rectangle.
 int glyphLeft=requestRect.extent.width,glyphRight=-1,glyphTop=last,glyphBottom=-1;
 if(first>=0)for(int y=first;y<=last;++y){
  const auto row=static_cast<unsigned char*>(mapped.pData)+(requestRect.offset.y+y)*mapped.RowPitch+requestRect.offset.x*4;
  for(int x=left;x<=right;++x){const auto* p=row+x*4;
   if(p[3]>16&&std::max(p[0],std::max(p[1],p[2]))>64){glyphLeft=std::min(glyphLeft,x);glyphRight=std::max(glyphRight,x);glyphTop=std::min(glyphTop,y);glyphBottom=y;}
  }
 }
 overlay::context->Unmap(staging,0);staging->Release();
 // Verified native DialogMenu: speaker name is the isolated top-right band.
 // Refuse unexpected layouts instead of slicing dialogue choices or subtitles.
 if(first<0||last-first<3||last-first>requestRect.extent.height/5||left<requestRect.extent.width/2||right<=left||gap<6)return;
 if(glyphRight<=glyphLeft||glyphBottom-glyphTop<3)return;
 const int x0=std::max(0,glyphLeft-2),x1=std::min(requestRect.extent.width,glyphRight+3);
 const int y0=std::max(0,glyphTop-2),y1=std::min(requestRect.extent.height,glyphBottom+3);
 nameRect={{requestRect.offset.x+x0,requestRect.offset.y+y0},{x1-x0,y1-y0}};
 bodyTop=std::min(requestRect.extent.height,last+3);
 ready=bodyTop<requestRect.extent.height-8;
}
inline XrResult XRAPI_CALL onRelease(XrSwapchain chain,const XrSwapchainImageReleaseInfo* info){
 {
  std::lock_guard<std::mutex> lock(mutex);
  auto it=acquired.find(chain);
  // Read pixels while the application still OWNS the image, before release.
  if(chain==requested&&!ready&&GetTickCount64()>=nextInspect&&it!=acquired.end())inspect(chain,it->second);
  if(it!=acquired.end())acquired.erase(it);
 }
 return release(chain,info);
}
inline XrVector3f rotate(XrQuaternionf q,XrVector3f v){
 XrVector3f t{2*(q.y*v.z-q.z*v.y),2*(q.z*v.x-q.x*v.z),2*(q.x*v.y-q.y*v.x)};
 return {v.x+q.w*t.x+q.y*t.z-q.z*t.y,v.y+q.w*t.y+q.z*t.x-q.x*t.z,v.z+q.w*t.z+q.x*t.y-q.y*t.x};
}
inline XrResult XRAPI_CALL submit(XrSession session,const XrFrameEndInfo* info){
 DialogueAnchor target{};
 if(!anchor(target)){
  {std::lock_guard<std::mutex> lock(mutex);requested=XR_NULL_HANDLE;ready=false;}
  return overlay::onEndFrame(session,info);
 }
 int selected=-1;
 for(unsigned i=0;i<info->layerCount;++i){
  if(info->layers[i]->type!=XR_TYPE_COMPOSITION_LAYER_QUAD)continue;
  auto q=reinterpret_cast<const XrCompositionLayerQuad*>(info->layers[i]);
  if(q->size.width>.7f&&(q->layerFlags&XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT)&&!(q->layerFlags&XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT)){if(selected>=0)return overlay::onEndFrame(session,info);selected=int(i);}
 }
 if(selected<0)return overlay::onEndFrame(session,info);
 auto original=*reinterpret_cast<const XrCompositionLayerQuad*>(info->layers[selected]);
 XrRect2Di crop{};int cut=0;bool split=false;
 {
  std::lock_guard<std::mutex> lock(mutex);
  if(requested!=original.subImage.swapchain||epoch!=target.epoch||memcmp(&requestRect,&original.subImage.imageRect,sizeof(requestRect))){
   requested=original.subImage.swapchain;requestRect=original.subImage.imageRect;epoch=target.epoch;ready=false;attempts=0;nextInspect=0;
  }
  split=ready;crop=nameRect;cut=bodyTop;
 }
 if(!split||!height_layer::lastBase)return overlay::onEndFrame(session,info);
 auto body=original,name=original;
 const float removed=float(cut)/original.subImage.imageRect.extent.height;
 body.subImage.imageRect.offset.y+=cut;body.subImage.imageRect.extent.height-=cut;
 body.size.height*=1-removed;
 const auto shift=rotate(body.pose.orientation,{0,-original.size.height*removed*.5f,0});
 body.pose.position.x+=shift.x;body.pose.position.y+=shift.y;body.pose.position.z+=shift.z;
 // Speaker tracking coordinates belong to the world-view reference space,
 // not necessarily the native dialogue quad's space.
 name.space=height_layer::resolve(height_layer::lastBase);
 name.subImage.imageRect=crop;
 name.pose.position={target.position[0],target.position[1],target.position[2]};
 const float yaw=std::atan2(target.head[0]-target.position[0],target.head[2]-target.position[2]);
 name.pose.orientation={0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};
 name.size.height=.055f;name.size.width=name.size.height*crop.extent.width/crop.extent.height;
 if(name.size.width>.65f){name.size.height*=.65f/name.size.width;name.size.width=.65f;}
 std::vector<const XrCompositionLayerBaseHeader*> layers(info->layers,info->layers+info->layerCount);
 layers[selected]=reinterpret_cast<const XrCompositionLayerBaseHeader*>(&body);
 layers.push_back(reinterpret_cast<const XrCompositionLayerBaseHeader*>(&name));
 auto submission=*info;submission.layerCount=unsigned(layers.size());submission.layers=layers.data();
 return overlay::onEndFrame(session,&submission);
}
}
