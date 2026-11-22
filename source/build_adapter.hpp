#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstring>
#include <vector>
#pragma comment(lib,"bcrypt.lib")
// Exact TTR 1.1.7 adapters. Unknown builds keep their original behavior.
namespace build_adapter {
inline void freeGameString(void* p) {
 // FalloutNV 1.4.0.525, runtime 040020D0; xNVSE GameAPI.cpp FormHeap_Free.
 reinterpret_cast<void(__cdecl*)(void*)>(0x401030)(p);
}
inline bool verifiedTTWVR(HMODULE m) {
 wchar_t path[MAX_PATH];if(!GetModuleFileNameW(m,path,MAX_PATH))return false;
 FILE* f=nullptr;if(_wfopen_s(&f,path,L"rb"))return false;
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
 DWORD objectSize=0,got=0;unsigned char digest[32]{};
 bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
 if(ok)ok=BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,(PUCHAR)&objectSize,sizeof(objectSize),&got,0)>=0;
 std::vector<unsigned char> object(objectSize),buf(65536);
 if(ok)ok=BCryptCreateHash(alg,&hash,object.data(),objectSize,nullptr,0,0)>=0;
 size_t n;while(ok&&(n=fread(buf.data(),1,buf.size(),f))>0)ok=BCryptHashData(hash,buf.data(),static_cast<ULONG>(n),0)>=0;
 if(ferror(f))ok=false;fclose(f);
 if(ok)ok=BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
 if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);
 const unsigned char expected[32]={0x11,0x9A,0x08,0x5A,0x5B,0x01,0x01,0xF0,0x61,0x8B,0x8E,0x07,0x6A,0x42,0x1B,0x4B,0x89,0xE3,0x0D,0x72,0xD9,0xB6,0xEA,0xE8,0x74,0xF4,0xE5,0xAF,0x7B,0xD3,0xFB,0x27};
 return ok&&memcmp(digest,expected,32)==0;
}
inline const char* testDisableUpscaler() {
 char flag[8]{};if(!GetEnvironmentVariableA("SC_TEST_NO_UPSCALER",flag,sizeof(flag))||strcmp(flag,"1"))return "not requested";
 auto m=GetModuleHandleW(L"TTWVR.dll");if(!m||!verifiedTTWVR(m))return "refused build mismatch";
 struct Entry{const char* section;const char* key;const char* value;};
 auto e=reinterpret_cast<Entry*>(reinterpret_cast<unsigned char*>(m)+0x8FA18C);
 if(strcmp(e->section,"DLSS")||strcmp(e->key,"bEnabled")||strcmp(e->value,"1"))return "refused table mismatch";
 static const char off[]="0";DWORD old=0;if(!VirtualProtect(&e->value,4,PAGE_READWRITE,&old))return "protection failed";
 InterlockedExchangePointer((void* volatile*)(void*)&e->value,const_cast<char*>(off));DWORD unused;VirtualProtect(&e->value,4,old,&unused);
 return "DLSS bEnabled=0 (test only)";
}
inline const char* requestHandDraw(int hand=1) {
 auto m=GetModuleHandleW(L"TTWVR.dll");
 static HMODULE verified=nullptr;
 if(!m)return "refused module absent";
 if(verified!=m){if(!verifiedTTWVR(m))return "refused build mismatch";verified=m;}
 auto b=reinterpret_cast<unsigned char*>(m);
 // Verified original holster draw transition, RVA 1ED055..1ED0A6:
 // clear the released-weapon latch, choose the requested hand convention,
 // set draw timestamp/pending hand, and reset release timeout.
 *reinterpret_cast<void**>(b+0x1614B3C)=nullptr;
 reinterpret_cast<void(__cdecl*)(bool,const char*)>(b+0x219940)(hand==0,"SeatedControls selection");
 *reinterpret_cast<ULONGLONG*>(b+0x1614B48)=GetTickCount64();
 *reinterpret_cast<int*>(b+0x1038D74)=hand;
 *reinterpret_cast<unsigned char*>(b+0x1614B29)=0;
 *reinterpret_cast<float*>(b+0x1614B58)=0;
 *reinterpret_cast<void**>(b+0x1614B68)=nullptr; // Native default draw transition202894.
 return "hand draw armed";
}
// Native held-item entry points for the exact hash above. These execute on the
// game's main thread. The selector does not clone aid meshes or consume items.
inline unsigned char* heldModule() {
 auto m=GetModuleHandleW(L"TTWVR.dll");static HMODULE verified=nullptr;
 if(!m)return nullptr;if(verified!=m){if(!verifiedTTWVR(m))return nullptr;verified=m;}
 return reinterpret_cast<unsigned char*>(m);
}
inline bool temporalEyeCopyActive() {
 auto b=heldModule();if(!b)return false;
 // TTR 1.1.7: this byte is set after the first successful temporal eye
 // copy and cleared by the native upscaler setup/reset function.
 const unsigned char copied[]={0xc6,0x05};
 if(memcmp(b+0x5AF67B,copied,sizeof(copied))||b[0x5AF681]!=1||
    *reinterpret_cast<unsigned char**>(b+0x5AF67D)!=b+0x1B771B0)return false;
 return b[0x1B771B0]!=0;
}
inline void* grabbedItem() {
 auto b=heldModule();if(!b)return nullptr;
 if(b[0x15388D4]&&*reinterpret_cast<int*>(b+0x15388D8)==1)return *reinterpret_cast<void**>(b+0x15388DC);
 return b[0x1538805]?*reinterpret_cast<void**>(b+0x15395E0):nullptr;
}
inline void ignoreSyntheticBookSlotHold(int hand) {
 // A wheel-drawn weapon must remain squeezed until the real hand takes over.
 // In TTR 1.1.7 the same action advances a per-hand holster edit timer. Only
 // discard elapsed time from our synthetic book hold, before a drag has begun.
 // A real press starts its own native timer; the other hand is independent.
 auto b=heldModule();if(!b||unsigned(hand)>1)return;
 const unsigned char storeElapsed[]={0xf3,0x0f,0x11,0x47,0xf8};
 if(memcmp(b+0x224f38,storeElapsed,sizeof storeElapsed))return;
 auto slot=b+0x1038e4c+16*hand;
 if(*reinterpret_cast<int*>(slot)>=0&&!slot[8])*reinterpret_cast<float*>(slot+4)=0.f;
}
inline int grabbedHand() {auto b=heldModule();if(!b)return -1;if(b[0x15388D4]&&*reinterpret_cast<int*>(b+0x15388D8)==1)return 1;return b[0x1538805]?*reinterpret_cast<int*>(b+0x1035E60):-1;}
inline int stimHand() {auto b=heldModule();return b?*reinterpret_cast<int*>(b+0x1041574):-1;}
inline int drawnWeaponHand() {
 auto b=heldModule();if(!b)return -1;
 // 209E80 excludes the offhand stored at 15F51D6 from primary ownership.
 const unsigned char code[]={0x8A,0x44,0x24,0x04,0x3A,0x05};
 if(memcmp(b+0x2111D2,code,sizeof(code))||*reinterpret_cast<unsigned char**>(b+0x2111D8)!=b+0x1611D36)return -1;
 int offhand=b[0x1611D36];return offhand<2?1-offhand:-1;
}
inline void releaseGrab() {if(auto b=heldModule())reinterpret_cast<void(__cdecl*)(const char*)>(b+0x1C8DD0)("selector replacement");}
inline void clearStim(void* player) {if(auto b=heldModule())reinterpret_cast<void(__cdecl*)(void*,const char*)>(b+0x44CB70)(player,"selector replacement");}
inline bool stowSelectedGrab(void* player,void* wanted){
 auto b=heldModule();if(!b||!player||!wanted)return false;
 void* first=b[0x1538805]?*reinterpret_cast<void**>(b+0x15395E0):nullptr;
 void* second=b[0x15388D4]?*reinterpret_cast<void**>(b+0x15388DC):nullptr;
 bool primary=first==wanted,secondary=second==wanted;
 if(!primary&&!secondary)return false;
 // Native IsSameGrabbedObject (194E00) recognizes a legitimate two-hand hold.
 // 197479..1974A3 releases both aliases before activating that reference once.
 bool shared=primary&&secondary;
 // The optional slot context may be null in both aliases; native equality permits it.
 if(shared&&*reinterpret_cast<void**>(b+0x15395E4)!=*reinterpret_cast<void**>(b+0x15388E0))return false;
 // Exact native release-all routine 1930E0 uses this swap/release/swap sequence
 // to address slot two. Use it for the selected slot only, preserving the other.
 const unsigned char swapCode[]={0x81,0xEC,0xCC,0,0,0,0x56,0x57};
 if(memcmp(b+0x1CC310,swapCode,sizeof(swapCode)))return false;
 int otherHand=*reinterpret_cast<int*>(b+0x1035E60);
 int hand=*reinterpret_cast<int*>(b+(secondary?0x15388D8:0x1035E60));
 auto swap=reinterpret_cast<void(__cdecl*)()>(b+0x1CC310);
 auto release=reinterpret_cast<void(__cdecl*)(const char*)>(b+0x1C8DD0);
 if(shared){release("sticky shared item stow");swap();release("sticky shared item stow");swap();}
 else {if(secondary)swap();release("sticky item stow");if(secondary)swap();}
 auto cancelPull=reinterpret_cast<void(__cdecl*)(int,const char*)>(b+0x1BD270);
 cancelPull(hand,"sticky item stow");
 if(shared&&otherHand!=hand)cancelPull(otherHand,"sticky shared item stow");
 if((b[0x1538805]&&*reinterpret_cast<void**>(b+0x15395E0)==wanted)||(b[0x15388D4]&&*reinterpret_cast<void**>(b+0x15388DC)==wanted))return false;
 reinterpret_cast<void(__thiscall*)(void*,void*,int,int,int)>(0x573170)(wanted,player,0,0,1);
 return true;
}
inline void stowGrab(void* player) {
 auto b=heldModule();if(!b||!player)return;
 void* first=b[0x1538805]?*reinterpret_cast<void**>(b+0x15395E0):nullptr;
 void* second=b[0x15388D4]?*reinterpret_cast<void**>(b+0x15388DC):nullptr;
 // The native dual-hand cleanup swaps/releases both slots and cancels pulls.
 // Releasing only the primary slot can leave an already-stowed ref in slot two.
 reinterpret_cast<void(__cdecl*)(const char*)>(b+0x1C8D80)("selector replacement");
 // Same TESObjectREFR::Activate call used by TTWVR's backpack at RVA 19C4DD.
 if(first)reinterpret_cast<void(__thiscall*)(void*,void*,int,int,int)>(0x573170)(first,player,0,0,1);
 if(second&&second!=first)reinterpret_cast<void(__thiscall*)(void*,void*,int,int,int)>(0x573170)(second,player,0,0,1);
}
inline int grabSelected(void* player,const float* head,void* item,int hand=1) {
 auto b=heldModule();if(!b||!player||!head||!item)return 0;
 // cdecl(player, world head position, right-hand bool, required reference).
 // Required reference restricts native proximity picking to the selected item.
 return reinterpret_cast<int(__cdecl*)(void*,const float*,bool,void*)>(b+0x1D1C40)(player,head,hand==1,item);
}
inline void* dropOneAid(void* player,void* form,void* extra) {
 if(!player||!form)return nullptr;
 // TESObjectREFR::RemoveItem virtual 0x17C, JIP LN GameObjects.h. Returns the
 // actual dropped reference; GetDroppedRefs is not populated for player aid.
 using Drop=void*(__thiscall*)(void*,void*,void*,unsigned,bool,bool,void*,unsigned,unsigned,bool,bool);
 auto vt=*reinterpret_cast<void***>(player);
 return reinterpret_cast<Drop>(vt[0x17C/4])(player,form,extra,1,true,true,nullptr,0,0,true,false);
}
inline bool selectStim(void* player) {
 auto b=heldModule();if(!b||!player)return false;
 if(reinterpret_cast<int(__cdecl*)(void*)>(b+0x9AF2)(player)<=0)return false;
 auto bone=reinterpret_cast<void*(__cdecl*)(void*,int)>(b+0x44BD20)(player,1);
 if(!bone||!reinterpret_cast<bool(__cdecl*)(void*,int)>(b+0x44BEA0)(bone,1))return false;
 // Reproduce only the successful native neck-selection transition (316641).
 *reinterpret_cast<ULONGLONG*>(b+0x1A08590)=GetTickCount64();
 *reinterpret_cast<int*>(b+0x1041574)=1;b[0x1A08586]=0;b[0x1A08587]=0;b[0x1041578]=1;
 *reinterpret_cast<int*>(b+0x1A088B4)=0;b[0x1A0858A]=0;b[0x1A0858B]=0; // New native stim reset3DA67D..3DA68E.
 *reinterpret_cast<int*>(b+0x1A08898)=0;*reinterpret_cast<void**>(b+0x1A0889C)=nullptr;
 *reinterpret_cast<int*>(b+0x1A08990)=0;*reinterpret_cast<void**>(b+0x1A08994)=nullptr;b[0x1A0858C]=0;
 reinterpret_cast<void(__cdecl*)(bool,const void*,int)>(b+0xF0C4)(true,b+0xAC9BE0,0);
 return true;
}
}
