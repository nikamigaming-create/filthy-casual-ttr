#pragma once
#include "../build_adapter.hpp"
namespace vehicle_body {
inline bool verified(){auto base=build_adapter::heldModule();const unsigned char bytes[]={0x83,0xEC,0x64,0xFF,0x74,0x24,0x68,0xE8,0x24,0xFD,0xFF,0xFF,0x8B,0xC8,0x83,0xC4};return base&&!memcmp(base+0x3EB1E0,bytes,sizeof bytes);}
// Same remove-before-local-write path used by native skeleton restoration at
// 920E5. Remove only the mounted lower branch, never clear global pose overrides.
inline void releaseWorldOverride(void* node){auto base=build_adapter::heldModule();reinterpret_cast<void(__cdecl*)(void*)>(base+0x3EB1E0)(node);}
}
