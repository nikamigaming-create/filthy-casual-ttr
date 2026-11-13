#include "../native_worldmag_recovery.hpp"
#include <cassert>
#include <cstdio>
using namespace native_worldmag_recovery;
int main() {
    assert(exactDelta(0, 12, 12)); assert(exactDelta(200, 205, 5));
    assert(!exactDelta(200, 206, 5)); assert(!exactDelta(200, 200, 5));
    assert(!exactDelta(-1, 4, 5)); assert(!exactDelta(0, 0, 0));
    assert(!exactDelta(INT_MAX, INT_MAX, 1));
    assert(samePath("meshes\\Weapons\\Mag.NIF", "MESHES/Weapons/Mag.nif"));
    assert(!samePath("meshes\\A\\Mag.nif", "meshes\\B\\Mag.nif"));
    Candidate a{}, b{}; a.id=b.id=0xFF000001; a.count=b.count=5;
    assert(sameCandidate(a,b)); b.count=6; assert(!sameCandidate(a,b));
    b=a; b.installedMode=4; assert(!sameCandidate(a,b));
    unsigned char native[24]{}; memcpy(native,"##Magazine",11);
    unsigned size=10, cap=15; memcpy(native+16,&size,4); memcpy(native+20,&cap,4);
    char text[512]{}; assert(nativeString(native,text)); assert(!strcmp(text,"##Magazine"));
    size=16; memcpy(native+16,&size,4); assert(!nativeString(native,text));
    auto page=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    assert(page); DWORD prior=0; assert(VirtualProtect(page+4096,4096,PAGE_NOACCESS,&prior));
    assert(readable(page+4090,6)); assert(!readable(page+4090,7));
    VirtualFree(page,0,MEM_RELEASE);
    assert(!readable(nullptr,4)); assert(!readable(reinterpret_cast<void*>(UINTPTR_MAX-3),8));
    puts("PASS: transfer arithmetic, exact identity, bounded native string and cross-page pointer checks");
    // Emit/compile both ABI thunks; never install or execute them in this process.
    volatile auto thunk1=&firstHook; volatile auto thunk2=&secondHook;
    return thunk1&&thunk2 ? 0 : 1;
}
