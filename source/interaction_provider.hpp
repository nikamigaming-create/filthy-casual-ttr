#pragma once
#include <cstdint>
// Process-lifetime descriptors. Callbacks run on the XR thread: cache inputs
// only; perform game commands on the provider's native MainLoop callback.
namespace fc_interaction {
constexpr uint32_t version=1;
struct Ray {uint32_t size,valid;float origin[3],direction[3];int64_t time;};
struct Provider {uint32_t size,abi;
 uint32_t (__cdecl* input)(const char*,int,int,float,float);
 void (__cdecl* aim)(int,const Ray*);
};
using Query=const Provider* (__cdecl*)(); // FC_InteractionProviderV1
}
