#pragma once
#include "../nvse_function.hpp"

#include "../nvse_api.hpp"
#include "native_scene.hpp"
#include "scene_replay.hpp"

namespace eyebot {

class RecorderCamera {
public:
    void compile(Scripts& scripts) {
        attachSnapshotCamera = fc::nvse::loadFunction(scripts, "Eyebot/AttachCamera");
        detachSnapshotCamera = fc::nvse::loadFunction(scripts, "Eyebot/DetachCamera");
        projectSnapshot = fc::nvse::loadFunction(scripts, "Eyebot/ProjectStill");
        projectMovie = fc::nvse::loadFunction(scripts, "Eyebot/ProjectMovie");
    }

    bool hasStill() const { return attachSnapshotCamera&&detachSnapshotCamera&&projectSnapshot; }
    bool hasMovie() const { return attachSnapshotCamera&&detachSnapshotCamera&&projectMovie; }

    int project(Scripts& scripts,Form* device,const Lens& lens,scene::Transform& cameraPose,bool largeFrame=false) const {
        if(largeFrame?!hasMovie():!hasStill())return -1;
        Element attached;
        scripts.call(attachSnapshotCamera, nullptr, nullptr, &attached, 1, device);
        if (attached.type != 1 || attached.value.number != 1) return -1;
        auto camera = scene::find(scene::root(device), "RobCoEyebotStill");
        int status = -4;
        if (camera) {
            const auto unit = native_scene::UnitsPerMeter;
            auto lateral = scene::cross({lens.front.x,lens.front.y,lens.front.z},{0,0,1});
            auto upright = scene::Vec{lens.up.x,lens.up.y,lens.up.z};
            const float length = scene::distance(lateral, {});
            if (length > .025f) {
                lateral = lateral * (1.f / length);
                upright = scene::cross(lateral, {lens.front.x,lens.front.y,lens.front.z});
            } else {
                lateral = {lens.right.x,lens.right.y,lens.right.z};
            }
            // JIP's camera starts with its own local mount orientation. Publish
            // the exact parked lens pose before the independent camera draw.
            const scene::Transform view = {{{lens.front.x,upright.x,lateral.x},
                                            {lens.front.y,upright.y,lateral.y},
                                            {lens.front.z,upright.z,lateral.z}},
                                           {lens.eye.x*unit,lens.eye.y*unit,lens.eye.z*unit},1.f};
            if (scene::setWorld(camera, view)) {
                cameraPose = scene::world(camera);
                auto root = scene::root(device);
                void* body[] = {scene::find(root,"Eyebot_Antique:0"), scene::find(root,"Eyebot_Antique:1"),
                                scene::find(root,"RobCoEyebotScreen")};
                unsigned previous[3]{};
                for (unsigned i=0; i<3; ++i) if (body[i]) {
                    auto& flags = scene::field<unsigned>(body[i], 0x30);
                    previous[i] = flags;
                    flags |= 1u;
                }
                Element result;
                const auto script = largeFrame ? projectMovie : projectSnapshot;
                const bool called = script && scripts.call(script, nullptr, nullptr, &result, 1, device);
                for (unsigned i=0; i<3; ++i) if (body[i]) scene::field<unsigned>(body[i], 0x30)=previous[i];
                if (called && result.type == 1) status = int(result.value.number);
            }
        }
        scripts.call(detachSnapshotCamera, nullptr, nullptr, nullptr, 1, device);
        return status;
    }

private:
    Script* attachSnapshotCamera{};
    Script* detachSnapshotCamera{};
    Script* projectSnapshot{};
    Script* projectMovie{};
};

} // namespace eyebot
