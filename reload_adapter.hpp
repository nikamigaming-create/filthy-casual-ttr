#pragma once
#include "build_adapter.hpp"

// Exact 7EB72860 native reload settings adapter (INI loader A18B8/A1B29). All entry points must run on
// the game's main thread. Static contract only; runtime switching still needs
// acceptance with a weapon, magazine and both reload modes.
namespace build_adapter {
inline unsigned char* liveReloadSettings() {
    auto b = heldModule();
    if (!b || !b[0x18B3C10]) return nullptr;
    // 7EB72860: native install success sets18B3C10 at32326D.
    // ADC1 ->9D5E0 is cdecl/no args;9D61E returns101FA88.
    auto settings = reinterpret_cast<unsigned char*(__cdecl*)()>(b + 0xADC1)();
    return settings == b + 0x101FA88 ? settings : nullptr;
}
inline bool readManualReload(bool& enabled) {
    auto settings = liveReloadSettings();
    if (!settings || settings[0x49C] > 1) return false;
    enabled = settings[0x49C] != 0;
    return true;
}
inline bool setManualReload(bool enabled) {
    auto settings = liveReloadSettings();
    if (!settings || settings[0x49C] > 1) return false;
    // Native INI loader A18B8 writes this same one-byte bManual cache.
    // Native reload frame 280B25 and hooks 25E629 etc read it live.
    // This does not install hooks, change inventory/ammunition, persist a
    // setting or synthesize native magazine/chamber state.
    *reinterpret_cast<volatile unsigned char*>(settings + 0x49C) = enabled ? 1 : 0;
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
        float applied[4]{};
    };
    static Snapshot original;
    if (original.active && original.module != b) return false;
    if (!enabled) {
        if (!original.active) return true;
        settings[0x4BC] = original.spareEnabled;
        memcpy(settings + 0x4C0, original.settings, sizeof(original.settings));
        // Kupo owns calibrated placement. Never restore a stale snapshot over
        // a position the player moved/saved while our seated mode was active.
        if (!b[0x18B3EB1]) b[0x18B3EC4] = 0;
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
    if (b[0x18B3EB0] > 1) return false;
    if (!b[0x18B3EB0]) reinterpret_cast<void(__cdecl*)()>(b + 0x327C40)();
    if (b[0x18B3EB0] != 1 || b[0x18B3EB1] > 1 || b[0x18B3EC4] > 1 ||
        settings[0x4BC] > 1) return false;
    const float placement[4] = {forward, side, height, belowEye};
    if (original.active && !memcmp(original.applied, placement, sizeof(placement)))
        return true;
    if (!original.active) {
        original.module = b;
        original.spareEnabled = settings[0x4BC];
        memcpy(original.settings, settings + 0x4C0, sizeof(original.settings));
        original.active = true;
    }
    settings[0x4BC] = 1; // native Reload/bSpare creation gate at 27838B
    memcpy(settings + 0x4C0, placement, sizeof(placement));
    memcpy(original.applied, placement, sizeof(placement));
    // 2602F0 uses fChest* when saved-offset override is false. 257556 builds a
    // new native torso anchor when its valid byte is false. 2575E8..25762B only
    // automatically writes SavedPouch when the saved-offset override is true.
    // Leave the previous transform bytes inert until native code replaces them;
    // do not manufacture bone indices or transforms, or write the SavedPouch file.
    // Saved native placement wins on startup and every mode toggle. Seated
    // values are a fallback for players who have not placed their own pouch.
    if (!b[0x18B3EB1]) b[0x18B3EC4] = 0;
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
    bool boundValid = false;
    float bound[4]{};
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
    int heldRounds = 0;
    int detachedMode = -1, detachedRounds = 0;
    int installedMode = -1, installedRounds = 0;
    bool discardAmmo = false;
    float flightAge = 0;
    float flightPosition[3]{}, flightVelocity[3]{};
    float metresToUnits = 0;
    float forward = 0, side = 0, height = 0, belowEye = 0;
    // Read-only TTR1.1 loose-round insertion facts (311E8B..312DCD).
    bool roundTargetValid = false;
    float roundTargetLocal[3]{};
    unsigned char roundFlags[5]{}; // 8A1,8A3,85B,88E,88F; no inferred labels.
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
    // NiAVObject+20 is a pointer to NiBound, not an inline center.
    // Native insertion uses this center when within25cm of the reload hand.
    auto bound = *reinterpret_cast<const float**>(n + 0x20);
    MEMORY_BASIC_INFORMATION br{};
    if (out.readable && bound && VirtualQuery(bound,&br,sizeof(br)) && br.State==MEM_COMMIT &&
        !(br.Protect&(PAGE_GUARD|PAGE_NOACCESS))) {
        auto bp=br.Protect&0xff;
        auto at=reinterpret_cast<ULONG_PTR>(bound), endBound=reinterpret_cast<ULONG_PTR>(br.BaseAddress)+br.RegionSize;
        if ((bp==PAGE_READONLY||bp==PAGE_READWRITE||bp==PAGE_WRITECOPY||bp==PAGE_EXECUTE_READ||bp==PAGE_EXECUTE_READWRITE||bp==PAGE_EXECUTE_WRITECOPY) && endBound>=at && endBound-at>=sizeof(out.bound)) {
            float value[4];memcpy(value,bound,sizeof(value));
            bool sane=std::isfinite(value[3])&&value[3]>0&&value[3]<=100000;
            for(int i=0;i<3;i++)sane=sane&&std::isfinite(value[i])&&value[i]>-1e9f&&value[i]<1e9f;
            if(sane){memcpy(out.bound,value,sizeof(value));out.boundValid=true;}
        }
    }
    if (!out.readable) { for (float& v : out.world) v = 0; out.scale = 0; }
    return out;
}
inline bool readPouchDiagnostics(PouchDiagnostics& out) {
    out = {};
    auto b = heldModule(); auto settings = liveReloadSettings();
    if (!b || !settings) return false;
    out.primaryNode = *reinterpret_cast<void**>(b + 0x18B3E80);
    out.copyNode = *reinterpret_cast<void**>(b + 0x18B3E94);
    out.weaponRoot = *reinterpret_cast<void**>(b + 0x18B3C90);
    out.nativeInitialized = b[0x18B3C4D] != 0;
    out.spareCenterValid = b[0x18B3E72] != 0;
    memcpy(out.spareLocalCenter, b + 0x18B3E84, sizeof(out.spareLocalCenter));
    out.spareLocalRadius = *reinterpret_cast<float*>(b + 0x18B3E90);
    out.spareEnabled = settings[0x4BC] != 0;
    out.settingsLoaded = b[0x18B3EB0] != 0;
    out.savedOverride = b[0x18B3EB1] != 0;
    out.torsoAnchor = b[0x18B3EC4] != 0;
    out.primaryShown = b[0x18B3E73] != 0;
    out.nativeMode = *reinterpret_cast<int*>(b + 0x18B3F60);
    // Read-only native lifecycle facts. This build owns dropped magazine
    // world references, Havok placement and save restoration itself.
    out.heldRounds = *reinterpret_cast<int*>(b + 0x18B3F64);
    out.detachedMode = *reinterpret_cast<int*>(b + 0x18B3D14);
    out.detachedRounds = *reinterpret_cast<int*>(b + 0x18B3D18);
    out.installedMode = *reinterpret_cast<int*>(b + 0x18B3E3C);
    out.installedRounds = *reinterpret_cast<int*>(b + 0x18B3E40);
    out.discardAmmo = settings[0x49D] != 0;
    out.flightAge = *reinterpret_cast<float*>(b + 0x18B3D7C);
    memcpy(out.flightPosition, b + 0x18B3D58, sizeof(out.flightPosition));
    memcpy(out.flightVelocity, b + 0x18B3D64, sizeof(out.flightVelocity));
    if (!std::isfinite(out.flightAge)) out.flightAge = 0;
    for (auto& v : out.flightPosition) if (!std::isfinite(v)) v = 0;
    for (auto& v : out.flightVelocity) if (!std::isfinite(v)) v = 0;
    out.metresToUnits = *reinterpret_cast<float*>(settings + 0x28);
    out.forward = *reinterpret_cast<float*>(settings + 0x4C0);
    out.side = *reinterpret_cast<float*>(settings + 0x4C4);
    out.height = *reinterpret_cast<float*>(settings + 0x4C8);
    out.belowEye = *reinterpret_cast<float*>(settings + 0x4CC);
    // Generic tube path at312ACF: root17EDF14 transforms local17EEAA8.
    memcpy(out.roundTargetLocal,b+0x18B4820,sizeof(out.roundTargetLocal));
    out.roundTargetValid=out.nativeInitialized&&readPouchNode(out.weaponRoot).readable;
    for(float& v:out.roundTargetLocal)if(!std::isfinite(v)||std::abs(v)>100000){v=0;out.roundTargetValid=false;}
    const unsigned offsets[]={0x18B461A,0x18B462C,0x18B45F8,0x18B4607,0x18B4618};
    for(int i=0;i<5;i++)out.roundFlags[i]=b[offsets[i]];
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
