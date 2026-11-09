#pragma once
#include "build_adapter.hpp"
// Native pose extension for the hash verified by build_adapter::heldModule.
// Same-frame bone edits occur inside TTR's own pose pass, before its downstream
// contact/wrapping and skin publication. No controller, hand-root or item transform
// is changed; authored joint bases and bend axes stay owned by the installed mod.
namespace finger_adapter {
using Query=bool(*)(int,float&,float&,float&,float&);
using Pose=void(__cdecl*)(void*,int,float);
using Apply=void(__cdecl*)(void*,const void*,int,const float*);
inline unsigned char* module=nullptr;
inline Query query=nullptr;
inline bool installed=false;
inline std::atomic<unsigned long> applied[2]{};
inline std::atomic<float> freePoseWeight[2]{{1.f},{1.f}};
inline scene::Vec thumbLiftAxis[2][2];
inline float thumbLiftAngle[2][2]{};
inline scene::Vec thumbTuckAxis[2];
inline float thumbTuckAngle[2]{};
inline scene::Vec rotateAxis(scene::Vec v,scene::Vec axis,float angle){
 float c=std::cos(angle),s=std::sin(angle),d=axis.x*v.x+axis.y*v.y+axis.z*v.z;
 return v*c+scene::cross(axis,v)*s+axis*(d*(1-c));
}
inline void rotateJoint(scene::Transform& t,scene::Vec axis,float angle){
 for(int col=0;col<3;col++){auto v=rotateAxis({t.r[0][col],t.r[1][col],t.r[2][col]},axis,angle);t.r[0][col]=v.x;t.r[1][col]=v.y;t.r[2][col]=v.z;}
}
inline void blendRotation(scene::Transform& target,const scene::Transform& native,float weight){
 if(weight>=.9999f)return;
 if(weight<=.0001f){target=native;return;}
 auto unit=[](scene::Vec v){float n=scene::distance(v,{});return n>.00001f?v*(1.f/n):scene::Vec{};};
 scene::Vec cols[2];
 for(int c=0;c<2;c++)cols[c]={native.r[0][c]*(1-weight)+target.r[0][c]*weight,native.r[1][c]*(1-weight)+target.r[1][c]*weight,native.r[2][c]*(1-weight)+target.r[2][c]*weight};
 auto x=unit(cols[0]);float d=x.x*cols[1].x+x.y*cols[1].y+x.z*cols[1].z;
 auto y=unit(cols[1]-x*d),z=scene::cross(x,y);
 if(scene::distance(x,{})<.99f||scene::distance(y,{})<.99f){target=native;return;}
 scene::Vec axes[]{x,y,z};for(int c=0;c<3;c++){target.r[0][c]=axes[c].x;target.r[1][c]=axes[c].y;target.r[2][c]=axes[c].z;}
 target.p=native.p;target.scale=native.scale;
}
inline void __cdecl pose(void* root,int hand,float amount,unsigned posedObject,void* object,float approachWeight){
 reinterpret_cast<Pose>(module+0x1C4870)(root,hand,amount);
 if(!root||hand<0||hand>1||!query||(posedObject&255))return;
 float freeWeight=std::isfinite(approachWeight)?std::clamp(approachWeight,0.f,1.f):0.f;
 freePoseWeight[hand]=freeWeight;
 // The object argument is shared by BOTH hands. Ask TTR which hand actually
 // owns a weapon/support pose, and retain its per-hand item/transition checks.
 if(object&&reinterpret_cast<bool(__cdecl*)(int,bool)>(module+0x1DB4C0)(hand,true))return;
 if(reinterpret_cast<bool(__cdecl*)(int)>(module+0x18A630)(hand)||*reinterpret_cast<int*>(module+0xDCE99C)==hand)return;
 if(freeWeight<=.0001f)return;
 void* joints[15]{};scene::Transform nativePose[15]{};
 auto nativeTable=module+(hand?0x8F0730:0x8F0BE0);
 for(int n=0;n<15;n++){joints[n]=scene::find(root,*reinterpret_cast<const char**>(nativeTable+n*80));if(joints[n])nativePose[n]=scene::local(joints[n]);}
 float thumb=0,index=0,lift=0,freeGrip=-1;if(!query(hand,thumb,index,lift,freeGrip))return;
 if(!std::isfinite(thumb)||!std::isfinite(index)||!std::isfinite(lift))return;
 thumb=std::clamp(thumb,0.f,1.f);index=std::clamp(index,0.f,1.f);
 // Use the authored bend at every thumb joint. Reducing distal flexion here
 // compounds the input blend and leaves a fully squeezed fist half open.
 float curls[15]{};for(int j=0;j<3;j++)curls[j]=thumb;
 for(int j=0;j<3;j++)curls[3+j]=index;
 // Modal controls suppress native grip actions, but an empty hand still needs
 // the real grip value for its remaining three fingers. This pose-only feed
 // runs after per-hand occupancy checks and never reaches gameplay input.
 bool maskedGrip=std::isfinite(freeGrip)&&freeGrip>=0;
 if(maskedGrip)for(int j=6;j<15;j++)curls[j]=std::clamp(freeGrip,0.f,1.f);
 reinterpret_cast<Apply>(module+0x1DF020)(root,module+(hand?0x8F0730:0x8F0BE0),maskedGrip?15:6,curls);
 // Opposition moves the thumb toward the inside of the curled index/middle
 // fingers. Flexion alone bends it sideways past the fist. Blend from the
 // relaxed pose; preserve every joint position, length, and native held pose.
 float tuck=std::clamp((thumb-.20f)/.80f,0.f,1.f);tuck=tuck*tuck*(3-2*tuck);
 if(tuck>.0001f){
  const char* names[2][3]={{"Bip01 L Thumb1","Bip01 L Thumb11","Bip01 L Thumb12"},{"Bip01 R Thumb1","Bip01 R Thumb11","Bip01 R Thumb12"}};
  auto table=module+(hand?0x8F0730:0x8F0BE0);
  for(int j=0;j<3;j++)if(auto node=scene::find(root,names[hand][j])){
   auto axis=j?*reinterpret_cast<scene::Vec*>(table+j*80+56):thumbTuckAxis[hand];
   float angle=j?(j==1?.43633231f:.08726646f):thumbTuckAngle[hand];
   rotateJoint(scene::local(node),axis,angle*tuck);
  }
 }
 // Keep the long metacarpal inside the palm: limit its abduction and share
 // extension with the MCP joint, instead of swinging the whole thumb out.
 if(lift>0.0001f)for(int joint=0;joint<2;joint++){
  const char* names[2][2]={{"Bip01 L Thumb1","Bip01 L Thumb11"},{"Bip01 R Thumb1","Bip01 R Thumb11"}};
  if(auto node=scene::find(root,names[hand][joint])){
   auto& t=scene::local(node);float angle=thumbLiftAngle[hand][joint]*std::clamp(lift,0.f,1.f);
   for(int col=0;col<3;col++){auto v=rotateAxis({t.r[0][col],t.r[1][col],t.r[2][col]},thumbLiftAxis[hand][joint],angle);t.r[0][col]=v.x;t.r[1][col]=v.y;t.r[2][col]=v.z;}
  }
 }
 // Reuse TTR's exact distance fade. Its contact/wrapping and smoothing passes
 // still run afterward. Do not replace native hand/root positions or held poses.
 for(int n=0;n<15;n++)if(joints[n])blendRotation(scene::local(joints[n]),nativePose[n],freeWeight);
 ++applied[hand];
}
// Exact outer routine's saved EBX is its original argument frame. Preserve the
// original caller's stack and all nonvolatile registers using a cdecl bridge.
__declspec(naked) inline void bridge(){
 __asm {
  push dword ptr [ebp-148h]
  push dword ptr [ebx+14h]
  push dword ptr [ebx+10h]
  push dword ptr [esp+24]
  push dword ptr [esp+24]
  push dword ptr [esp+24]
  call pose
  add esp,24
  ret
 }
}
inline const char* install(Query fn){
 if(installed)return "already installed";
 auto b=build_adapter::heldModule();if(!b)return "refused: absent or unknown TTWVR build";
 const unsigned char context[]={0xF3,0x0F,0x11,0x04,0x24,0xFF,0x73,0x0C,0x56,0xE8,0xB3,0x46,0xFE,0xFF,0x6A,0x00,0xFF,0x73,0x0C,0x56,0xE8,0x18,0xE4,0xFE,0xFF};
 const unsigned char apply[]={0x83,0xEC,0x58,0x8B,0x4C,0x24,0x5C,0x55,0x8B,0x6C,0x24,0x68,0x89,0x4C,0x24,0x10,0x57,0x8B};
 const unsigned char original[]={0x55,0x8B,0xEC,0x83,0xE4,0xF8,0x81,0xEC,0x1C,0x01,0x00,0x00,0x53,0x8B,0x5D,0x08};
 if(memcmp(b+0x1E01AF,context,sizeof(context))||memcmp(b+0x1DF020,apply,sizeof(apply))||memcmp(b+0x1C4870,original,sizeof(original)))return "refused: pose instruction guard";
 const unsigned char distanceInit[]={0xF3,0x0F,0x11,0x85,0xB8,0xFE,0xFF,0xFF};
 const unsigned char distanceEnd[]={0xF3,0x0F,0x11,0x95,0xB8,0xFE,0xFF,0xFF};
 if(memcmp(b+0x1E0043,distanceInit,sizeof(distanceInit))||memcmp(b+0x1E0183,distanceEnd,sizeof(distanceEnd)))return "refused: native approach blend guard";
 const unsigned char policy[]={0x80,0x7C,0x24,0x08,0x00,0x74,0x24,0x8B,0x4C,0x24,0x04,0x51,0xE8,0x7F,0xDD,0xFF,0xFF,0x83,0xC4,0x04,0x84,0xC0,0x75,0x10};
 const unsigned char held[]={0x8B,0x44,0x24,0x04,0x74,0x08,0x39,0x05};
 if(memcmp(b+0x1DB4C0,policy,sizeof(policy))||b[0x18A630]!=0x80||b[0x18A631]!=0x3D||*reinterpret_cast<unsigned char**>(b+0x18A632)!=b+0x1114215||memcmp(b+0x18A637,held,sizeof(held)))return "refused: per-hand native policy guard";
 const char* names[2][6]={{"Bip01 L Thumb1","Bip01 L Thumb11","Bip01 L Thumb12","Bip01 L Finger1","Bip01 L Finger11","Bip01 L Finger12"},{"Bip01 R Thumb1","Bip01 R Thumb11","Bip01 R Thumb12","Bip01 R Finger1","Bip01 R Finger11","Bip01 R Finger12"}};
 const unsigned nameRva[2][6]={{0x8F13E8,0x8F13FC,0x8F1410,0x743388,0x8F1424,0x8F1438},{0x8F12D0,0x8F12E4,0x8F12F8,0x743374,0x8F130C,0x8F1320}};
 for(int h=0;h<2;h++)for(int n=0;n<6;n++){
  auto row=b+(h?0x8F0730:0x8F0BE0)+n*80;auto name=*reinterpret_cast<const char**>(row);
  if(name!=reinterpret_cast<char*>(b+nameRva[h][n])||strcmp(name,names[h][n])||*reinterpret_cast<int*>(row+72)!=n/3||*reinterpret_cast<int*>(row+76)!=n%3)return "refused: native joint table guard";
 }
 for(int h=0;h<2;h++){
  auto table=b+(h?0x8F0730:0x8F0BE0);
  for(int n=6;n<15;n++){
   auto row=table+n*80;char expected[40];int finger=n/3,joint=n%3;
   if(joint)sprintf_s(expected,"Bip01 %c Finger%d%d",h?'R':'L',finger,joint);
   else sprintf_s(expected,"Bip01 %c Finger%d",h?'R':'L',finger);
   if(strcmp(*reinterpret_cast<const char**>(row),expected)||*reinterpret_cast<int*>(row+72)!=finger||*reinterpret_cast<int*>(row+76)!=joint)return "refused: free-finger table guard";
  }
  auto pinky=table+12*80;
  if(strcmp(*reinterpret_cast<const char**>(pinky),h?"Bip01 R Finger4":"Bip01 L Finger4")||*reinterpret_cast<int*>(pinky+72)!=4||*reinterpret_cast<int*>(pinky+76)!=0)return "refused: palm frame guard";
  scene::Transform t[3]{};for(int n=0;n<3;n++)memcpy(&t[n],table+n*80+4,sizeof(scene::Transform));
  auto indexBase=*reinterpret_cast<scene::Vec*>(table+3*80+40),pinkyBase=*reinterpret_cast<scene::Vec*>(pinky+40);
  auto unit=[](scene::Vec v){float len=scene::distance(v,{});return len>.0001f?v*(1/len):scene::Vec{};};
  auto forward=unit((indexBase+pinkyBase)*.5f),radial=indexBase-pinkyBase;
  float projection=radial.x*forward.x+radial.y*forward.y+radial.z*forward.z;
  radial=unit(radial-forward*projection);
  // Calibrate opposition from this build's closed joint table, in hand-local
  // coordinates. Aim the thumb's distal joint inside the index/middle curl,
  // slightly toward their knuckles rather than through their outer surface.
  auto closed=[&](int first,float mcpExtra){
   scene::Transform joints[3]{};
   for(int j=0;j<3;j++){
    auto row=table+(first+j)*80;memcpy(&joints[j],row+4,sizeof(scene::Transform));
    auto axis=*reinterpret_cast<scene::Vec*>(row+56);float degrees=*reinterpret_cast<float*>(row+68)+(j==1?mcpExtra:0.f);
    rotateJoint(joints[j],axis,degrees*.01745329252f);
   }
   return joints[0].p+scene::rotate(joints[0],joints[1].p+scene::rotate(joints[1],joints[2].p));
  };
  auto middle=table+6*80;
  if(strcmp(*reinterpret_cast<const char**>(middle),h?"Bip01 R Finger2":"Bip01 L Finger2"))return "refused: middle finger frame guard";
  auto middleBase=*reinterpret_cast<scene::Vec*>(middle+40);
  auto targetClosed=(closed(3,0)+closed(6,0))*.4f+(indexBase+middleBase)*.1f;
  auto tuckFrom=unit(closed(0,25)-t[0].p),tuckTo=unit(targetClosed-t[0].p);
  thumbTuckAxis[h]=unit(scene::cross(tuckFrom,tuckTo));
  thumbTuckAngle[h]=std::acos(std::clamp(tuckFrom.x*tuckTo.x+tuckFrom.y*tuckTo.y+tuckFrom.z*tuckTo.z,-1.f,1.f));
  if(!std::isfinite(thumbTuckAngle[h])||thumbTuckAngle[h]>.69813170f||scene::distance(thumbTuckAxis[h],{})<.9f)return "refused: thumb opposition frame";
  auto from=unit(scene::rotate(t[0],t[1].p+scene::rotate(t[1],t[2].p)));
  // Raised thumb cants 30 degrees forward of the palm's radial edge. Limit
  // metacarpal movement to 12 degrees to preserve the palm/web silhouette.
  auto to=unit(radial*.8660254f+forward*.5f);
  float dot=std::clamp(from.x*to.x+from.y*to.y+from.z*to.z,-1.f,1.f);
  thumbLiftAxis[h][0]=unit(scene::cross(from,to));thumbLiftAngle[h][0]=std::min(std::acos(dot),.20943951f);
  for(int col=0;col<3;col++){auto v=rotateAxis({t[0].r[0][col],t[0].r[1][col],t[0].r[2][col]},thumbLiftAxis[h][0],thumbLiftAngle[h][0]);t[0].r[0][col]=v.x;t[0].r[1][col]=v.y;t[0].r[2][col]=v.z;}
  auto outer=unit(scene::rotate(t[1],{1,0,0})),target=unit(scene::inverseRotate(t[0],to));
  dot=std::clamp(outer.x*target.x+outer.y*target.y+outer.z*target.z,-1.f,1.f);
  thumbLiftAxis[h][1]=unit(scene::cross(outer,target));thumbLiftAngle[h][1]=std::acos(dot);
  for(int joint=0;joint<2;joint++)if(!std::isfinite(thumbLiftAngle[h][joint])||scene::distance(thumbLiftAxis[h][joint],{})<.9f)return "refused: thumb anatomical frame";
 }
 auto site=b+0x1E01B8;DWORD old=0;if(!VirtualProtect(site,5,PAGE_EXECUTE_READWRITE,&old))return "failed: pose site protection";
 module=b;query=fn;int relative=int(reinterpret_cast<unsigned char*>(&bridge)-(site+5));memcpy(site+1,&relative,4);
 FlushInstructionCache(GetCurrentProcess(),site,5);DWORD unused=0;VirtualProtect(site,5,old,&unused);installed=true;
 return "installed: native empty-hand thumb/index pose extension";
}
}
