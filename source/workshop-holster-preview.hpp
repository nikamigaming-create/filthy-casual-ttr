#pragma once

// Uses the numbered markers authored by TTR for the preview and grip target.
namespace workshop_holsters {
struct Marker { Vec position; bool live = false; float radius = 3.5f; };
std::array<Marker,8> markers{};
Script* hotkey = nullptr;
Script* modelPath = nullptr;
void* miniature = nullptr;
void* orb = nullptr;
u32 miniatureModel = 0;
unsigned nextScan = 0, nextQuery = 0;
int previewSlot = -1, hoverSlot = -1;
unsigned hoverSince = 0;
enum class DrawStep { NativeGrip, Equip, Draw };
struct PendingForearmDraw {
    unsigned slot=8, start=0;
    u32 item=0;
    DrawStep step=DrawStep::NativeGrip;
};
PendingForearmDraw pendingDraw{};
float modelExtent = 1;
Vec modelCenter{};

void prepareVisual(void* node) {
    auto& collision = scene::field<void*>(node,0x1C);
    auto original = collision;
    collision = nullptr;
    scene::release(original);
    const auto vtable = scene::field<void**>(node,0);
    if (!reinterpret_cast<void*(__thiscall*)(void*)>(vtable[3])(node)) return;
    auto children = scene::field<void**>(node,0xA0);
    const auto count = scene::field<unsigned short>(node,0xA6);
    if (!children || count > 4096) return;
    for (unsigned i=0;i<count;++i) if (children[i]) prepareVisual(children[i]);
}

void clear() {
    for (auto node : {miniature,orb}) if (node) {
        if (auto p = scene::parent(node))
            reinterpret_cast<void(__thiscall*)(void*,void*)>(scene::field<void**>(p,0)[0xE8/4])(p,node);
        scene::release(node);
    }
    miniature = orb = nullptr;
    miniatureModel = 0;
    previewSlot = -1;
}

void scan(void* node, unsigned depth, unsigned& budget) {
    if (!node || !budget || depth > 20) return;
    --budget;
    const auto name = scene::name(node);
    constexpr char prefix[] = "NVVRHolsterNumber";
    if (!strncmp(name,prefix,sizeof(prefix)-1)) {
        const auto digit = name[sizeof(prefix)-1];
        if (digit >= '1' && digit <= '8' && !name[sizeof(prefix)])
            markers[digit-'1'] = {scene::world(node).p,true,3.5f};
    }
    const auto vtable = scene::field<void**>(node,0);
    if (!reinterpret_cast<void*(__thiscall*)(void*)>(vtable[3])(node)) return;
    const auto children = scene::field<void**>(node,0xA0);
    const auto count = scene::field<unsigned short>(node,0xA6);
    if (!children || count > 4096) return;
    for (unsigned i = 0; i < count && budget; ++i) scan(children[i],depth+1,budget);
}

void show(Form* item, void* anchor) {
    if (miniature && miniatureModel == item->id) return;
    clear();
    const auto path = call(modelPath,nullptr,item);
    if (path.type != 3 || !path.value.string) return;
    if (*path.value.string) miniature = pooladapter::loadOwnVisual(path.value.string);
    build_adapter::freeGameString(path.value.string);
    if (!miniature) return;
    scene::retain(miniature);
    prepareVisual(miniature);
    auto& flags = scene::field<unsigned>(miniature,0x30);
    flags &= ~1u;
    flags |= 0x800;
    std::vector<scene::Box> mesh;
    scene::update(miniature);
    scene::boxes(miniature,mesh);
    if (mesh.empty()) { clear(); return; }
    Vec extent;
    modelCenter = scene::boxCenter(mesh,&extent)-scene::world(miniature).p;
    modelExtent = std::max({extent.x,extent.y,extent.z,1.f});
    miniatureModel = item->id;
    scene::moveParent(miniature,anchor);
    orb = pooladapter::loadOwnVisual("FilthyCasual\\HolsterPreviewOrb.nif");
    if (orb) {
        scene::retain(orb);
        scene::moveParent(orb,anchor);
    }
}

void finishForearmGrip(Form* player,unsigned now,unsigned gripping,unsigned pressed,bool busy) {
    constexpr unsigned right=2;
    if(busy || (gripping!=right && pendingDraw.step==DrawStep::NativeGrip) ||
       workshop_native_holsters::markersVisible() ||
       build_adapter::grabbedItem() || build_adapter::stimHand()>=0){
        pendingDraw={};
        return;
    }
    workshop_native_holsters::Layout layout{};
    if(!hotkey || !workshop_native_holsters::read(layout))return;
    const auto touch=palm(player,1);
    const auto inside=[&](unsigned slot) {
        scene::Vec center{};
        return slot<markers.size() && workshop_native_holsters::volume(slot,layout,center)
            && scene::distance(touch,center)<=markers[slot].radius;
    };
    if(pressed&right){
        pendingDraw={};
        if(hoverSlot<0 || !inside(unsigned(hoverSlot)))return;
        const auto bone=layout.anchor[hoverSlot].bone;
        if(bone!=8 && bone!=9)return;
        auto item=form(call(hotkey,nullptr,hoverSlot+1));
        if(item && item->type==0x28)pendingDraw={unsigned(hoverSlot),now,item->id};
    }
    if(!pendingDraw.item || now-pendingDraw.start<110)return;
    const auto request=pendingDraw;
    pendingDraw={};
    auto item=form(call(hotkey,nullptr,request.slot+1));
    if(!item || item->id!=request.item)return;
    static auto holster=scripts->compile("begin function {}\nplayer.SetWeaponOut 0\nend");
    static auto equip=scripts->compile("ref item\nbegin function {item}\n"
        "if player.GetItemCount item > 0\nplayer.EquipItemAlt item 0 1\nSetFunctionValue 1\nendif\nend");
    static auto draw=scripts->compile("begin function {}\nplayer.SetWeaponOut 1\nend");
    if(!holster || !equip || !draw)return;
    static auto equippedBase=scripts->expression("player.GetEquippedObject 5");
    const auto current=form(call(equippedBase,nullptr));
    if(current && current->id==request.item && number(call(drawn,nullptr)))return;
    switch(request.step){
    case DrawStep::NativeGrip:
        if(!inside(request.slot))return;
        // Match the wheel's native holster, equip and draw timing. The held
        // model must be released before switching inventory and arming draw.
        call(holster,nullptr);
        pendingDraw={request.slot,now,request.item,DrawStep::Equip};
        break;
    case DrawStep::Equip:
        if(number(call(equip,nullptr,item))==1)
            pendingDraw={request.slot,now,request.item,DrawStep::Draw};
        break;
    case DrawStep::Draw:
        if(current && current->id==request.item){
            build_adapter::requestHandDraw(1);
            call(draw,nullptr);
        }
        break;
    }
}

void update(Form* player, unsigned now, unsigned gripping, unsigned pressed, bool busy) {
    if(pendingDraw.item || (pressed&2))finishForearmGrip(player,now,gripping,pressed,busy);
    const auto rig = scene::root(player);
    if (!hotkey) hotkey = scripts->compile("int slot\nbegin function {slot}\nSetFunctionValue (GetHotkeyItem slot)\nend");
    if (!modelPath) modelPath = scripts->compile("ref item\nbegin function {item}\nSetFunctionValue (GetModelPath item)\nend");
    if (!hotkey || !modelPath || !rig || busy || gripping) {
        clear();
        if (!gripping) hoverSlot = -1;
        return;
    }
    auto world = rig;
    for (unsigned i = 0; i < 12 && scene::parent(world); ++i) world = scene::parent(world);
    if (now >= nextScan) {
        nextScan = now+135;
        markers = {};
        unsigned cap = 3200;
        scan(world,0,cap);
        workshop_native_holsters::Layout layout{};
        if (workshop_native_holsters::read(layout)) {
            const auto ten = scene::distance(pooladapter::tracking({0,0,0}),pooladapter::tracking({0,0,10}));
            const auto scale = ten > .01f ? 10.f/ten : 70.f;
            for (unsigned i=0;i<markers.size();++i) {
                if (!markers[i].live && workshop_native_holsters::volume(i,layout,markers[i].position))
                    markers[i].live=true;
                markers[i].radius = std::clamp(layout.point[i].radius*scale,1.4f,12.f);
            }
        }
    }
    int chosen = -1;
    float best = 11.f;
    const auto left = palm(player,0), right = palm(player,1);
    for (unsigned i = 0; i < markers.size(); ++i) if (markers[i].live) {
        const auto d = std::min(scene::distance(left,markers[i].position),scene::distance(right,markers[i].position));
        if (d < best) { best = d; chosen = i; }
    }
    if (chosen != hoverSlot) { hoverSlot = chosen; hoverSince = now; }
    const auto age = now-hoverSince;
    if (chosen < 0 || age > 1200) { clear(); return; }
    if (chosen != previewSlot || now >= nextQuery) {
        if (now < nextQuery && !miniature) return;
        nextQuery = now+175;
        const auto item = form(call(hotkey,nullptr,chosen+1));
        if (!item) { clear(); return; }
        show(item,world);
        if (!miniature) return;
        previewSlot = chosen;
    }
    const auto z = Vec{0,0,1};
    const auto anchor = markers[chosen].position;
    const auto radius = markers[chosen].radius;
    const auto head = scene::find(rig,"Bip01 Head");
    auto face = (head ? scene::world(head).p : scene::world(rig).p)-anchor;
    face.z = 0;
    const auto yaw = (age / 1200.f)*1.75f;
    const auto s = std::sin(yaw), c = std::cos(yaw);
    auto turn = Vec{face.x*c-face.y*s,face.x*s+face.y*c,0};
    const auto opening = std::clamp(age/130.f,.12f,1.f);
    const auto closing = std::clamp((1200.f-age)/170.f,.05f,1.f);
    auto transform = segment(anchor,anchor+turn,z,radius*1.35f/modelExtent*opening*closing);
    transform.p = transform.p-scene::rotate(transform,modelCenter)*transform.scale;
    if (!scene::setWorld(miniature,transform)) clear();
    if (orb) {
        scene::setWorld(orb,segment(anchor,anchor-face,z,radius*.20f*opening*closing));
    }
}
} // namespace workshop_holsters
