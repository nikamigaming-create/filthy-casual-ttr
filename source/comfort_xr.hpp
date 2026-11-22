#pragma once
#include "comfort_controls.hpp"
#include "comfort_pose.hpp"
// Poses use OpenXR metres, XYZW quaternions, and the same reference space/time
// as the native locate. One grip delta also transforms aim; never filter bones.
namespace comfort_xr {
inline fc_comfort::Config config;inline fc_comfort::Grip grips[2];inline fc_comfort::Walker walker;
inline std::mutex mutex;inline uint64_t configGeneration=~uint64_t(0);inline bool focused=false;
inline XrHandTrackerEXT trackers[2]{};inline XrSession session{};inline bool attempted[2]{};
inline PFN_xrCreateHandTrackerEXT createTracker=nullptr;inline PFN_xrDestroyHandTrackerEXT destroyTracker=nullptr;inline PFN_xrLocateHandJointsEXT locateJoints=nullptr;
inline bool handExtension=false,motionRange=false,dataSource=false;
inline fc_comfort::Config read(){fc_comfort::Config c;auto m=GetModuleHandleW(L"SeatedControls.dll");auto f=m?reinterpret_cast<bool(__cdecl*)(fc_comfort::Config*,unsigned)>(GetProcAddress(m,"FC_ComfortConfig")):nullptr;if(f)f(&c,sizeof c);fc_comfort::sanitize(c);return c;}
inline void refresh(){if(configGeneration!=gesture_input::generation){config=read();configGeneration=gesture_input::generation;} }
inline void pose(int h,XrSpace base,XrTime time,XrSpaceLocation* out){std::lock_guard<std::recursive_mutex> frameLock(height_layer::mutex);std::lock_guard<std::mutex> lock(mutex);refresh();constexpr auto flags=XR_SPACE_LOCATION_POSITION_TRACKED_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT|XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
 auto ref=height_layer::refs.find(base);
 // Never alter controller-to-controller or controller-to-item relative poses.
 bool worldBase=ref!=height_layer::refs.end()&&height_layer::adjustable(ref->second);
 if(!worldBase||!focused||!config.smoothing||!config.gameplay||!gesture_input::grip[h]||(out->locationFlags&flags)!=flags){filters[h]={};return;}XrSpaceLocation raw{XR_TYPE_SPACE_LOCATION};if(XR_FAILED(height_layer::onLocateSpace(gesture_input::grip[h],base,time,&raw))||(raw.locationFlags&flags)!=flags){filters[h]={};return;}auto smooth=filtered(filters[h],raw.pose,base,time,config.smoothMs);out->pose=transport(raw.pose,smooth,out->pose);
 // A velocity chain would describe the unfiltered pose. Do not publish stale velocities.
 for(auto n=reinterpret_cast<XrBaseOutStructure*>(out->next);n;n=n->next)if(n->type==XR_TYPE_SPACE_VELOCITY)reinterpret_cast<XrSpaceVelocity*>(n)->velocityFlags=0;
}
inline bool squeeze(int h,fc_controls::Sample& v){std::lock_guard<std::mutex> lock(mutex);refresh();if(!config.grip){grips[h]={};return false;}v.value=grips[h].value(v.value,v.active&&focused,config);return true;}
inline bool walk(float& x,float& y,bool active){std::lock_guard<std::mutex> lock(mutex);refresh();walker.apply(x,y,active&&focused,config);return config.walking&&config.gameplay;}
inline void report(int h,const float* curls){auto m=GetModuleHandleW(L"SeatedControls.dll");auto fn=m?reinterpret_cast<void(__cdecl*)(int,const float*)>(GetProcAddress(m,"FC_SkeletalFingers")):nullptr;if(fn)fn(h,curls);}
inline void stop(){std::lock_guard<std::mutex> lock(mutex);for(int h=0;h<2;h++){if(trackers[h]&&destroyTracker)destroyTracker(trackers[h]);trackers[h]=XR_NULL_HANDLE;attempted[h]=false;filters[h]={};grips[h]={};report(h,nullptr);}session=XR_NULL_HANDLE;walker={};focused=false;configGeneration=~uint64_t(0);}
inline void init(PFN_xrGetInstanceProcAddr g,XrInstance i){handExtension=gesture_input::enabledExtensions.count("XR_EXT_hand_tracking")!=0;motionRange=gesture_input::enabledExtensions.count("XR_EXT_hand_joints_motion_range")!=0;dataSource=gesture_input::enabledExtensions.count("XR_EXT_hand_tracking_data_source")!=0;createTracker=nullptr;destroyTracker=nullptr;locateJoints=nullptr;if(!handExtension)return;g(i,"xrCreateHandTrackerEXT",reinterpret_cast<PFN_xrVoidFunction*>(&createTracker));g(i,"xrDestroyHandTrackerEXT",reinterpret_cast<PFN_xrVoidFunction*>(&destroyTracker));g(i,"xrLocateHandJointsEXT",reinterpret_cast<PFN_xrVoidFunction*>(&locateJoints));}
inline void sample(XrSession s,bool focus){std::lock_guard<std::mutex> lock(mutex);refresh();focused=focus;session=s;if(!focus||!config.gameplay){walker.active=walker.running=false;walker.request=config.walkRequest;for(auto& f:filters)f={};}if(!focus)for(auto& g:grips)g={};
 for(int h=0;h<2;h++){
  bool index=gesture_input::devices[h]&&!strcmp(gesture_input::devices[h]->id,"index");
  if(!config.fingers||!focus||!index||!handExtension||!createTracker||!destroyTracker||!locateJoints||!height_layer::lastBase||!height_layer::frameTime.load()){report(h,nullptr);continue;}
  if(!attempted[h]){attempted[h]=true;XrHandTrackingDataSourceEXT source=XR_HAND_TRACKING_DATA_SOURCE_CONTROLLER_EXT;XrHandTrackingDataSourceInfoEXT ds{XR_TYPE_HAND_TRACKING_DATA_SOURCE_INFO_EXT};ds.requestedDataSourceCount=1;ds.requestedDataSources=&source;XrHandTrackerCreateInfoEXT ci{XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};ci.hand=h?XR_HAND_RIGHT_EXT:XR_HAND_LEFT_EXT;ci.handJointSet=XR_HAND_JOINT_SET_DEFAULT_EXT;ci.next=dataSource?&ds:nullptr;auto r=createTracker(s,&ci,&trackers[h]);gesture_input::note("Index hand tracker",r);}
  if(!trackers[h]){report(h,nullptr);continue;}
  XrHandJointLocationEXT joints[XR_HAND_JOINT_COUNT_EXT]{};XrHandTrackingDataSourceStateEXT state{XR_TYPE_HAND_TRACKING_DATA_SOURCE_STATE_EXT};XrHandJointLocationsEXT out{XR_TYPE_HAND_JOINT_LOCATIONS_EXT};out.jointCount=XR_HAND_JOINT_COUNT_EXT;out.jointLocations=joints;out.next=dataSource?&state:nullptr;
  XrHandJointsMotionRangeInfoEXT range{XR_TYPE_HAND_JOINTS_MOTION_RANGE_INFO_EXT};range.handJointsMotionRange=XR_HAND_JOINTS_MOTION_RANGE_UNOBSTRUCTED_EXT;XrHandJointsLocateInfoEXT li{XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT};li.next=motionRange?&range:nullptr;li.baseSpace=height_layer::resolve(height_layer::lastBase);li.time=height_layer::frameTime.load();float curls[5]{};bool ok=XR_SUCCEEDED(locateJoints(trackers[h],&li,&out))&&out.isActive&&(!dataSource||(state.isActive&&state.dataSource==XR_HAND_TRACKING_DATA_SOURCE_CONTROLLER_EXT));
  int first[]={2,6,11,16,21};for(int f=0;ok&&f<5;f++){fc_comfort::V p[5]{};int count=f?5:4;for(int k=0;k<count;k++){auto& j=joints[first[f]+k];ok=ok&&(j.locationFlags&XR_SPACE_LOCATION_POSITION_VALID_BIT);p[k]={j.pose.position.x,j.pose.position.y,j.pose.position.z};}ok=ok&&fc_comfort::fingerCurl(p,count,curls[f]);}report(h,ok?curls:nullptr);
 }
}
}
