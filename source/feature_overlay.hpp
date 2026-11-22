#pragma once
#include "feature_setup.hpp"
// Uses the existing RobCo art, swapchain and compositor context. No new device,
// renderer hook or head-locked panel. Game mutations stay in the core MainLoop.
namespace feature_overlay {
inline fc_features::View view;
inline unsigned epoch=~0u;inline XrPosef pose{};inline XrSpace space=XR_NULL_HANDLE;
inline float cursorX=-1,cursorY=-1;inline int hovered=-1;
inline bool visible(){auto module=GetModuleHandleW(L"SeatedControls.dll");auto fn=module?reinterpret_cast<bool(__cdecl*)(fc_features::View*,unsigned)>(GetProcAddress(module,"SC_FeaturesView")):nullptr;return fn&&fn(&view,sizeof(view));}
inline void text(HDC dc,const char* value,int x,int y,int w,int h,int size=27,COLORREF colour=robco_theme::phosphor){
 auto font=CreateFontA(-size,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FIXED_PITCH,"Consolas");auto old=SelectObject(dc,font);SetTextColor(dc,colour);SetBkMode(dc,TRANSPARENT);RECT r{x,y,x+w,y+h};DrawTextA(dc,value,-1,&r,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);SelectObject(dc,old);DeleteObject(font);
}
inline RECT rect(int id){
 if(id<fc_features::Count){
  if(id/8!=int(view.page))return {-10,-10,-5,-5};
  int n=id%8,x=90+(n%2)*620,y=285+(n/2)*96;
  return {x,y,x+590,y+84};
 }
 if(id==fc_features::Count)return {90,780,390,834};
 if(id==fc_features::Count+1)return {940,780,1310,834};
 if(id==fc_features::Count+2)return {530,780,820,834};
 if(id==fc_features::Count+3)return {90,211,350,264};
 return {1050,211,1310,264};
}
inline void outline(HDC dc,RECT r,COLORREF colour){auto pen=CreatePen(PS_SOLID,2,colour);auto old=SelectObject(dc,pen);auto brush=SelectObject(dc,GetStockObject(NULL_BRUSH));Rectangle(dc,r.left,r.top,r.right,r.bottom);SelectObject(dc,brush);SelectObject(dc,old);DeleteObject(pen);}
inline bool paint(){
 auto dc=CreateCompatibleDC(nullptr);if(!dc)return false;BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=1400;bi.bmiHeader.biHeight=-1000;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;void* data=nullptr;auto bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&data,nullptr,0);if(!bitmap){DeleteDC(dc);return false;}auto old=SelectObject(dc,bitmap);
 if(!robco_theme::background(data,1400,1000,false)){SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);return false;}
 text(dc,"FILTHY CASUALS / FEATURE SETUP",90,119,1200,48,35);
 text(dc,"Select features to enable. Unchecked features remain off.",90,179,1220,38,25);
 char pageLabel[80];sprintf_s(pageLabel,"PAGE %u / 3",view.page+1);
 text(dc,pageLabel,545,220,330,42,29);
 for(int id=0;id<fc_features::Count;++id){if(id/8!=int(view.page))continue;
  auto r=rect(id);bool available=(view.available&fc_features::bit(id))!=0;
  auto colour=available?robco_theme::phosphor:RGB(107,125,107);
  outline(dc,r,id==int(view.selected)||id==hovered?colour:RGB(48,75,45));
  RECT check{r.left+15,r.top+26,r.left+44,r.top+55};outline(dc,check,colour);
  if(view.pending&fc_features::bit(id))text(dc,"X",r.left+18,r.top+22,32,38,30,colour);
  text(dc,fc_features::definitions[id].label,r.left+64,r.top+12,r.right-r.left-80,70,30,colour);
 }
 for(int id=fc_features::Count;id<fc_features::Count+5;++id){auto r=rect(id);
  outline(dc,r,view.selected==unsigned(id)||hovered==id?robco_theme::phosphor:RGB(74,112,67));
  const char* label=id==fc_features::Count?"SELECT NONE":id==fc_features::Count+1?"SAVE AND CLOSE":id==fc_features::Count+2?"CANCEL":id==fc_features::Count+3?"< PREVIOUS":"NEXT >";
  text(dc,label,r.left+18,r.top+12,r.right-r.left-30,42,27);
 }
 const char* detail=view.selected<fc_features::Count?fc_features::definitions[view.selected].detail:"Changes are applied only when saved.";
 text(dc,detail,90,680,1220,90,29,robco_theme::secondary);
 text(dc,view.restart?"Restart after saving vehicle or riding changes.":view.status,90,850,1220,34,23,robco_theme::secondary);
 if(cursorX>=0){RECT cursor{int(cursorX)-7,int(cursorY)-7,int(cursorX)+7,int(cursorY)+7};outline(dc,cursor,RGB(233,245,181));}
 GdiFlush();controls_overlay::pixels.assign((unsigned char*)data,(unsigned char*)data+1400*1000*4);for(size_t i=3;i<controls_overlay::pixels.size();i+=4)controls_overlay::pixels[i]=255;
 SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);return true;
}
inline void pointer(XrTime time){
 cursorX=cursorY=-1;hovered=-1;float best=100;
 auto input=controls_runtime::snapshot();
 if(space)for(int hand=0;hand<2;++hand)if(gesture_input::aim[hand]){
  XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};if(XR_FAILED(gesture_input::locateSpace(gesture_input::aim[hand],height_layer::resolve(space),time,&location)))continue;
  constexpr auto required=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT|XR_SPACE_LOCATION_POSITION_TRACKED_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;if((location.locationFlags&required)!=required)continue;
  auto inv=pose.orientation;inv.x=-inv.x;inv.y=-inv.y;inv.z=-inv.z;
  auto origin=controls_overlay::rotate(inv,{location.pose.position.x-pose.position.x,location.pose.position.y-pose.position.y,location.pose.position.z-pose.position.z});
  auto dir=controls_overlay::rotate(inv,controls_overlay::rotate(location.pose.orientation,{0,0,-1}));if(dir.z>=-.001f)continue;float t=-origin.z/dir.z;if(t<0||t>5)continue;
  float x=((origin.x+t*dir.x)/1.4f+.5f)*1400,y=(.5f-(origin.y+t*dir.y))*1000;
  if(x<0||x>1400||y<0||y>1000)continue;
  int hit=-1;for(int id=0;id<fc_features::Count+5;++id){auto r=rect(id);if(x>=r.left&&x<r.right&&y>=r.top&&y<r.bottom){hit=id;break;}}
  float score=t+(hit<0?10.f:0.f)-(input.physical[hand*5].value>.55f?20.f:0.f);if(score>best)continue;best=score;cursorX=x;cursorY=y;hovered=hit;
 }
 auto module=GetModuleHandleW(L"SeatedControls.dll");auto fn=module?reinterpret_cast<void(__cdecl*)(int)>(GetProcAddress(module,"SC_FeatureHover")):nullptr;if(fn)fn(hovered);
}
inline XrResult XRAPI_CALL submit(XrSession s,const XrFrameEndInfo* info){
 if(!info||!info->layerCount||s!=overlay::session||!controls_overlay::ensure())return overlay::endFrame(s,info);
 if((epoch!=view.epoch||!space)&&height_layer::lastBase){epoch=view.epoch;space=height_layer::lastBase;pose=height_layer::lastHead;auto q=pose.orientation;float yaw=std::atan2(2*(q.w*q.y+q.x*q.z),1-2*(q.y*q.y+q.x*q.x));pose.orientation={0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};pose.position.x-=std::sin(yaw)*1.15f;pose.position.z-=std::cos(yaw)*1.15f;pose.position.y-=.04f;}
 if(!space)return overlay::endFrame(s,info);pointer(info->displayTime);
 if(!paint())return overlay::endFrame(s,info);
 uint32_t index=0;XrSwapchainImageAcquireInfo ai{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};if(XR_FAILED(overlay::acquire(controls_overlay::chain,&ai,&index)))return overlay::endFrame(s,info);
 XrSwapchainImageWaitInfo wi{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wi.timeout=XR_INFINITE_DURATION;auto result=overlay::wait(controls_overlay::chain,&wi);XrSwapchainImageReleaseInfo ri{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
 if(XR_FAILED(result)||index>=controls_overlay::images.size())return overlay::endFrame(s,info);
 overlay::context->UpdateSubresource(controls_overlay::images[index].texture,0,nullptr,controls_overlay::pixels.data(),1400*4,0);overlay::context->Flush();if(XR_FAILED(overlay::release(controls_overlay::chain,&ri)))return overlay::endFrame(s,info);
 XrCompositionLayerQuad panel{XR_TYPE_COMPOSITION_LAYER_QUAD};panel.space=height_layer::resolve(space);panel.eyeVisibility=XR_EYE_VISIBILITY_BOTH;panel.pose=pose;panel.size={1.4f,1.f};panel.subImage.swapchain=controls_overlay::chain;panel.subImage.imageRect.extent={1400,1000};
 std::vector<const XrCompositionLayerBaseHeader*> layers(info->layers,info->layers+info->layerCount);layers.push_back(reinterpret_cast<const XrCompositionLayerBaseHeader*>(&panel));auto submission=*info;submission.layers=layers.data();submission.layerCount=uint32_t(layers.size());return overlay::endFrame(s,&submission);
}
}
