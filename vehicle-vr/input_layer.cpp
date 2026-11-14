#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstring>
#include <string>
#include <mutex>
#include <unordered_map>
#include "openxr.h"
#include "openxr_loader_negotiation.h"
namespace {
PFN_xrGetInstanceProcAddr next=nullptr; XrInstance instance=XR_NULL_HANDLE;
PFN_xrCreateAction createAction=nullptr; PFN_xrDestroyAction destroyAction=nullptr;
PFN_xrCreateActionSpace createSpace=nullptr; PFN_xrDestroySpace destroySpace=nullptr;
PFN_xrLocateSpace locate=nullptr; PFN_xrPathToString pathString=nullptr;
PFN_xrGetActionStateBoolean getBool=nullptr; PFN_xrGetActionStateFloat getFloat=nullptr;
PFN_xrGetActionStateVector2f getVector=nullptr;PFN_xrSyncActions syncActions=nullptr; PFN_xrDestroyInstance destroyInstance=nullptr;
std::mutex mutex; std::unordered_map<XrAction,std::string> names; std::unordered_map<XrSpace,int> spaces;
uint64_t generation=0;
PFN_xrApplyHapticFeedback applyHaptic=nullptr;XrAction rideHaptic=XR_NULL_HANDLE;XrPath hapticHands[2]{};unsigned seenRidePulse=0;
void pulseRide(XrSession session){
 auto module=GetModuleHandleW(L"FilthyCasualVehicles.dll");auto read=module?reinterpret_cast<unsigned(__cdecl*)()>(GetProcAddress(module,"FC_RidePulse")):nullptr;
 if(!read)return;auto serial=read();if(serial==seenRidePulse)return;seenRidePulse=serial;
 if(!applyHaptic||!rideHaptic)return;
 for(auto path:hapticHands){if(!path)continue;XrHapticActionInfo info{XR_TYPE_HAPTIC_ACTION_INFO};info.action=rideHaptic;info.subactionPath=path;
  XrHapticVibration pulse{XR_TYPE_HAPTIC_VIBRATION};pulse.amplitude=.32f;pulse.duration=150000000;pulse.frequency=XR_FREQUENCY_UNSPECIFIED;
  applyHaptic(session,&info,reinterpret_cast<const XrHapticBaseHeader*>(&pulse));
 }
}

struct Sample { bool value=false,changed=false;uint64_t generation=~uint64_t(0); };
std::unordered_map<XrAction,Sample> samples[2];
using Filter=unsigned(__cdecl*)(const char*,int,int,float,float);
bool delegated(){auto m=GetModuleHandleW(L"SeatedInput.dll");auto f=m?reinterpret_cast<unsigned(__cdecl*)(uint64_t)>(GetProcAddress(m,"SC_VehicleBridgeVersion")):nullptr;return f&&f(uint64_t(instance))>=1;}
Filter bridge(){if(delegated())return nullptr;auto legacy=GetModuleHandleW(L"SeatedInput.dll");if(legacy&&!GetProcAddress(legacy,"SC_VehicleBridgeVersion"))return nullptr;auto m=GetModuleHandleW(L"FilthyCasualVehicles.dll");return m?reinterpret_cast<Filter>(GetProcAddress(m,"FC_VehicleInput")):nullptr;}
int hand(XrPath p){char b[XR_MAX_PATH_LENGTH]{};uint32_t n=0;if(pathString&&XR_SUCCEEDED(pathString(instance,p,sizeof b,&n,b))){if(!strcmp(b,"/user/hand/left"))return 0;if(!strcmp(b,"/user/hand/right"))return 1;}return -1;}
std::string name(XrAction a){std::lock_guard<std::mutex> l(mutex);auto it=names.find(a);return it==names.end()?"":it->second;}
XrResult XRAPI_CALL action(XrActionSet s,const XrActionCreateInfo* i,XrAction* o){auto r=createAction(s,i,o);if(XR_SUCCEEDED(r)){std::lock_guard<std::mutex> l(mutex);names[*o]=i->actionName;if(i->actionType==XR_ACTION_TYPE_VIBRATION_OUTPUT&&i->countSubactionPaths==2){rideHaptic=*o;hapticHands[0]=i->subactionPaths[0];hapticHands[1]=i->subactionPaths[1];}}return r;}
XrResult XRAPI_CALL eraseAction(XrAction a){auto r=destroyAction(a);if(XR_SUCCEEDED(r)){std::lock_guard<std::mutex> l(mutex);if(rideHaptic==a)rideHaptic=XR_NULL_HANDLE;names.erase(a);samples[0].erase(a);samples[1].erase(a);}return r;}
XrResult XRAPI_CALL space(XrSession s,const XrActionSpaceCreateInfo* i,XrSpace* o){auto r=createSpace(s,i,o);if(XR_SUCCEEDED(r)){int h=hand(i->subactionPath);if(h>=0){std::lock_guard<std::mutex> l(mutex);spaces[*o]=h;}}return r;}
XrResult XRAPI_CALL eraseSpace(XrSpace s){auto r=destroySpace(s);if(XR_SUCCEEDED(r)){std::lock_guard<std::mutex> l(mutex);spaces.erase(s);}return r;}
XrResult XRAPI_CALL position(XrSpace s,XrSpace b,XrTime t,XrSpaceLocation* o){auto r=locate(s,b,t,o);auto f=bridge();if(f){int h=-1;{std::lock_guard<std::mutex> l(mutex);auto it=spaces.find(s);if(it!=spaces.end())h=it->second;}if(h>=0){constexpr auto mask=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT|XR_SPACE_LOCATION_POSITION_TRACKED_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;f("vehicle_pose",h,XR_SUCCEEDED(r)&&(o->locationFlags&mask)==mask?8:0,0,0);}}return r;}
XrResult XRAPI_CALL boolean(XrSession s,const XrActionStateGetInfo* i,XrActionStateBoolean* o){auto r=getBool(s,i,o);auto f=bridge();if(XR_SUCCEEDED(r)&&f){int h=hand(i->subactionPath);auto n=name(i->action);if(h<0&&i->subactionPath==XR_NULL_PATH&&n=="menu_click")h=0;if(h>=0){auto mode=f(n.c_str(),h,o->isActive?8:0,o->isActive?float(o->currentState):0,0);std::lock_guard<std::mutex> l(mutex);auto& a=samples[h][i->action];if(a.generation!=generation){bool v=mode?mode==2&&o->isActive:o->currentState!=0;a.changed=v!=a.value;a.value=v;a.generation=generation;}if(mode){o->currentState=a.value;o->changedSinceLastSync=a.changed;}}}return r;}
XrResult XRAPI_CALL number(XrSession s,const XrActionStateGetInfo* i,XrActionStateFloat* o){auto r=getFloat(s,i,o);auto f=bridge();if(XR_SUCCEEDED(r)&&f){int h=hand(i->subactionPath);auto n=name(i->action);if(h>=0&&f(n.c_str(),h,o->isActive?9:1,o->isActive?o->currentState:0,0)){o->currentState=0;o->changedSinceLastSync=XR_FALSE;}}return r;}
XrResult XRAPI_CALL vectorState(XrSession s,const XrActionStateGetInfo* i,XrActionStateVector2f* o){auto r=getVector(s,i,o);auto f=bridge();if(XR_SUCCEEDED(r)&&f){int h=hand(i->subactionPath);auto n=name(i->action);if(h>=0&&f(n.c_str(),h,o->isActive?10:2,o->isActive?o->currentState.x:0,o->isActive?o->currentState.y:0)){o->currentState={0,0};o->changedSinceLastSync=XR_FALSE;}}return r;}
XrResult XRAPI_CALL sync(XrSession s,const XrActionsSyncInfo* i){auto r=syncActions(s,i);if(XR_SUCCEEDED(r)){std::lock_guard<std::mutex> l(mutex);++generation;}if(XR_SUCCEEDED(r))pulseRide(s);return r;}
XrResult XRAPI_CALL eraseInstance(XrInstance i){auto r=destroyInstance(i);if(XR_SUCCEEDED(r)){std::lock_guard<std::mutex> l(mutex);names.clear();spaces.clear();samples[0].clear();samples[1].clear();instance=XR_NULL_HANDLE;generation=0;}return r;}
XrResult XRAPI_CALL gipa(XrInstance i,const char* n,PFN_xrVoidFunction* o){if(!n||!o)return XR_ERROR_VALIDATION_FAILURE;if(!strcmp(n,"xrGetInstanceProcAddr")){*o=(PFN_xrVoidFunction)gipa;return XR_SUCCESS;}if(!next)return XR_ERROR_INITIALIZATION_FAILED;auto r=next(i,n,o);if(XR_FAILED(r))return r;
#define WRAP(nm,real,fn) if(!strcmp(n,nm)){real=reinterpret_cast<decltype(real)>(*o);*o=(PFN_xrVoidFunction)fn;}
WRAP("xrCreateAction",createAction,action) WRAP("xrDestroyAction",destroyAction,eraseAction)
WRAP("xrCreateActionSpace",createSpace,space) WRAP("xrDestroySpace",destroySpace,eraseSpace)
WRAP("xrLocateSpace",locate,position) WRAP("xrGetActionStateBoolean",getBool,boolean)
WRAP("xrGetActionStateVector2f",getVector,vectorState) WRAP("xrGetActionStateFloat",getFloat,number) WRAP("xrSyncActions",syncActions,sync)
WRAP("xrDestroyInstance",destroyInstance,eraseInstance)
#undef WRAP
return r;}
XrResult XRAPI_CALL create(const XrInstanceCreateInfo* i,const XrApiLayerCreateInfo* l,XrInstance* o){if(!l||!l->nextInfo||!o)return XR_ERROR_INITIALIZATION_FAILED;auto n=l->nextInfo;next=n->nextGetInstanceProcAddr;auto c=*l;c.nextInfo=n->next;auto r=n->nextCreateApiLayerInstance(i,&c,o);if(XR_SUCCEEDED(r)){instance=*o;next(*o,"xrPathToString",(PFN_xrVoidFunction*)&pathString);next(*o,"xrApplyHapticFeedback",(PFN_xrVoidFunction*)&applyHaptic);seenRidePulse=0;}return r;}
}
extern "C" __declspec(dllexport) XrResult XRAPI_CALL xrNegotiateLoaderApiLayerInterface(const XrNegotiateLoaderInfo* i,const char* n,XrNegotiateApiLayerRequest* r){if(!i||!n||strcmp(n,"XR_APILAYER_NIKAMI_vehicles")||!r||i->minInterfaceVersion>1||i->maxInterfaceVersion<1)return XR_ERROR_INITIALIZATION_FAILED;r->layerInterfaceVersion=1;r->layerApiVersion=XR_MAKE_VERSION(1,0,0);r->getInstanceProcAddr=gipa;r->createApiLayerInstance=create;return XR_SUCCESS;}
