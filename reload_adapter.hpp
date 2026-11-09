#pragma once
#include "build_adapter.hpp"

// Private-build native reload settings adapter. All entry points must run on
// the game's main thread. Static contract only; runtime switching still needs
// acceptance with a weapon, magazine and both reload modes.
namespace build_adapter {
inline unsigned char* liveReloadSettings() {
    auto b = heldModule();
    if (!b || !b[0x121EF58]) return nullptr;
    // Exact-hash startup: RVA 1F095B unconditionally calls 859E -> 25F2C0.
    // Successful native reload-hook installation sets 121EF58 at 25F55D.
    // Getter 8F7B -> 92290 is cdecl/no args; its initialized return is
    // module+E77408 (922CE). Calling it preserves native initialization.
    auto settings = reinterpret_cast<unsigned char*(__cdecl*)()>(b + 0x8F7B)();
    return settings == b + 0xE77408 ? settings : nullptr;
}
inline bool readManualReload(bool& enabled) {
    auto settings = liveReloadSettings();
    if (!settings || settings[0x494] > 1) return false;
    enabled = settings[0x494] != 0;
    return true;
}
inline bool setManualReload(bool enabled) {
    auto settings = liveReloadSettings();
    if (!settings || settings[0x494] > 1) return false;
    // INI load 96445..9645D writes this same one-byte bManual cache.
    // Native reload frame 280B25 and hooks 25E629 etc read it live.
    // This does not install hooks, change inventory/ammunition, persist a
    // setting or synthesize native magazine/chamber state.
    *reinterpret_cast<volatile unsigned char*>(settings + 0x494) = enabled ? 1 : 0;
    return true;
}
}

#include <cmath>
namespace build_adapter {
// Native pouch placement is character-relative until the native routine binds
// its world transform to a torso bone. We only seed that normal placement path.
// Values are metres: forward, right (+), below eyes, and fallback above feet.
// Initializes the native SavedPouch reader if needed, then changes placement only
// when the preference changes. No SavedPouch writer is called by this adapter.
inline bool setSeatedPouch(bool enabled, float forward = 0.28f,
                           float side = -0.20f, float belowEye = 0.55f,
                           float height = 0.80f) {
    auto b = heldModule();
    auto settings = liveReloadSettings();
    if (!b || !settings) return false;
    struct Snapshot {
        unsigned char* module = nullptr;
        bool active = false;
        unsigned char spareEnabled = 0;
        float settings[4]{};
        unsigned char native[0x4C]{};
        float applied[4]{};
    };
    static Snapshot original;
    if (original.active && original.module != b) return false;
    if (!enabled) {
        if (!original.active) return true;
        settings[0x4B4] = original.spareEnabled;
        memcpy(settings + 0x4B8, original.settings, sizeof(original.settings));
        memcpy(b + 0x121F1B0, original.native, sizeof(original.native));
        original.active = false;
        return true;
    }
    if (!std::isfinite(forward) || forward < 0.10f || forward > 0.70f ||
        !std::isfinite(side) || side < -0.60f || side > 0.60f ||
        !std::isfinite(belowEye) || belowEye < 0.20f || belowEye > 1.20f ||
        !std::isfinite(height) || height < 0.20f || height > 1.40f) return false;
    // Original call at 257549 passes no arguments; reader 263F30 preserves
    // registers and ends with plain RET at 2646F6: cdecl, no args. It reads
    // SavedPouch and sets loaded=1 even if no file exists. Run it before the
    // snapshot so an original bSpare=false cannot prevent initialization, and
    // the user's parsed native placement remains available for restoration.
    if (b[0x121F1B0] > 1) return false;
    if (!b[0x121F1B0]) reinterpret_cast<void(__cdecl*)()>(b + 0x263F30)();
    if (b[0x121F1B0] != 1 || b[0x121F1B1] > 1 || b[0x121F1C4] > 1 ||
        settings[0x4B4] > 1) return false;
    const float placement[4] = {forward, side, height, belowEye};
    if (original.active && !memcmp(original.applied, placement, sizeof(placement)))
        return true;
    if (!original.active) {
        original.module = b;
        original.spareEnabled = settings[0x4B4];
        memcpy(original.settings, settings + 0x4B8, sizeof(original.settings));
        memcpy(original.native, b + 0x121F1B0, sizeof(original.native));
        original.active = true;
    }
    settings[0x4B4] = 1; // native Reload/bSpare creation gate at 27838B
    memcpy(settings + 0x4B8, placement, sizeof(placement));
    memcpy(original.applied, placement, sizeof(placement));
    // 2602F0 uses fChest* when saved-offset override is false. 257556 builds a
    // new native torso anchor when its valid byte is false. 2575E8..25762B only
    // automatically writes SavedPouch when the saved-offset override is true.
    // Leave the previous transform bytes inert until native code replaces them;
    // do not manufacture bone indices or transforms, or write the SavedPouch file.
    b[0x121F1B1] = 0;
    b[0x121F1C4] = 0;
    return true;
}
}

namespace build_adapter {
struct PouchNodeDiagnostics {
    void* node = nullptr;
    void* parent = nullptr;
    bool readable = false;
    bool culled = true;
    float world[3]{};
    float scale = 0;
};
struct PouchDiagnostics {
    void* primaryNode = nullptr;
    void* copyNode = nullptr;
    void* weaponRoot = nullptr;
    bool nativeInitialized = false;
    bool spareCenterValid = false;
    float spareLocalCenter[3]{};
    float spareLocalRadius = 0;
    bool spareEnabled = false;
    bool settingsLoaded = false;
    bool savedOverride = false;
    bool torsoAnchor = false;
    bool primaryShown = false;
    int nativeMode = -1;
    float metresToUnits = 0;
    float forward = 0, side = 0, height = 0, belowEye = 0;
    PouchNodeDiagnostics primary, copy;
};
inline PouchNodeDiagnostics readPouchNode(void* node) {
    PouchNodeDiagnostics out; out.node = node;
    if (!node) return out;
    MEMORY_BASIC_INFORMATION region{};
    if (!VirtualQuery(node, &region, sizeof(region)) || region.State != MEM_COMMIT ||
        (region.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return out;
    const auto protection = region.Protect & 0xff;
    if (protection != PAGE_READONLY && protection != PAGE_READWRITE &&
        protection != PAGE_WRITECOPY && protection != PAGE_EXECUTE_READ &&
        protection != PAGE_EXECUTE_READWRITE && protection != PAGE_EXECUTE_WRITECOPY) return out;
    const auto begin = reinterpret_cast<ULONG_PTR>(node);
    const auto end = reinterpret_cast<ULONG_PTR>(region.BaseAddress) + region.RegionSize;
    if (end < begin || end - begin < 0x9C) return out;
    auto n = static_cast<unsigned char*>(node);
    out.parent = *reinterpret_cast<void**>(n + 0x18);
    out.culled = (*reinterpret_cast<unsigned*>(n + 0x30) & 1) != 0;
    memcpy(out.world, n + 0x8C, sizeof(out.world));
    out.scale = *reinterpret_cast<float*>(n + 0x98);
    out.readable = std::isfinite(out.world[0]) && std::isfinite(out.world[1]) &&
        std::isfinite(out.world[2]) && std::isfinite(out.scale);
    if (!out.readable) { for (float& v : out.world) v = 0; out.scale = 0; }
    return out;
}
inline bool readPouchDiagnostics(PouchDiagnostics& out) {
    out = {};
    auto b = heldModule(); auto settings = liveReloadSettings();
    if (!b || !settings) return false;
    out.primaryNode = *reinterpret_cast<void**>(b + 0x121F180);
    out.copyNode = *reinterpret_cast<void**>(b + 0x121F194);
    out.weaponRoot = *reinterpret_cast<void**>(b + 0x121EF8C);
    out.nativeInitialized = b[0x121EFB4] != 0;
    out.spareCenterValid = b[0x121F17D] != 0;
    memcpy(out.spareLocalCenter, b + 0x121F184, sizeof(out.spareLocalCenter));
    out.spareLocalRadius = *reinterpret_cast<float*>(b + 0x121F190);
    out.spareEnabled = settings[0x4B4] != 0;
    out.settingsLoaded = b[0x121F1B0] != 0;
    out.savedOverride = b[0x121F1B1] != 0;
    out.torsoAnchor = b[0x121F1C4] != 0;
    out.primaryShown = b[0x121F17E] != 0;
    out.nativeMode = *reinterpret_cast<int*>(b + 0x121F260);
    out.metresToUnits = *reinterpret_cast<float*>(settings + 0x28);
    out.forward = *reinterpret_cast<float*>(settings + 0x4B8);
    out.side = *reinterpret_cast<float*>(settings + 0x4BC);
    out.height = *reinterpret_cast<float*>(settings + 0x4C0);
    out.belowEye = *reinterpret_cast<float*>(settings + 0x4C4);
    out.primary = readPouchNode(out.primaryNode);
    out.copy = readPouchNode(out.copyNode);
    // Diagnostic values are serialized as JSON numbers. Invalid native data
    // must not produce NaN/Infinity tokens or an invented valid target center.
    for (float& v : out.spareLocalCenter) if (!std::isfinite(v)) {
        v = 0; out.spareCenterValid = false;
    }
    if (!std::isfinite(out.spareLocalRadius)) { out.spareLocalRadius = 0; out.spareCenterValid = false; }
    float* finiteFields[] = {&out.metresToUnits, &out.forward, &out.side, &out.height, &out.belowEye};
    for (float* v : finiteFields) if (!std::isfinite(*v)) *v = 0;
    // These are native state/cull facts, never a claim of final-eye visibility.
    return true;
}
}
