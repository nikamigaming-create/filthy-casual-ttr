#pragma once
#include "reload_adapter.hpp"
#include "scene_adapter.hpp"
#include "reload_carrier_data.hpp"
// Exact-build release transaction. The callback must commit a real world object
// before returning true; false resumes the untouched native release path.
namespace physical_reload_adapter {
struct Drop {void* player;void* ammo;void* visual;int rounds;scene::Transform pose;scene::Vec center;scene::Vec velocity;};
using Commit=bool(__cdecl*)(const Drop&);
inline Commit commit=nullptr;inline void* fallback=nullptr;inline void* completed=nullptr;
inline unsigned char* module=nullptr;inline DWORD ownerThread=0;
inline unsigned attempts=0,successes=0,calibration=0,pouchReturns=0,failures=0;
inline float lastDistance=0,lastThreshold=0;inline unsigned lastRefID=0;
inline bool __cdecl onRelease(unsigned char* frame){
 auto b=module;if(!b||GetCurrentThreadId()!=ownerThread||!commit)return false;
 if(b[0x121F1AC]){calibration++;return false;}
 auto settings=build_adapter::liveReloadSettings();if(!settings||settings[0x494]!=1||!settings[0x495])return false;
 // The primary mesh follows the hand. Use the visible pouch COPY for the
 // actual return zone; a distance measured to the held mesh is always tiny.
 auto visual=*reinterpret_cast<void**>(b+0x121F180);
 auto pouch=*reinterpret_cast<void**>(b+0x121F194);
 if(!build_adapter::readPouchNode(visual).readable||!build_adapter::readPouchNode(pouch).readable)return false;
 const auto localCenter=*reinterpret_cast<scene::Vec*>(b+0x121F184);
 const auto heldPose=scene::world(visual),pouchPose=scene::world(pouch);
 lastDistance=scene::distance(heldPose.p+scene::rotate(heldPose,localCenter)*heldPose.scale,pouchPose.p+scene::rotate(pouchPose,localCenter)*pouchPose.scale);
 lastThreshold=*reinterpret_cast<float*>(settings+0x4CC)* *reinterpret_cast<float*>(frame-0x15DC);
 if(!std::isfinite(lastDistance)||!std::isfinite(lastThreshold)||lastDistance<lastThreshold){pouchReturns++;return false;}
 Drop d{};d.player=*reinterpret_cast<void**>(frame-0x1600);d.ammo=*reinterpret_cast<void**>(frame-0x1628);
 d.visual=*reinterpret_cast<void**>(b+0x121F180);d.rounds=*reinterpret_cast<int*>(b+0x121F264);
 if(!d.player||!d.ammo||!d.visual||d.rounds<1||d.rounds>10000)return false;
 auto n=build_adapter::readPouchNode(d.visual);if(!n.readable)return false;d.pose=scene::world(d.visual);d.center=localCenter;
 if(b[0x121F001]&&b[0x121F003]){d.velocity=*reinterpret_cast<scene::Vec*>(b+0x121F064);if(!std::isfinite(d.velocity.x)||!std::isfinite(d.velocity.y)||!std::isfinite(d.velocity.z)||scene::distance(d.velocity,{})>6000)d.velocity={};}
 attempts++;if(!commit(d)){failures++;return false;}
 successes++;return true;
}
// Preserve SIMD/x87 as well as integer state: native continuation is mid-frame.
__declspec(naked) inline void entry(){
 __asm{
  pushfd
  pushad
  mov eax,esp
  sub esp,528
  and esp,0FFFFFFF0h
  mov [esp+512],eax
  fxsave [esp]
  fninit
  push ebp
  call onRelease
  add esp,4
  test al,al
  jz nativePath
  fxrstor [esp]
  mov esp,[esp+512]
  popad
  popfd
  jmp dword ptr [completed]
 nativePath:
  fxrstor [esp]
  mov esp,[esp+512]
  popad
  popfd
  jmp dword ptr [fallback]
 }
}

inline void* installedFallback=nullptr;inline void* installedCompleted=nullptr;
inline void* ejectOriginal=nullptr;inline void* ejectBContinue=nullptr;
inline bool ejectBCommitted=false;
inline unsigned installedDrops=0,ejectedDrops=0,emptyDrops=0;
inline bool installedSnapshot(void* player,void* ammo,int rounds){
 auto b=module;auto settings=build_adapter::liveReloadSettings();
 if(!b||GetCurrentThreadId()!=ownerThread||!commit||!settings||settings[0x494]!=1)return false;
 if(rounds<0||rounds>10000||(rounds>0&&!settings[0x495]))return false;
 Drop d{};d.player=player;d.ammo=ammo;d.rounds=rounds;d.visual=*reinterpret_cast<void**>(b+0x121EF98);
 if(!d.player||!d.visual||(rounds>0&&!d.ammo))return false;
 if(!build_adapter::readPouchNode(d.visual).readable)return false;
 d.pose=scene::world(d.visual);auto bound=scene::field<float*>(d.visual,0x20);
 if(!bound||!std::isfinite(bound[3])||bound[3]<=0||bound[3]>350||!std::isfinite(d.pose.scale)||std::abs(d.pose.scale)<.001f)return false;
 scene::Vec center{bound[0],bound[1],bound[2]};
 if(!std::isfinite(center.x)||!std::isfinite(center.y)||!std::isfinite(center.z))return false;
 d.center=scene::inverseRotate(d.pose,center-d.pose.p)*(1/d.pose.scale);
 attempts++;if(!commit(d)){failures++;return false;}successes++;if(!rounds)emptyDrops++;return true;
}
inline bool __cdecl onInstalled(unsigned char* frame,unsigned originalEax){
 if(!module||module[0x121F1AC]||(originalEax&255))return false;
 if(!installedSnapshot(*reinterpret_cast<void**>(frame-0x1600),*reinterpret_cast<void**>(frame-0x1628),*reinterpret_cast<int*>(module+0x121F140)))return false;
 installedDrops++;return true;
}
__declspec(naked) inline void installedEntry(){
 __asm{
  pushfd
  pushad
  mov eax,esp
  sub esp,528
  and esp,0FFFFFFF0h
  mov [esp+512],eax
  fxsave [esp]
  fninit
  mov eax,[eax+28]
  push eax
  push ebp
  call onInstalled
  add esp,8
  test al,al
  jz nativePath
  fxrstor [esp]
  mov esp,[esp+512]
  popad
  popfd
  jmp dword ptr [installedCompleted]
 nativePath:
  fxrstor [esp]
  mov esp,[esp+512]
  popad
  popfd
  jmp dword ptr [installedFallback]
 }
}
// Native call boundaries are void. Preserve the entire incoming CPU state,
// including x87/SIMD, around the transaction and original cleanup calls.
inline void __cdecl onEject(unsigned* args,int route){
 auto b=module;auto player=reinterpret_cast<void*>(args[0]);auto ammo=reinterpret_cast<void*>(args[1]);int count=int(args[2]);
 if(route==0){
  using Fn=void(__cdecl*)(void*,void*,int,unsigned);
  auto original=reinterpret_cast<Fn>(ejectOriginal);
  bool eligible=b[0x121F000]&&!b[0x121F001]&&!*reinterpret_cast<int*>(b+0x121F13C);
  bool ok=eligible&&installedSnapshot(player,ammo,std::max(0,count));
  original(player,ammo,ok?0:count,args[3]);
  if(ok){*reinterpret_cast<int*>(b+0x121F13C)=4;ejectedDrops++;}
 }else{
  bool ok=installedSnapshot(player,ammo,std::max(0,count));
  if(route==2)ejectBCommitted=ok;
  if(ok){if(route==1)*reinterpret_cast<int*>(b+0x121F13C)=4;ejectedDrops++;}
  else reinterpret_cast<void(__cdecl*)(void*,void*,int)>(b+0x280450)(player,ammo,count);
 }
}
#define FC_EJECT_ENTRY(name,route) \
 __declspec(naked) inline void name(){ \
 __asm { pushfd } \
 __asm { pushad } \
 __asm { mov eax,esp } \
 __asm { sub esp,528 } \
 __asm { and esp,0FFFFFFF0h } \
 __asm { mov [esp+512],eax } \
 __asm { fxsave [esp] } \
 __asm { fninit } \
 __asm { add eax,40 } \
 __asm { push route } \
 __asm { push eax } \
 __asm { call onEject } \
 __asm { add esp,8 } \
 __asm { fxrstor [esp] } \
 __asm { mov esp,[esp+512] } \
 __asm { popad } \
 __asm { popfd } \
 __asm { ret } \
 }
FC_EJECT_ENTRY(ejectEntry,0)
FC_EJECT_ENTRY(ejectAEntry,1)
FC_EJECT_ENTRY(ejectBEntry,2)
#undef FC_EJECT_ENTRY
__declspec(naked) inline void ejectBStateEntry(){
 __asm{
  pushfd
  push eax
  mov eax,module
  cmp byte ptr [ejectBCommitted],0
  je nativePath
  mov dword ptr [eax+121F13Ch],4
  mov byte ptr [ejectBCommitted],0
  jmp done
 nativePath:
  mov dword ptr [eax+121F13Ch],5
 done:
  pop eax
  popfd
  jmp dword ptr [ejectBContinue]
 }
}

// Recover only our real, normally grabbed world part into the original FULL
// installed-magazine initializer. Original insertion/cocking remains native.
using Recover=int(__cdecl*)(void* player,int hand);
inline Recover recover=nullptr;
inline void* recoverFallback=nullptr;inline void* recoverComplete=nullptr;
inline unsigned recoveredParts=0,recoveredPouch=0;inline int recoveredRounds=-1,recoverStage=0;inline float recoverDistance=-1;
inline int __cdecl onRecover(unsigned char* frame){
 recoverStage=1;auto b=module;auto settings=build_adapter::liveReloadSettings();
 if(!b||!recover||GetCurrentThreadId()!=ownerThread||!settings||settings[0x494]!=1)return -1;
 recoverStage=2;
 if(*reinterpret_cast<int*>(b+0x121F13C)!=4||*reinterpret_cast<int*>(b+0x121F260)||b[0x121F1AC]||b[0x121F000]||b[0x121F8FA])return -1;
 recoverStage=3;if(!build_adapter::readPouchNode(*reinterpret_cast<void**>(b+0x121EF98)).readable)return -1;
 int hand=reinterpret_cast<bool(__cdecl*)()>(b+0x25C610)()?1:0;
 if(!frame[-0x1576])return -1;
 int count=recover(*reinterpret_cast<void**>(frame-0x1600),hand);
 if(count>=0){recoveredParts++;recoveredRounds=count;}return count;
}
__declspec(naked) inline void recoverEntry(){
 __asm{
  pushfd
  pushad
  mov eax,esp
  sub esp,528
  and esp,0FFFFFFF0h
  mov [esp+512],eax
  fxsave [esp]
  fninit
  push ebp
  call onRecover
  add esp,4
  cmp eax,0
  jl nativePath
  mov edx,[esp+512]
  mov [edx+4],eax
  fxrstor [esp]
  mov esp,[esp+512]
  popad
  popfd
  jmp dword ptr [recoverComplete]
 nativePath:
  fxrstor [esp]
  mov esp,[esp+512]
  popad
  popfd
  jmp dword ptr [recoverFallback]
 }
}

inline void* trampoline(unsigned char* p,unsigned n){
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,n+5,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return nullptr;
 memcpy(code,p,n);code[n]=0xE9;*reinterpret_cast<int*>(code+n+1)=int((p+n)-(code+n+5));DWORD old;
 if(!VirtualProtect(code,n+5,PAGE_EXECUTE_READ,&old)){VirtualFree(code,0,MEM_RELEASE);return nullptr;}
 FlushInstructionCache(GetCurrentProcess(),code,n+5);return code;
}
inline const char* install(Commit callback,Recover recovery){
 if(module)return "already installed";auto b=build_adapter::heldModule();if(!b)return "refused hash";
 auto cmp=[&](unsigned rva,unsigned global){auto p=b+rva;return p[0]==0x80&&p[1]==0x3D&&*reinterpret_cast<void**>(p+2)==b+global&&p[6]==0;};
 auto callSite=[&](unsigned rva){auto p=b+rva;return p[0]==0xE8&&p+5+*reinterpret_cast<int*>(p+1)==b+0x280450;};
 auto state=b+0x284032;
 if(!cmp(0x288B96,0x121F1AC)||!cmp(0x2868FD,0x121F1AC)||!cmp(0x258F90,0x121F000)||b[0x25905B]!=0xC3||!callSite(0x283C8D)||!callSite(0x284005)||state[0]!=0xC7||state[1]!=0x05||*reinterpret_cast<void**>(state+2)!=b+0x121F13C||*reinterpret_cast<int*>(state+6)!=5)return "refused release/ejection instructions";
 const unsigned char recoveryCmp[]={0x80,0xBD,0x6E,0xEA,0xFF,0xFF,0};
 if(memcmp(b+0x28A422,recoveryCmp,7)||b[0x28C491]!=0xE8||b+0x28C496+*reinterpret_cast<int*>(b+0x28C492)!=b+0x257AD0)return "refused recovery instructions";
 const unsigned char clonePrefix[]={0x55,0x8B,0xEC};if(memcmp(b+0x2580E0,clonePrefix,3))return "refused clone entry";
 fallback=trampoline(b+0x288B96,7);installedFallback=trampoline(b+0x2868FD,7);ejectOriginal=trampoline(b+0x258F90,7);
 recoverFallback=trampoline(b+0x28A422,7);
 if(!fallback||!installedFallback||!ejectOriginal||!recoverFallback)return "trampoline allocation failed";
 struct Patch{unsigned rva,n;void* target;unsigned char op;DWORD protection;};
 Patch patches[]={{0x288B96,7,entry,0xE9,0},{0x2868FD,7,installedEntry,0xE9,0},{0x258F90,7,ejectEntry,0xE9,0},{0x283C8D,5,ejectAEntry,0xE8,0},{0x284005,5,ejectBEntry,0xE8,0},{0x284032,10,ejectBStateEntry,0xE9,0},{0x28A422,7,recoverEntry,0xE9,0}};
 // Protect all sites before changing any instruction. Restore permissions on
 // failure; never leave a partially installed set of state transitions.
 unsigned count=0;for(auto& p:patches){if(!VirtualProtect(b+p.rva,p.n,PAGE_EXECUTE_READWRITE,&p.protection)){for(int j=int(count)-1;j>=0;j--){DWORD ignored;auto& q=patches[j];VirtualProtect(b+q.rva,q.n,q.protection,&ignored);}return "site protection failed";}count++;}
 completed=b+0x288CA0;installedCompleted=b+0x286934;ejectBContinue=b+0x28403C;commit=callback;recover=recovery;recoverComplete=b+0x28C491;ownerThread=GetCurrentThreadId();module=b;
 for(auto& patch:patches){auto p=b+patch.rva;p[0]=patch.op;*reinterpret_cast<int*>(p+1)=int(reinterpret_cast<unsigned char*>(patch.target)-(p+5));for(unsigned j=5;j<patch.n;j++)p[j]=0x90;FlushInstructionCache(GetCurrentProcess(),p,patch.n);}
 // Several sites share a page. Restore in reverse acquisition order.
 for(int i=int(count)-1;i>=0;i--){auto& p=patches[i];DWORD ignored;VirtualProtect(b+p.rva,p.n,p.protection,&ignored);}
 return "guarded physical spare/installed/ejection transactions active";
}
// FNV 1.4.0.525: xNVSE GameAPI.cpp LookupFormByID / NiTPointerMap.
// Resolve IDs afresh on the main thread; never keep a world-reference pointer.
inline void* lookup(unsigned id){
 struct Entry{Entry* next;unsigned key;void* data;};
 struct Map{void* vt;unsigned buckets;Entry** table;unsigned count;};
 auto map=*reinterpret_cast<Map**>(0x11C54C0);
 if(!map||!map->table||!map->buckets||map->buckets>1000000)return nullptr;
 unsigned guard=0;for(auto e=map->table[id%map->buckets];e&&guard++<100000;e=e->next)if(e->key==id)return e->data;
 return nullptr;
}
inline void* clone(void* node){auto b=build_adapter::heldModule();return b&&node?reinterpret_cast<void*(__cdecl*)(void*)>(b+0x2580E0)(node):nullptr;}
inline bool momentum(void* node,scene::Vec linear){auto b=build_adapter::heldModule();scene::Vec angular{};return b&&node&&reinterpret_cast<bool(__cdecl*)(void*,const scene::Vec*,const scene::Vec*,scene::Vec*)>(b+0x194C80)(node,&linear,&angular,nullptr);}

// FNV/JIP NiAVObject::ExportToFile sequence and ModelLoader::LoadModel ABI.
// Only privately derived visual nodes are cached; collision remains native.
inline bool exportVisual(void* node,const char* path){
 alignas(16) unsigned char stream[0x5C8]{};
 reinterpret_cast<void(__thiscall*)(void*)>(0xA66150)(stream);
 reinterpret_cast<void(__thiscall*)(void*,void*)>(0xA66370)(stream,node);
 bool ok=reinterpret_cast<bool(__thiscall*)(void*,const char*)>(0xA640B0)(stream,path);
 reinterpret_cast<void(__thiscall*)(void*)>(0xA65300)(stream);return ok;
}
inline void* loadVisual(const char* path){
 auto loader=*reinterpret_cast<void**>(0x11C3B3C);if(!loader)return nullptr;
 auto model=reinterpret_cast<void*(__thiscall*)(void*,const char*,unsigned,bool,unsigned,bool,bool)>(0x447080)(loader,path,0,true,0,false,true);
 return model?clone(model):nullptr;
}
// Patch only our serialized carrier's box and inertia. Never mutate a live or
// shared Havok shape. Strict FNV NIF header and block layout checks fail closed.
inline bool fitCarrierFile(const char* path,scene::Vec half){
 FILE* f=nullptr;if(fopen_s(&f,path,"rb"))return false;fseek(f,0,SEEK_END);auto size=ftell(f);rewind(f);
 if(size<128||size>16*1024*1024){fclose(f);return false;}std::vector<unsigned char> data(size);bool ok=fread(data.data(),1,size,f)==size_t(size);fclose(f);if(!ok)return false;
 size_t at=0;auto read=[&](unsigned n)->unsigned{if(at+n>data.size()||n>4)throw 1;unsigned v=0;memcpy(&v,data.data()+at,n);at+=n;return v;};
 auto skip=[&](size_t n){if(n>data.size()-at)throw 1;at+=n;};
 auto str=[&](){auto n=read(4);if(n>4096||n>data.size()-at)throw 1;std::string v(reinterpret_cast<char*>(data.data()+at),n);at+=n;return v;};
 auto put=[&](size_t p,float value){if(p+4>data.size()||!std::isfinite(value))throw 1;memcpy(data.data()+p,&value,4);};
 try{
  while(at<data.size()&&data[at++]!='\n'){};
  if(read(4)!=0x14020007||read(1)!=1||read(4)!=11)return false;auto count=read(4);if(!count||count>4096||read(4)!=34)return false;
  for(int i=0;i<3;i++)skip(read(1));auto ntypes=read(2);if(!ntypes||ntypes>256)return false;std::vector<std::string> types;for(unsigned i=0;i<ntypes;i++)types.push_back(str());
  std::vector<unsigned> indices(count),sizes(count);for(auto& v:indices){v=read(2);if(v>=types.size())return false;}for(auto& v:sizes)v=read(4);
  auto strings=read(4);read(4);if(strings>8192)return false;for(unsigned i=0;i<strings;i++)str();if(read(4)!=0)return false;
  size_t box=0,body=0;unsigned boxes=0,bodies=0;
  for(unsigned i=0;i<count;i++){if(types[indices[i]]=="bhkBoxShape"){if(sizes[i]!=32)return false;box=at;boxes++;}if(types[indices[i]]=="bhkRigidBody"){if(sizes[i]!=236)return false;body=at;bodies++;}skip(sizes[i]);}
  if(boxes!=1||bodies!=1)return false;
  memcpy(data.data()+box,reload_carrier_data::box,sizeof(reload_carrier_data::box));
  memcpy(data.data()+body+4,reload_carrier_data::body+4,sizeof(reload_carrier_data::body)-4);
  float h[3]={half.x/7,half.y/7,half.z/7};for(auto v:h)if(!std::isfinite(v)||v<=0||v>50)return false;
  put(box+4,std::min(.02f,std::min({h[0],h[1],h[2]})*.2f));for(int i=0;i<3;i++)put(box+16+i*4,h[i]);
  for(int i=0;i<12;i++)put(body+116+i*4,0);for(int i=0;i<3;i++)put(body+116+i*20,.15f*(h[(i+1)%3]*h[(i+1)%3]+h[(i+2)%3]*h[(i+2)%3])/3);
 }catch(...){return false;}
 if(fopen_s(&f,path,"wb"))return false;ok=fwrite(data.data(),1,data.size(),f)==data.size();ok=fclose(f)==0&&ok;return ok;
}
inline void stripControllers(void* node,int depth=0){
 if(!node||depth>48)return;auto& controller=scene::field<void*>(node,0xC);auto old=controller;controller=nullptr;scene::release(old);
 if(!reinterpret_cast<void*(__thiscall*)(void*)>(scene::field<void**>(node,0)[3])(node))return;
 auto count=scene::field<unsigned short>(node,0xA6);auto children=scene::field<void**>(node,0xA0);if(!children||count>4096)return;
 for(unsigned i=0;i<count;i++)stripControllers(children[i],depth+1);
}
inline bool isHeld(void* ref){auto b=build_adapter::heldModule();return b&&ref&&((b[0x1114215]&&*reinterpret_cast<void**>(b+0x11146D0)==ref)||(b[0x11142E4]&&*reinterpret_cast<void**>(b+0x11142EC)==ref));}
inline void rebuildReference(void* ref){
 if(!ref||isHeld(ref)||!scene::root(ref))return;
 reinterpret_cast<void(__thiscall*)(void*,void*,bool)>(0x5702E0)(ref,nullptr,true);
 scene::field<unsigned char>(ref,0x5F)|=1;auto loader=*reinterpret_cast<void**>(0x11C3B3C);
 if(loader)reinterpret_cast<void(__thiscall*)(void*,void*,unsigned,bool)>(0x444850)(loader,ref,1,false);
}
inline std::string hashFile(const char* path){
 FILE* f=nullptr;if(fopen_s(&f,path,"rb"))return {};
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;DWORD size=0,got=0;unsigned char digest[32]{};
 bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
 if(ok)ok=BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&got,0)>=0;
 std::vector<unsigned char> object(size),buf(65536);
 if(ok)ok=BCryptCreateHash(alg,&hash,object.data(),size,nullptr,0,0)>=0;
 size_t n;while(ok&&(n=fread(buf.data(),1,buf.size(),f))>0)ok=BCryptHashData(hash,buf.data(),static_cast<ULONG>(n),0)>=0;
 if(ferror(f))ok=false;fclose(f);if(ok)ok=BCryptFinishHash(hash,digest,32,0)>=0;
 if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);if(!ok)return {};
 std::string result;for(auto b:digest){result.push_back("0123456789abcdef"[b>>4]);result.push_back("0123456789abcdef"[b&15]);}return result;
}
inline void* placeEmpty(void* player,void* base){return player&&base?reinterpret_cast<void*(__cdecl*)(void*,void*,unsigned,unsigned,unsigned,float)>(0x5C4B30)(player,base,1,0,0,1):nullptr;}
inline void* dropAmmo(void* player,void* ammo,unsigned count){
 using Fn=void*(__thiscall*)(void*,void*,void*,unsigned,bool,bool,void*,unsigned,unsigned,bool,bool);
 auto vt=*reinterpret_cast<void***>(player);return reinterpret_cast<Fn>(vt[0x17C/4])(player,ammo,nullptr,count,true,true,nullptr,0,0,true,false);
}
// FNV ExtraDataList+04 chain; ExtraCount type24 stores a signed16 count at0C.
inline unsigned stackCount(void* ref){if(!ref)return 0;auto p=scene::field<void*>(ref,0x48);unsigned guard=0;while(p&&guard++<256){if(scene::field<unsigned char>(p,4)==0x24)return unsigned(std::max(0,int(scene::field<short>(p,0xC))));p=scene::field<void*>(p,8);}return 1;}
inline scene::Vec trackingPoint(scene::Vec point){
 auto b=build_adapter::heldModule();scene::Vec result{};
 if(b){auto manager=reinterpret_cast<void*(__cdecl*)()>(b+0x101E0)();
  if(manager)reinterpret_cast<scene::Vec*(__thiscall*)(void*,scene::Vec*,const scene::Vec*)>(b+0x572320)(manager,&result,&point);}
 return result;
}
inline scene::Vec gripPoint(int hand){
 auto b=build_adapter::heldModule();scene::Vec point{};float rotation[9]{};
 if(b){auto manager=reinterpret_cast<void*(__cdecl*)()>(b+0x101E0)();
  if(manager)reinterpret_cast<bool(__thiscall*)(void*,bool,bool,scene::Vec*,float*)>(b+0x56C530)(manager,hand!=0,false,&point,rotation);}
 return point;
}

// Native state2 uses the installed REST transform, not the currently held or
// hidden magazine node. Same parent/rest composition as RVA285949..285969.
inline bool installedRest(scene::Transform& result,scene::Vec* axis=nullptr){
 auto b=build_adapter::heldModule();if(!b)return false;
 auto node=*reinterpret_cast<void**>(b+0x121EF98);auto weapon=*reinterpret_cast<void**>(b+0x121EF8C);
 if(!build_adapter::readPouchNode(node).readable||!build_adapter::readPouchNode(weapon).readable)return false;
 auto w=scene::world(weapon);scene::Transform parent{};
 if(!reinterpret_cast<bool(__cdecl*)(void*,const scene::Transform*,void*,scene::Transform*)>(b+0x26AD10)(weapon,&w,node,&parent))return false;
 reinterpret_cast<scene::Transform*(__cdecl*)(scene::Transform*,const scene::Transform*,const scene::Vec*,const void*,float)>(b+0x257DE0)(&result,&parent,reinterpret_cast<scene::Vec*>(b+0x121EFE8),b+0x121EB5C,1.f);
 for(unsigned i=0;i<13;i++)if(!std::isfinite(reinterpret_cast<float*>(&result)[i]))return false;
 if(result.scale<=0||result.scale>100)return false;
 if(axis){*axis=scene::rotate(parent,*reinterpret_cast<scene::Vec*>(b+0xDCE340));float length=scene::distance(*axis,{});if(!std::isfinite(length)||length<1e-5f)return false;*axis=*axis*(1.f/length);}
 return true;
}

inline void trackingBasis(std::ostream& out,const scene::Transform& pose){
 auto origin=trackingPoint(pose.p);out<<"[";
 for(int i=0;i<3;i++){auto v=trackingPoint(pose.p+scene::Vec{pose.r[0][i],pose.r[1][i],pose.r[2][i]})-origin;float length=scene::distance(v,{});if(length>1e-5f)v=v*(1.f/length);if(i)out<<",";out<<"["<<v.x<<","<<v.y<<","<<v.z<<"]";}out<<"]";
}
inline void gloveDiagnostics(std::ostream& out){
 auto b=build_adapter::heldModule();if(!b)return;auto settings=build_adapter::liveReloadSettings();out<<",\"gloves\":[";
 for(int hand=0;hand<2;hand++){if(hand)out<<",";scene::Vec origin{},forward{};bool valid=reinterpret_cast<bool(__cdecl*)(bool,scene::Vec*,scene::Vec*)>(b+0x190B30)(hand!=0,&origin,&forward);auto p=trackingPoint(origin),f=trackingPoint(origin+forward)-p;float length=scene::distance(f,{});if(length>1e-5f)f=f*(1.f/length);
  auto candidate=b+0x11144A0+hand*0x78;auto pull=b+0xDC53F8+hand*0x88;auto ref=*reinterpret_cast<void**>(candidate+4);auto pulling=*reinterpret_cast<void**>(pull+4);
  out<<"{\"valid\":"<<valid<<",\"origin\":["<<p.x<<","<<p.y<<","<<p.z<<"],\"forward\":["<<f.x<<","<<f.y<<","<<f.z<<"],\"candidate\":"<<(candidate[0]&&ref?scene::field<unsigned>(ref,0xC):0)<<",\"armedAge\":"<<*reinterpret_cast<float*>(candidate+0xC)<<",\"pull\":"<<(pull[0]&&pulling?scene::field<unsigned>(pulling,0xC):0)<<",\"flickBack\":"<<(settings?*reinterpret_cast<float*>(settings+0x3FC):0)<<"}";
 }out<<"]";
 auto pouch=*reinterpret_cast<void**>(b+0x121F194);if(!build_adapter::readPouchNode(pouch).readable&&!*reinterpret_cast<int*>(b+0x121F260))pouch=*reinterpret_cast<void**>(b+0x121F180);
 if(build_adapter::readPouchNode(pouch).readable){auto w=scene::world(pouch);auto p=trackingPoint(w.p+scene::rotate(w,*reinterpret_cast<scene::Vec*>(b+0x121F184))*w.scale);out<<",\"physicalPouchTracking\":["<<p.x<<","<<p.y<<","<<p.z<<"]";}
}
inline void insertionDiagnostics(std::ostream& out){
 auto b=build_adapter::heldModule();scene::Transform rest{};scene::Vec axis{};bool valid=installedRest(rest,&axis);
 out<<",\"insertion\":{\"valid\":"<<(valid?"true":"false");
 if(valid){auto target=trackingPoint(rest.p),along=trackingPoint(rest.p+axis)-target;auto held=trackingPoint(scene::world(*reinterpret_cast<void**>(b+0x121EF98)).p);
  out<<",\"target\":["<<target.x<<","<<target.y<<","<<target.z<<"],\"held\":["<<held.x<<","<<held.y<<","<<held.z<<"],\"axis\":["<<along.x<<","<<along.y<<","<<along.z<<"]";
  out<<",\"restBasis\":";trackingBasis(out,rest);out<<",\"heldBasis\":";trackingBasis(out,scene::world(*reinterpret_cast<void**>(b+0x121EF98)));
  out<<",\"armed\":"<<unsigned(b[0x121F27D])<<",\"snap\":"<<unsigned(b[0x121F28C])<<",\"axial\":"<<*reinterpret_cast<float*>(b+0x121F280)<<",\"radial\":"<<*reinterpret_cast<float*>(b+0x121F284)<<",\"alignment\":"<<*reinterpret_cast<float*>(b+0x121F288);
 }out<<"}";
}
inline scene::Vec installedPoint(){
 auto b=build_adapter::heldModule();if(!b)return {};auto node=*reinterpret_cast<void**>(b+0x121EF98);auto weapon=*reinterpret_cast<void**>(b+0x121EF8C);
 if(!build_adapter::readPouchNode(node).readable||!build_adapter::readPouchNode(weapon).readable)return {};
 auto bound=scene::field<float*>(node,0x20);if(!bound)return {};auto w=scene::world(weapon);scene::Transform parent{};
 scene::Vec axis{w.r[0][1],w.r[1][1],w.r[2][1]};
 if(reinterpret_cast<bool(__cdecl*)(void*,const scene::Transform*,void*,scene::Transform*)>(b+0x26AD10)(weapon,&w,node,&parent))axis=scene::rotate(parent,*reinterpret_cast<scene::Vec*>(b+0xDCE340));
 auto len=scene::distance(axis,{});if(len>1e-5f)axis=axis*(1.f/len);
 auto offset=b[0x121F107]?w.scale**reinterpret_cast<float*>(b+0x121F11C):-.95f*bound[3];
 return trackingPoint(scene::Vec{bound[0],bound[1],bound[2]}+axis*offset);
}
inline void meshDiagnostics(std::ostream& out,void* root){
 auto b=build_adapter::heldModule();auto settings=build_adapter::liveReloadSettings();if(!b||!settings||!root)return;
 auto point=gripPoint(0);float reach=*reinterpret_cast<float*>(settings+0x28) * *reinterpret_cast<float*>(settings+0x32C);
 alignas(16) unsigned char context[0xD0]{};char why[300]{};int count=0;
 reinterpret_cast<void*(__thiscall*)(void*)>(b+0x136E40)(context);
 bool meshHit=reinterpret_cast<bool(__thiscall*)(void*,void*,const scene::Vec*,float,int*,char*,unsigned)>(b+0x1A92C0)(context,root,&point,reach,&count,why,sizeof(why));
 reinterpret_cast<void(__thiscall*)(void*)>(b+0x138220)(context);
 for(auto& ch:why){if(!ch)break;if(ch=='\\')ch='/';if(ch=='\"')ch='\'';if(ch<32)ch=' ';}
 why[sizeof(why)-1]=0;out<<",\"nativeMeshNear\":"<<(meshHit?1:0)<<",\"nativeMeshCount\":"<<count<<",\"nativeMeshReason\":\""<<why<<"\"";
}
inline void physicsDiagnostics(std::ostream& out,void* root){
 auto collision=root?scene::field<void*>(root,0x1C):nullptr;auto wrapper=collision?scene::field<void*>(collision,0x10):nullptr;auto body=wrapper?scene::field<void*>(wrapper,8):nullptr;
 out<<",\"physics\":{\"collision\":"<<(collision?1:0)<<",\"flags\":"<<(collision?scene::field<unsigned short>(collision,0xC):0)<<",\"body\":"<<(body?1:0);
 if(body&&scene::field<unsigned char>(body,0x28)==1){auto p=scene::field<scene::Vec>(body,0x120)*7;auto v=scene::field<scene::Vec>(body,0x1B0);out<<",\"inWorld\":"<<(scene::field<void*>(body,8)?1:0)<<",\"motion\":"<<unsigned(scene::field<unsigned char>(body,0xE8))<<",\"inverseMass\":"<<scene::field<float>(body,0x1AC)<<",\"position\":["<<p.x<<","<<p.y<<","<<p.z<<"],\"velocity\":["<<v.x<<","<<v.y<<","<<v.z<<"]";}
 out<<"}";
}
inline void contactDiagnostics(std::ostream& out){
 auto b=build_adapter::heldModule();if(!b)return;
 bool hand=reinterpret_cast<bool(__cdecl*)()>(b+0x25C610)();scene::Vec point{},direction{};
 bool valid=reinterpret_cast<bool(__cdecl*)(bool,scene::Vec*,scene::Vec*)>(b+0x1DBAF0)(hand,&point,&direction);
 auto t=trackingPoint(point);
 out<<",\"installedContact\":{\"hand\":"<<int(hand)<<",\"valid\":"<<(valid?1:0)<<",\"tracking\":["<<t.x<<","<<t.y<<","<<t.z<<"],\"looseRounds\":"<<int(b[0x121F000])<<",\"blocked\":"<<int(b[0x121F8FA])<<",\"simple\":"<<int(b[0x121F2A0])<<"}";
}
inline void diagnostics(std::ostream& out){gloveDiagnostics(out);insertionDiagnostics(out);contactDiagnostics(out);auto point=installedPoint();auto b=build_adapter::heldModule();auto settings=build_adapter::liveReloadSettings();out<<",\"nativeGrabReach\":"<<(settings?*reinterpret_cast<float*>(settings+0x28)**reinterpret_cast<float*>(settings+0x32C):0)<<",\"nativeReloadPriority\":"<<(b?unsigned(b[0x121F73A]):0)<<",\"nativeReloadActive\":"<<(b?unsigned(b[0x121EFB4]):0);out<<",\"installedPartTracking\":["<<point.x<<","<<point.y<<","<<point.z<<"]";out<<",\"physicalMags\":{\"active\":"<<(module?"true":"false")<<",\"attempts\":"<<attempts<<",\"successes\":"<<successes<<",\"calibration\":"<<calibration<<",\"pouchReturns\":"<<pouchReturns<<",\"failures\":"<<failures<<",\"distance\":"<<lastDistance<<",\"threshold\":"<<lastThreshold<<",\"installedDrops\":"<<installedDrops<<",\"ejectedDrops\":"<<ejectedDrops<<",\"emptyDrops\":"<<emptyDrops<<",\"recovered\":"<<recoveredParts<<",\"recoveredPouch\":"<<recoveredPouch<<",\"recoveredRounds\":"<<recoveredRounds<<",\"recoverStage\":"<<recoverStage<<",\"recoverDistance\":"<<recoverDistance<<",\"lastRef\":"<<lastRefID<<"}";}
}
