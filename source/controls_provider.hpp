#pragma once
#include <cstdint>
// Stable C ABI. Providers own immutable strings; callbacks read atomics only.
// The host never loads a DLL to discover controls: only already-loaded modules.
namespace fc_provider {
constexpr uint32_t version=1,channels=26;
struct Mode {const char* id;const char* title;const char* description;
 const char* labels[channels]; // nullptr means this destination is unused
};
struct Provider {uint32_t size,abi,count;const Mode* modes;
 uint32_t (__cdecl* available)(); // bit per declared mode, even while unmounted
 int (__cdecl* current)(); // mode index, -1 when inactive
};
using Query=const Provider* (__cdecl*)(); // export FC_ControlsProviderV1
}
