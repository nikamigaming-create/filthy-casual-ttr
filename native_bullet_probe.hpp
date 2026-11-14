#pragma once
#include "glass_decal_adapter.hpp"
// Private opt-in diagnostic: captures native working bullet decal arguments.
namespace native_bullet_probe {
using D=glass_decal_adapter::Decal;
using Call=void(__thiscall*)(void*,D*,unsigned,bool);
inline void* replacement=nullptr;inline Call original=nullptr;inline void(*report)(const D&,unsigned,bool)=nullptr;
inline void __fastcall tap(void* m,void*,D* d,unsigned type,bool ignore){if(report&&d)report(*d,type,ignore);if(replacement&&d&&type==1){d->textureSet=replacement;d->quadrant=4;}auto oldTail=scene::field<void*>(m,0xC);original(m,d,type,ignore);
 if(replacement){auto entry=oldTail?scene::field<void*>(oldTail,0):scene::field<void*>(m,8);for(unsigned n=0;entry&&n<16;n++,entry=scene::field<void*>(entry,0)){auto e=scene::field<void*>(entry,8);if(e){scene::retain(e);glass_decal_adapter::probes.push_back({e,0});}}}
}
inline bool install(void(*callback)(const D&,unsigned,bool)){
 if(original){report=callback;return true;}if(!glass_decal_adapter::supported())return false;
 auto stub=static_cast<unsigned char*>(VirtualAlloc(nullptr,16,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(!stub)return false;
 auto address=reinterpret_cast<unsigned char*>(0x4A10D0);memcpy(stub,address,5);stub[5]=0xE9;*reinterpret_cast<int*>(stub+6)=int(address+5-stub-10);
 DWORD old;if(!VirtualProtect(address,5,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(stub,0,MEM_RELEASE);return false;}
 original=reinterpret_cast<Call>(stub);report=callback;address[0]=0xE9;*reinterpret_cast<int*>(address+1)=int(reinterpret_cast<unsigned char*>(tap)-address-5);VirtualProtect(address,5,old,&old);FlushInstructionCache(GetCurrentProcess(),address,5);return true;
}
}
