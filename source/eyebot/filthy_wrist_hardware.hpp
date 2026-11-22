#pragma once

#include "filthy_case_paint.hpp"

namespace eyebot {

// A separate owned part attached to the current Pip-Boy model. The next spot
// in the row is measured from the live Items and Data buttons for either rig.
class FilthyWristHardware {
public:
    void initialize(Scripts& scripts) {
        attach = scripts.compile(R"(
string_var modelPath
begin function {modelPath}
    SetFunctionValue (AttachModel player 3 "%z" modelPath)
end
)");
    }

    void reset() {
        paint.reset();
        scene::release(key);
        rig = key = faceLit = nullptr;
        retry = 0;
    }

    void update(Scripts& scripts, void* root, bool opened, uint64_t now) {
        if (root != rig) { reset(); rig = root; }
        if (!root || !attach || !build_adapter::heldModule()) return;
        if (now >= retry) {
            retry = now + 750;
            auto data = scene::find(root,"PipBoyButton03");
            auto items = scene::find(root,"PipBoyButton02");
            if (!items || !data || !scene::parent(data)) { reset(); rig=root; retry=now+750; return; }
            auto parent = scene::parent(data);
            auto current = scene::find(parent,"FCFilthyWristKey");
            if (!current && *scene::name(parent)) {
                const auto path = std::string(scene::name(parent)) + "|FilthyCasual\\filthy-wrist-key.nif";
                scripts.call(attach,nullptr,nullptr,nullptr,1,path.c_str());
                current = scene::find(parent,"FCFilthyWristKey");
            }
            if(current!=key){scene::retain(current);scene::release(key);key=current;}
            if (!key) return;
            auto position = scene::world(data);
            // Fit between DATA and the raised end of the case. Keep the
            // stock button plane and cap height across the whole row.
            position.p = position.p + (position.p-scene::world(items).p)*.82f;
            scene::setWorld(key,position);
            auto body=scene::find(parent,"PipBoyArm:0");
            if(!body)body=scene::find(parent,"PipBoyArmFemale:0");
            paint.update(body);
            faceLit = scene::find(key,"FCFilthyKeyLit");
        }
        setSelected(opened);
    }

    void setSelected(bool opened) {
        if (!key || !scene::parent(key) || !faceLit) return;
        auto& flags=scene::field<unsigned>(faceLit,0x30);
        flags=opened?flags&~1u:flags|1u;
        const float scale=opened?1.f:.001f;
        auto& glow=scene::local(faceLit);
        if(glow.scale!=scale){glow.scale=scale;scene::update(faceLit);}
    }

private:
    Script* attach{};
    filthy_case_paint::Paint paint;
    void* rig{};
    void* key{};
    void* faceLit{};
    uint64_t retry{};
};

} // namespace eyebot
