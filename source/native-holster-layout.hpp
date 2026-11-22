#pragma once

// TTR 1.1.7 (119A085A…) reads SavedSlots at 2272F0. The eight native
// fallback positions start at 1614A98, their bone anchors at 1038D78,
// and their enable bytes at 1614B18. 229920 obtains each bone frame through
// 225590 and transforms the same local offset before native hit testing.
namespace workshop_native_holsters {
struct Point { float forward, right, up, radius; };
struct Anchor {
    int bone;
    scene::Vec offset;
    unsigned char rebind;
    unsigned char reserved[3];
};
static_assert(offsetof(Anchor,reserved)==17);
struct Layout {
    Point point[8];
    Anchor anchor[8];
    unsigned char enabled[8];
};
static_assert(sizeof(Point)==16 && sizeof(Anchor)==20 && sizeof(Layout)==296);

inline unsigned char* module() {
    const auto b = build_adapter::heldModule();
    const unsigned char entry[]{0x83,0xEC,0x74,0xA1};
    if (!b || std::memcmp(b+0x225590,entry,sizeof entry)) return nullptr;
    if (*reinterpret_cast<void**>(b+0x22989B) != b+0x1614A98 ||
        *reinterpret_cast<void**>(b+0x229896) != b+0x1038D78) return nullptr;
    if (!b[0x1614A83]) reinterpret_cast<void(__cdecl*)()>(b+0x2272F0)();
    if (!b[0x1614A83]) return nullptr;
    return b;
}

inline bool read(Layout& result) {
    const auto b = module();
    if (!b) return false;
    std::memcpy(result.point,b+0x1614A98,sizeof result.point);
    for(unsigned i=0;i<8;++i){
        result.anchor[i]={};
        std::memcpy(&result.anchor[i],b+0x1038D78+i*sizeof(Anchor),offsetof(Anchor,reserved));
    }
    std::memcpy(result.enabled,b+0x1614B18,sizeof result.enabled);
    return true;
}

inline bool bonePose(int bone, scene::Transform& pose) {
    const auto b=module();
    scene::Vec angles{};
    return b && reinterpret_cast<bool(__cdecl*)(int,scene::Vec*,scene::Vec*,float*)>(b+0x225590)
        (bone,&pose.p,&angles,&pose.r[0][0]);
}

inline bool volume(unsigned i, const Layout& layout, scene::Vec& world) {
    if (i >= 8 || !layout.enabled[i]) return false;
    const auto& a = layout.anchor[i];
    if (a.bone < 0 || a.bone > 9 || a.rebind) return false;
    // cdecl(bone, position, extra vector, row-major rotation). All three
    // caller-owned outputs are initialized and separate as in native 229A27.
    scene::Transform pose{};
    if (!bonePose(a.bone,pose)) return false;
    world = pose.p + scene::rotate(pose,a.offset);
    return std::isfinite(world.x) && std::isfinite(world.y) && std::isfinite(world.z);
}

// At 224F3D, the native long-grip timer selects the branch that arms
// marker dragging. Jump to its existing wait path while locked. The ordinary
// weapon use path at 225C50 and the marker visibility flag are independent.
inline bool lockMovement(bool locked) {
    const auto b = module();
    if (!b) return false;
    auto entry = b+0x224F3D;
    constexpr unsigned char movable[]{0x0F,0x87,0xF5,0x02,0x00,0x00};
    constexpr unsigned char fixed[]{0x90,0xE9,0xF5,0x02,0x00,0x00};
    const auto target = locked ? fixed : movable;
    const auto prior = locked ? movable : fixed;
    if (!std::memcmp(entry,target,sizeof movable)) return true;
    if (std::memcmp(entry,prior,sizeof movable)) return false;
    DWORD protect=0;
    if (!VirtualProtect(entry,2,PAGE_EXECUTE_READWRITE,&protect)) return false;
    std::memcpy(entry,target,2);
    FlushInstructionCache(GetCurrentProcess(),entry,2);
    DWORD unused=0;
    VirtualProtect(entry,2,protect,&unused);
    return true;
}

inline bool markersVisible() {
    const auto b = module();
    return b && b[0x1614B28] != 0;
}

inline bool showMarkers(bool visible) {
    const auto b = module();
    if (!b) return false;
    if (bool(b[0x1614B28]) != visible)
        reinterpret_cast<bool(__cdecl*)()>(b+0x22A970)();
    return bool(b[0x1614B28]) == visible;
}

inline bool apply(const Layout& layout) {
    const auto b = module();
    if (!b || build_adapter::grabbedItem()) return false;
    for (unsigned i=0;i<8;++i) {
        const auto& p=layout.point[i]; const auto& a=layout.anchor[i];
        if (!std::isfinite(p.forward) || !std::isfinite(p.right) || !std::isfinite(p.up) ||
            !std::isfinite(p.radius) || p.radius < .02f || p.radius > .50f ||
            std::abs(p.forward)>3 || std::abs(p.right)>3 || std::abs(p.up)>3 ||
            a.bone < -1 || a.bone > 9 || !std::isfinite(a.offset.x) || !std::isfinite(a.offset.y) ||
            !std::isfinite(a.offset.z) || scene::distance(a.offset,{})>250.f ||
            a.rebind>1 || layout.enabled[i]>1) return false;
    }
    std::memcpy(b+0x1614A98,layout.point,sizeof layout.point);
    // Older profile files may carry unrelated native bytes after the flag.
    // Leave those three bytes with TTR when restoring each anchor.
    for(unsigned i=0;i<8;++i)
        std::memcpy(b+0x1038D78+i*sizeof(Anchor),&layout.anchor[i],offsetof(Anchor,reserved));
    std::memcpy(b+0x1614B18,layout.enabled,sizeof layout.enabled);
    // Stock numbered-holster movement uses this same no-argument writer.
    reinterpret_cast<void(__cdecl*)()>(b+0x228140)();
    return true;
}
} // namespace workshop_native_holsters
