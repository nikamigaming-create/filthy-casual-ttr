#pragma once
#include <cstdint>

namespace fc_equipment {
struct Controllers { char model[2][48]{};uint64_t assigned{}; };
using ReadControllers=bool (__cdecl*)(Controllers*,unsigned);
enum class Kind : unsigned { None, Holsters, WeaponWheel, AidWheel };
struct Slot {
    float right{}, up{};
    uint32_t form{};
};
struct LayoutView {
    Kind kind{};
    unsigned count{};
    int focus=-1;
    Slot slots[8]{};
};
// Negative hand/profile requests the current wheel editor, including its unsaved draft.
using ReadWheel = bool (__cdecl*)(int,int,LayoutView*,unsigned);
using ReadHolsters = bool (__cdecl*)(unsigned,LayoutView*,unsigned);
}
