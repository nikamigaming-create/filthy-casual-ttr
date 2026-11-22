#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../build_adapter.hpp"
#include "../scene_adapter.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
// Read-only native equipment observation for 119A085A / PE 6AFA9B4F.
// initialize()/snapshot()/readReference() belong to the native game thread.
// Returned pointers are borrowed identities, valid for this callback only.
// Never store them across load, cell/root rebuild, or a native pickup operation.
namespace pool_equipment {
inline unsigned char* module=nullptr;
inline DWORD ownerThread=0;
struct Reference {
 void* reference=nullptr; void* root=nullptr; unsigned refID=0,baseID=0;
 bool misc=false,poseValid=false; scene::Transform pose{};
};
struct Slot : Reference {
 bool active=false; int hand=-1; void* heldNode=nullptr;
 bool heldNodeInRoot=false;
};
struct Pull {bool active=false;void* reference=nullptr;};
struct Candidate {
 bool active=false;void* reference=nullptr;void* root=nullptr;
 // verified means current MISC/root identity and a valid native hand ray, not
 // ownership or a promise that the engine will accept a keyframed body.
 bool verified=false;float distance=-1;float forwardDistance=0;
 scene::Vec handOrigin{},handForward{};
};
struct Snapshot {bool available=false;Slot slots[2];Pull pulls[2];Candidate candidates[2];};
inline bool readable(const void* p,size_t n){
 if(!p||!n)return false;MEMORY_BASIC_INFORMATION m{};
 if(!VirtualQuery(p,&m,sizeof m)||m.State!=MEM_COMMIT||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
 const auto prot=m.Protect&255;
 if(prot!=PAGE_READONLY&&prot!=PAGE_READWRITE&&prot!=PAGE_WRITECOPY&&prot!=PAGE_EXECUTE_READ&&prot!=PAGE_EXECUTE_READWRITE&&prot!=PAGE_EXECUTE_WRITECOPY)return false;
 auto a=reinterpret_cast<uintptr_t>(p),b=reinterpret_cast<uintptr_t>(m.BaseAddress);
 return a>=b&&a-b<=m.RegionSize&&n<=m.RegionSize-(a-b);
}
inline bool initialize(){
 if(module)return ownerThread==GetCurrentThreadId();
 auto b=build_adapter::heldModule();if(!b)return false;
 auto nt=reinterpret_cast<IMAGE_NT_HEADERS32*>(b+reinterpret_cast<IMAGE_DOS_HEADER*>(b)->e_lfanew);
 if(nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||nt->FileHeader.TimeDateStamp!=0x6AFA9B4F)return false;
 // Native shared-object predicate reads both active flags and ref/node equality.
 // Check relocated operands as well as the exact small function's opcodes.
 auto absolute=[&](unsigned at,unsigned target){return *reinterpret_cast<void**>(b+at)==b+target;};
 const unsigned char end[]={0xB0,1,0xC3,0x32,0xC0,0xC3};
 if(b[0x1CB210]!=0x80||b[0x1CB211]!=0x3D||!absolute(0x1CB212,0x1538805)||
    b[0x1CB219]!=0x80||b[0x1CB21A]!=0x3D||!absolute(0x1CB21B,0x15388D4)||
    !absolute(0x1CB223,0x15388DC)||!absolute(0x1CB229,0x15395E0)||
    !absolute(0x1CB230,0x15388E0)||!absolute(0x1CB236,0x15395E4)||memcmp(b+0x1CB23C,end,sizeof end))return false;
 module=b;ownerThread=GetCurrentThreadId();return true;
}
inline bool onOwnerThread(){return module&&ownerThread==GetCurrentThreadId();}
inline bool validPose(const scene::Transform& pose){
 for(unsigned i=0;i<13;i++)if(!std::isfinite(reinterpret_cast<const float*>(&pose)[i]))return false;
 return pose.scale>0.00001f&&pose.scale<10000;
}
inline Reference readReference(void* ref){
 Reference r;r.reference=ref;if(!onOwnerThread()||!readable(ref,0x68))return r;
 // FNV1.4.0.525 TESObjectREFR +20 base, +64 render state, state+14 root.
 // Public xNVSE/JIP GameForms.h: REFR3A, MISC1F; no other reference class accepted.
 if(scene::field<unsigned char>(ref,4)!=0x3A)return r;
 r.refID=scene::field<unsigned>(ref,0xC);
 auto base=scene::field<void*>(ref,0x20);
 if(readable(base,0x10)){r.baseID=scene::field<unsigned>(base,0xC);r.misc=scene::field<unsigned char>(base,4)==0x1F;}
 auto render=scene::field<void*>(ref,0x64);if(!readable(render,0x18))return r;
 auto root=scene::field<void*>(render,0x14);if(!readable(root,0x9C))return r;
 r.root=root;r.pose=scene::world(root);r.poseValid=validPose(r.pose);return r;
}
// Same manager grip query used by native ordinary pickup on this exact build.
struct GripPoint {bool valid=false;scene::Vec world{};};
inline GripPoint gripPoint(unsigned hand){
 GripPoint out;if(!onOwnerThread()||hand>1)return out;
 auto manager=reinterpret_cast<void*(__cdecl*)()>(module+0x141CD)();if(!manager)return out;
 float rotation[9]{};
 if(!reinterpret_cast<bool(__thiscall*)(void*,bool,bool,scene::Vec*,float*)>(module+0x70ACB0)(manager,hand!=0,false,&out.world,rotation))return {};
 out.valid=std::isfinite(out.world.x)&&std::isfinite(out.world.y)&&std::isfinite(out.world.z);return out.valid?out:GripPoint{};
}
// Read-only native gravity-glove ray, independent of the optional ammo addon.
// 119A085A 1C2630: cdecl(bool right, Vec* worldOrigin, Vec* worldForward).
struct GloveRay {bool valid=false;scene::Vec origin{},forward{};};
inline GloveRay gloveRay(unsigned hand){
 GloveRay out;if(!onOwnerThread()||hand>1)return out;
 if(!reinterpret_cast<bool(__cdecl*)(bool,scene::Vec*,scene::Vec*)>(module+0x1C2630)(hand!=0,&out.origin,&out.forward))return {};
 auto finite=[](scene::Vec p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);};
 float length=scene::distance(out.forward,{});
 out.valid=finite(out.origin)&&finite(out.forward)&&length>.99f&&length<1.01f;
 return out.valid?out:GloveRay{};
}
// Native glove's actual collision-ray hit BEFORE its dynamic-mass eligibility
// filter. Reading the same ray permits our table writer to yield before native
// pickup, without publishing a fake candidate or invoking a grab. Game-thread only.
inline Candidate prePickupTarget(unsigned hand){
 Candidate out;if(!onOwnerThread()||hand>1)return out;
 if(!reinterpret_cast<bool(__cdecl*)(bool,scene::Vec*,scene::Vec*)>(module+0x1C2630)(hand!=0,&out.handOrigin,&out.handForward))return out;
 auto finite=[](scene::Vec v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);};
 float length=scene::distance(out.handForward,{});
 if(!finite(out.handOrigin)||!finite(out.handForward)||length<.99f||length>1.01f)return out;
 float reach=reinterpret_cast<float(__cdecl*)()>(module+0x1B70D0)();
 if(!std::isfinite(reach)||reach<=0)return out;
 reach=std::min(reach,500.f);
 auto ref=reinterpret_cast<void*(__cdecl*)(scene::Vec*,scene::Vec*,float)>(module+0x1B46E0)(&out.handOrigin,&out.handForward,reach);
 auto item=readReference(ref);if(!item.misc||!item.poseValid)return out;
 out.distance=scene::distance(item.pose.p,out.handOrigin);
 auto delta=item.pose.p-out.handOrigin;
 out.forwardDistance=delta.x*out.handForward.x+delta.y*out.handForward.y+delta.z*out.handForward.z;
 if(!std::isfinite(out.distance)||!std::isfinite(out.forwardDistance)||out.distance>500||out.forwardDistance<=0)return out;
 out.active=out.verified=true;out.reference=ref;out.root=item.root;return out;
}
// Read-only 119A085A gravity-flight fields. No catch/grab call or state edit.
struct PullInspection {
 bool active=false,valid=false,rootMatches=false,launchPending=false;
 unsigned refID=0;float elapsed=0,plannedDuration=0,radius=0;
 scene::Vec localAnchor{},worldBoundCenter{};float worldBoundRadius=0;
};
inline PullInspection inspectPull(unsigned hand){
 PullInspection out;if(!onOwnerThread()||hand>1)return out;
 auto p=module+0x1035E70+hand*0x88;if(!readable(p,0x88))return out;
 out.active=p[0]!=0;if(!out.active)return out;
 auto ref=*reinterpret_cast<void**>(p+4);auto root=*reinterpret_cast<void**>(p+8);
 auto item=readReference(ref);out.refID=item.refID;
 out.rootMatches=item.poseValid&&item.root==root;if(!out.rootMatches)return out;
 out.elapsed=*reinterpret_cast<float*>(p+0x10);
 out.plannedDuration=*reinterpret_cast<float*>(p+0x14);
 out.radius=*reinterpret_cast<float*>(p+0x18);
 out.localAnchor=*reinterpret_cast<scene::Vec*>(p+0x1C);
 out.launchPending=p[0x35]!=0;
 if(!readable(root,0x24))return out;
 auto bound=scene::field<void*>(root,0x20);if(!readable(bound,16))return out;
 out.worldBoundCenter=*reinterpret_cast<scene::Vec*>(bound);
 out.worldBoundRadius=scene::field<float>(bound,12);
 for(float v:{out.elapsed,out.plannedDuration,out.radius,out.localAnchor.x,out.localAnchor.y,
              out.localAnchor.z,out.worldBoundCenter.x,out.worldBoundCenter.y,
              out.worldBoundCenter.z,out.worldBoundRadius})if(!std::isfinite(v))return out;
 if(out.radius<=0||out.radius>=10000||out.worldBoundRadius<=0||out.worldBoundRadius>=10000)return out;
 out.valid=p[0]!=0&&*reinterpret_cast<void**>(p+4)==ref&&*reinterpret_cast<void**>(p+8)==root;
 return out;
}
inline bool belongsTo(void* node,void* root){
 for(unsigned i=0;node&&i<64;i++){
  if(node==root)return true;if(!readable(node,0x1C))return false;
  auto parent=scene::field<void*>(node,0x18);if(parent==node)return false;node=parent;
 }return false;
}
inline Snapshot snapshot(){
 Snapshot out;if(!onOwnerThread())return out;auto b=module;
 // Slots swap when native logic needs to process the secondary hand. They are
 // NOT left/right slots. 1035E60 and 15388D8 contain actual 0=left/1=right identity.
 constexpr unsigned flags[]={0x1538805,0x15388D4},hands[]={0x1035E60,0x15388D8};
 constexpr unsigned refs[]={0x15395E0,0x15388DC},nodes[]={0x15395E4,0x15388E0};
 for(unsigned i=0;i<2;i++){
  auto& s=out.slots[i];s.active=b[flags[i]]!=0;if(!s.active)continue;
  s.hand=*reinterpret_cast<int*>(b+hands[i]);s.reference=*reinterpret_cast<void**>(b+refs[i]);
  s.heldNode=*reinterpret_cast<void**>(b+nodes[i]);
  if(s.hand<0||s.hand>1||!s.reference)return Snapshot{};
  auto r=readReference(s.reference);static_cast<Reference&>(s)=r;
  // The native context is an optional child node, not necessarily the root.
  // A NULL context is legitimate, including when two hands own the same item.
  s.heldNodeInRoot=s.heldNode&&r.root&&belongsTo(s.heldNode,r.root);
 }
 for(unsigned h=0;h<2;h++){
  auto p=b+0x1035E70+h*0x88;out.pulls[h]={p[0]!=0,*reinterpret_cast<void**>(p+4)};
  if(out.pulls[h].active&&!out.pulls[h].reference)return Snapshot{};
  // Native glove candidate publication: active/ref/root.
  // Candidate is only targeting intent, not held ownership or proof of eligibility.
  auto c=b+0x15393B0+h*0x78;auto& target=out.candidates[h];
  target.active=c[0]!=0;target.reference=*reinterpret_cast<void**>(c+4);target.root=*reinterpret_cast<void**>(c+8);
  if(target.active&&target.reference&&target.root){
   auto current=readReference(target.reference);
   // 119A085A native glove ray getter 1C2630: cdecl(bool right, Vec*, Vec*).
   // It obtains the current native game-space hand pose; no state is changed.
   bool ray=current.misc&&current.poseValid&&current.root==target.root&&
    reinterpret_cast<bool(__cdecl*)(bool,scene::Vec*,scene::Vec*)>(b+0x1C2630)(h!=0,&target.handOrigin,&target.handForward);
   if(ray){
    const auto delta=current.pose.p-target.handOrigin;
    target.distance=scene::distance(current.pose.p,target.handOrigin);
    target.forwardDistance=delta.x*target.handForward.x+delta.y*target.handForward.y+delta.z*target.handForward.z;
    const auto length=scene::distance(target.handForward,{});
    target.verified=std::isfinite(target.distance)&&std::isfinite(target.forwardDistance)&&std::isfinite(length)&&length>0.99f&&length<1.01f;
   }
  }
 }
 // Fail closed on a native slot exchange or release during observation. This
 // is not a substitute for thread affinity and does not claim an atomic frame.
 for(unsigned i=0;i<2;i++){
  const auto& s=out.slots[i];if((b[flags[i]]!=0)!=s.active)return Snapshot{};
  if(s.active&&(s.hand!=*reinterpret_cast<int*>(b+hands[i])||s.reference!=*reinterpret_cast<void**>(b+refs[i])||s.heldNode!=*reinterpret_cast<void**>(b+nodes[i])))return Snapshot{};
  auto p=b+0x1035E70+i*0x88;
  if((p[0]!=0)!=out.pulls[i].active||*reinterpret_cast<void**>(p+4)!=out.pulls[i].reference)return Snapshot{};
  auto c=b+0x15393B0+i*0x78;const auto& target=out.candidates[i];
  if((c[0]!=0)!=target.active||*reinterpret_cast<void**>(c+4)!=target.reference||*reinterpret_cast<void**>(c+8)!=target.root)return Snapshot{};
 }
 out.available=true;return out;
}
inline unsigned heldMask(const Snapshot& s,const void* ref){
 unsigned result=0;if(s.available&&ref)for(const auto& slot:s.slots)if(slot.active&&slot.reference==ref)result|=1u<<slot.hand;return result;
}
inline unsigned pullingMask(const Snapshot& s,const void* ref){
 unsigned result=0;if(s.available&&ref)for(unsigned h=0;h<2;h++)if(s.pulls[h].active&&s.pulls[h].reference==ref)result|=1u<<h;return result;
}
inline unsigned candidateMask(const Snapshot& s,const void* ref){
 unsigned result=0;if(s.available&&ref)for(unsigned h=0;h<2;h++)if(s.candidates[h].active&&s.candidates[h].reference==ref)result|=1u<<h;return result;
}
// A native active target is intent only. The caller must bound distance for its
// own small table zone and stop table writes BEFORE changing body motion type.
// Raw candidateMask can retain a conservative pause if verification is lost.
inline unsigned targetedMask(const Snapshot& s,const void* ref){
 unsigned result=0;if(s.available&&ref)for(unsigned h=0;h<2;h++)if(s.candidates[h].active&&s.candidates[h].verified&&s.candidates[h].reference==ref)result|=1u<<h;return result;
}
inline bool sharedHold(const Snapshot& s,const void* ref){
 return s.available&&ref&&heldMask(s,ref)==3&&s.slots[0].active&&s.slots[1].active&&
  s.slots[0].reference==ref&&s.slots[1].reference==ref&&s.slots[0].heldNode==s.slots[1].heldNode;
}
inline const Slot* forHand(const Snapshot& s,int hand){
 if(!s.available||hand<0||hand>1)return nullptr;
 const Slot* result=nullptr;for(const auto& slot:s.slots)if(slot.active&&slot.hand==hand){if(result)return nullptr;result=&slot;}return result;
}
// True means table code MUST NOT write/refit/move this reference. It includes
// native gravity pull and unavailable state; releasing the last native owner
// is not permission to teleport until the caller's free-world policy resolves.
inline bool nativeOwnsOrUnknown(const Snapshot& s,const void* ref){return !s.available||heldMask(s,ref)||pullingMask(s,ref);}
}
