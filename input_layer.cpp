#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <string>
#include "openxr.h"
#include "openxr_loader_negotiation.h"
#include "height_layer.hpp"
#include "text_overlay.hpp"
#include "dialogue_label.hpp"
#include "gesture_input.hpp"
#include "controls_overlay.hpp"
extern "C" __declspec(dllexport) void __cdecl SC_LabelDebug(char* out,unsigned capacity){
 std::lock_guard<std::mutex> lock(dialogue_label::mutex);
 snprintf(out,capacity,"{\"ready\":%s,\"attempts\":%u,\"epoch\":%u,\"bodyTop\":%d,\"rect\":[%d,%d,%d,%d]}",dialogue_label::ready?"true":"false",dialogue_label::attempts,dialogue_label::epoch,dialogue_label::bodyTop,dialogue_label::nameRect.offset.x,dialogue_label::nameRect.offset.y,dialogue_label::nameRect.extent.width,dialogue_label::nameRect.extent.height);
}
namespace {
 PFN_xrGetInstanceProcAddr nextGipa=nullptr;
 XrInstance currentInstance=XR_NULL_HANDLE;
 PFN_xrCreateAction createAction=nullptr;
 PFN_xrCreateActionSpace createActionSpace=nullptr;
 std::unordered_map<XrSpace,int> poolSpaces;
 PFN_xrDestroyAction destroyAction=nullptr;
 PFN_xrGetActionStateBoolean getBool=nullptr;
 PFN_xrGetActionStateFloat getFloat=nullptr;
 PFN_xrGetActionStateVector2f getVector=nullptr;
 PFN_xrPathToString pathString=nullptr;
 PFN_xrDestroyInstance destroyInstance=nullptr;
 std::mutex mapMutex;
 std::unordered_map<XrAction,std::string> names;
 std::unordered_map<XrPath,int> hands;
 struct BooleanSample {bool value=false,changed=false;uint64_t generation=~uint64_t(0);};
 std::unordered_map<XrAction,BooleanSample> booleanPrevious[2];
 using Filter=uint32_t(__cdecl*)(const char*,int,int,float,float);
 uint32_t __cdecl optionalFilters(const char* name,int h,int kind,float x,float y){
  auto module=GetModuleHandleW(L"SeatedControls.dll");auto fn=module?reinterpret_cast<Filter>(GetProcAddress(module,"SC_Input")):nullptr;
  HMODULE extra[]={GetModuleHandleW(L"FilthyCasualPool.dll"),GetModuleHandleW(L"FilthyCasualVehicles.dll")};
  const char* activeName[]={"FC_PoolActive","FC_VehicleActive"};const char* inputName[]={"FC_PoolInput","FC_VehicleInput"};
  // Pool permits the normal wheels/settings; only vehicle driving owns this gate.
  if(!strcmp(name,"fc_controls"))for(int i=1;i<2;i++)if(extra[i]){
   auto active=reinterpret_cast<bool(__cdecl*)()>(GetProcAddress(extra[i],activeName[i]));
   if(active&&active()){if(fn)fn(name,h,0,0,0);return 0;}
  }
  // Raw inputs reach articulation first; only native actions are consumed.
  auto seated=fn?fn(name,h,kind,x,y):0;
  for(int i=0;i<2;i++)if(extra[i])if(auto filter=reinterpret_cast<Filter>(GetProcAddress(extra[i],inputName[i]))){// A seated UI consumes B before pool can treat it as leave-match.
   const bool uiOwnsB=i==0&&seated&&h==1&&!strcmp(name,"secondary_click");
   auto mode=filter(name,h,kind,uiOwnsB?0.f:x,y);if(mode)return mode;}
  return seated;
 }
 Filter bridge(){return optionalFilters;}
 void gestureReport(int hand,bool active,bool thumb,bool index){if(auto fn=bridge())fn("fc_gestures",hand,active?8:0,thumb?1.f:0.f,index?1.f:0.f);}
 void controlsReport(unsigned mask,bool active){fc_controls::Sample menu{(mask&4)?1.f:0.f,active,0};if(controls_runtime::route(10,menu)){mask=(mask&~4u)|(menu.value>.55f?4u:0u);}if(auto fn=bridge())fn("fc_controls",0,active?8:0,float(mask),0);}
 void gestureDiagnostic(const char* stage,int result){if(auto fn=bridge()){std::string name=std::string("fc_diag:")+stage;fn(name.c_str(),0,0,float(result),0);}}
 std::string actionName(XrAction a){std::lock_guard<std::mutex> lock(mapMutex);auto i=names.find(a);return i==names.end()?"":i->second;}
 int hand(XrPath p){
  {std::lock_guard<std::mutex> lock(mapMutex);auto i=hands.find(p);if(i!=hands.end())return i->second;}
  char path[XR_MAX_PATH_LENGTH]{};uint32_t count=0;int h=-1;
  if(pathString&&XR_SUCCEEDED(pathString(currentInstance,p,sizeof(path),&count,path))){if(!strcmp(path,"/user/hand/left"))h=0;if(!strcmp(path,"/user/hand/right"))h=1;}
  std::lock_guard<std::mutex> lock(mapMutex);hands[p]=h;return h;
 }
 XrResult XRAPI_CALL onCreateAction(XrActionSet s,const XrActionCreateInfo* info,XrAction* out){gesture_input::ensure();auto r=createAction(s,info,out);if(XR_SUCCEEDED(r)){std::lock_guard<std::mutex> lock(mapMutex);names[*out]=info->actionName;}return r;}
 XrResult XRAPI_CALL onDestroyAction(XrAction a){auto r=destroyAction(a);if(XR_SUCCEEDED(r)){std::lock_guard<std::mutex> lock(mapMutex);names.erase(a);}return r;}
 XrResult XRAPI_CALL onCreateActionSpace(XrSession session,const XrActionSpaceCreateInfo* info,XrSpace* out){
  auto r=createActionSpace(session,info,out);if(XR_SUCCEEDED(r)){int h=hand(info->subactionPath);if(h>=0){std::lock_guard<std::mutex> lock(mapMutex);poolSpaces[*out]=h;}}return r;
 }
 XrResult XRAPI_CALL onPoolLocate(XrSpace space,XrSpace base,XrTime time,XrSpaceLocation* out){
  auto r=height_layer::onLocateSpace(space,base,time,out);int h=-1;
  {std::lock_guard<std::mutex> lock(mapMutex);auto it=poolSpaces.find(space);if(it!=poolSpaces.end())h=it->second;}
  if(h>=0)if(auto m=GetModuleHandleW(L"FilthyCasualPool.dll"))if(auto fn=reinterpret_cast<Filter>(GetProcAddress(m,"FC_PoolInput"))){
   constexpr auto required=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT|XR_SPACE_LOCATION_POSITION_TRACKED_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;
   bool valid=XR_SUCCEEDED(r)&&(out->locationFlags&required)==required;
   fn("pool_pose",h,valid?8:0,0,0);
  }  if(h>=0)if(auto m=GetModuleHandleW(L"FilthyCasualVehicles.dll"))if(auto fn=reinterpret_cast<Filter>(GetProcAddress(m,"FC_VehicleInput"))){
   constexpr auto required=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT|XR_SPACE_LOCATION_POSITION_TRACKED_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;
   bool valid=XR_SUCCEEDED(r)&&(out->locationFlags&required)==required;
   fn("vehicle_pose",h,valid?8:0,0,0);
  }return r;
 }
 XrResult XRAPI_CALL onSessionDestroy(XrSession session){gesture_input::endSession();return height_layer::onDestroySession(session);}
 XrResult XRAPI_CALL onPoolDestroySpace(XrSpace space){auto r=height_layer::onDestroy(space);if(XR_SUCCEEDED(r)){std::lock_guard<std::mutex> lock(mapMutex);poolSpaces.erase(space);}return r;}

 bool nativeBlocked[fc_controls::nativeCount]{};
 void respectNative(int destination,bool nativeActive,fc_controls::Sample&v,bool mapped){if(destination<0)return;std::lock_guard<std::mutex> lock(mapMutex);auto& blocked=nativeBlocked[destination];if(mapped&&!nativeActive&&v.value>.15f)blocked=true;if(v.value<=.15f)blocked=false;v.active=v.active&&nativeActive;if(!v.active||blocked)v.value=0;}
 struct VirtualSample {float x=0,y=0;bool active=false,changed=false,overridden=false;XrTime time=0;uint64_t generation=~uint64_t(0);};
 std::unordered_map<XrAction,VirtualSample> virtualSamples[2];
 int destination(const std::string&name,int h){int local=name=="trigger"?0:name=="squeeze"?1:name=="primary_click"?2:name=="secondary_click"?3:name=="thumbstick_click"?4:-1;return local>=0?h*5+local:name=="menu_click"&&h==0?10:name=="thumbrest_touch"?19+h:-1;}
 bool virtualState(XrAction action,int h,bool overridden,float x,float y,bool active,XrTime inputTime,VirtualSample&result){
  std::lock_guard<std::mutex> lock(mapMutex);auto&v=virtualSamples[h][action];bool replace=overridden||v.overridden;
  if(v.generation!=gesture_input::generation){bool changed=active&&v.active&&(x!=v.x||y!=v.y);if(changed)v.time=std::max(inputTime,height_layer::frameTime.load());v.x=x;v.y=y;v.active=active;v.changed=changed;v.overridden=overridden;v.generation=gesture_input::generation;}result=v;return replace;
 }
 XrResult XRAPI_CALL onBoolean(XrSession s,const XrActionStateGetInfo* info,XrActionStateBoolean* out){
  auto r=getBool(s,info,out);if(XR_FAILED(r))return r;auto n=actionName(info->action);int h=hand(info->subactionPath);if(h<0&&info->subactionPath==XR_NULL_PATH&&n=="menu_click")h=0;if(h<0)return r;
  fc_controls::Sample v{out->isActive?float(out->currentState):0,out->isActive!=0,out->lastChangeTime};bool mapped=controls_runtime::route(destination(n,h),v);respectNative(destination(n,h),out->isActive!=0,v,mapped);
  auto mode=optionalFilters(n.c_str(),h,v.active?8:0,v.value,0);if(mode)v.value=mode==2&&v.active?1.f:0.f;
  VirtualSample cached;if(virtualState(info->action,h,mapped||mode!=0,v.value>.55f?1.f:0.f,0,v.active,v.time,cached)){out->currentState=cached.x>.55f;out->isActive=cached.active;out->changedSinceLastSync=cached.changed;out->lastChangeTime=cached.time;}
  return r;
 }
 XrResult XRAPI_CALL onFloat(XrSession s,const XrActionStateGetInfo* info,XrActionStateFloat* out){
  auto r=getFloat(s,info,out);if(XR_FAILED(r))return r;auto n=actionName(info->action);int h=hand(info->subactionPath);if(h<0)return r;
  fc_controls::Sample v{out->isActive?out->currentState:0,out->isActive!=0,out->lastChangeTime};bool mapped=controls_runtime::route(destination(n,h),v);respectNative(destination(n,h),out->isActive!=0,v,mapped);
  auto mode=optionalFilters(n.c_str(),h,v.active?9:1,v.value,0);if(mode)v.value=mode==2&&v.active?1.f:0.f;
  VirtualSample cached;if(virtualState(info->action,h,mapped||mode!=0,v.value,0,v.active,v.time,cached)){out->currentState=cached.x;out->isActive=cached.active;out->changedSinceLastSync=cached.changed;out->lastChangeTime=cached.time;}
  return r;
 }
 XrResult XRAPI_CALL onVector(XrSession s,const XrActionStateGetInfo* info,XrActionStateVector2f* out){
  auto r=getVector(s,info,out);if(XR_FAILED(r))return r;auto n=actionName(info->action);int h=hand(info->subactionPath);if(h<0)return r;bool mapped=false;float x=out->currentState.x,y=out->currentState.y;bool active=out->isActive!=0;XrTime time=out->lastChangeTime;
  if(n=="thumbstick"){
   fc_controls::Sample v[4]={{std::max(0.f,-x),active,time},{std::max(0.f,x),active,time},{std::max(0.f,-y),active,time},{std::max(0.f,y),active,time}};
   for(int k=0;k<4;k++){bool changed=controls_runtime::route(11+h*4+k,v[k]);respectNative(11+h*4+k,out->isActive!=0,v[k],changed);mapped=changed||mapped;time=std::max(time,v[k].time);}x=v[1].value-v[0].value;y=v[3].value-v[2].value;active=v[0].active||v[1].active||v[2].active||v[3].active;
  }
  auto mode=optionalFilters(n.c_str(),h,active?10:2,x,y);if(mode==3)x=0;else if(mode)x=y=0;
  VirtualSample cached;if(virtualState(info->action,h,mapped||mode!=0,x,y,active,time,cached)){out->currentState={cached.x,cached.y};out->isActive=cached.active;out->changedSinceLastSync=cached.changed;out->lastChangeTime=cached.time;}
  return r;
 }
 XrResult XRAPI_CALL onDestroyInstance(XrInstance i){auto r=destroyInstance(i);if(XR_SUCCEEDED(r)){std::lock_guard<std::mutex> lock(mapMutex);names.clear();hands.clear();poolSpaces.clear();virtualSamples[0].clear();virtualSamples[1].clear();booleanPrevious[0].clear();booleanPrevious[1].clear();currentInstance=XR_NULL_HANDLE;gesture_input::reset();}return r;}
 XrResult XRAPI_CALL gipa(XrInstance i,const char* name,PFN_xrVoidFunction* out){
  if(!name||!out)return XR_ERROR_VALIDATION_FAILURE;
  if(!strcmp(name,"xrGetInstanceProcAddr")){*out=(PFN_xrVoidFunction)gipa;return XR_SUCCESS;}
  if(!nextGipa)return XR_ERROR_INITIALIZATION_FAILED;
  auto r=nextGipa(i,name,out);if(XR_FAILED(r))return r;
  #define WRAP(n,real,wrapper) if(!strcmp(name,n)){real=reinterpret_cast<decltype(real)>(*out);*out=(PFN_xrVoidFunction)wrapper;}
  WRAP("xrCreateAction",createAction,onCreateAction)
  WRAP("xrCreateActionSpace",createActionSpace,onCreateActionSpace)
  WRAP("xrDestroyAction",destroyAction,onDestroyAction)
  WRAP("xrGetActionStateBoolean",getBool,onBoolean)
  WRAP("xrGetActionStateFloat",getFloat,onFloat)
  WRAP("xrGetActionStateVector2f",getVector,onVector)
  WRAP("xrDestroyInstance",destroyInstance,onDestroyInstance)
  WRAP("xrSuggestInteractionProfileBindings",gesture_input::suggest,gesture_input::onSuggest)
  WRAP("xrAttachSessionActionSets",gesture_input::attach,gesture_input::onAttach)
  WRAP("xrSyncActions",gesture_input::sync,gesture_input::onSync)
  WRAP("xrCreateSession",overlay::createSession,overlay::onCreateSession)
  WRAP("xrDestroySession",height_layer::realDestroySession,onSessionDestroy)
  WRAP("xrEndFrame",height_layer::realEnd,height_layer::onEndFrame)
  WRAP("xrAcquireSwapchainImage",dialogue_label::acquire,dialogue_label::onAcquire)
  WRAP("xrReleaseSwapchainImage",dialogue_label::release,dialogue_label::onRelease)
  WRAP("xrDestroySwapchain",dialogue_label::destroy,dialogue_label::onDestroy)
  WRAP("xrCreateReferenceSpace",height_layer::create,height_layer::onCreate)
  WRAP("xrDestroySpace",height_layer::destroy,onPoolDestroySpace)
  WRAP("xrLocateSpace",height_layer::locateSpace,onPoolLocate)
  WRAP("xrLocateViews",height_layer::locateViews,height_layer::onLocateViews)
  WRAP("xrWaitFrame",height_layer::waitFrame,height_layer::onWaitFrame)
  #undef WRAP
  return r;
 }
 XrResult XRAPI_CALL createLayer(const XrInstanceCreateInfo* info,const XrApiLayerCreateInfo* layerInfo,XrInstance* instance){
  if(!layerInfo||!layerInfo->nextInfo||!instance)return XR_ERROR_INITIALIZATION_FAILED;
  const auto* n=layerInfo->nextInfo;nextGipa=n->nextGetInstanceProcAddr;
  XrApiLayerCreateInfo chained=*layerInfo;chained.nextInfo=n->next;
  auto r=n->nextCreateApiLayerInstance(info,&chained,instance);
  if(XR_SUCCEEDED(r)){
   currentInstance=*instance;nextGipa(*instance,"xrPathToString",(PFN_xrVoidFunction*)&pathString);
   gesture_input::diagnostic=gestureDiagnostic;
   char context[128]{};sprintf_s(context,"layer instance=0x%llx dispatch=%p state=%p",uint64_t(*instance),nextGipa,&currentInstance);gestureDiagnostic(context,0);
   gesture_input::init(nextGipa,*instance,gestureReport,controlsReport);
   overlay::controlsSubmit=controls_overlay::submit;overlay::controlsCleanup=controls_overlay::cleanup;overlay::init(nextGipa,*instance);height_layer::init(nextGipa,*instance);dialogue_label::reset();height_layer::forwardEnd=dialogue_label::submit;height_layer::forwardDestroySession=dialogue_label::onDestroySession;
  }
  return r;
 }
}
extern "C" __declspec(dllexport) XrResult XRAPI_CALL xrNegotiateLoaderApiLayerInterface(const XrNegotiateLoaderInfo* info,const char* name,XrNegotiateApiLayerRequest* request){
 if(!info||!name||strcmp(name,"XR_APILAYER_LOCAL_seated_controls")||!request||info->minInterfaceVersion>1||info->maxInterfaceVersion<1)return XR_ERROR_INITIALIZATION_FAILED;
 request->layerInterfaceVersion=1;request->layerApiVersion=XR_MAKE_VERSION(1,0,0);request->getInstanceProcAddr=gipa;request->createApiLayerInstance=createLayer;return XR_SUCCESS;
}

extern "C" __declspec(dllexport) unsigned __cdecl SC_PoolBridgeVersion(uint64_t expected){return currentInstance!=XR_NULL_HANDLE&&(!expected||uint64_t(currentInstance)==expected)?1:0;}

extern "C" __declspec(dllexport) unsigned __cdecl SC_VehicleBridgeVersion(uint64_t expected){return currentInstance!=XR_NULL_HANDLE&&(!expected||uint64_t(currentInstance)==expected)?1:0;}

extern "C" __declspec(dllexport) void __cdecl SC_OpenControls(){controls_runtime::request();}
extern "C" __declspec(dllexport) void __cdecl SC_ControlsDebug(char* out,unsigned capacity){auto text=controls_runtime::diagnostics();text.pop_back();text+=",\"modelsReady\":";text+=controls_models::renderer.ready?"true":"false";text+=",\"queried\":[";{std::lock_guard<std::mutex> lock(mapMutex);bool first=true;for(int h=0;h<2;h++)for(auto& e:virtualSamples[h]){auto n=names.find(e.first);if(n==names.end())continue;if(!first)text+=",";first=false;auto&v=e.second;text+="{\"name\":\""+n->second+"\",\"hand\":"+std::to_string(h)+",\"value\":"+std::to_string(v.x)+",\"active\":"+(v.active?"true":"false")+"}";}}text+="]}";if(out&&capacity)strncpy_s(out,capacity,text.c_str(),_TRUNCATE);}
