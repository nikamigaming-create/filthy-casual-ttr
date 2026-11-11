#pragma once
#include "build_adapter.hpp"
#include "scene_adapter.hpp"
#include <atomic>
#include <algorithm>
#include <cmath>
#include "scope_surface_adapter.hpp"
#include "floating_hud_adapter.hpp"
#include "dialogue_anchor.hpp"
#include "dialogue_layout.hpp"
// Exact C9D047D4... TTWVR build only. Keep positioning in the native transaction:
// its helper produces BOTH the visual pose and the world-space anchor direction.
namespace presentation_adapter {
using Vec=scene::Vec;
struct Pose {Vec position;float rotation[4];bool valid;unsigned char pad[3];};
static_assert(sizeof(Pose)==32);
inline unsigned char* base=nullptr;
inline thread_local float targetDistance=0;
inline std::atomic<unsigned> placements{0};
inline std::atomic<float> lastDistance{0},lastAngle{0};
inline Vec lensTracking{};
inline Vec speakerTracking{};
inline unsigned lensSamples=0;
inline std::mutex speakerMutex;
inline Vec speakerWorld{};
inline unsigned speakerId=0,speakerEpoch=0;
inline DialogueAnchor speakerAnchor{};
inline ULONGLONG speakerPublished=0;
inline std::atomic<unsigned> speakerFrameUpdates{0};
inline void setSpeaker(unsigned id,Vec world){
 std::lock_guard<std::mutex> lock(speakerMutex);
 if(id!=speakerId){speakerId=id;++speakerEpoch;speakerPublished=0;}
 speakerWorld=world;
}
inline Vec* __fastcall publishSpeaker(void* manager,void*,Vec* out,const Vec* world){
 ++speakerFrameUpdates;
 auto convert=reinterpret_cast<Vec*(__thiscall*)(void*,Vec*,const Vec*)>(base+0x64B0B0);
 auto result=convert(manager,out,world);
 std::lock_guard<std::mutex> lock(speakerMutex);
 if(speakerId&&reinterpret_cast<int(__cdecl*)()>(base+0xD094)()==0x3F1){
  Vec point{};convert(manager,&point,&speakerWorld);
  const auto eye=*reinterpret_cast<Vec*>(static_cast<unsigned char*>(manager)+0x1C);
  speakerAnchor={speakerEpoch,{point.x,point.y,point.z},{eye.x,eye.y,eye.z}};
  speakerPublished=GetTickCount64();
 }
 return result;
}
inline bool readSpeaker(DialogueAnchor& out){
 std::lock_guard<std::mutex> lock(speakerMutex);
 if(!speakerId||!speakerPublished||GetTickCount64()-speakerPublished>200)return false;
 out=speakerAnchor;return true;
}
inline bool __fastcall inspectLens(Vec* center,void*){
 auto result=reinterpret_cast<bool(__thiscall*)(const Vec*)>(base+0x5D600)(center);
 auto manager=reinterpret_cast<void*(__cdecl*)()>(base+0x12BC5)();
 if(result)reinterpret_cast<Vec*(__thiscall*)(void*,Vec*,const Vec*)>(base+0x64B0B0)(manager,&lensTracking,center);
 ++lensSamples;return result;
}
// Read-only diagnostics for the private test mailbox. Exact-build globals:
// 2A7070 returns DCE548; 32C71C increments 122F068 on scope render.
inline void diagnostics(std::ostream& out){
 auto b=build_adapter::heldModule();if(!b)return;
 auto s=b+0xECCD68;
 out<<",\"nativeScopeActive\":"<<unsigned(*s)<<",\"nativeScopeFrames\":"<<*reinterpret_cast<unsigned*>(b+0x17FE728);
 out<<",\"nativeTopMenu\":"<<reinterpret_cast<int(__cdecl*)()>(b+0xD094)();
 auto settings=reinterpret_cast<unsigned char*(__cdecl*)()>(b+0xA641)();
 out<<",\"nativeScopeInFrame\":"<<unsigned(settings[0x531]);
 out<<",\"scopeSurfaceAttempts\":"<<scope_surface_adapter::attempts.load()
    <<",\"scopeSurfaceRestored\":"<<scope_surface_adapter::restored.load()
    <<",\"scopeSurfaceFailures\":"<<scope_surface_adapter::failures.load()
    <<",\"scopeSurfaceHRESULT\":"<<scope_surface_adapter::lastResult.load()
    <<",\"scopeSurfaceSize\":["<<scope_surface_adapter::width.load()<<","<<scope_surface_adapter::height.load()<<"]"
    <<",\"scopeSurfaceSamples\":"<<scope_surface_adapter::samples.load();
 out<<",\"lensSamples\":"<<lensSamples<<",\"lensTracking\":["<<lensTracking.x<<","<<lensTracking.y<<","<<lensTracking.z<<"]";
 auto manager=static_cast<unsigned char*>(reinterpret_cast<void*(__cdecl*)()>(b+0x12BC5)());
 auto backend=*reinterpret_cast<unsigned char**>(manager);
 if(backend&&*reinterpret_cast<void**>(backend)==b+0xA17AD0){
  out<<",\"scopeTextureReady\":"<<unsigned(backend[0x2E80])<<",\"scopeLayerGates\":["<<unsigned(backend[0x2C2C])<<","<<unsigned(backend[0x2C2D])<<","<<unsigned(backend[0x2C2E])<<"]";
  out<<",\"nativeAuxPanels\":[";
  for(unsigned n=0;n<2;++n){
   auto d=backend+0x3754+n*0x58;auto p=reinterpret_cast<Vec*>(d+4);
   if(n)out<<",";
   out<<"{\"active\":"<<unsigned(d[0])<<",\"ready\":"<<unsigned(d[0x54])
      <<",\"position\":["<<p->x<<","<<p->y<<","<<p->z<<"],\"size\":["
      <<*reinterpret_cast<float*>(d+0x20)<<","<<*reinterpret_cast<float*>(d+0x24)<<"]}";
  }
  out<<"]";
 }
 out<<",\"speakerTracking\":["<<speakerTracking.x<<","<<speakerTracking.y<<","<<speakerTracking.z<<"]";
 auto head=reinterpret_cast<Vec*>(manager+0x1C);
 out<<",\"nativeHeadTracking\":["<<head->x<<","<<head->y<<","<<head->z<<"]";
 out<<",\"nativePanelAnchored\":"<<unsigned(manager[0xBA8])<<",\"nativePopupMode\":"<<*reinterpret_cast<unsigned*>(b+0x10D65B8);
 out<<",\"worldHud\":{\"active\":"<<unsigned(manager[0x3040])<<",\"target\":"<<*reinterpret_cast<unsigned*>(b+0x10D6F38)
    <<",\"pages\":"<<floating_hud_adapter::pages.load()<<",\"selected\":"<<*reinterpret_cast<int*>(b+0x10D6F4C)
    <<",\"top\":"<<*reinterpret_cast<int*>(b+0xE8AD30)<<",\"paths\":[";
 for(unsigned i=0;i<2;++i){if(i)out<<",";const auto p=floating_hud_adapter::lastWorld[i];
  out<<"{\"samples\":"<<floating_hud_adapter::samples[i].load()<<",\"relocations\":"<<floating_hud_adapter::relocations[i].load()
     <<",\"world\":["<<p.x<<","<<p.y<<","<<p.z<<"]}";
 }auto hudPos=reinterpret_cast<Vec*>(manager+0x3044);out<<"],\"tracking\":["<<hudPos->x<<","<<hudPos->y<<","<<hudPos->z<<"]}";
}
inline void testScopeSchedule(bool inFrame){
 // Caller is the simulator-only game-thread mailbox. Never changes disk INIs.
 auto b=build_adapter::heldModule();if(!b)return;
 auto settings=reinterpret_cast<unsigned char*(__cdecl*)()>(b+0xA641)();settings[0x531]=inFrame?1:0;
}
inline Vec* __fastcall targetToTracking(void* manager,void*,Vec* out,const Vec* world){
 auto result=reinterpret_cast<Vec*(__thiscall*)(void*,Vec*,const Vec*)>(base+0x64B0B0)(manager,out,world);
 const auto head=*reinterpret_cast<Vec*>(static_cast<unsigned char*>(manager)+0x1C);
 targetDistance=scene::distance(*result,head);
 speakerTracking=*result;
 return result;
}
inline bool __cdecl besideSpeaker(const Pose* head,const Vec* toward,Pose* out,Vec* direction,float nativeDegrees){
 Vec offset=*toward;
 // Only Dialogue (3F1). Other target panels keep their native placement.
 int menu=reinterpret_cast<int(__cdecl*)()>(base+0xD094)();
 if(menu==0x3F1&&std::isfinite(targetDistance)&&targetDistance>0.01f){
  const auto settings=reinterpret_cast<unsigned char*(__cdecl*)()>(base+0xA641)();
  float distance=*reinterpret_cast<float*>(settings+0x5D8);
  float width=*reinterpret_cast<float*>(settings+0x5D4);
  if(std::isfinite(distance)&&distance>=0.75f&&std::isfinite(width)&&width>0){
   // Clear the speaker's silhouette by the panel's half-width plus a small
   // body margin. Choose whichever side has more forward-view space.
   // Native anchoring keeps this initial choice fixed during the conversation.
   const auto q=head->rotation;
   const Vec right{1-2*(q[1]*q[1]+q[2]*q[2]),2*(q[0]*q[1]+q[3]*q[2]),2*(q[0]*q[2]-q[3]*q[1])};
   const float actorRight=toward->x*right.x+toward->y*right.y+toward->z*right.z;
   const float degrees=dialogue_layout::sideAngle(width,distance,targetDistance,actorRight);
   // The helper's degree argument is DOWNWARD PITCH, not yaw. Rotate its
   // incoming horizontal direction instead, preserving native 12-degree tilt.
   const float radians=degrees*.01745329252f,c=std::cos(radians),s=std::sin(radians);
   offset.x=toward->x*c-toward->z*s;offset.z=toward->x*s+toward->z*c;
   lastDistance=targetDistance;lastAngle=degrees;placements++;
  }
 }
 targetDistance=0;
 return reinterpret_cast<bool(__cdecl*)(const Pose*,const Vec*,Pose*,Vec*,float)>(base+0x641930)(head,&offset,out,direction,nativeDegrees);
}
inline bool callMatches(unsigned char* at,unsigned char* to){
 int rel{};memcpy(&rel,at+1,4);return at[0]==0xE8&&at+5+rel==to;
}
inline const char* install(bool configured=false){
 char test[8]{};const bool privateTest=GetEnvironmentVariableA("SC_TEST_PRESENTATION",test,8)&&!strcmp(test,"1");
 if(!configured&&!privateTest)return "world panels disabled by configuration";
 if(base)return "already installed";
 auto b=build_adapter::heldModule();if(!b)return "refused unsupported build";
 const unsigned char conversion[]={0x83,0xEC,0x60,0x56,0x8B,0xF1,0x57};
 const unsigned char helper[]={0x55,0x8B,0xEC,0x83,0xE4,0xF8,0x83,0xEC,0x5C};
 if(!callMatches(b+0x644B44,b+0x273FE)||!callMatches(b+0x644C12,b+0x1375)||memcmp(b+0x64B0B0,conversion,sizeof(conversion))||memcmp(b+0x641930,helper,sizeof(helper)))return "refused dialogue call guards";
 if(!callMatches(b+0x649FA0,b+0x273FE))return "refused speaker publication guard";
 // Reserve both sites before changing either; install only at DeferredInit.
 DWORD old1{},old2{};
 if(!VirtualProtect(b+0x644B44,5,PAGE_EXECUTE_READWRITE,&old1))return "refused target protection";
 if(!VirtualProtect(b+0x644C12,5,PAGE_EXECUTE_READWRITE,&old2)){DWORD ignored;VirtualProtect(b+0x644B44,5,old1,&ignored);return "refused pose protection";}
 base=b;
 // Read-only lens probe, only in this explicitly enabled diagnostic route.
 if(privateTest&&callMatches(b+0x33368E,b+0x1E65)){
  DWORD old{};if(VirtualProtect(b+0x33368E,5,PAGE_EXECUTE_READWRITE,&old)){
   int rel=int(reinterpret_cast<unsigned char*>(&inspectLens)-(b+0x333693));memcpy(b+0x33368F,&rel,4);
   FlushInstructionCache(GetCurrentProcess(),b+0x33368E,5);DWORD ignored;VirtualProtect(b+0x33368E,5,old,&ignored);
  }
 }
 int first=int(reinterpret_cast<unsigned char*>(&targetToTracking)-(b+0x644B49));
 int second=int(reinterpret_cast<unsigned char*>(&besideSpeaker)-(b+0x644C17));
 memcpy(b+0x644B45,&first,4);memcpy(b+0x644C13,&second,4);
 FlushInstructionCache(GetCurrentProcess(),b+0x644B44,5);FlushInstructionCache(GetCurrentProcess(),b+0x644C12,5);
 DWORD ignored;VirtualProtect(b+0x644C12,5,old2,&ignored);VirtualProtect(b+0x644B44,5,old1,&ignored);
 DWORD oldSpeaker{};
 if(VirtualProtect(b+0x649FA0,5,PAGE_EXECUTE_READWRITE,&oldSpeaker)){
  int rel=int(reinterpret_cast<unsigned char*>(&publishSpeaker)-(b+0x649FA5));memcpy(b+0x649FA1,&rel,4);
  FlushInstructionCache(GetCurrentProcess(),b+0x649FA0,5);VirtualProtect(b+0x649FA0,5,oldSpeaker,&ignored);
 }
 // In normal play LocalMetaLink owns the graphics fix. The addon only owns
 // world panel positioning and input. The private test also exercises scope.
 if(privateTest&&!scope_surface_adapter::install(b))return "world panels: scope test guard refused";
 if(!floating_hud_adapter::install(b))return "dialogue installed; floating HUD guards refused";
 return "installed dialogue placement and stable world HUDs with loot paging";
}
}
