#pragma once
#include <mutex>
#include <unordered_map>
#include <vector>
#include <cmath>
#include "height_sample.hpp"
// OpenXR reference-space origin offsets, applied atomically between frames.
// Logical application handles remain stable; every locate/submission uses the
// same physical replacement space. Both eyes, hands and original UI follow it.
namespace height_layer {
inline PFN_xrCreateReferenceSpace create=nullptr;inline PFN_xrDestroySpace destroy=nullptr;
inline PFN_xrLocateViews locateViews=nullptr;inline PFN_xrLocateSpace locateSpace=nullptr;
inline PFN_xrLocateBodyJointsFB locateBody=nullptr;inline PFN_xrLocateHandJointsEXT locateHand=nullptr;
inline PFN_xrWaitFrame waitFrame=nullptr;inline PFN_xrEndFrame realEnd=nullptr,forwardEnd=nullptr;
inline PFN_xrDestroySession realDestroySession=nullptr,forwardDestroySession=nullptr;
struct Ref {XrSession session;XrReferenceSpaceCreateInfo original;XrSpace current;};
inline std::recursive_mutex mutex;inline std::unordered_map<XrSpace,Ref> refs;
inline std::atomic<int64_t> frameTime{0};
inline void(*trace)(const char*,int)=nullptr;
inline bool hadViews=false,headTracked=false;
inline unsigned trackingEdges=0;
inline unsigned bodyCalls=0,bodyActive=0,traceSamples=0;inline float bodyHeadY=0,bodyHipsY=0;
inline height_sample::Window heightWindow;inline XrSpace heightBase=XR_NULL_HANDLE;
inline ULONGLONG heightBaseSeen=0;
inline float applied=0;inline XrPosef lastHead{{0,0,0,1},{0,0,0}};inline XrSpace lastBase=XR_NULL_HANDLE;
using Height=float(__cdecl*)(float);
inline Height bridge(){auto m=GetModuleHandleA("SeatedControls.dll");return m?reinterpret_cast<Height>(GetProcAddress(m,"SC_Height")):nullptr;}
inline bool adjustable(const Ref& r){return r.original.referenceSpaceType==XR_REFERENCE_SPACE_TYPE_STAGE||r.original.referenceSpaceType==XR_REFERENCE_SPACE_TYPE_LOCAL;}
inline XrSpace resolve(XrSpace s){auto i=refs.find(s);return i==refs.end()?s:i->second.current;}
inline void init(PFN_xrGetInstanceProcAddr get,XrInstance instance){
 #define LOAD(n,v) get(instance,n,reinterpret_cast<PFN_xrVoidFunction*>(&v))
 LOAD("xrCreateReferenceSpace",create);LOAD("xrDestroySpace",destroy);LOAD("xrLocateViews",locateViews);LOAD("xrLocateSpace",locateSpace);
 LOAD("xrWaitFrame",waitFrame);LOAD("xrEndFrame",realEnd);LOAD("xrDestroySession",realDestroySession);
 #undef LOAD
 if(auto f=bridge())applied=f(0);
}
inline XrResult XRAPI_CALL onCreate(XrSession session,const XrReferenceSpaceCreateInfo* info,XrSpace* out){
 std::lock_guard<std::recursive_mutex> lock(mutex);if(info->next)return create(session,info,out);
 Ref ref{session,*info,XR_NULL_HANDLE};auto ci=*info;if(adjustable(ref))ci.poseInReferenceSpace.position.y-=applied;
 auto result=create(session,&ci,out);if(XR_SUCCEEDED(result)){ref.current=*out;refs[*out]=ref;}return result;
}
inline XrResult XRAPI_CALL onDestroy(XrSpace s){
 std::lock_guard<std::recursive_mutex> lock(mutex);auto i=refs.find(s);if(i!=refs.end()){if(i->second.current!=s)destroy(i->second.current);refs.erase(i);}if(s==heightBase){heightBase=XR_NULL_HANDLE;heightWindow.reset();if(auto f=bridge())f(NAN);}return destroy(s);
}
inline void applyTarget(float target){
 if(!std::isfinite(target)||std::abs(target-applied)<.0001f)return;
 std::vector<std::pair<XrSpace,XrSpace>> pending;
 for(auto& e:refs){if(!adjustable(e.second))continue;auto ci=e.second.original;ci.poseInReferenceSpace.position.y-=target;XrSpace fresh=XR_NULL_HANDLE;
  if(XR_FAILED(create(e.second.session,&ci,&fresh))){for(auto& p:pending)destroy(p.second);return;}pending.push_back({e.first,fresh});}
 for(auto& p:pending){auto& ref=refs[p.first];if(ref.current!=p.first)destroy(ref.current);ref.current=p.second;}applied=target;
}
inline void applyPending(){if(auto f=bridge())applyTarget(f(0));}
inline XrResult XRAPI_CALL onWaitFrame(XrSession s,const XrFrameWaitInfo* i,XrFrameState* out){
 {std::lock_guard<std::recursive_mutex> lock(mutex);applyPending();}auto result=waitFrame(s,i,out);if(XR_SUCCEEDED(result))frameTime=out->predictedDisplayTime;return result;
}
inline XrResult XRAPI_CALL onLocateSpace(XrSpace space,XrSpace base,XrTime time,XrSpaceLocation* out){std::lock_guard<std::recursive_mutex> lock(mutex);return locateSpace(resolve(space),resolve(base),time,out);}
// Extension joints must follow the same replacement origin as eyes/action spaces.
// Forward the runtime's joints, confidence, flags and next chains unchanged.
inline XrResult XRAPI_CALL onLocateBody(XrBodyTrackerFB tracker,const XrBodyJointsLocateInfoFB* info,XrBodyJointLocationsFB* out){
 std::lock_guard<std::recursive_mutex> lock(mutex);auto translated=*info;translated.baseSpace=resolve(info->baseSpace);auto result=locateBody(tracker,&translated,out);++bodyCalls;
 const bool active=XR_SUCCEEDED(result)&&out->isActive;
 if(trace&&active!=bool(bodyActive))trace(active?"OpenXR body track active":"OpenXR body track inactive",int(result));
 bodyActive=active;
 if(bodyActive&&out->jointLocations&&out->jointCount>XR_BODY_JOINT_HEAD_FB){
  const auto& head=out->jointLocations[XR_BODY_JOINT_HEAD_FB];const auto& hips=out->jointLocations[XR_BODY_JOINT_HIPS_FB];
  bodyHeadY=(head.locationFlags&XR_SPACE_LOCATION_POSITION_VALID_BIT)?head.pose.position.y:NAN;
  bodyHipsY=(hips.locationFlags&XR_SPACE_LOCATION_POSITION_VALID_BIT)?hips.pose.position.y:NAN;
 }else bodyHeadY=bodyHipsY=NAN;
 return result;
}
inline XrResult XRAPI_CALL onLocateHand(XrHandTrackerEXT tracker,const XrHandJointsLocateInfoEXT* info,XrHandJointLocationsEXT* out){
 std::lock_guard<std::recursive_mutex> lock(mutex);auto translated=*info;translated.baseSpace=resolve(info->baseSpace);return locateHand(tracker,&translated,out);
}
inline XrResult XRAPI_CALL onLocateViews(XrSession s,const XrViewLocateInfo* info,XrViewState* state,uint32_t cap,uint32_t* count,XrView* views){
 std::lock_guard<std::recursive_mutex> lock(mutex);auto translated=*info;translated.space=resolve(info->space);auto result=locateViews(s,&translated,state,cap,count,views);
 auto base=refs.find(info->space);bool gameplayBase=base!=refs.end()&&adjustable(base->second);
 constexpr auto tracked=XR_VIEW_STATE_POSITION_VALID_BIT|XR_VIEW_STATE_ORIENTATION_VALID_BIT|XR_VIEW_STATE_POSITION_TRACKED_BIT|XR_VIEW_STATE_ORIENTATION_TRACKED_BIT;
 const bool tracking=XR_SUCCEEDED(result)&&(state->viewStateFlags&tracked)==tracked;
 if(gameplayBase&&(!hadViews||headTracked!=tracking)){
  hadViews=true;headTracked=tracking;
  if(trace&&trackingEdges++<128)trace(tracking?"head tracking valid":"head tracking lost",XR_SUCCEEDED(result)?int(state->viewStateFlags):int(result));
 }
 if(gameplayBase&&XR_SUCCEEDED(result)&&cap>=2&&*count>=2&&(state->viewStateFlags&XR_VIEW_STATE_POSITION_VALID_BIT)){
  lastHead=views[0].pose;lastHead.position.x=(views[0].pose.position.x+views[1].pose.position.x)*.5f;lastHead.position.y=(views[0].pose.position.y+views[1].pose.position.y)*.5f;lastHead.position.z=(views[0].pose.position.z+views[1].pose.position.z)*.5f;lastBase=info->space;
  if(traceSamples++%600==0&&traceSamples<=72001){
   using Diagnostic=unsigned(__cdecl*)(const char*,int,int,float,float);auto m=GetModuleHandleA("SeatedControls.dll");auto log=m?reinterpret_cast<Diagnostic>(GetProcAddress(m,"SC_Input")):nullptr;
   if(log){char message[256];const auto a=views[0].pose.position,b=views[1].pose.position;
    snprintf(message,sizeof(message),"fc_diag:tracking offset=%.3f rawEye=%.3f eye=%.3f ipd=%.4f flags=%llu bodyCalls=%u bodyActive=%u bodyHead=%.3f bodyHips=%.3f",applied,lastHead.position.y-applied,lastHead.position.y,std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z)),static_cast<unsigned long long>(state->viewStateFlags),bodyCalls,bodyActive,bodyHeadY,bodyHipsY);log(message,0,0,0,0);}
  }
  const auto now=GetTickCount64();
  if(tracking&&(!heightBase||now-heightBaseSeen>250)){
   heightWindow.reset();heightBase=info->space;
  }
  if(heightBase==info->space){
   if(tracking)heightBaseSeen=now;
   // Undo the application's reference-space pose before removing our offset.
   // A translated/rotated local space is not itself a floor-height measurement.
   const auto q=base->second.original.poseInReferenceSpace.orientation;
   const auto v=lastHead.position;
   const float tx=2*(q.y*v.z-q.z*v.y),ty=2*(q.z*v.x-q.x*v.z),tz=2*(q.x*v.y-q.y*v.x);
   const float raw=base->second.original.poseInReferenceSpace.position.y+v.y+q.w*ty+q.z*tx-q.x*tz-applied;
   const bool stable=heightWindow.sample(raw,info->displayTime,tracking);
   if(auto f=bridge())f(stable?heightWindow.value:NAN);
  }
  using Head=void(__cdecl*)(float,float,float);auto module=GetModuleHandleA("SeatedControls.dll");auto publish=module?reinterpret_cast<Head>(GetProcAddress(module,"SC_Head")):nullptr;
  if(publish)publish(lastHead.position.x,lastHead.position.y,lastHead.position.z);
  auto mouth=module?reinterpret_cast<Head>(GetProcAddress(module,"SC_Mouth")):nullptr;
  if(mouth&&(state->viewStateFlags&XR_VIEW_STATE_ORIENTATION_VALID_BIT)){
   // Approximate mouth below and slightly forward of the actual stereo midpoint.
   const auto q=lastHead.orientation;const XrVector3f v{0,-.11f,-.04f};
   XrVector3f t{2*(q.y*v.z-q.z*v.y),2*(q.z*v.x-q.x*v.z),2*(q.x*v.y-q.y*v.x)};
   mouth(lastHead.position.x+v.x+q.w*t.x+q.y*t.z-q.z*t.y,
         lastHead.position.y+v.y+q.w*t.y+q.z*t.x-q.x*t.z,
         lastHead.position.z+v.z+q.w*t.z+q.x*t.y-q.y*t.x);
  }else if(mouth)mouth(NAN,NAN,NAN);
 }else if(gameplayBase){
  if(info->space==heightBase){heightWindow.reset();if(auto f=bridge())f(NAN);}
  using Head=void(__cdecl*)(float,float,float);auto module=GetModuleHandleA("SeatedControls.dll");
  if(auto mouth=module?reinterpret_cast<Head>(GetProcAddress(module,"SC_Mouth")):nullptr)mouth(NAN,NAN,NAN);
 }return result;
}
inline XrResult XRAPI_CALL onEndFrame(XrSession s,const XrFrameEndInfo* info){
 std::lock_guard<std::recursive_mutex> lock(mutex);
 std::vector<XrCompositionLayerProjection> projections;std::vector<XrCompositionLayerQuad> quads;
 projections.reserve(info->layerCount);quads.reserve(info->layerCount);std::vector<const XrCompositionLayerBaseHeader*> layers;
 for(uint32_t i=0;i<info->layerCount;i++){auto original=info->layers[i];
  if(original->type==XR_TYPE_COMPOSITION_LAYER_PROJECTION){projections.push_back(*reinterpret_cast<const XrCompositionLayerProjection*>(original));projections.back().space=resolve(original->space);layers.push_back(reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projections.back()));}
  else if(original->type==XR_TYPE_COMPOSITION_LAYER_QUAD){quads.push_back(*reinterpret_cast<const XrCompositionLayerQuad*>(original));quads.back().space=resolve(original->space);layers.push_back(reinterpret_cast<const XrCompositionLayerBaseHeader*>(&quads.back()));}
  else layers.push_back(original);
 }auto submission=*info;submission.layers=layers.data();return forwardEnd(s,&submission);
}
inline XrResult XRAPI_CALL onDestroySession(XrSession session){
 std::lock_guard<std::recursive_mutex> lock(mutex);for(auto i=refs.begin();i!=refs.end();){if(i->second.session==session){if(i->second.current!=i->first)destroy(i->second.current);i=refs.erase(i);}else ++i;}lastBase=XR_NULL_HANDLE;heightBase=XR_NULL_HANDLE;heightWindow.reset();hadViews=false;headTracked=false;trackingEdges=0;bodyCalls=bodyActive=traceSamples=0;bodyHeadY=bodyHipsY=NAN;if(auto f=bridge())f(NAN);return forwardDestroySession(session);
}
}
