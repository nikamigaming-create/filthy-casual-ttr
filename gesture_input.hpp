#pragma once
#include <vector>
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
inline XrAction actions[6]{};
inline XrPath hands[2]{};
inline bool attached=false,ready=false;
inline uint64_t generation=0;
inline Report report=nullptr;
inline Controls controls=nullptr;
inline void reset(){instance=XR_NULL_HANDLE;set=XR_NULL_HANDLE;for(auto& a:actions)a=XR_NULL_HANDLE;attached=ready=false;}
inline std::vector<XrActionSuggestedBinding> bindings(){
 std::vector<XrActionSuggestedBinding> out;
 for(int h=0;h<2;h++)for(int a=0;a<6;a++){
  if(a==5&&h==1)continue;
  const char* leaf=a==0?"thumbrest/touch":a==1?(h?"a/touch":"x/touch"):a==2?(h?"b/touch":"y/touch"):a==3?"thumbstick/touch":a==4?"trigger/touch":"menu/click";
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
 if(XR_FAILED(r)){ready=false;if(report){report(0,false,false,false);report(1,false,false,false);}if(controls)controls(0,false);return attach(s,info);}
 return r;
}
inline XrResult XRAPI_CALL onSync(XrSession s,const XrActionsSyncInfo* info){
 ++generation;
 if(!attached||!info)return sync(s,info);
 std::vector<XrActiveActionSet> all(info->activeActionSets,info->activeActionSets+info->countActiveActionSets);all.push_back({set,XR_NULL_PATH});
 auto merged=*info;merged.countActiveActionSets=uint32_t(all.size());merged.activeActionSets=all.data();
 auto r=sync(s,&merged);
 if(XR_FAILED(r)){note("sync",r);attached=false;ready=false;if(report){report(0,false,false,false);report(1,false,false,false);}if(controls)controls(0,false);return sync(s,info);}
 unsigned controlMask=0;int activeControls=0;
 for(int h=0;h<2;h++){
  bool thumb=false,index=false,thumbActive=false,indexActive=false;
  if(r==XR_SUCCESS)for(int a=0;a<6;a++){
  if(a==5&&h==1)continue;
   XrActionStateGetInfo query{XR_TYPE_ACTION_STATE_GET_INFO};query.action=actions[a];query.subactionPath=hands[h];
   XrActionStateBoolean value{XR_TYPE_ACTION_STATE_BOOLEAN};
   if(XR_SUCCEEDED(booleanState(s,&query,&value))&&value.isActive){
    if(a==5){++activeControls;if(value.currentState)controlMask|=4;}
    else if(a==4){indexActive=true;index=value.currentState!=0;}else{thumbActive=true;thumb|=value.currentState!=0;if(a==0){++activeControls;if(value.currentState)controlMask|=1u<<h;}if(a==3){++activeControls;if(value.currentState)controlMask|=8u<<h;}}
   }
  }
  if(report)report(h,thumbActive&&indexActive,thumb,index);
 }
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
 SC_XR_GET("xrCreateActionSet",createSet);SC_XR_GET("xrCreateAction",create);SC_XR_GET("xrDestroyActionSet",destroySet);
 SC_XR_GET("xrSuggestInteractionProfileBindings",suggest);SC_XR_GET("xrAttachSessionActionSets",attach);SC_XR_GET("xrSyncActions",sync);
 SC_XR_GET("xrGetActionStateBoolean",booleanState);SC_XR_GET("xrStringToPath",stringPath);SC_XR_GET("xrPathToString",pathString);
 #undef SC_XR_GET
 if(XR_FAILED(stringPath(i,"/user/hand/left",&hands[0]))||XR_FAILED(stringPath(i,"/user/hand/right",&hands[1])))return;
 XrActionSetCreateInfo ci{XR_TYPE_ACTION_SET_CREATE_INFO};strcpy_s(ci.actionSetName,"fc_gestures");strcpy_s(ci.localizedActionSetName,"Filthy Casual finger gestures");ci.priority=0;
 auto created=createSet(i,&ci,&set);note("create-set",created);if(XR_FAILED(created))return;
 const char* names[]={"fc_thumb_rest","fc_thumb_primary","fc_thumb_secondary","fc_thumb_stick","fc_index_touch","fc_menu_click"};
 for(int a=0;a<6;a++){
  XrActionCreateInfo ai{XR_TYPE_ACTION_CREATE_INFO};strcpy_s(ai.actionName,names[a]);strcpy_s(ai.localizedActionName,names[a]);ai.actionType=XR_ACTION_TYPE_BOOLEAN_INPUT;ai.countSubactionPaths=a==5?1:2;ai.subactionPaths=hands;
  auto result=create(set,&ai,&actions[a]);if(XR_FAILED(result)){note(names[a],result);destroySet(set);set=XR_NULL_HANDLE;return;}
 }
 auto list=bindings();XrPath profile=XR_NULL_PATH;stringPath(i,"/interaction_profiles/oculus/touch_controller",&profile);
 XrInteractionProfileSuggestedBinding bi{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};bi.interactionProfile=profile;bi.countSuggestedBindings=uint32_t(list.size());bi.suggestedBindings=list.data();
 // Native Touch suggestions later are merged by onSuggest, never replaced.
 auto result=suggest(i,&bi);note("suggest-touch",result);ready=list.size()==11&&XR_SUCCEEDED(result);
 if(!ready){destroySet(set);set=XR_NULL_HANDLE;}
}
inline void ensure(){if(!ready&&attempts<2&&upstream&&instance)init(upstream,instance,report,controls);}
}
