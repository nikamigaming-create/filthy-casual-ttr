#pragma once
#include <vector>
#include "controls_runtime.hpp"
// Additional, non-consuming Touch sensor actions. Native suggestions and active
// sets are retained. These actions never generate button presses or grip input.
namespace gesture_input {
using Report=void(*)(int,bool,bool,bool);
using Controls=void(*)(unsigned,bool);
using Diagnostic=void(*)(const char*,int);
inline Diagnostic diagnostic=nullptr;
inline PFN_xrGetInstanceProcAddr upstream=nullptr;
inline unsigned attempts=0;
inline bool initializing=false;
inline void note(const char* stage,XrResult result){if(diagnostic)diagnostic(stage,int(result));}
inline PFN_xrSuggestInteractionProfileBindings suggest=nullptr;
inline PFN_xrAttachSessionActionSets attach=nullptr;
inline PFN_xrSyncActions sync=nullptr;
inline PFN_xrGetActionStateBoolean booleanState=nullptr;
inline PFN_xrStringToPath stringPath=nullptr;
inline PFN_xrPathToString pathString=nullptr;
inline XrInstance instance=XR_NULL_HANDLE;
inline XrActionSet set=XR_NULL_HANDLE;
inline XrAction actions[13]{};
inline PFN_xrGetActionStateFloat floatState=nullptr;
inline PFN_xrGetActionStateVector2f vectorState=nullptr;
inline XrPath hands[2]{};inline XrSpace aim[2]{};
inline PFN_xrCreateActionSpace createActionSpace=nullptr;inline PFN_xrLocateSpace locateSpace=nullptr;inline PFN_xrDestroySpace destroySpace=nullptr;
inline void endSession(){for(auto&space:aim){if(space&&destroySpace)destroySpace(space);space=XR_NULL_HANDLE;}controls_runtime::invalidate();}
inline bool attached=false,ready=false;
inline uint64_t generation=0;
inline Report report=nullptr;
inline Controls controls=nullptr;
inline void reset(){controls_runtime::invalidate();instance=XR_NULL_HANDLE;set=XR_NULL_HANDLE;for(auto& a:actions)a=XR_NULL_HANDLE;attached=ready=false;aim[0]=aim[1]=XR_NULL_HANDLE;}
inline std::vector<XrActionSuggestedBinding> bindings(){
 std::vector<XrActionSuggestedBinding> out;
 for(int h=0;h<2;h++)for(int a=0;a<13;a++){
  if(a==5&&h==1)continue;
  const char* leaf=a==0?"thumbrest/touch":a==1?(h?"a/touch":"x/touch"):a==2?(h?"b/touch":"y/touch"):a==3?"thumbstick/touch":a==4?"trigger/touch":a==5?"menu/click":a==6?"trigger/value":a==7?"squeeze/value":a==8?(h?"a/click":"x/click"):a==9?(h?"b/click":"y/click"):a==10?"thumbstick/click":a==11?"thumbstick":"aim/pose";
  std::string p=std::string(h?"/user/hand/right/input/":"/user/hand/left/input/")+leaf;XrPath path=XR_NULL_PATH;
  if(XR_SUCCEEDED(stringPath(instance,p.c_str(),&path)))out.push_back({actions[a],path});
 }
 return out;
}
inline XrResult XRAPI_CALL onSuggest(XrInstance i,const XrInteractionProfileSuggestedBinding* info){
 if(!ready||!info)return suggest(i,info);
 char name[XR_MAX_PATH_LENGTH]{};uint32_t count=0;
 if(XR_FAILED(pathString(i,info->interactionProfile,sizeof(name),&count,name)))return suggest(i,info);
 if(strcmp(name,"/interaction_profiles/oculus/touch_controller")&&strcmp(name,"/interaction_profiles/meta/touch_controller_plus")&&strcmp(name,"/interaction_profiles/facebook/touch_controller_pro"))return suggest(i,info);
 auto all=bindings();all.insert(all.end(),info->suggestedBindings,info->suggestedBindings+info->countSuggestedBindings);
 auto merged=*info;merged.countSuggestedBindings=uint32_t(all.size());merged.suggestedBindings=all.data();
 auto r=suggest(i,&merged);return XR_SUCCEEDED(r)?r:suggest(i,info);
}
inline XrResult XRAPI_CALL onAttach(XrSession s,const XrSessionActionSetsAttachInfo* info){
 if(!ready||!info)return attach(s,info);
 std::vector<XrActionSet> all(info->actionSets,info->actionSets+info->countActionSets);all.push_back(set);
 auto merged=*info;merged.countActionSets=uint32_t(all.size());merged.actionSets=all.data();
 auto r=attach(s,&merged);attached=XR_SUCCEEDED(r);note("attach",r);
 if(XR_FAILED(r)){controls_runtime::invalidate();ready=false;if(report){report(0,false,false,false);report(1,false,false,false);}if(controls)controls(0,false);return attach(s,info);}
 if(XR_SUCCEEDED(r))for(int h=0;h<2;h++){XrActionSpaceCreateInfo ci{XR_TYPE_ACTION_SPACE_CREATE_INFO};ci.action=actions[12];ci.subactionPath=hands[h];ci.poseInActionSpace.orientation.w=1;if(!aim[h])createActionSpace(s,&ci,&aim[h]);}
 return r;
}
inline XrResult XRAPI_CALL onSync(XrSession s,const XrActionsSyncInfo* info){
 ++generation;
 if(!attached||!info)return sync(s,info);
 std::vector<XrActiveActionSet> all(info->activeActionSets,info->activeActionSets+info->countActiveActionSets);all.push_back({set,XR_NULL_PATH});
 auto merged=*info;merged.countActiveActionSets=uint32_t(all.size());merged.activeActionSets=all.data();
 auto r=sync(s,&merged);
 if(XR_FAILED(r)){controls_runtime::invalidate();note("sync",r);attached=false;ready=false;if(report){report(0,false,false,false);report(1,false,false,false);}if(controls)controls(0,false);return sync(s,info);}
 unsigned controlMask=0;int activeControls=0;fc_controls::Frame physical{};
 for(int h=0;h<2;h++){
  bool thumb=false,index=false,thumbActive=false,indexActive=false;
  if(r==XR_SUCCESS)for(int a=0;a<13;a++){
  if(a==5&&h==1)continue;
   XrActionStateGetInfo query{XR_TYPE_ACTION_STATE_GET_INFO};query.action=actions[a];query.subactionPath=hands[h];
   if(a==12)continue;
   if(a>=6){
    if(a==11){XrActionStateVector2f v{XR_TYPE_ACTION_STATE_VECTOR2F};if(XR_SUCCEEDED(vectorState(s,&query,&v))){int base=11+h*4;physical[base]={std::max(0.f,-v.currentState.x),v.isActive!=0,v.lastChangeTime};physical[base+1]={std::max(0.f,v.currentState.x),v.isActive!=0,v.lastChangeTime};physical[base+2]={std::max(0.f,-v.currentState.y),v.isActive!=0,v.lastChangeTime};physical[base+3]={std::max(0.f,v.currentState.y),v.isActive!=0,v.lastChangeTime};}}
    else if(a<=7){XrActionStateFloat v{XR_TYPE_ACTION_STATE_FLOAT};if(XR_SUCCEEDED(floatState(s,&query,&v)))physical[h*5+a-6]={v.currentState,v.isActive!=0,v.lastChangeTime};}
    else{XrActionStateBoolean v{XR_TYPE_ACTION_STATE_BOOLEAN};if(XR_SUCCEEDED(booleanState(s,&query,&v)))physical[h*5+a-6]={float(v.currentState),v.isActive!=0,v.lastChangeTime};}
    continue;
   }
   XrActionStateBoolean value{XR_TYPE_ACTION_STATE_BOOLEAN};
   if(XR_SUCCEEDED(booleanState(s,&query,&value))&&value.isActive){
    int source=a==0?19+h:a==1?23+h*2:a==2?24+h*2:a==3?21+h:a==4?27+h:10;
    physical[source]={float(value.currentState),true,value.lastChangeTime};
    if(a==5){++activeControls;if(value.currentState)controlMask|=4;}
    else if(a==4){indexActive=true;index=value.currentState!=0;}else{thumbActive=true;thumb|=value.currentState!=0;if(a==0){++activeControls;if(value.currentState)controlMask|=1u<<h;}if(a==3){++activeControls;if(value.currentState)controlMask|=8u<<h;}}
   }
  }
  if(report)report(h,thumbActive&&indexActive,thumb,index);
 }
 controls_runtime::update(physical,GetTickCount64());
 if(controls)controls(controlMask,activeControls==5);
 return r;
}
inline void init(PFN_xrGetInstanceProcAddr gipa,XrInstance i,Report callback,Controls controlCallback){
 // A repeated layer creation callback must not discard an already registered
 // action set and then fail to recreate it with XR_ERROR_NAME_DUPLICATED.
 if(initializing)return;
 if(instance==i&&set!=XR_NULL_HANDLE){note("init-already-registered",XR_SUCCESS);return;}
 struct Initializing {Initializing(){initializing=true;}~Initializing(){initializing=false;}} initializingScope;
 if(instance!=i)attempts=0;
 ++attempts;upstream=gipa;note("init-attempt",XrResult(attempts));
 reset();instance=i;report=callback;controls=controlCallback;
 PFN_xrCreateActionSet createSet=nullptr;PFN_xrCreateAction create=nullptr;PFN_xrDestroyActionSet destroySet=nullptr;
 #define SC_XR_GET(name,ptr) {auto result=gipa(i,name,reinterpret_cast<PFN_xrVoidFunction*>(&ptr));if(XR_FAILED(result)){note(name,result);return;}}
 SC_XR_GET("xrCreateActionSpace",createActionSpace);SC_XR_GET("xrLocateSpace",locateSpace);SC_XR_GET("xrDestroySpace",destroySpace);SC_XR_GET("xrCreateActionSet",createSet);SC_XR_GET("xrCreateAction",create);SC_XR_GET("xrDestroyActionSet",destroySet);
 SC_XR_GET("xrSuggestInteractionProfileBindings",suggest);SC_XR_GET("xrAttachSessionActionSets",attach);SC_XR_GET("xrSyncActions",sync);
 SC_XR_GET("xrGetActionStateFloat",floatState);SC_XR_GET("xrGetActionStateVector2f",vectorState);SC_XR_GET("xrGetActionStateBoolean",booleanState);SC_XR_GET("xrStringToPath",stringPath);SC_XR_GET("xrPathToString",pathString);
 #undef SC_XR_GET
 if(XR_FAILED(stringPath(i,"/user/hand/left",&hands[0]))||XR_FAILED(stringPath(i,"/user/hand/right",&hands[1])))return;
 XrActionSetCreateInfo ci{XR_TYPE_ACTION_SET_CREATE_INFO};strcpy_s(ci.actionSetName,"fc_gestures");strcpy_s(ci.localizedActionSetName,"Filthy Casual finger gestures");ci.priority=0;
 auto created=createSet(i,&ci,&set);note("create-set",created);if(XR_FAILED(created))return;
 const char* names[]={"fc_thumb_rest","fc_thumb_primary","fc_thumb_secondary","fc_thumb_stick","fc_index_touch","fc_menu_click","fc_raw_trigger","fc_raw_grip","fc_raw_primary","fc_raw_secondary","fc_raw_stick_click","fc_raw_stick","fc_editor_aim"};
 for(int a=0;a<13;a++){
  XrActionCreateInfo ai{XR_TYPE_ACTION_CREATE_INFO};strcpy_s(ai.actionName,names[a]);strcpy_s(ai.localizedActionName,names[a]);ai.actionType=a==12?XR_ACTION_TYPE_POSE_INPUT:a==11?XR_ACTION_TYPE_VECTOR2F_INPUT:(a==6||a==7)?XR_ACTION_TYPE_FLOAT_INPUT:XR_ACTION_TYPE_BOOLEAN_INPUT;ai.countSubactionPaths=a==5?1:2;ai.subactionPaths=hands;
  auto result=create(set,&ai,&actions[a]);if(XR_FAILED(result)){note(names[a],result);destroySet(set);set=XR_NULL_HANDLE;return;}
 }
 auto list=bindings();XrPath profile=XR_NULL_PATH;stringPath(i,"/interaction_profiles/oculus/touch_controller",&profile);
 XrInteractionProfileSuggestedBinding bi{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};bi.interactionProfile=profile;bi.countSuggestedBindings=uint32_t(list.size());bi.suggestedBindings=list.data();
 // Native Touch suggestions later are merged by onSuggest, never replaced.
 auto result=suggest(i,&bi);note("suggest-touch",result);ready=list.size()==25&&XR_SUCCEEDED(result);
 if(!ready){destroySet(set);set=XR_NULL_HANDLE;}
}
inline void ensure(){if(!ready&&attempts<2&&upstream&&instance)init(upstream,instance,report,controls);}
}
