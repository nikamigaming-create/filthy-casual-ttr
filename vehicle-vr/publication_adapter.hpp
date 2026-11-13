#pragma once
#include "../build_adapter.hpp"
#include "../native_pose_dispatch.hpp"
// Exact 7EB72860 host only. Native camera/pose publication is cdecl(void):
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
 if(memcmp(b+0x3F5893,flags,7))return false;
 if(b[0x3F5A7B]!=0xE8||b+0x3F5A80+*reinterpret_cast<int*>(b+0x3F5A7C)!=b+0x3FE510)return false;
 if(b[0x3F6AF5]!=0xE8||b+0x3F6AFA+*reinterpret_cast<int*>(b+0x3F6AF6)!=b+0x3FAFC0)return false;
 if(b[0x3F78C9]!=0xE8||b+0x3F78CE+*reinterpret_cast<int*>(b+0x3F78CA)!=b+0x2065D)return false;
 if(b[0x2065D]!=0xE9||b+0x20662+*reinterpret_cast<int*>(b+0x2065E)!=b+0x68B90)return false;
 if(b[0x3F7501]!=0xE8||b+0x3F7506+*reinterpret_cast<int*>(b+0x3F7502)!=b+0x1E902)return false;
 if(b[0x1E902]!=0xE9||b+0x1E907+*reinterpret_cast<int*>(b+0x1E903)!=b+0x15FEE0)return false;
 struct Site{unsigned rva,length;unsigned char opcode;void* target;DWORD old;};
 Site sites[]={{0x3F5A7B,5,0xE8,reinterpret_cast<void*>(&origin),0},{0x3F6AF5,5,0xE8,reinterpret_cast<void*>(&finalize),0},{0x3F78C9,5,0xE8,reinterpret_cast<void*>(&body),0},{0x3F7501,5,0xE8,reinterpret_cast<void*>(&contacts),0}};
 unsigned prepared=0;for(;prepared<4;prepared++)if(!VirtualProtect(b+sites[prepared].rva,sites[prepared].length,PAGE_EXECUTE_READWRITE,&sites[prepared].old))break;
 if(prepared!=4){for(int i=int(prepared)-1;i>=0;i--){DWORD unused;VirtualProtect(b+sites[i].rva,sites[i].length,sites[i].old,&unused);}return false;}
 subscribed=native_pose_dispatch::subscribe([](){++nesting;if(before)before();},[](bool completed){if(completed&&after)after();--nesting;});
 if(!subscribed){for(int i=3;i>=0;i--){DWORD unused;VirtualProtect(b+sites[i].rva,sites[i].length,sites[i].old,&unused);}return false;}
 originalContact=reinterpret_cast<Contact>(b+0x15FEE0);beforeContact=contactObserver;
 originalBody=reinterpret_cast<Body>(b+0x68B90);observeBody=observer;
 before=begin;after=finish;anchor=source;finalAnchor=completed;
 originalOrigin=reinterpret_cast<Origin>(b+0x3FE510);originalFinalize=reinterpret_cast<Finalize>(b+0x3FAFC0);
 for(auto& site:sites){b[site.rva]=site.opcode;*reinterpret_cast<int*>(b+site.rva+1)=int(reinterpret_cast<unsigned char*>(site.target)-(b+site.rva+5));if(site.length==6)b[site.rva+5]=0x90;FlushInstructionCache(GetCurrentProcess(),b+site.rva,site.length);}
 // Final-anchor and contact calls share a code page. Write all sites first,
 // then unwind protection changes in reverse so the original RX wins.
 for(int i=3;i>=0;i--){DWORD unused;VirtualProtect(b+sites[i].rva,sites[i].length,sites[i].old,&unused);}
 return true;
}
}
