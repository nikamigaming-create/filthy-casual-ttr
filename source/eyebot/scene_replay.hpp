#pragma once

#include <map>
#include <string>
#include <unordered_set>
#include "../world_shadow_head_adapter.hpp"

namespace eyebot::native_scene {

using RecordedRigs = std::map<uint32_t, std::vector<std::string>>;

// Runs on the main thread within one independent camera draw. Native object
// transforms are restored before the normal headset's render continues.
class ReplayFrame {
public:
    ~ReplayFrame() { restore(); }

    bool restore() {
        for (const auto& change : saved) {
            scene::local(change.node) = change.local;
            scene::field<scene::Transform>(change.node, 0x68) = change.world;
            scene::field<unsigned>(change.node, 0x30) = change.flags;
            if (change.hasBound) if (auto bound = scene::field<float*>(change.node, 0x20)) {
                memcpy(bound, change.bound.data(), sizeof(change.bound));
            }
        }
        bool matches = true;
        for (const auto& change : saved) {
            if (memcmp(&scene::local(change.node), &change.local, sizeof(change.local))) {
                matches = false;
                mismatch = scene::name(change.node);
                break;
            }
        }
        saved.clear();
        roots.clear();
        return matches;
    }

    std::string mismatch;
    std::string applyError;

    bool apply(const Replay& recording, uint64_t timeUs, Form* player,
               const Element* references, unsigned count, const RecordedRigs& playerRigs) {
        add(player);
        for (unsigned i = 0; i < count; ++i) {
            if (references[i].type == 2) add(references[i].value.form);
        }
        for (const auto& entry : roots) {
            if (!keepOriginal(entry.root)) return false;
        }
        bool compatible = true;
        const bool available = recording.playback(timeUs,
            [&](uint64_t id, uint64_t asset, const Pose& root,
                const Pose* joints, uint32_t count, uint32_t brokenPanes) {
                const auto* layouts = player && id == player->id ? &playerRigs : nullptr;
                if (compatible) compatible = applyEntity(uint32_t(id), asset, root, joints, count, layouts, brokenPanes);
            });
        if (!available || !compatible) return false;
        for (const auto& entry : roots) {
            auto& flags = scene::field<unsigned>(entry.root, 0x30);
            flags = entry.visible ? flags & ~1u : flags | 1u;
        }
        return true;
    }

    void revealPlayerHead(Form* player, scene::Vec cameraPosition) {
        if (!player || !byId.count(player->id) || !roots[byId.at(player->id)].visible) return;
        // Honor the native dismemberment state. The first-person rig's tiny
        // head and neck are restored only while rendering the exported frame.
        constexpr unsigned dismemberedLimbs = 0x5F;
        if (scene::field<unsigned char>(player, 0x44 + 8 + dismemberedLimbs / 8) &
            (1u << (dismemberedLimbs % 8))) return;
        world_shadow_head::View view;
        view.prepare(player);
        if (!view.ready || view.count < 2) return;
        const float distance = scene::distance(cameraPosition, view.poses[1].value.p);
        if (!std::isfinite(distance) || distance < 35.f) return;
        if (!setPose(view.poses[0].node, view.poses[0].value) ||
            !setPose(view.poses[1].node, view.poses[1].value)) return;
        for (const auto* name : {"BSFaceGenNiNodeBiped", "FaceGenFace", "FaceGenHairNoHat",
                                 "FaceGenMouth", "FaceGenTeeth", "FaceGenTongue",
                                 "FaceGenEyeLeft", "FaceGenEyeRight"}) {
            auto node = scene::find(view.root, name);
            if (!node) continue;
            scene::field<unsigned>(node, 0x30) &= ~1u;
            publishTransforms(node);
        }
    }

private:
    struct Root { void* root; uint32_t base; bool visible; };
    struct SavedNode {
        void* node;
        scene::Transform local, world;
        unsigned flags;
        std::array<float,4> bound{};
        bool hasBound = false;
    };
    std::vector<Root> roots;
    std::vector<SavedNode> saved;
    std::unordered_set<void*> seen;
    std::map<uint32_t, size_t> byId;

    static void publishTransforms(void* node, unsigned depth = 0) {
        if (!node || depth > 48) return;
        auto vtable = scene::field<void**>(node, 0);
        // NiAVObject::UpdateWorldData and UpdateWorldBound, from the game's
        // vtable contract. Do not run animation controllers during restore.
        static const unsigned char updateData[12]{};
        reinterpret_cast<void (__thiscall*)(void*, const void*)>(vtable[0xB8 / 4])(node, updateData);
        if (reinterpret_cast<void* (__thiscall*)(void*)>(vtable[3])(node)) {
            const auto children = scene::field<void**>(node, 0xA0);
            const auto count = scene::field<unsigned short>(node, 0xA6);
            if (children && count <= 4096) for (unsigned i = 0; i < count; ++i) {
                publishTransforms(children[i], depth + 1);
            }
        }
        reinterpret_cast<void (__thiscall*)(void*)>(vtable[0xBC / 4])(node);
    }

    static bool setPose(void* node, const scene::Transform& wanted) {
        auto parent = scene::parent(node);
        if (!parent) return false;
        const auto base = scene::world(parent);
        if (!std::isfinite(base.scale) || std::abs(base.scale) < .0001f) return false;
        scene::Transform local{};
        local.p = scene::inverseRotate(base, wanted.p - base.p) * (1.f / base.scale);
        local.scale = wanted.scale / base.scale;
        for (unsigned r = 0; r < 3; ++r) for (unsigned c = 0; c < 3; ++c) {
            for (unsigned k = 0; k < 3; ++k) local.r[r][c] += base.r[k][r] * wanted.r[k][c];
        }
        scene::local(node) = local;
        publishTransforms(node);
        return true;
    }

    bool keepOriginal(void* node, unsigned depth = 0) {
        if (!node || !seen.insert(node).second) return true;
        if (depth > 48 || saved.size() == 16384) return false;
        SavedNode original{node, scene::local(node), scene::world(node), scene::field<unsigned>(node,0x30)};
        if (const auto bound = scene::field<const float*>(node,0x20)) {
            memcpy(original.bound.data(), bound, sizeof original.bound);
            original.hasBound = true;
        }
        saved.push_back(original);
        auto vtable = scene::field<void**>(node, 0);
        if (!reinterpret_cast<void* (__thiscall*)(void*)>(vtable[3])(node)) return true;
        const auto children = scene::field<void**>(node,0xA0);
        const auto count = scene::field<unsigned short>(node,0xA6);
        if (!children || count > 4096) return count == 0;
        for (unsigned i = 0; i < count; ++i) if (!keepOriginal(children[i],depth + 1)) return false;
        return true;
    }

    void add(Form* reference) {
        if (!reference || byId.count(reference->id)) return;
        auto root = scene::root(reference);
        auto base = scene::field<Form*>(reference, 0x20);
        if (!root || !base || !scene::parent(root)) return;
        byId.emplace(reference->id, roots.size());
        roots.push_back({root, base->id, false});
    }

    bool applyEntity(uint32_t id, uint64_t asset, const Pose& pose, const Pose* joints, uint32_t count,
                     const RecordedRigs* playerRigs, uint32_t brokenPanes) {
        auto found = byId.find(id);
        if (found == byId.end()) {
            // GetRefs is scoped to the current cell. A recorded actor may have
            // crossed into another loaded cell before the take is exported.
            if (auto module = build_adapter::heldModule()) {
                auto lookup = reinterpret_cast<Form*(__cdecl*)(unsigned)>(module + 0x48E0D0);
                auto reference = lookup(id);
                if (reference && reference->id == id &&
                    (reference->type == 0x3A || reference->type == 0x3B || reference->type == 0x3C)) {
                    add(reference);
                    found = byId.find(id);
                    if (found != byId.end() && !keepOriginal(roots[found->second].root)) return false;
                }
            }
        }
        if (found == byId.end()) {
            applyError = "reference is no longer loaded: " + std::to_string(id);
            return false;
        }
        auto& entry = roots[found->second];
        if (entry.base != uint32_t(asset)) {
            applyError = "base form changed: " + std::to_string(id);
            return false;
        }
        RigFrame rig;
        const auto layout = uint32_t(asset >> 32);
        if (count && (!collectRig(entry.root, rig) || rig.count != count || rig.layout != layout)) {
            // An XR hand pose can change which native joints publish valid
            // transforms. The player still uses the same skin and bone names.
            if (!playerRigs) return false;
            const auto recorded = playerRigs->find(layout);
            if (recorded == playerRigs->end() || recorded->second.size() != count) {
                applyError = "player rig layout unavailable: " + std::to_string(layout);
                return false;
            }
            for (uint32_t i = 0; i < count; ++i) {
                rig.nodes[i] = scene::find(entry.root, recorded->second[i].c_str());
                if (!rig.nodes[i]) {
                    applyError = "player rig no longer contains " + recorded->second[i];
                    return false;
                }
            }
        }
        if (!setPose(entry.root, world(pose))) {
            applyError = "cannot apply root: " + std::to_string(id);
            return false;
        }
        for (uint32_t i = 0; i < count; ++i) {
            if (!setPose(rig.nodes[i], world(joints[i]))) {
                applyError = "cannot apply joint: ";
                applyError += scene::name(rig.nodes[i]);
                return false;
            }
        }
        for(uint32_t i=0;i<rig.paneCount;++i){
            const auto pane=rig.panes[i];
            auto& flags=scene::field<unsigned>(pane.node,0x30);
            flags=(brokenPanes&pane.bit)?flags|1u:flags&~1u;
        }
        entry.visible = true;
        return true;
    }
};

}
