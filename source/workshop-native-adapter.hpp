#pragma once

#include "nvse_api.hpp"
#include "scene_adapter.hpp"

namespace workshop_native {

// FalloutNV 1.4.0.525 x86. TESObjectREFR and BSExtraData layouts are
// documented in JIP LN GameObjects.h and xNVSE GameBSExtraData.h.
inline Form* lookup(u32 id) {
    struct Entry { Entry* next; u32 key; Form* value; };
    struct Map { void* vtable; unsigned buckets; Entry** table; unsigned count; };
    const auto map = *reinterpret_cast<Map**>(0x11C54C0);
    if (!id || !map || !map->table || !map->buckets || map->buckets > 1000000) return nullptr;
    unsigned traversed = 0;
    for (auto entry = map->table[id % map->buckets]; entry && traversed++ < 4096; entry = entry->next)
        if (entry->key == id) return entry->value;
    return nullptr;
}

// Same BSStream ABI and signature check as the existing authored reload parts.
inline void* loadModel(const char* path) {
    constexpr unsigned char signature[]{0xC7,0x00,0x04,0x69,0x01,0x01};
    if (memcmp(reinterpret_cast<const void*>(0x43D008), signature, sizeof(signature))) return nullptr;
    alignas(16) unsigned char stream[0x5D4]{};
    reinterpret_cast<void(__thiscall*)(void*)>(0x43CFD0)(stream);
    auto vt = scene::field<void**>(stream, 0);
    const auto loaded = reinterpret_cast<bool(__thiscall*)(void*,const char*)>(vt[3])(stream,path);
    void* root = nullptr;
    auto entries = scene::field<void**>(stream,0x220);
    if (loaded && entries && scene::field<unsigned>(stream,0x228) == 1) {
        root = entries[0];
        scene::retain(root);
    }
    reinterpret_cast<void(__thiscall*)(void*,bool)>(vt[0])(stream,false);
    return root;
}

inline bool live(Form* ref) {
    return ref && (ref->type == 0x3A || ref->type == 0x3B || ref->type == 0x3C) &&
           !(ref->flags & 0x20) && scene::root(ref);
}

inline Form* base(Form* ref) {
    return live(ref) ? scene::field<Form*>(ref, 0x20) : nullptr;
}

inline void* extra(Form* ref, unsigned kind) {
    if (!live(ref)) return nullptr;
    void* entry = scene::field<void*>(ref, 0x48);
    for (unsigned i = 0; entry && i < 64; ++i) {
        if (scene::field<uint8_t>(entry, 4) == kind) return entry;
        entry = scene::field<void*>(entry, 8);
    }
    return nullptr;
}

inline float* condition(Form* ref) {
    auto model = base(ref);
    auto health = model && model->type == 0x28 ? extra(ref, 0x25) : nullptr;
    return health ? &scene::field<float>(health, 0x0C) : nullptr;
}

inline unsigned count(Form* ref) {
    const auto stack = extra(ref, 0x24);
    return stack ? unsigned(std::max<int>(1, scene::field<int16_t>(stack, 0x0C))) : 1;
}

} // namespace workshop_native
