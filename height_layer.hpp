#pragma once
#include <mutex>
#include <unordered_map>
#include <vector>
#include <cmath>
// OpenXR reference-space origin offsets, applied atomically between frames.
// Logical application handles remain stable; every locate/submission uses the
// same physical replacement space. Both eyes, hands and original UI follow it.
namespace height_layer {
inline PFN_xrCreateReferenceSpace create=nullptr;inline PFN_xrDestroySpace destroy=nullptr;
inline PFN_xrLocateViews locateViews=nullptr;inline PFN_xrLocateSpace locateSpace=nullptr;
inline PFN_xrWaitFrame waitFrame=nullptr;inline PFN_xrEndFrame realEnd=nullptr,forwardEnd=nullptr;
inline PFN_xrDestroySession realDestroySession=nullptr,forwardDestroySession=nullptr;
struct Ref {XrSession session;XrReferenceSpaceCreateInfo original;XrSpace current;};
inline std::recursive_mutex mutex;inline std::unordered_map<XrSpace,Ref> refs;
inline std::atomic<int64_t> frameTime{0};
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
 std::lock_guard<std::recursive_mutex> lock(mutex);auto i=refs.find(s);if(i!=refs.end()){if(i->second.current!=s)destroy(i->second.current);refs.erase(i);}return destroy(s);
}
inline void applyPending(){
 auto f=bridge();if(!f)return;float target=f(0);if(!std::isfinite(target)||std::abs(target-applied)<.0001f)return;
 std::vector<std::pair<XrSpace,XrSpace>> pending;
 for(auto& e:refs){if(!adjustable(e.second))continue;auto ci=e.second.original;ci.poseInReferenceSpace.position.y-=target;XrSpace fresh=XR_NULL_HANDLE;
  if(XR_FAILED(create(e.second.session,&ci,&fresh))){for(auto& p:pending)destroy(p.second);return;}pending.push_back({e.first,fresh});}
 for(auto& p:pending){auto& ref=refs[p.first];if(ref.current!=p.first)destroy(ref.current);ref.current=p.second;}applied=target;
}
inline XrResult XRAPI_CALL onWaitFrame(XrSession s,const XrFrameWaitInfo* i,XrFrameState* out){
 {std::lock_guard<std::recursive_mutex> lock(mutex);applyPending();}auto result=waitFrame(s,i,out);if(XR_SUCCEEDED(result))frameTime=out->predictedDisplayTime;return result;
}
inline XrResult XRAPI_CALL onLocateSpace(XrSpace space,XrSpace base,XrTime time,XrSpaceLocation* out){std::lock_guard<std::recursive_mutex> lock(mutex);return locateSpace(resolve(space),resolve(base),time,out);}
inline XrResult XRAPI_CALL onLocateViews(XrSession s,const XrViewLocateInfo* info,XrViewState* state,uint32_t cap,uint32_t* count,XrView* views){
 std::lock_guard<std::recursive_mutex> lock(mutex);auto translated=*info;translated.space=resolve(info->space);auto result=locateViews(s,&translated,state,cap,count,views);
 auto base=refs.find(info->space);bool gameplayBase=base!=refs.end()&&adjustable(base->second);
 if(gameplayBase&&XR_SUCCEEDED(result)&&cap>=2&&*count>=2&&(state->viewStateFlags&XR_VIEW_STATE_POSITION_VALID_BIT)){
  lastHead=views[0].pose;lastHead.position.x=(views[0].pose.position.x+views[1].pose.position.x)*.5f;lastHead.position.y=(views[0].pose.position.y+views[1].pose.position.y)*.5f;lastHead.position.z=(views[0].pose.position.z+views[1].pose.position.z)*.5f;lastBase=info->space;
  auto r=refs.find(info->space);if(r!=refs.end()&&adjustable(r->second))if(auto f=bridge())f(lastHead.position.y-applied);
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
 std::lock_guard<std::recursive_mutex> lock(mutex);for(auto i=refs.begin();i!=refs.end();){if(i->second.session==session){if(i->second.current!=i->first)destroy(i->second.current);i=refs.erase(i);}else ++i;}lastBase=XR_NULL_HANDLE;return forwardDestroySession(session);
}
}
