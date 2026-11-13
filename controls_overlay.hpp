#pragma once
#include "controls_runtime.hpp"
#include "controls_models.hpp"
#include "robco_theme.hpp"
// Licensed 3D Touch Plus models over original RobCo artwork.
namespace controls_overlay {
constexpr int width=1400,height=1000;
inline XrSwapchain chain=XR_NULL_HANDLE;
inline std::vector<XrSwapchainImageD3D11KHR> images;
inline std::vector<unsigned char> pixels;
inline unsigned anchorEpoch=~0u;
inline XrPosef pose{};inline XrSpace space=XR_NULL_HANDLE;
inline float cursorX=-1,cursorY=-1;inline int hover=-1;
inline uint64_t painted=0;inline bool failed=false;
inline void cleanup(){controls_runtime::invalidate();controls_models::cleanup();if(chain)overlay::destroySwapchain(chain);chain=XR_NULL_HANDLE;images.clear();pixels.clear();failed=false;painted=0;anchorEpoch=~0u;space=XR_NULL_HANDLE;}
inline void text(HDC dc,const std::string&s,int x,int y,int w,int h,int size=26,COLORREF color=RGB(168,221,144)){
 auto font=CreateFontA(-size,0,0,0,FW_MEDIUM,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,"Segoe UI");auto old=SelectObject(dc,font);SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);RECT r{x,y,x+w,y+h};DrawTextA(dc,s.c_str(),-1,&r,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);SelectObject(dc,old);DeleteObject(font);
}
inline void box(HDC dc,int x,int y,int w,int h,COLORREF fill,COLORREF stroke,bool round=true){auto b=CreateSolidBrush(fill),oldB=(HBRUSH)SelectObject(dc,b);auto p=CreatePen(PS_SOLID,2,stroke),oldP=(HPEN)SelectObject(dc,p);if(round)RoundRect(dc,x,y,x+w,y+h,16,16);else Rectangle(dc,x,y,x+w,y+h);SelectObject(dc,oldB);SelectObject(dc,oldP);DeleteObject(b);DeleteObject(p);}
inline void button(HDC dc,int x,int y,int radius,const char* label,int source,const controls_runtime::View&v){
 bool live=v.physical[source].active&&v.physical[source].value>.55f;
 auto binding=v.selected<fc_controls::actionCount?(v.phase==3?v.proposal:v.profile.bindings[v.selected]):fc_controls::Binding{};
 bool chosen=binding.source==source||binding.modifier==source;
 auto b=CreateSolidBrush(live?RGB(80,139,100):RGB(31,42,39)),oldB=(HBRUSH)SelectObject(dc,b);auto p=CreatePen(PS_SOLID,chosen?5:2,chosen?RGB(198,229,134):RGB(143,157,139)),oldP=(HPEN)SelectObject(dc,p);
 Ellipse(dc,x-radius,y-radius,x+radius,y+radius);SelectObject(dc,oldB);SelectObject(dc,oldP);DeleteObject(b);DeleteObject(p);text(dc,label,x-radius+8,y-15,radius*2,40,23);
}
inline std::vector<unsigned char> artwork;
inline bool loadArtwork(){if(!artwork.empty())return true;std::vector<unsigned char> candidate(width*height*4);if(!robco_theme::background(candidate.data(),width,height,false))return false;artwork=std::move(candidate);return true;}
inline bool paint(const controls_runtime::View&v){
 auto dc=CreateCompatibleDC(nullptr);if(!dc)return false;BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=width;bi.bmiHeader.biHeight=-height;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;void* data=nullptr;auto bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&data,nullptr,0);if(!bitmap){DeleteDC(dc);return false;}auto old=SelectObject(dc,bitmap);
 if(loadArtwork())memcpy(data,artwork.data(),artwork.size());else box(dc,0,0,width,height,RGB(13,24,23),RGB(137,158,123));
 text(dc,"CONTROL CONFIGURATION",75,110,900,50,38,RGB(165,238,133));text(dc,"PROFILE "+std::to_string(v.slot)+(v.dirty?" *":""),1060,115,250,40,28,RGB(165,238,133));
 text(dc,"QUEST TOUCH PLUS",75,185,410,40,28,RGB(145,191,158));
 text(dc,v.title+(v.automatic?" / AUTO":" / EDIT"),75,228,410,38,25);
 for(int i=0;i<3;i++){box(dc,75+i*130,268,120,38,RGB(15,40,18),RGB(106,168,86));text(dc,i==0?"< MODE":i==1?"MODE >":"AUTO",83+i*130,273,115,32,21);}

 text(dc,"LEFT",95,650,180,35,28);text(dc,"RIGHT",315,650,160,35,28);
 text(dc,"Gold: assigned   Green: pressed\nBlue: capacitive touch",75,700,400,62,23);
 auto list=v.displayedRows;int page=controls_runtime::position(list,v.selected)/6;for(int k=0;k<6;k++){int index=page*6+k;if(index>=int(list.size()))break;int row=list[index];int y=184+k*76;bool chosen=row==v.selected;
  box(dc,500,y,820,69,chosen?RGB(28,55,27):RGB(8,25,13),(chosen||hover==row)?RGB(198,229,134):RGB(59,79,62));
  std::string label=row<fc_controls::actionCount?v.labels[row]:row==fc_controls::actionCount?"Save to profile "+std::to_string(v.slot):row==fc_controls::actionCount+1?"Load profile "+std::to_string(v.slot):row==fc_controls::actionCount+2?"Reset this mode to defaults":"Close controls";
  text(dc,label,520,y+4,780,34,27,chosen?RGB(173,245,139):RGB(168,221,144));if(row<fc_controls::actionCount)text(dc,fc_controls::bindingText(v.profile.bindings[row]),520,y+37,780,30,23,RGB(156,188,165));
 }
 text(dc,"PAGE "+std::to_string(page+1)+" / "+std::to_string((list.size()+5)/6),865,656,240,40,23,RGB(145,191,158));
 box(dc,505,650,95,42,RGB(15,40,18),RGB(106,168,86));text(dc,"<",540,653,50,38,28);
 box(dc,1220,650,95,42,RGB(15,40,18),RGB(106,168,86));text(dc,">",1255,653,50,38,28);
 for(int i=0;i<5;i++){box(dc,75+i*78,780,72,42,RGB(15,40,18),RGB(106,168,86));text(dc,i==0?"SAVE":i==1?"LOAD":i==2?"RESET":i==3?"CLOSE":"OFF",80+i*78,789,68,30,22);}
 if(v.phase==3){box(dc,1080,845,110,70,RGB(22,64,26),RGB(165,230,130));text(dc,"APPLY",1093,865,100,40,24);box(dc,1200,845,110,70,RGB(55,32,18),RGB(222,180,100));text(dc,"CANCEL",1205,865,105,40,23);}
 if(v.selected<fc_controls::actionCount)text(dc,v.mode=="onfoot"?fc_controls::descriptions[v.selected]:v.description.c_str(),505,700,800,100,25);
 box(dc,65,840,1270,86,RGB(8,25,13),RGB(127,148,110));text(dc,v.status,80,845,1240,39,23,RGB(173,245,139));
 const char* help=v.phase==2?"Press / squeeze / move a stick / tap a thumb rest. For a combo, press both within 350ms.":v.phase==3?"A: apply   B: cancel   X: change Direct / Press / Tap / Hold / Double tap":"A: edit / select   B: close   X: gesture   Y: reset binding   Point at OFF: unbind";
 text(dc,help,80,886,1240,40,21);text(dc,"Recovery: hold physical Left Menu + Right stick click for 1 second",75,157,1240,28,20,RGB(145,191,158));
 if(v.phase==3){box(dc,1080,845,110,70,RGB(22,64,26),RGB(165,230,130));text(dc,"APPLY",1093,865,100,40,24);box(dc,1200,845,110,70,RGB(55,32,18),RGB(222,180,100));text(dc,"CANCEL",1205,865,105,40,23);}
 if(cursorX>=0){auto pen=CreatePen(PS_SOLID,3,RGB(239,245,143));auto oldPen=SelectObject(dc,pen);auto oldBrush=SelectObject(dc,GetStockObject(NULL_BRUSH));Ellipse(dc,int(cursorX)-9,int(cursorY)-9,int(cursorX)+9,int(cursorY)+9);SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(pen);}
 GdiFlush();pixels.assign((unsigned char*)data,(unsigned char*)data+width*height*4);for(size_t i=3;i<pixels.size();i+=4)pixels[i]=255;SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);return true;
}
inline bool ensure(){if(failed||!overlay::device||!overlay::context)return false;if(chain)return true;
 uint32_t count=0;if(XR_FAILED(overlay::formats(overlay::session,0,&count,nullptr)))return false;std::vector<int64_t> fm(count);if(XR_FAILED(overlay::formats(overlay::session,count,&count,fm.data())))return false;int64_t format=0;for(auto f:fm)if(f==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB||f==DXGI_FORMAT_B8G8R8A8_UNORM)format=f;if(!format)return false;
 XrSwapchainCreateInfo ci{XR_TYPE_SWAPCHAIN_CREATE_INFO};ci.usageFlags=XR_SWAPCHAIN_USAGE_SAMPLED_BIT|XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;ci.format=format;ci.sampleCount=1;ci.width=width;ci.height=height;ci.faceCount=ci.arraySize=ci.mipCount=1;
 if(XR_FAILED(overlay::createSwapchain(overlay::session,&ci,&chain))){failed=true;return false;}
 if(XR_FAILED(overlay::enumerateImages(chain,0,&count,nullptr))||!count){failed=true;return false;}images.resize(count);for(auto&i:images)i={XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR};if(XR_FAILED(overlay::enumerateImages(chain,count,&count,(XrSwapchainImageBaseHeader*)images.data()))){failed=true;return false;}return true;
}
inline XrVector3f rotate(XrQuaternionf q,XrVector3f v){XrVector3f t{2*(q.y*v.z-q.z*v.y),2*(q.z*v.x-q.x*v.z),2*(q.x*v.y-q.y*v.x)};return {v.x+q.w*t.x+q.y*t.z-q.z*t.y,v.y+q.w*t.y+q.z*t.x-q.x*t.z,v.z+q.w*t.z+q.x*t.y-q.y*t.x};}
inline void updatePointer(const controls_runtime::View&v,XrTime time){cursorX=cursorY=-1;hover=-1;int owner=-1;float best=100;
 if(space)for(int h=0;h<2;h++)if(gesture_input::aim[h]){XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};if(XR_FAILED(gesture_input::locateSpace(gesture_input::aim[h],height_layer::resolve(space),time,&location)))continue;constexpr auto required=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT|XR_SPACE_LOCATION_POSITION_TRACKED_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;if((location.locationFlags&required)!=required)continue;
 auto inv=pose.orientation;inv.x=-inv.x;inv.y=-inv.y;inv.z=-inv.z;auto origin=rotate(inv,{location.pose.position.x-pose.position.x,location.pose.position.y-pose.position.y,location.pose.position.z-pose.position.z});auto direction=rotate(inv,rotate(location.pose.orientation,{0,0,-1}));if(direction.z>=-.001f)continue;float t=-origin.z/direction.z;if(t<0||t>10)continue;float x=((origin.x+t*direction.x)/1.4f+.5f)*width,y=(.5f-(origin.y+t*direction.y)/(1.4f*height/width))*height;if(x<0||x>width||y<0||y>height)continue;
 int hit=-1;if(y>=268&&y<=306&&x>=75&&x<465)hit=controls_runtime::rows+5+int((x-75)/130);if(x>=500&&x<=1320&&y>=184&&y<640){auto list=v.displayedRows;int index=(controls_runtime::position(list,v.selected)/6)*6+int((y-184)/76);hit=index<int(list.size())?list[index]:-1;}
 if(y>=650&&y<=692){if(x>=505&&x<=600)hit=controls_runtime::rows;if(x>=1220&&x<=1315)hit=controls_runtime::rows+1;}
 if(y>=780&&y<=822&&x>=75&&x<467){int button=int((x-75)/78);hit=button==4?controls_runtime::rows+4:fc_controls::actionCount+button;}
 if(v.phase==3&&y>=845&&y<=915){if(x>=1080&&x<=1190)hit=controls_runtime::rows+2;if(x>=1200&&x<=1310)hit=controls_runtime::rows+3;}
 float score=t+(hit<0?10.f:0.f)-(v.physical[h*5].value>.55f?20.f:0.f);if(score>best)continue;best=score;owner=h;cursorX=x;cursorY=y;hover=hit;
 }controls_runtime::pointer(hover,owner<0?1:owner,cursorX,cursorY);
}
inline XrResult XRAPI_CALL submit(XrSession s,const XrFrameEndInfo*info){auto v=controls_runtime::snapshot();if(!v.visible||!info||!info->layerCount||s!=overlay::session||!ensure())return overlay::endFrame(s,info);
 if(anchorEpoch!=v.epoch&&height_layer::lastBase){anchorEpoch=v.epoch;space=height_layer::lastBase;pose=height_layer::lastHead;auto q=pose.orientation;float yaw=std::atan2(2*(q.w*q.y+q.x*q.z),1-2*(q.y*q.y+q.x*q.x));pose.orientation={0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};pose.position.x-=std::sin(yaw)*1.15f;pose.position.z-=std::cos(yaw)*1.15f;pose.position.y-=.04f;}
 updatePointer(v,info->displayTime);
 auto now=GetTickCount64();if(now-painted>=50||!painted){if(!paint(v))return overlay::endFrame(s,info);uint32_t index=0;XrSwapchainImageAcquireInfo ai{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};if(XR_FAILED(overlay::acquire(chain,&ai,&index)))return overlay::endFrame(s,info);XrSwapchainImageWaitInfo wi{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wi.timeout=XR_INFINITE_DURATION;if(XR_FAILED(overlay::wait(chain,&wi))||index>=images.size()){failed=true;return overlay::endFrame(s,info);}auto rendered=controls_models::draw(overlay::device,overlay::context,v,pixels);
 if(rendered)overlay::context->CopyResource(images[index].texture,rendered);else overlay::context->UpdateSubresource(images[index].texture,0,nullptr,pixels.data(),width*4,0);overlay::context->Flush();XrSwapchainImageReleaseInfo ri{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};if(XR_FAILED(overlay::release(chain,&ri)))return overlay::endFrame(s,info);painted=now;}
 if(!space)return overlay::endFrame(s,info);
 XrCompositionLayerQuad panel{XR_TYPE_COMPOSITION_LAYER_QUAD};panel.space=height_layer::resolve(space);panel.eyeVisibility=XR_EYE_VISIBILITY_BOTH;panel.pose=pose;panel.size={1.4f,1.4f*height/width};panel.subImage.swapchain=chain;panel.subImage.imageRect.extent={width,height};std::vector<const XrCompositionLayerBaseHeader*> layers(info->layers,info->layers+info->layerCount);layers.push_back((const XrCompositionLayerBaseHeader*)&panel);auto submission=*info;submission.layers=layers.data();submission.layerCount=uint32_t(layers.size());return overlay::endFrame(s,&submission);
}
}
