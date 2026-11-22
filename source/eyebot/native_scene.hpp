#pragma once

#include <cstring>
#include "../scene_adapter.hpp"
#include "eyebot_replay.hpp"

namespace eyebot::native_scene {

// Fallout's scene uses 70 game units to a meter in the TTR tracking path.
// Keep native Z up and convert both transform translations and lens range.
constexpr float UnitsPerMeter = 70.f;

inline V3 metres(scene::Vec value) {
    return {value.x / UnitsPerMeter, value.y / UnitsPerMeter, value.z / UnitsPerMeter};
}

inline V3 column(const scene::Transform& transform, int axis) {
    return {transform.r[0][axis], transform.r[1][axis], transform.r[2][axis]};
}

inline Lens lens(const scene::Transform& transform) {
    return {metres(transform.p), column(transform, 0), column(transform, 2),
            column(transform, 1)};
}

inline Pose pose(const scene::Transform& transform) {
    Pose result;
    result.position = metres(transform.p);
    result.scale = transform.scale;
    const auto& m = transform.r;
    const float trace = m[0][0] + m[1][1] + m[2][2];
    if (trace > 0.f) {
        const float s = 2.f * std::sqrt(trace + 1.f);
        result.qw = s * .25f;
        result.qx = (m[2][1] - m[1][2]) / s;
        result.qy = (m[0][2] - m[2][0]) / s;
        result.qz = (m[1][0] - m[0][1]) / s;
    } else {
        const int axis = m[0][0] > m[1][1]
            ? (m[0][0] > m[2][2] ? 0 : 2)
            : (m[1][1] > m[2][2] ? 1 : 2);
        const int next = (axis + 1) % 3;
        const int last = (axis + 2) % 3;
        const float s = 2.f * std::sqrt(1.f + m[axis][axis] - m[next][next] - m[last][last]);
        float xyz[3]{};
        xyz[axis] = s * .25f;
        xyz[next] = (m[axis][next] + m[next][axis]) / s;
        xyz[last] = (m[axis][last] + m[last][axis]) / s;
        result.qx = xyz[0];
        result.qy = xyz[1];
        result.qz = xyz[2];
        result.qw = (m[last][next] - m[next][last]) / s;
    }
    return result;
}

struct RigFrame {
    Pose joints[Replay::MaxJointsPerActor]{};
    void* nodes[Replay::MaxJointsPerActor]{};
    uint32_t count = 0;
    uint32_t layout = 2166136261u;
    uint32_t traversed = 0, brokenPanes = 0, paneCount = 0;
    struct Pane { void* node; uint32_t bit; };
    Pane panes[32]{};
};

inline bool collectRig(void* node, RigFrame& frame, unsigned depth = 0) {
    if (!node) return true;
    if (++frame.traversed > 2048 || depth > 48) return false;
    auto vtable = scene::field<void**>(node, 0);
    const auto name = scene::name(node);
    if (std::strncmp(name,"FCGlassPane",11)==0 && std::strlen(name)==13 &&
        name[11]>='0' && name[11]<='3' && name[12]>='0' && name[12]<='9') {
        const unsigned index=10*(name[11]-'0')+(name[12]-'0');
        if(index<32 && frame.paneCount<32){
            const unsigned bit=1u<<index;
            frame.panes[frame.paneCount++]={node,bit};
            if(scene::field<unsigned>(node,0x30)&1u)frame.brokenPanes|=bit;
        }
    }
    if (!reinterpret_cast<void* (__thiscall*)(void*)>(vtable[3])(node)) return true;
    if (std::strncmp(name, "Bip01", 5) == 0) {
        auto joint = pose(scene::world(node));
        // TTR hides tracked bones using zero or sub-millimetre scales. Keep
        // their joint slots stable even while the native skin is collapsed.
        if (joint.scale >= 0.f && joint.scale < .001f) joint.scale = .001f;
        if (joint.valid()) {
            if (frame.count == Replay::MaxJointsPerActor) return false;
            frame.nodes[frame.count] = node;
            frame.joints[frame.count++] = joint;
            for (auto letter = name; *letter; ++letter) {
                frame.layout = (frame.layout ^ static_cast<unsigned char>(*letter)) * 16777619u;
            }
            frame.layout = (frame.layout ^ 0xffu) * 16777619u;
        }
    }
    const auto children = scene::field<void**>(node, 0xA0);
    const auto count = scene::field<unsigned short>(node, 0xA6);
    if (!children || count > 4096) return count == 0;
    for (unsigned index = 0; index < count; ++index) {
        if (!collectRig(children[index], frame, depth + 1)) return false;
    }
    return true;
}

inline scene::Transform world(const Pose& pose) {
    const float x = pose.qx, y = pose.qy, z = pose.qz, w = pose.qw;
    return {{
        {1.f - 2.f * (y*y + z*z), 2.f * (x*y - w*z), 2.f * (x*z + w*y)},
        {2.f * (x*y + w*z), 1.f - 2.f * (x*x + z*z), 2.f * (y*z - w*x)},
        {2.f * (x*z - w*y), 2.f * (y*z + w*x), 1.f - 2.f * (x*x + y*y)}
    }, {pose.position.x * UnitsPerMeter, pose.position.y * UnitsPerMeter,
        pose.position.z * UnitsPerMeter}, pose.scale};
}

}
