#pragma once
// Native world-mag recovery, TTWVR SHA256 E14567BC1C7D183130F902F9489B8CFFE6269DE06EEA0D79BF8D1D3098C595B1 only.
// x86 / main thread. No drop hooks, save records, AddItem, clip writes or registry writes.
#include "reload_adapter.hpp"
#include <cstdint>
#include <climits>
#include <atomic>
#include <cstring>

namespace native_worldmag_recovery {
static_assert(sizeof(void*) == 4, "This adapter is x86 only");
struct Diagnostics {
    const char* reason = "not installed";
    unsigned candidate = 0, transferred = 0, prepared = 0, committed = 0, failures = 0;
    unsigned firstEntries = 0, secondEntries = 0, attempts = 0;
    int rounds = 0, hand = -1, inventoryBefore = -1, inventoryAfter = -1;
    bool installed = false, poisoned = false;
};
// Both callbacks run inside the original native update, not only in MainLoop.
// player must return null during menus/loading/selector use/unsupported contexts.
// inventory must report an exact nonnegative integer, or return false.
struct Callbacks {
    void* (__cdecl* player)() noexcept = nullptr;
    bool (__cdecl* inventory)(void* player, void* ammo, int* result) noexcept = nullptr;
};
inline unsigned char* module = nullptr;
inline Callbacks callbacks{};
inline Diagnostics diagnostics{};
inline DWORD ownerThread = 0;
inline bool enabled = false, busy = false;
inline uintptr_t normalFirst = 0, eligibleFirst = 0, normalSecond = 0, initializeSpare = 0, initializeInstalled = 0;
inline bool commitInstalled = false;
inline std::atomic<int> cycleRequiredHand{-1};inline void* cycleWeapon=nullptr;

inline bool readable(const void* pointer, size_t bytes) noexcept {
    uintptr_t p = reinterpret_cast<uintptr_t>(pointer);
    if (!p || !bytes || p > UINTPTR_MAX - bytes) return false;
    const uintptr_t end = p + bytes;
    while (p < end) {
        MEMORY_BASIC_INFORMATION info{};
        if (!VirtualQuery(reinterpret_cast<void*>(p), &info, sizeof(info)) ||
            info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) ||
            !(info.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
                             PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) return false;
        const uintptr_t next = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
        if (next <= p) return false;
        p = next;
    }
    return true;
}
template<class T> inline T field(const void* p, size_t offset) noexcept {
    T value{}; memcpy(&value, static_cast<const unsigned char*>(p) + offset, sizeof(value)); return value;
}
template<class T> inline T global(size_t rva) noexcept { return field<T>(module, rva); }
inline bool cstring(const char* p, char (&out)[512]) noexcept {
    out[0] = 0;
    for (unsigned i = 0; i < sizeof(out); ++i) {
        if (!readable(p + i, 1)) return false;
        out[i] = p[i]; if (!out[i]) return i != 0;
    }
    out[0] = 0; return false;
}
inline bool nativeString(const void* p, char (&out)[512]) noexcept {
    if (!readable(p, 24)) return false;
    unsigned size = field<unsigned>(p, 16), capacity = field<unsigned>(p, 20);
    if (!size || size >= sizeof(out) || size > capacity || capacity > 1024 * 1024) return false;
    const char* chars = capacity < 16 ? static_cast<const char*>(p) : field<const char*>(p, 0);
    if (!readable(chars, size + 1) || chars[size] || memchr(chars, 0, size)) return false;
    memcpy(out, chars, size + 1); return true;
}
inline bool samePath(const char* a, const char* b) noexcept {
    // Windows resource path spelling only; no suffix or basename matching.
    for (;; ++a, ++b) {
        unsigned char x = static_cast<unsigned char>(*a), y = static_cast<unsigned char>(*b);
        if (x == '/') x = '\\'; if (y == '/') y = '\\';
        if (x >= 'A' && x <= 'Z') x += 'a' - 'A';
        if (y >= 'A' && y <= 'Z') y += 'a' - 'A';
        if (x != y) return false; if (!x) return true;
    }
}
inline bool exactDelta(int before, int after, int rounds) noexcept {
    return before >= 0 && rounds > 0 && before <= INT_MAX - rounds && after == before + rounds;
}
struct Candidate {
    unsigned id = 0, ammoID = 0, weaponID = 0;
    void* ref = nullptr; void* ammo = nullptr; void* player = nullptr; void* weapon = nullptr; void* visual = nullptr;
    int count = 0, hand = -1, installedMode = -1;
    char model[512]{}, node[512]{};
};
struct Pending { Candidate candidate{}; uintptr_t frameCookie = 0; ULONGLONG at = 0; };
inline Pending pending{};
inline bool gates() noexcept {
    bool manual = false;
    if (!module || !enabled || diagnostics.poisoned || !ownerThread || GetCurrentThreadId() != ownerThread ||
        !build_adapter::readManualReload(manual) || !manual || !module[0x198D03D] ||
        module[0x198D3AD] || global<int>(0x198D368) != 0) return false;
    const int mode = global<int>(0x198D22C);
    return mode == 0 || mode == 4 || mode == 5;
}
inline void* lookup(unsigned id) noexcept {
    // 41A910: cdecl form-ID wrapper over FalloutNV LookupFormByID 4839C0, with native SEH.
    return reinterpret_cast<void*(__cdecl*)(unsigned)>(module + 0x444DE0)(id);
}
inline bool readCandidate(Candidate& out) noexcept {
    if (!gates() || !callbacks.player || !callbacks.inventory || !(out.player = callbacks.player())) return false;
    out.installedMode = global<int>(0x198D22C);
    if (global<int>(0xFEBFCC) >= 0) return false;
    out.hand = reinterpret_cast<bool(__cdecl*)()>(module + 0x348C50)() ? 1 : 0;
    // Conservative: exactly one physical held slot; alias/multiple-object holds stay entirely native.
    const bool first = module[0x14DA7FD] != 0, second = module[0x14DA8CC] != 0;
    if (first == second) return false;
    const int heldHand = global<int>(first ? 0xFE0E40 : 0x14DA8D0);
    if (heldHand != out.hand) return false;
    out.ref = global<void*>(first ? 0x14DB5D8 : 0x14DA8D4);
    if (!readable(out.ref, 0x68) || (field<unsigned>(out.ref, 8) & 0x20)) return false;
    out.id = field<unsigned>(out.ref, 0xC);
    if (!out.id || lookup(out.id) != out.ref) return false;
    out.ammo = field<void*>(out.ref, 0x20);
    if (!readable(out.ammo, 0x10) || field<unsigned char>(out.ammo, 4) != 41) return false;
    out.ammoID = field<unsigned>(out.ammo, 0xC);
    if (reinterpret_cast<void*(__cdecl*)(void*)>(module + 0x33E600)(out.player) != out.ammo) return false;
    out.count = reinterpret_cast<int(__cdecl*)(void*)>(module + 0x446820)(out.ref);
    const int capacity = reinterpret_cast<int(__cdecl*)()>(module + 0x343FD0)();
    if (out.count <= 0 || out.count > 32767 || capacity <= 0 || out.count > capacity) return false;
    // Preserve the native per-acquisition limit34E8A2..34E90C; reject rather than truncate a real stack.
    int limit = capacity;
    if (out.installedMode != 4) {
        if (module[0x198DA4F]) limit = 1;
        else if (!module[0x198DA4E]) {
            if (module[0x198DA4D]) limit = 2;
            else if (!module[0x198DA1F] && !module[0x198D9D7] && !module[0x198DA33]) {
                if (module[0x198DA01]) limit = 1;
                else if (module[0x198D0F2]) limit = reinterpret_cast<int(__cdecl*)()>(module + 0x341430)();
            }
        }
    }
    if (limit <= 0 || out.count > limit) return false;
    out.weapon = global<void*>(0x198D0A4);
    if (!readable(out.weapon, 0x10) || field<unsigned char>(out.weapon, 4) != 40) return false;
    out.weaponID = field<unsigned>(out.weapon, 0xC);
    out.visual = global<void*>(0x198D270);
    if (!readable(out.visual, 0x9C)) return false;
    const uintptr_t begin = global<uintptr_t>(0x199FD40), end = global<uintptr_t>(0x199FD44);
    if (!begin || end < begin || (end - begin) % 0x1E8 || (end - begin) / 0x1E8 > 256 ||
        !readable(reinterpret_cast<void*>(begin), end - begin)) return false;
    unsigned matches = 0;
    for (uintptr_t p = begin; p < end; p += 0x1E8) {
        auto entry = reinterpret_cast<unsigned char*>(p);
        if (field<unsigned>(entry, 0) != out.id || entry[0x1E5]) continue;
        if (++matches != 1 || field<unsigned>(entry, 4) != out.ammoID || field<int>(entry, 8) != out.count ||
            !nativeString(entry + 0xC, out.model) || !nativeString(entry + 0x24, out.node)) return false;
    }
    if (matches != 1) return false;
    // Same factory used by native spare drop34CED3: out[0x54], node, pos3, rot9, velocity3.
    // Only reads its inputs/native selected weapon; +48=model C string, +4C=node C string.
    unsigned char description[0x54]{};
    const float zero[3]{}, identity[9]{1,0,0,0,1,0,0,0,1};
    reinterpret_cast<void*(__cdecl*)(void*,void*,const float*,const float*,const float*)>(module + 0x351610)
        (description, out.visual, zero, identity, zero);
    char model[512]{}, node[512]{};
    if (!cstring(field<const char*>(description, 0x48), model) ||
        !cstring(field<const char*>(description, 0x4C), node) ||
        !samePath(model, out.model) || strcmp(node, out.node)) return false;
    return true;
}
inline bool sameCandidate(const Candidate& a, const Candidate& b) noexcept {
    return a.id == b.id && a.ref == b.ref && a.ammoID == b.ammoID && a.ammo == b.ammo &&
        a.player == b.player && a.weapon == b.weapon && a.weaponID == b.weaponID &&
        a.visual == b.visual && a.installedMode == b.installedMode && a.count == b.count && a.hand == b.hand &&
        samePath(a.model, b.model) && !strcmp(a.node, b.node);
}
inline bool prepareBody(uintptr_t frame) noexcept {
    pending = {};
    if (!diagnostics.poisoned) diagnostics.reason = "waiting for eligible native-held registered magazine";
    Candidate c{}; if (!readCandidate(c)) return false;
    pending.candidate = c; pending.frameCookie = frame; pending.at = GetTickCount64();
    diagnostics.candidate = c.id; diagnostics.rounds = c.count; diagnostics.hand = c.hand;
    ++diagnostics.prepared; diagnostics.reason = "eligible native-held registered magazine"; return true;
}
inline int commitBody(uintptr_t frame) noexcept {
    const Pending saved = pending; pending = {};
    if (!saved.candidate.id || saved.frameCookie != frame || GetTickCount64() - saved.at > 50) return 0;
    Candidate c{}; if (!readCandidate(c) || !sameCandidate(c, saved.candidate)) return 0;
    // Native SetCurrentAmmoRounds mirrors the live clip count at F76030.
    // A fired/empty gun can retain its chamber pose flag. Recovering its magazine
    // must not turn that stale pose into a live chamber after native insertion.
    const bool emptyChamber=c.installedMode==4&&global<int>(0xFEBA88)==0;
    int before = -1;
    if (!callbacks.inventory(c.player, c.ammo, &before) || before < 0 || before > INT_MAX - c.count) return 0;
    diagnostics.inventoryBefore = before; diagnostics.inventoryAfter = -1;
    // Poison BEFORE release/Activate: a failed or interrupted transaction is never retried.
    // There is no invented rollback: native Activate may already have restored real ammunition.
    ++diagnostics.attempts; diagnostics.poisoned = true; diagnostics.reason = "native transfer incomplete; recovery disabled";
    if (!build_adapter::stowSelectedGrab(c.player, c.ref)) { ++diagnostics.failures; return 0; }
    // c.ref may be destroyed now. Resolve its ID afresh; never dereference the stale pointer.
    int after = -1;
    const bool counted = callbacks.inventory(c.player, c.ammo, &after);
    diagnostics.inventoryAfter = after;
    void* remaining = lookup(c.id);
    const bool consumed = !remaining || (readable(remaining, 0x10) && (field<unsigned>(remaining, 8) & 0x20));
    const bool unchanged = global<void*>(0x198D0A4) == c.weapon && global<void*>(0x198D270) == c.visual &&
        global<int>(0x198D368) == 0 && !module[0x198D3AD] && global<int>(0x198D22C) == c.installedMode;
    bool manual = false;
    if (!counted || !exactDelta(before, after, c.count) || !consumed || !unchanged ||
        !build_adapter::readManualReload(manual) || !manual || callbacks.player() != c.player ||
        reinterpret_cast<void*(__cdecl*)(void*)>(module + 0x33E600)(c.player) != c.ammo) {
        ++diagnostics.failures; return 0;
    }
    diagnostics.poisoned = false; diagnostics.transferred = c.id; ++diagnostics.committed;
    commitInstalled = c.installedMode == 4;
    if(emptyChamber){module[0xFEB9A8]=0;cycleWeapon=c.weapon;cycleRequiredHand=1-c.hand;}
    // Keep the native empty-trigger auto-load fallback from cycling this recovered
    // magazine. Off-hand trigger/physical slide remain untouched and clear the gate.
    // A real round already in the chamber never enters this path.
    diagnostics.reason = commitInstalled ? "transferred once; entering native installed initializer" : "transferred once; entering native spare initializer";
    return c.count;
}
inline int __cdecl prepare(uintptr_t frame) noexcept {
    ++diagnostics.firstEntries;
    if (busy) return 0; busy = true; int result = 0;
    __try { result = prepareBody(frame) ? 1 : 0; }
    __except(EXCEPTION_EXECUTE_HANDLER) { pending = {}; diagnostics.reason = "read fault; candidate refused"; }
    busy = false; return result;
}
inline int __cdecl commit(uintptr_t frame) noexcept {
    ++diagnostics.secondEntries;
    if (busy) return 0; busy = true; int result = 0;
    __try { result = commitBody(frame); }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        pending = {}; diagnostics.poisoned = true; ++diagnostics.failures;
        diagnostics.reason = "transfer fault; recovery disabled";
    }
    busy = false; return result;
}
__declspec(naked) inline void firstHook() {
    __asm {
        pushfd
        pushad
        mov ebx, esp
        sub esp, 528
        and esp, -16
        fxsave [esp]
        fninit
        push ebp
        call prepare
        add esp, 4
        test eax, eax
        jz original
        fxrstor [esp]
        mov esp, ebx
        popad
        popfd
        jmp dword ptr [eligibleFirst]
    original:
        fxrstor [esp]
        mov esp, ebx
        popad
        popfd
        cmp byte ptr [ebp-17ADh], 0
        jmp dword ptr [normalFirst]
    }
}
__declspec(naked) inline void secondHook() {
    __asm {
        pushfd
        pushad
        mov ebx, esp
        sub esp, 528
        and esp, -16
        fxsave [esp]
        fninit
        push ebp
        call commit
        add esp, 4
        test eax, eax
        jz original
        mov [ebx+28], eax
        mov [ebx+4], eax
        cmp byte ptr [commitInstalled], 0
        jne installed
        fxrstor [esp]
        mov esp, ebx
        popad
        popfd
        jmp dword ptr [initializeSpare]
    installed:
        fxrstor [esp]
        mov esp, ebx
        popad
        popfd
        jmp dword ptr [initializeInstalled]
    original:
        fxrstor [esp]
        mov esp, ebx
        popad
        popfd
        cmp byte ptr [ebp-1739h], 0
        jmp dword ptr [normalSecond]
    }
}
inline bool pinnedHash(HMODULE m) {
    wchar_t path[MAX_PATH]{}; if (!GetModuleFileNameW(m, path, MAX_PATH)) return false;
    FILE* file = nullptr; if (_wfopen_s(&file, path, L"rb")) return false;
    BCRYPT_ALG_HANDLE alg = nullptr; BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD length = 0, got = 0; unsigned char digest[32]{};
    bool ok = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0;
    if (ok) ok = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&length), sizeof(length), &got, 0) >= 0;
    std::vector<unsigned char> object(length), buffer(65536);
    if (ok) ok = BCryptCreateHash(alg, &hash, object.data(), length, nullptr, 0, 0) >= 0;
    size_t n = 0; while (ok && (n = fread(buffer.data(), 1, buffer.size(), file)) != 0)
        ok = BCryptHashData(hash, buffer.data(), static_cast<ULONG>(n), 0) >= 0;
    if (ferror(file)) ok = false; fclose(file);
    if (ok) ok = BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0;
    if (hash) BCryptDestroyHash(hash); if (alg) BCryptCloseAlgorithmProvider(alg, 0);

    const unsigned char expected[32]{0xE1,0x45,0x67,0xBC,0x1C,0x7D,0x18,0x31,0x30,0xF9,0x02,0xF9,0x48,0x9B,0x8C,0xFF,0xE6,0x26,0x9D,0xE0,0x6E,0xEA,0x0D,0x79,0xBF,0x8D,0x1D,0x30,0x98,0xC5,0x95,0xB1};
    return ok && !memcmp(digest, expected, 32);
}
inline void branch(unsigned char* site, void* destination) noexcept {
    const uint32_t relative = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(destination) - reinterpret_cast<uintptr_t>(site) - 5);
    site[0] = 0xE9; memcpy(site + 1, &relative, 4); site[5] = site[6] = 0x90;
}
// Install ONLY during plugin startup, before any native reload update can run.
// The callbacks/hooks remain resident until process exit; no hot unload/unhook.
inline const char* install(Callbacks supplied) {
    if (diagnostics.installed) return "already installed";
    if (!supplied.player || !supplied.inventory) return "REFUSED: missing callbacks";
    auto handle = GetModuleHandleW(L"TTWVR.dll");
    if (!handle || !pinnedHash(handle)) return "REFUSED: exact TTWVR hash";
    auto base = reinterpret_cast<unsigned char*>(handle);
    const auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    const auto nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
    if (nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 || nt->FileHeader.TimeDateStamp != 0x6AF8509E ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC || nt->OptionalHeader.SizeOfImage <= 0x199FD4C)
        return "REFUSED: PE32 identity";
    const unsigned char first[]{0x80,0xBD,0x53,0xE8,0xFF,0xFF,0x00};
    const unsigned char second[]{0x80,0xBD,0xC7,0xE8,0xFF,0xFF,0x00};
    unsigned char destination[]{0xFF,0x35,0x00,0x00,0x00,0x00,0xA3,0x00,0x00,0x00,0x00,0xE8,0xC8,0xCC,0xFC,0xFF};
    const uint32_t visual = reinterpret_cast<uint32_t>(base + 0x198D270), count = reinterpret_cast<uint32_t>(base + 0x198D36C);
    memcpy(destination + 2, &visual, 4); memcpy(destination + 7, &count, 4);
    if (memcmp(base + 0x376DE3, first, sizeof(first)) || memcmp(base + 0x37792A, second, sizeof(second)) ||
        memcmp(base + 0x3779D8, destination, sizeof(destination))) return "REFUSED: native instruction guard";
    unsigned char installedDestination[]{0xE8,0x45,0xA6,0xFC,0xFF,0xFF,0x35,0x00,0x00,0x00,0x00,0x3B,0xF0,0x0F,0x4C,0xC6,0xA3,0x00,0x00,0x00,0x00};
    const uint32_t installedNode = reinterpret_cast<uint32_t>(base + 0x198D08C), installedCount = reinterpret_cast<uint32_t>(base + 0x198D230);
    memcpy(installedDestination + 7, &installedNode, 4); memcpy(installedDestination + 17, &installedCount, 4);
    if (memcmp(base + 0x379986, installedDestination, sizeof(installedDestination))) return "REFUSED: installed initializer guard";
    unsigned char chamberRead[]={0x8A,0x0D,0,0,0,0},clipWrite[]={0xA3,0,0,0,0};
    const uint32_t chamberAddress=reinterpret_cast<uint32_t>(base+0xFEB9A8),clipAddress=reinterpret_cast<uint32_t>(base+0xFEBA88);
    memcpy(chamberRead+2,&chamberAddress,4);memcpy(clipWrite+1,&clipAddress,4);
    if(memcmp(base+0x379E65,chamberRead,sizeof chamberRead)||memcmp(base+0x35C485,clipWrite,sizeof clipWrite))return "REFUSED: native chamber/clip contract";
    struct Guard { unsigned rva; unsigned length; unsigned char bytes[8]; };
    const Guard helpers[]{
        {0x348C50,5,{0xE9,0x54,0xBB,0xCC,0xFF}}, {0x33E600,4,{0xFF,0x74,0x24,0x04}},
        {0x446820,4,{0x8B,0x4C,0x24,0x04}}, {0x351610,8,{0x83,0xEC,0x24,0x56,0x57,0x0F,0x57,0xC0}},
        {0x444DE0,5,{0x55,0x8B,0xEC,0x6A,0xFE}}, {0x343FD0,6,{0x83,0xEC,0x08,0x53,0x56,0x57}},
        {0x1C33B0,8,{0x81,0xEC,0xCC,0,0,0,0x56,0x57}}};
    for (const auto& g : helpers) if (memcmp(base + g.rva, g.bytes, g.length)) return "REFUSED: helper instruction guard";
    DWORD protection1 = 0, protection2 = 0, unused = 0;
    if (!VirtualProtect(base + 0x376DE3, 7, PAGE_EXECUTE_READWRITE, &protection1)) return "REFUSED: first page protection";
    if (!VirtualProtect(base + 0x37792A, 7, PAGE_EXECUTE_READWRITE, &protection2)) {
        VirtualProtect(base + 0x376DE3, 7, protection1, &unused); return "REFUSED: second page protection";
    }
    module = base; callbacks = supplied;
    normalFirst = reinterpret_cast<uintptr_t>(base + 0x376DEA); eligibleFirst = reinterpret_cast<uintptr_t>(base + 0x376DF9);
    normalSecond = reinterpret_cast<uintptr_t>(base + 0x377931); initializeSpare = reinterpret_cast<uintptr_t>(base + 0x3779D8);
    initializeInstalled = reinterpret_cast<uintptr_t>(base + 0x379986);
    branch(base + 0x376DE3, reinterpret_cast<void*>(&firstHook));
    branch(base + 0x37792A, reinterpret_cast<void*>(&secondHook));
    FlushInstructionCache(GetCurrentProcess(), base + 0x376DE3, 7);
    FlushInstructionCache(GetCurrentProcess(), base + 0x37792A, 7);
    const bool restored1 = VirtualProtect(base + 0x376DE3, 7, protection1, &unused) != 0;
    const bool restored2 = VirtualProtect(base + 0x37792A, 7, protection2, &unused) != 0;
    diagnostics.installed = true;
    diagnostics.reason = restored1 && restored2 ? "installed; awaiting main-thread enable" : "installed; WARNING page protection restore failed";
    return diagnostics.reason;
}
// Read-only current-build insertion/pouch geometry. Layout matches NiTransform (0x34).
struct Pose { float r[3][3]{}, p[3]{}, scale = 0; };
static_assert(sizeof(Pose)==0x34);
struct Insertion { bool valid=false, pouchValid=false; Pose rest{}, held{}; float pouch[3]{}; };
inline bool validPose(const Pose& value) noexcept {
    const auto f=reinterpret_cast<const float*>(&value);
    for(unsigned i=0;i<13;i++) if(!std::isfinite(f[i]) || std::abs(f[i])>1e9f) return false;
    return value.scale>0 && value.scale<10000;
}
inline Insertion insertionBody() noexcept {
    Insertion out{}; if(!module || GetCurrentThreadId()!=ownerThread)return out;
    auto weapon=global<void*>(0x198D080), node=global<void*>(0x198D08C);
    if(!readable(weapon,0x9C)||!readable(node,0x9C))return out;
    const unsigned char parentGuard[]{0x81,0xEC,0xCC,0,0,0};
    const unsigned char composeGuard[]{0x83,0xEC,0x24,0x56,0x8B,0x74,0x24,0x2C};
    if(memcmp(module+0x3575D0,parentGuard,sizeof(parentGuard))||memcmp(module+0x3442E0,composeGuard,sizeof(composeGuard)))return out;
    Pose world=field<Pose>(weapon,0x68), parent{}; out.held=field<Pose>(node,0x68);
    if(!validPose(world)||!validPose(out.held))return out;
    if(!reinterpret_cast<bool(__cdecl*)(void*,const Pose*,void*,Pose*)>(module+0x3575D0)(weapon,&world,node,&parent)||!validPose(parent))return out;
    reinterpret_cast<Pose*(__cdecl*)(Pose*,const Pose*,const void*,const void*,float)>(module+0x3442E0)
        (&out.rest,&parent,module+0x198D0D8,module+0x198CC04,1.f);
    out.valid=validPose(out.rest);
    // Installed-held return34A917 uses342890: the visible PRIMARY pouch, not its optional copy.
    // This diagnostic covers mode2 while spareMode==0. Do not mistake a spare already in hand for its pouch.
    auto primary=global<void*>(0x198D270);
    if(global<int>(0x198D368)==0 && module[0x198D263] && readable(primary,0x9C)) {
        const Pose pose=field<Pose>(primary,0x68); const auto center=reinterpret_cast<const float*>(module+0x198D274);
        bool centerValid=module[0x198D262]!=0;
        for(unsigned i=0;i<3;i++)if(!std::isfinite(center[i]))centerValid=false;
        if(validPose(pose)) {
            out.pouchValid=true;
            for(unsigned i=0;i<3;i++) {
                out.pouch[i]=pose.p[i];
                if(centerValid)out.pouch[i]+=pose.scale*(pose.r[i][0]*center[0]+pose.r[i][1]*center[1]+pose.r[i][2]*center[2]);
                if(!std::isfinite(out.pouch[i])||std::abs(out.pouch[i])>1e9f)out.pouchValid=false;
            }
        }
    }
    return out;
}
inline Insertion readInsertion() noexcept {
    __try { return insertionBody(); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return {}; }
}
// Read-only QA view of UPSTREAM records, not addon-owned PhysicalMag objects.
struct WorldRef {
    unsigned id = 0, ammoID = 0;
    int recordedCount = 0, actualCount = 0;
    bool valid = false, rootValid = false, boundValid = false;
    float rootCenter[3]{}, bound[4]{};
};
inline unsigned worldRefsBody(WorldRef* output, unsigned capacity) noexcept {
    if (!module || !output || !capacity || GetCurrentThreadId() != ownerThread) return 0;
    const uintptr_t begin = global<uintptr_t>(0x199FD40), end = global<uintptr_t>(0x199FD44);
    if (!begin || end < begin || (end-begin)%0x1E8 || (end-begin)/0x1E8 > 256 ||
        !readable(reinterpret_cast<void*>(begin),end-begin)) return 0;
    unsigned count = 0;
    for (uintptr_t p=begin; p<end && count<capacity; p+=0x1E8) {
        const auto entry=reinterpret_cast<unsigned char*>(p); if(entry[0x1E5]) continue;
        WorldRef item{}; item.id=field<unsigned>(entry,0); item.ammoID=field<unsigned>(entry,4); item.recordedCount=field<int>(entry,8);
        auto ref=lookup(item.id);
        if(readable(ref,0x68) && !(field<unsigned>(ref,8)&0x20)) {
            auto ammo=field<void*>(ref,0x20);
            if(readable(ammo,0x10) && field<unsigned char>(ammo,4)==41 && field<unsigned>(ammo,0xC)==item.ammoID) {
                item.actualCount=reinterpret_cast<int(__cdecl*)(void*)>(module+0x446820)(ref);
                item.valid=item.actualCount>0 && item.actualCount==item.recordedCount;
                auto state=field<void*>(ref,0x64);
                auto root=readable(state,0x18)?field<void*>(state,0x14):nullptr;
                if(readable(root,0x9C)) {
                    memcpy(item.rootCenter,static_cast<unsigned char*>(root)+0x8C,12);
                    item.rootValid=true; for(float v:item.rootCenter) if(!std::isfinite(v)||std::abs(v)>1e9f)item.rootValid=false;
                    auto bound=field<float*>(root,0x20);
                    if(readable(bound,16)) {
                        memcpy(item.bound,bound,16); item.boundValid=true;
                        for(float v:item.bound) if(!std::isfinite(v)||std::abs(v)>1e9f)item.boundValid=false;
                        if(item.bound[3]<=0 || item.bound[3]>100000)item.boundValid=false;
                    }
                    if(!item.rootValid) memset(item.rootCenter,0,sizeof(item.rootCenter));
                    if(!item.boundValid) memset(item.bound,0,sizeof(item.bound));
                }
            }
        }
        output[count++]=item;
    }
    return count;
}
inline unsigned readWorldRefs(WorldRef* output, unsigned capacity) noexcept {
    __try { return worldRefsBody(output,capacity); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return 0; }
}
// Call every MainLoop, before returning on menus/focus/other early exits.
// Clears stale phase tokens. No inventory or native scene mutation.
inline void tick(bool allow) noexcept {
    if (!ownerThread) ownerThread = GetCurrentThreadId();
    if (GetCurrentThreadId() != ownerThread || busy) return;
    pending = {}; enabled = allow && diagnostics.installed && !diagnostics.poisoned;
    if(cycleRequiredHand.load()>=0){bool manual=false;
      if(!module||global<void*>(0x198D0A4)!=cycleWeapon||!build_adapter::readManualReload(manual)||!manual||module[0xFEB9A8]){cycleRequiredHand=-1;cycleWeapon=nullptr;}
    }
}
// PreLoad/NewGame/exit: never reset poison in-process; an ambiguous Activate is terminal for this session.
inline void reset() noexcept {cycleRequiredHand=-1;cycleWeapon=nullptr; enabled = false; pending = {}; }
}
