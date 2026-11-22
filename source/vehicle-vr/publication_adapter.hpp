#pragma once
#include "../build_adapter.hpp"
#include "../native_pose_dispatch.hpp"
// Exact 119A085A host only. Native camera/pose publication is cdecl(void):
// RVA 3F52D0 has a six-byte relocatable prologue; 3F5893 reads player+680.
// No instruction is patched until both the file hash and byte guards match.
namespace vehicle_publication {
using Contact=void(__cdecl*)(void*,void*,bool,void*);
using ContactObserver=void(*)(void*);
inline Contact originalContact=nullptr;inline ContactObserver beforeContact=nullptr;
inline void __cdecl contacts(void* actor,void* opaque,bool enabled,void* extra){if(beforeContact)beforeContact(actor);originalContact(actor,opaque,enabled,extra);}
using Callback=void(*)();
using Origin=scene::Vec*(__cdecl*)(scene::Vec*,void*,const scene::Vec*);
using Finalize=void(__cdecl*)(void*,unsigned);
using FinalAnchor=void(*)(void*);
using Anchor=bool(*)(void*,scene::Vec&);
using Body=void(__cdecl*)(void*,void*,const void*);
using BodyObserver=void(*)(void*,void*,bool);
inline Body originalBody=nullptr;inline BodyObserver observeBody=nullptr;
inline void __cdecl body(void* actor,void* root,const void* head){
 if(observeBody)observeBody(actor,root,false);
 originalBody(actor,root,head);
 if(observeBody)observeBody(actor,root,true);
}
inline Callback before=nullptr,after=nullptr;
inline Origin originalOrigin=nullptr;inline Finalize originalFinalize=nullptr;inline FinalAnchor finalAnchor=nullptr;
inline Anchor anchor=nullptr;
inline scene::Vec* __cdecl origin(scene::Vec* out,void* actor,const scene::Vec* camera){
 auto result=originalOrigin(out,actor,camera);if(anchor)anchor(actor,*result);return result;
}
inline thread_local unsigned nesting=0;
inline void __cdecl finalize(void* manager,unsigned mode){
 originalFinalize(manager,mode);if(nesting==1&&finalAnchor)finalAnchor(manager);
}
inline bool install(Callback begin,Callback finish,Anchor source,FinalAnchor completed,BodyObserver observer,ContactObserver contactObserver){
 static bool subscribed=false;if(subscribed)return true;auto b=build_adapter::heldModule();if(!b)return false;
 const unsigned char flags[]={0x0F,0xB6,0x87,0x80,0x06,0x00,0x00};
 if(memcmp(b+0x4691D7,flags,7))return false;
 if(b[0x4693BF]!=0xE8||b+0x4693C4+*reinterpret_cast<int*>(b+0x4693C0)!=b+0x471F20)return false;
 if(b[0x46A439]!=0xE8||b+0x46A43E+*reinterpret_cast<int*>(b+0x46A43A)!=b+0x46E9D0)return false;
 if(b[0x46B28C]!=0xE8||b+0x46B291+*reinterpret_cast<int*>(b+0x46B28D)!=b+0x21512)return false;
 if(b[0x21512]!=0xE9||b+0x21517+*reinterpret_cast<int*>(b+0x21513)!=b+0x6B2F0)return false;
 if(b[0x46AE39]!=0xE8||b+0x46AE3E+*reinterpret_cast<int*>(b+0x46AE3A)!=b+0x1F6F9)return false;
 if(b[0x1F6F9]!=0xE9||b+0x1F6FE+*reinterpret_cast<int*>(b+0x1F6FA)!=b+0x168FF0)return false;
 struct Site{unsigned rva,length;unsigned char opcode;void* target;DWORD old;};
 Site sites[]={{0x4693BF,5,0xE8,reinterpret_cast<void*>(&origin),0},{0x46A439,5,0xE8,reinterpret_cast<void*>(&finalize),0},{0x46B28C,5,0xE8,reinterpret_cast<void*>(&body),0},{0x46AE39,5,0xE8,reinterpret_cast<void*>(&contacts),0}};
 unsigned prepared=0;for(;prepared<4;prepared++)if(!VirtualProtect(b+sites[prepared].rva,sites[prepared].length,PAGE_EXECUTE_READWRITE,&sites[prepared].old))break;
 if(prepared!=4){for(int i=int(prepared)-1;i>=0;i--){DWORD unused;VirtualProtect(b+sites[i].rva,sites[i].length,sites[i].old,&unused);}return false;}
 subscribed=native_pose_dispatch::subscribe([](){++nesting;if(before)before();},[](bool completed){if(completed&&after)after();--nesting;});
 if(!subscribed){for(int i=3;i>=0;i--){DWORD unused;VirtualProtect(b+sites[i].rva,sites[i].length,sites[i].old,&unused);}return false;}
 originalContact=reinterpret_cast<Contact>(b+0x168FF0);beforeContact=contactObserver;
 originalBody=reinterpret_cast<Body>(b+0x6B2F0);observeBody=observer;
 before=begin;after=finish;anchor=source;finalAnchor=completed;
 originalOrigin=reinterpret_cast<Origin>(b+0x471F20);originalFinalize=reinterpret_cast<Finalize>(b+0x46E9D0);
 for(auto& site:sites){b[site.rva]=site.opcode;*reinterpret_cast<int*>(b+site.rva+1)=int(reinterpret_cast<unsigned char*>(site.target)-(b+site.rva+5));if(site.length==6)b[site.rva+5]=0x90;FlushInstructionCache(GetCurrentProcess(),b+site.rva,site.length);}
 // Final-anchor and contact calls share a code page. Write all sites first,
 // then unwind protection changes in reverse so the original RX wins.
 for(int i=3;i>=0;i--){DWORD unused;VirtualProtect(b+sites[i].rva,sites[i].length,sites[i].old,&unused);}
 return true;
}
}
