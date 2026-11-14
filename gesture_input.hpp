#pragma once
#include <vector>
#include <set>
#include "controls_runtime.hpp"
// Controller-specific raw sources. Physical input stays separate from remapped
// game channels, so rebinding never changes finger sensing or native suggestions.
namespace gesture_input {
using Report=void(*)(int,bool,bool,bool);using Controls=void(*)(unsigned,bool);using Diagnostic=void(*)(const char*,int);
inline Diagnostic diagnostic=nullptr;inline Report report=nullptr;inline Controls controls=nullptr;
inline PFN_xrGetInstanceProcAddr upstream=nullptr;
inline PFN_xrSuggestInteractionProfileBindings suggest=nullptr;inline PFN_xrAttachSessionActionSets attach=nullptr;inline PFN_xrSyncActions sync=nullptr;
inline PFN_xrGetActionStateBoolean booleanState=nullptr;inline PFN_xrGetActionStateFloat floatState=nullptr;inline PFN_xrGetActionStateVector2f vectorState=nullptr;
inline PFN_xrStringToPath stringPath=nullptr;inline PFN_xrPathToString pathString=nullptr;inline PFN_xrGetCurrentInteractionProfile currentProfile=nullptr;
inline PFN_xrGetInputSourceLocalizedName localizedName=nullptr;
inline PFN_xrCreateActionSpace createActionSpace=nullptr;inline PFN_xrLocateSpace locateSpace=nullptr;inline PFN_xrDestroySpace destroySpace=nullptr;
inline XrInstance instance=XR_NULL_HANDLE;inline XrActionSet set=XR_NULL_HANDLE;inline XrAction actions[70]{},aimActions[2]{};
inline XrPath hands[2]{};inline XrSpace aim[2]{};inline unsigned attempts=0;inline uint64_t generation=0,lastDevicePoll=0;
inline bool initializing=false,attached=false,ready=false;
inline std::set<std::string> enabledExtensions,accepted,nativeProfiles;
inline std::vector<XrActionSuggestedBinding> nativeTouch;
inline const controller_profiles::Profile* devices[2]{};
inline void note(const char* s,XrResult r){if(diagnostic)diagnostic(s,int(r));}
inline std::string pathText(XrPath p){char text[XR_MAX_PATH_LENGTH]{};uint32_t n=0;if(pathString&&XR_SUCCEEDED(pathString(instance,p,sizeof(text),&n,text)))return text;return {};}
inline bool permitted(const controller_profiles::Profile&p){return !*p.extension||enabledExtensions.count(p.extension);}
inline void endSession(){for(auto&s:aim){if(s&&destroySpace)destroySpace(s);s=XR_NULL_HANDLE;}attached=false;controls_runtime::invalidate();}
inline void reset(){controls_runtime::invalidate();instance=XR_NULL_HANDLE;set=XR_NULL_HANDLE;for(auto&a:actions)a=XR_NULL_HANDLE;for(auto&a:aimActions)a=XR_NULL_HANDLE;attached=ready=false;aim[0]=aim[1]=XR_NULL_HANDLE;devices[0]=devices[1]=nullptr;accepted.clear();nativeProfiles.clear();nativeTouch.clear();lastDevicePoll=0;}
inline std::vector<XrActionSuggestedBinding> bindings(const controller_profiles::Profile&p){std::vector<XrActionSuggestedBinding> v;
 for(int i=0;i<70;i++)if(actions[i]&&*p.paths[i]){XrPath path{};if(XR_SUCCEEDED(stringPath(instance,p.paths[i],&path)))v.push_back({actions[i],path});}
 for(int h=0;h<2;h++){auto s=std::string(h?"/user/hand/right":"/user/hand/left")+"/input/aim/pose";XrPath path{};if(XR_SUCCEEDED(stringPath(instance,s.c_str(),&path)))v.push_back({aimActions[h],path});}return v;
}
inline XrResult submitProfile(const controller_profiles::Profile&p,const std::vector<XrActionSuggestedBinding>&v){XrPath path{};if(XR_FAILED(stringPath(instance,p.path,&path)))return XR_ERROR_PATH_UNSUPPORTED;XrInteractionProfileSuggestedBinding bi{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};bi.interactionProfile=path;bi.countSuggestedBindings=uint32_t(v.size());bi.suggestedBindings=v.data();auto result=suggest(instance,&bi);if(XR_SUCCEEDED(result))accepted.insert(p.path);return result;}
inline XrResult XRAPI_CALL onSuggest(XrInstance i,const XrInteractionProfileSuggestedBinding*info){if(!ready||!info)return suggest(i,info);auto name=pathText(info->interactionProfile);auto p=controller_profiles::find(name);if(!p||!permitted(*p))return suggest(i,info);
 auto v=bindings(*p);v.insert(v.end(),info->suggestedBindings,info->suggestedBindings+info->countSuggestedBindings);auto merged=*info;merged.countSuggestedBindings=uint32_t(v.size());merged.suggestedBindings=v.data();auto result=suggest(i,&merged);
 if(XR_SUCCEEDED(result)){nativeProfiles.insert(name);accepted.insert(name);if(name=="/interaction_profiles/oculus/touch_controller")nativeTouch.assign(info->suggestedBindings,info->suggestedBindings+info->countSuggestedBindings);return result;}return suggest(i,info);
}
// For profiles not supplied by TTR, translate its existing Touch actions. Never
// replace a native controller-specific binding table supplied by the host.
inline void fillNativeProfiles(){auto touch=controller_profiles::find("touch");if(nativeTouch.empty()||!touch)return;
 for(auto&p:controller_profiles::profiles){if(!permitted(p)||nativeProfiles.count(p.path))continue;auto v=bindings(p);
  for(auto&b:nativeTouch){auto old=pathText(b.binding);std::string replacement;
   for(int j=0;j<70;j++)if(*touch->paths[j]&&old==touch->paths[j]){replacement=p.paths[j];break;}
   if(old.find("/input/grip/pose")!=std::string::npos||old.find("/input/aim/pose")!=std::string::npos||old.find("/output/haptic")!=std::string::npos)replacement=old;
   if(replacement.empty())continue;XrPath path{};if(XR_SUCCEEDED(stringPath(instance,replacement.c_str(),&path)))v.push_back({b.action,path});
  }note(p.id,submitProfile(p,v));
 }
}
inline XrResult XRAPI_CALL onAttach(XrSession s,const XrSessionActionSetsAttachInfo*info){if(!ready||!info)return attach(s,info);fillNativeProfiles();std::vector<XrActionSet> all(info->actionSets,info->actionSets+info->countActionSets);all.push_back(set);auto merged=*info;merged.countActionSets=uint32_t(all.size());merged.actionSets=all.data();auto result=attach(s,&merged);attached=XR_SUCCEEDED(result);note("attach",result);
 if(XR_FAILED(result)){controls_runtime::invalidate();ready=false;return attach(s,info);}
 for(int h=0;h<2;h++){XrActionSpaceCreateInfo ci{XR_TYPE_ACTION_SPACE_CREATE_INFO};ci.action=aimActions[h];ci.subactionPath=hands[h];ci.poseInActionSpace.orientation.w=1;if(!aim[h])createActionSpace(s,&ci,&aim[h]);}lastDevicePoll=0;return result;
}
inline void pollDevices(XrSession s,uint64_t now){if(now-lastDevicePoll<1000)return;lastDevicePoll=now;std::array<std::string,2> names,paths,assets;bool changed=false;
 for(int h=0;h<2;h++){XrInteractionProfileState st{XR_TYPE_INTERACTION_PROFILE_STATE};if(!currentProfile||XR_FAILED(currentProfile(s,hands[h],&st)))continue;paths[h]=pathText(st.interactionProfile);auto*p=controller_profiles::find(paths[h]);if(p!=devices[h]||paths[h]!=controls_runtime::devicePaths[h])changed=true;devices[h]=p;names[h]=p?p->title:(paths[h].empty()?"Disconnected":"Unknown controller");assets[h]=p?p->asset:"";}
 if(!changed)return;std::lock_guard<std::mutex> lock(controls_runtime::mutex);controls_runtime::devicePaths=paths;controls_runtime::deviceTitles=names;controls_runtime::deviceAssets=assets;
 std::ostringstream dump;dump<<"{\"version\":1,\"controllers\":[";for(int h=0;h<2;h++){if(h)dump<<',';dump<<"{\"hand\":"<<h<<",\"profile\":"<<controls_runtime::jsonString(paths[h])<<",\"name\":"<<controls_runtime::jsonString(names[h])<<",\"model\":"<<controls_runtime::jsonString(assets[h])<<"}";}dump<<"],\"inputs\":[";
 for(int i=0;i<70;i++){int h=controller_profiles::hands[i];auto*p=devices[h];std::string path=p?p->paths[i]:"",label=fc_controls::sourceNames[i];if(!path.empty()){
  XrPath xp{};if(localizedName&&XR_SUCCEEDED(stringPath(instance,path.c_str(),&xp))){XrInputSourceLocalizedNameGetInfo ni{XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO};ni.sourcePath=xp;ni.whichComponents=XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT|XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT;char text[512]{};uint32_t n=0;if(XR_SUCCEEDED(localizedName(s,&ni,sizeof(text),&n,text))&&*text)label=text;}
  if(label==fc_controls::sourceNames[i]){auto leaf=path.substr(path.find("/input/")+7);label=std::string(h?"Right ":"Left ")+leaf;}
  if(controller_profiles::child(i)||controller_profiles::vector(i)){int k=i>=38?i-38:i>=34?i-34:i>=15?i-15:i-11;label+=std::string(" / ")+std::array<const char*,4>{"left","right","down","up"}[k];}
 }else label+=" (unavailable)";fc_controls::sourceLabels[i]=label;
 if(i)dump<<',';dump<<"{\"id\":"<<i<<",\"token\":"<<controls_runtime::jsonString(fc_controls::sourceNames[i])<<",\"label\":"<<controls_runtime::jsonString(label)<<",\"path\":"<<controls_runtime::jsonString(path)<<",\"supported\":"<<(!path.empty()?"true":"false")<<"}";
 }dump<<"]}";controls_runtime::atomicWrite(controls_runtime::directory+"\\inputs.json",dump.str());
}
inline XrResult XRAPI_CALL onSync(XrSession s,const XrActionsSyncInfo*info){++generation;if(!attached||!info)return sync(s,info);std::vector<XrActiveActionSet> all(info->activeActionSets,info->activeActionSets+info->countActiveActionSets);all.push_back({set,XR_NULL_PATH});auto merged=*info;merged.countActiveActionSets=uint32_t(all.size());merged.activeActionSets=all.data();auto result=sync(s,&merged);if(XR_FAILED(result)){controls_runtime::invalidate();note("sync",result);attached=ready=false;return result;}
 controls_runtime::init();auto now=GetTickCount64();pollDevices(s,now);fc_controls::Frame physical{};
 if(result==XR_SUCCESS)for(int i=0;i<70;i++){int h=controller_profiles::hands[i];auto*p=devices[h];if(!actions[i]||!p||!*p->paths[i])continue;XrActionStateGetInfo q{XR_TYPE_ACTION_STATE_GET_INFO};q.action=actions[i];q.subactionPath=hands[h];
  if(controller_profiles::vector(i)){XrActionStateVector2f v{XR_TYPE_ACTION_STATE_VECTOR2F};if(XR_SUCCEEDED(vectorState(s,&q,&v))){float xy[]={std::max(0.f,-v.currentState.x),std::max(0.f,v.currentState.x),std::max(0.f,-v.currentState.y),std::max(0.f,v.currentState.y)};for(int k=0;k<4;k++)physical[i+k]={xy[k],v.isActive!=0,v.lastChangeTime};}}
  else if(controller_profiles::analog(i)){XrActionStateFloat v{XR_TYPE_ACTION_STATE_FLOAT};if(XR_SUCCEEDED(floatState(s,&q,&v)))physical[i]={v.isActive?v.currentState:0,v.isActive!=0,v.lastChangeTime};}
  else{XrActionStateBoolean v{XR_TYPE_ACTION_STATE_BOOLEAN};if(XR_SUCCEEDED(booleanState(s,&q,&v)))physical[i]={v.isActive?float(v.currentState):0,v.isActive!=0,v.lastChangeTime};}
 }
 for(int h=0;h<2;h++){bool thumb=false,ta=false;for(int i=0;i<70;i++)if(controller_profiles::hands[i]==h&&controller_profiles::touch(i)&&i!=27+h&&i!=54+h&&i!=56+h){ta|=physical[i].active;thumb|=physical[i].active&&physical[i].value>.5f;}if(report)report(h,ta&&physical[27+h].active,thumb,physical[27+h].value>.5f);}
 controls_runtime::update(physical,now);unsigned mask=0;int ids[]={19,20,10,21,22};for(int j=0;j<5;j++)if(physical[ids[j]].active&&physical[ids[j]].value>.5f)mask|=1u<<j;
 if(controls)controls(mask,physical[10].active||(physical[19].active&&physical[20].active));return result;
}
inline void init(PFN_xrGetInstanceProcAddr gipa,XrInstance i,Report callback,Controls controlCallback){if(initializing)return;if(instance==i&&set!=XR_NULL_HANDLE)return;struct Guard{Guard(){initializing=true;}~Guard(){initializing=false;}}guard;if(instance!=i)attempts=0;++attempts;upstream=gipa;reset();instance=i;report=callback;controls=controlCallback;
 PFN_xrCreateActionSet createSet=nullptr;PFN_xrCreateAction create=nullptr;PFN_xrDestroyActionSet destroySet=nullptr;
 #define SC_XR_GET(name,ptr) if(XR_FAILED(gipa(i,name,reinterpret_cast<PFN_xrVoidFunction*>(&ptr))))return;
 SC_XR_GET("xrCreateActionSpace",createActionSpace);SC_XR_GET("xrLocateSpace",locateSpace);SC_XR_GET("xrDestroySpace",destroySpace);SC_XR_GET("xrCreateActionSet",createSet);SC_XR_GET("xrCreateAction",create);SC_XR_GET("xrDestroyActionSet",destroySet);SC_XR_GET("xrSuggestInteractionProfileBindings",suggest);SC_XR_GET("xrAttachSessionActionSets",attach);SC_XR_GET("xrSyncActions",sync);SC_XR_GET("xrGetActionStateFloat",floatState);SC_XR_GET("xrGetActionStateVector2f",vectorState);SC_XR_GET("xrGetActionStateBoolean",booleanState);SC_XR_GET("xrStringToPath",stringPath);SC_XR_GET("xrPathToString",pathString);SC_XR_GET("xrGetCurrentInteractionProfile",currentProfile);
 #undef SC_XR_GET
 gipa(i,"xrGetInputSourceLocalizedName",reinterpret_cast<PFN_xrVoidFunction*>(&localizedName));
 if(XR_FAILED(stringPath(i,"/user/hand/left",&hands[0]))||XR_FAILED(stringPath(i,"/user/hand/right",&hands[1])))return;
 XrActionSetCreateInfo ci{XR_TYPE_ACTION_SET_CREATE_INFO};strcpy_s(ci.actionSetName,"fc_gestures");strcpy_s(ci.localizedActionSetName,"Filthy Casuals physical controls");if(XR_FAILED(createSet(i,&ci,&set)))return;
 for(int a=0;a<72;a++){if(a<70&&controller_profiles::child(a))continue;int h=a<70?controller_profiles::hands[a]:a-70;XrActionCreateInfo ai{XR_TYPE_ACTION_CREATE_INFO};sprintf_s(ai.actionName,"fc_source_%02d",a);strcpy_s(ai.localizedActionName,a<70?fc_controls::sourceNames[a]:(a==70?"Left controls editor aim":"Right controls editor aim"));ai.actionType=a>=70?XR_ACTION_TYPE_POSE_INPUT:controller_profiles::vector(a)?XR_ACTION_TYPE_VECTOR2F_INPUT:controller_profiles::analog(a)?XR_ACTION_TYPE_FLOAT_INPUT:XR_ACTION_TYPE_BOOLEAN_INPUT;ai.countSubactionPaths=1;ai.subactionPaths=&hands[h];auto result=create(set,&ai,a<70?&actions[a]:&aimActions[h]);if(XR_FAILED(result)){note("create-source",result);destroySet(set);set=XR_NULL_HANDLE;return;}}
 for(auto&p:controller_profiles::profiles)if(permitted(p))note(p.id,submitProfile(p,bindings(p)));ready=!accepted.empty();if(!ready){destroySet(set);set=XR_NULL_HANDLE;}
}
inline void ensure(){if(!ready&&attempts<2&&upstream&&instance)init(upstream,instance,report,controls);}
}
