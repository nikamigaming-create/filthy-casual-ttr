#pragma once
#include "settlement-vr/ui.hpp"
// Optional RobCo construction palette. Native Forts owns preview, costs and saves.
namespace settlement_overlay {
constexpr int width=1400,height=1000;constexpr float panelWidth=.90f;
inline XrSwapchain chain=XR_NULL_HANDLE;inline std::vector<XrSwapchainImageD3D11KHR> images;
inline std::vector<unsigned char> pixels;inline XrSpace space=XR_NULL_HANDLE;inline XrPosef pose{};
inline unsigned epoch=~0u;inline uint64_t painted=0;inline bool failed=false,wasVisible=false;
inline float cursorX=-1,cursorY=-1;inline int hovered=-1;
inline settlement_ui::Pointer pointer=nullptr;
inline void cleanup(){if(pointer)pointer(-1,-1,false);if(chain&&overlay::destroySwapchain)overlay::destroySwapchain(chain);chain=XR_NULL_HANDLE;images.clear();pixels.clear();space=XR_NULL_HANDLE;epoch=~0u;painted=0;failed=false;wasVisible=false;}
inline bool ensure(){
 if(failed||!overlay::device||!overlay::context)return false;if(chain)return true;
 uint32_t count=0;if(XR_FAILED(overlay::formats(overlay::session,0,&count,nullptr))||!count)return false;
 std::vector<int64_t> formats(count);if(XR_FAILED(overlay::formats(overlay::session,count,&count,formats.data())))return false;
 int64_t fmt=0;for(auto f:formats)if(f==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB||f==DXGI_FORMAT_B8G8R8A8_UNORM)fmt=f;if(!fmt)return false;
 XrSwapchainCreateInfo ci{XR_TYPE_SWAPCHAIN_CREATE_INFO};ci.usageFlags=XR_SWAPCHAIN_USAGE_SAMPLED_BIT|XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;ci.format=fmt;ci.sampleCount=1;ci.width=width;ci.height=height;ci.faceCount=ci.arraySize=ci.mipCount=1;
 if(XR_FAILED(overlay::createSwapchain(overlay::session,&ci,&chain))){failed=true;return false;}
 if(XR_FAILED(overlay::enumerateImages(chain,0,&count,nullptr))||!count){failed=true;return false;}images.resize(count);for(auto& i:images)i={XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR};
 if(XR_FAILED(overlay::enumerateImages(chain,count,&count,(XrSwapchainImageBaseHeader*)images.data()))){failed=true;return false;}return true;
}
inline bool paint(const settlement_ui::State& v){
 HDC dc=CreateCompatibleDC(nullptr);if(!dc)return false;BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=width;bi.bmiHeader.biHeight=-height;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
 void* data=nullptr;auto bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&data,nullptr,0);if(!bitmap){DeleteDC(dc);return false;}auto old=SelectObject(dc,bitmap);
 if(!robco_theme::background(data,width,height,false))controls_overlay::box(dc,0,0,width,height,RGB(13,24,23),RGB(137,158,123));
 using controls_overlay::text;using controls_overlay::box;
 text(dc,"CITY BUILDER",75,108,900,48,40);text(dc,v.category,75,168,850,45,30);text(dc,v.page,1120,168,210,40,27);
 for(int i=0;i<9;i++){int y=225+i*51;bool chosen=i==v.selected;box(dc,75,y,580,46,chosen?RGB(40,62,28):RGB(9,24,15),chosen?RGB(211,220,140):RGB(50,76,51));text(dc,v.items[i],91,y+6,550,39,25);}
 text(dc,v.item,700,227,620,90,35);text(dc,v.stage==5?"CONFIRM PLACEMENT":"LIVE WORLD PREVIEW",700,326,615,42,25,RGB(229,194,108));
 text(dc,v.cost,700,382,610,125,28);text(dc,v.supplies,700,511,610,100,25,RGB(145,191,158));
 text(dc,"L stick: walk   R stick: browse\nGrips: categories   Triggers: rotate",700,615,610,80,24);
 for(unsigned i=0;i<settlement_ui::actions;i++){int x=75+(i%4)*313,y=713+(i/4)*58;bool enabled=v.bindings[i]!=0;box(dc,x,y,300,49,hovered==int(i)?RGB(53,74,34):RGB(15,32,20),enabled?RGB(139,169,94):RGB(57,67,57));text(dc,settlement_ui::labels[i],x+10,y+12,282,34,20,enabled?RGB(205,231,155):RGB(90,110,90));}
 if(cursorX>=0){auto pen=CreatePen(PS_SOLID,3,RGB(250,226,130));auto op=SelectObject(dc,pen);auto ob=SelectObject(dc,GetStockObject(NULL_BRUSH));Ellipse(dc,int(cursorX)-10,int(cursorY)-10,int(cursorX)+10,int(cursorY)+10);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(pen);}
 GdiFlush();pixels.assign((unsigned char*)data,(unsigned char*)data+width*height*4);for(size_t i=3;i<pixels.size();i+=4)pixels[i]=255;SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);return true;
}
inline void aim(XrTime time,const settlement_ui::State& v){
 cursorX=cursorY=-1;hovered=-1;int owner=-1;float closest=100;
 if(space)for(int h=0;h<2;h++)if(gesture_input::aim[h]){
  XrSpaceLocation loc{XR_TYPE_SPACE_LOCATION};if(XR_FAILED(gesture_input::locateSpace(gesture_input::aim[h],height_layer::resolve(space),time,&loc)))continue;
  constexpr auto bits=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT|XR_SPACE_LOCATION_POSITION_TRACKED_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;if((loc.locationFlags&bits)!=bits)continue;
  auto inv=pose.orientation;inv.x=-inv.x;inv.y=-inv.y;inv.z=-inv.z;
  auto origin=controls_overlay::rotate(inv,{loc.pose.position.x-pose.position.x,loc.pose.position.y-pose.position.y,loc.pose.position.z-pose.position.z});
  auto direction=controls_overlay::rotate(inv,controls_overlay::rotate(loc.pose.orientation,{0,0,-1}));if(direction.z>=-.001f)continue;
  float t=-origin.z/direction.z;if(t<0||t>3||t>closest)continue;
  float x=((origin.x+t*direction.x)/panelWidth+.5f)*width,y=(.5f-(origin.y+t*direction.y)/(panelWidth*height/width))*height;
  if(x<0||x>=width||y<0||y>=height)continue;int hit=-1;
  for(unsigned i=0;i<settlement_ui::actions;i++){int bx=75+(i%4)*313,by=713+(i/4)*58;if(x>=bx&&x<bx+300&&y>=by&&y<by+49&&v.bindings[i])hit=i;}
  closest=t;cursorX=x;cursorY=y;hovered=hit;owner=h;
 }
 if(pointer)pointer(hovered,owner,true);
}
inline void anchor(){space=height_layer::lastBase;pose=height_layer::lastHead;auto q=pose.orientation;float yaw=std::atan2(2*(q.w*q.y+q.x*q.z),1-2*(q.y*q.y+q.x*q.x));pose.orientation={0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};
 // Lower-left reading zone leaves the forward placement preview unobscured.
 auto off=controls_overlay::rotate(pose.orientation,{-.60f,-.22f,-1.05f});pose.position.x+=off.x;pose.position.y+=off.y;pose.position.z+=off.z;
}
inline XrResult XRAPI_CALL submit(XrSession s,const XrFrameEndInfo*info){
 auto module=GetModuleHandleW(L"FilthyCasualSettlements.dll");auto read=module?reinterpret_cast<settlement_ui::Snapshot>(GetProcAddress(module,"FC_SettlementUI")):nullptr;
 pointer=module?reinterpret_cast<settlement_ui::Pointer>(GetProcAddress(module,"FC_SettlementPointer")):nullptr;settlement_ui::State v;
 if(!read||!read(&v,sizeof(v))||v.abi!=settlement_ui::version||v.mode!=1||!v.item[0]||GetTickCount64()-v.updated>400||controls_runtime::snapshot().visible||!info||!info->layerCount||s!=overlay::session||!height_layer::lastBase||!ensure()){
  if(pointer)pointer(-1,-1,false);wasVisible=false;return overlay::onEndFrame(s,info);
 }
 if(!wasVisible||epoch!=v.epoch){anchor();epoch=v.epoch;painted=0;}wasVisible=true;
 aim(info->displayTime,v);
 auto now=GetTickCount64();if(!painted||now-painted>=50){
  if(!paint(v)){if(pointer)pointer(-1,-1,false);return overlay::onEndFrame(s,info);}
  uint32_t i=0;XrSwapchainImageAcquireInfo ai{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};if(XR_FAILED(overlay::acquire(chain,&ai,&i)))return overlay::onEndFrame(s,info);
  XrSwapchainImageWaitInfo wi{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wi.timeout=XR_INFINITE_DURATION;if(XR_FAILED(overlay::wait(chain,&wi))||i>=images.size()){failed=true;return overlay::onEndFrame(s,info);}
  overlay::context->UpdateSubresource(images[i].texture,0,nullptr,pixels.data(),width*4,0);overlay::context->Flush();XrSwapchainImageReleaseInfo ri{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};if(XR_FAILED(overlay::release(chain,&ri)))return overlay::onEndFrame(s,info);painted=now;
 }
 XrCompositionLayerQuad quad{XR_TYPE_COMPOSITION_LAYER_QUAD};quad.space=height_layer::resolve(space);quad.eyeVisibility=XR_EYE_VISIBILITY_BOTH;quad.pose=pose;quad.size={panelWidth,panelWidth*height/width};quad.subImage.swapchain=chain;quad.subImage.imageRect.extent={width,height};
 std::vector<const XrCompositionLayerBaseHeader*> layers(info->layers,info->layers+info->layerCount);layers.push_back((const XrCompositionLayerBaseHeader*)&quad);auto end=*info;end.layers=layers.data();end.layerCount=uint32_t(layers.size());return overlay::onEndFrame(s,&end);
}
}
