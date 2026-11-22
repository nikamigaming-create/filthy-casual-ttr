#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <shlobj.h>
#include <string>
#include "workshop-native-adapter.hpp"
#include "build_adapter.hpp"
#include "interaction_provider.hpp"
#include "pool-vr/adapter.hpp"
#include "pool-vr/equipment_adapter.hpp"
#include "native-holster-layout.hpp"
#include "equipment_layout_view.hpp"

namespace {
using scene::Vec;
using scene::Transform;
constexpr u32 Workbench = 0x75005;
constexpr u32 RepairKit = 0x140A68;
// FalloutNV.esm, RecipeWeaponRepairKit (00140A67), Repair 50.
constexpr std::array<u32, 6> recipe{0x31944, 0x31944, 0x13B2B1, 0x5B6D0, 0x2210A, 0x13B2B2};

Scripts* scripts = nullptr;
Arrays* arrays = nullptr;
Console* console = nullptr;
InventoryRef* (__cdecl* inventoryRef)(u32) = nullptr;
Script *findBenches, *findActors, *findDivideED, *motion, *readMotion, *position, *baseHealth;
Script *equipped, *equippedCondition, *drawn, *menu, *unsafe, *discard, *spawnKit, *skill;
Script *eligibility, *chargeED, *repairSound;
bool loaded = false, initialized = false, compatible = false;
bool holstersOnly = false;
std::atomic<unsigned> inputEdges{};
std::atomic<unsigned> canAccept{};
std::atomic<bool> aHeld[2]{};
std::atomic<unsigned> grips{};
std::atomic<unsigned> gripEdges{};
std::atomic<unsigned> primarySamples{};
unsigned offers = 0, drops = 0;
unsigned receivedBack = 0;
char diagnosticPath[MAX_PATH]{};
unsigned sequence = 0;

template<class... Args>
Element call(Script* fn, Form* caller, Args... args) {
    Element out{};
    if (fn) scripts->call(fn, caller, nullptr, &out, sizeof...(Args), args...);
    return out;
}
Form* form(Element e) { return e.type == 2 ? e.value.form : nullptr; }
double number(Element e) { return e.type == 1 ? e.value.number : 0; }
Form* id(u32 value) {
    return workshop_native::lookup(value);
}
Form* base(Form* ref) { return workshop_native::base(ref); }
Vec palm(Form* player, unsigned hand) {
    const auto point = pool_equipment::gripPoint(hand);
    return point.valid ? point.world : scene::palm(scene::root(player),hand);
}

struct Location {
    u32 id = 0;
    Vec top{};
};
struct Held {
    u32 id = 0;
    int motion = 0;
};
struct Job {
    Held weapon{};
    u32 receiver = 0;
    bool robot = false, repaired = false, pending = false;
    unsigned hand = 1;
    unsigned started = 0;
    Vec initial{};
    float before = 0, after = 0;
} job;
std::array<Held, recipe.size()> ingredients{};
Held kit{};
u32 activeBench = 0, previousGrab = 0, previousReceiver = 0, resultsRef = 0, lastWeapon = 0;
unsigned nextScan = 0, nextReport = 0;
float lastBefore = 0, lastAfter = 0;
std::array<u32, 8> benches{};
u32 robot = 0, cell = 0;
bool previousWasRobot = false;
Vec previousOffer{};
void* grippers = nullptr;
void* gripperHost = nullptr;
unsigned scannedActors = 0, scannedBenches = 0;

bool valid(Form* ref, Form* player) {
    return workshop_native::live(ref) && scene::cell(ref) == scene::cell(player);
}
void notify(const char* message) {
    char buffer[180]; sprintf_s(buffer, "MessageEx \"%s\"", message);
    console->run(buffer, nullptr);
}

bool initialize() {
    if (initialized) return compatible;
    auto ttr = GetModuleHandleW(L"TTWVR.dll");
    if (!ttr) return false;
    initialized = true;
    if (!build_adapter::verifiedTTWVR(ttr)) return false;
    drawn = scripts->expression("player.IsWeaponOut");
    menu = scripts->expression("MenuMode");
    if (holstersOnly) {
        compatible = drawn && menu;
        return compatible;
    }
    findBenches = scripts->expression("GetRefs 21 0 0");
    findActors = scripts->expression("GetRefs 200 0 1");
    findDivideED = scripts->expression(R"(GetFormFromMod "LonesomeRoad.esm" "004CDA")");
    readMotion = scripts->compile(R"(ref item
begin function {item}
SetFunctionValue (item.GetCollProp (sv_Construct "%e") 8)
end)");
    motion = scripts->compile(R"(ref item
int kind
begin function {item,kind}
item.SetCollProp "*" 8 kind
SetFunctionValue (item.GetCollProp (sv_Construct "%e") 8)
end)");
    position = scripts->compile(R"(ref item
float x
float y
float z
begin function {item,x,y,z}
item.SetPosEx x y z
SetFunctionValue 1
end)");
    baseHealth = scripts->compile("ref item\nbegin function {item}\nSetFunctionValue (item.GetItemRefCurrentHealth 2)\nend");
    equipped = scripts->expression("player.GetEquippedItemRef 5");
    equippedCondition = scripts->compile("ref item\nbegin function {item}\nSetFunctionValue (item.GetItemRefCurrentHealth 1)\nend");
    unsafe = scripts->compile("ref actor\nbegin function {actor}\nSetFunctionValue (actor.GetDead || actor.IsInCombat)\nend");
    skill = scripts->expression("player.GetAV Repair");
    spawnKit = scripts->compile("ref bench\nbegin function {bench}\nSetFunctionValue (bench.PlaceAtMe NVRepairKit 1)\nend");
    discard = scripts->compile("ref item\nbegin function {item}\nitem.Disable\nitem.MarkForDelete\nSetFunctionValue 1\nend");
    eligibility = scripts->compile(R"(begin function {}
if eval NVDLC04GenericPerkQuest.nCamaraderERank < 1
 return
endif
if eval (player.AuxVarGetFlt "_FC.Workshop.EDNextDay") > GameDaysPassed
 return
endif
SetFunctionValue (25 + 10 * (NVDLC04GenericPerkQuest.nRepairCheck > 0))
end)");
    chargeED = scripts->compile(R"(begin function {}
player.AuxVarSetFlt "_FC.Workshop.EDNextDay" (GameDaysPassed + 1)
end)");
    repairSound = scripts->compile("begin function {}\nPlaySound UIRepairWeapon\nend");
    compatible = findBenches && findActors && readMotion && motion && position && baseHealth &&
                 equipped && equippedCondition && drawn && menu && unsafe && skill && spawnKit && discard;
    if (*diagnosticPath) {
        FILE* f = nullptr;
        if (!fopen_s(&f, diagnosticPath, "w")) {
            fprintf(f, "{\"initialized\":%d,\"scripts\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}\n", compatible,
                    !!findBenches, !!readMotion, !!motion, !!position, !!baseHealth,
                    !!equipped, !!drawn, !!menu, !!unsafe, !!skill, !!spawnKit, !!discard);
            fclose(f);
        }
    }
    return compatible;
}

int floatBits(float value) { int raw; memcpy(&raw, &value, sizeof(raw)); return raw; }
void setPosition(Form* ref, Vec p) {
    call(position, nullptr, ref, floatBits(p.x), floatBits(p.y), floatBits(p.z));
}

bool pin(Held& slot, Form* ref) {
    const int previous = int(number(call(readMotion, nullptr, ref)));
    if (previous != 2 && previous != 3 && previous != 6) return false;
    if (number(call(motion, nullptr, ref, 4)) != 4) {
        call(motion, nullptr, ref, previous);
        return false;
    }
    slot = {ref->id, previous};
    return true;
}

void unpin(Held& slot) {
    if (auto ref = id(slot.id); workshop_native::live(ref))
        call(motion, nullptr, ref, slot.motion);
    slot = {};
}

void removeGrippers() {
    if (!grippers) return;
    if (auto p = scene::parent(grippers))
        reinterpret_cast<void(__thiscall*)(void*,void*)>(scene::field<void**>(p,0)[0xE8/4])(p,grippers);
    scene::release(grippers);
    scene::release(gripperHost);
    grippers = gripperHost = nullptr;
}

void endScene() {
    for (auto& item : ingredients) unpin(item);
    unpin(kit);
    unpin(job.weapon);
    removeGrippers();
    activeBench = previousGrab = previousReceiver = 0;
    job = {};
    canAccept = 0;
    gripEdges = inputEdges = 0;
}

Location benchPoint(Form* ref) {
    auto root = scene::root(ref);
    auto t = scene::world(root);
    const auto top = t.p + scene::rotate(t, {0, -2, 49}) * t.scale;
    return {ref->id, top};
}

Vec robotPoint(Form* ref, Form* player) {
    auto root = scene::root(ref);
    auto head = scene::find(root, "Bip01 Head");
    auto t = scene::world(head ? head : root);
    auto toPlayer = scene::world(scene::root(player)).p - t.p;
    toPlayer.z = 0;
    const auto length = scene::distance(toPlayer, {});
    return t.p + toPlayer * (length > 1.f ? 25.f / length : 0) + Vec{0,0,-26};
}

void scan(Form* player, unsigned now) {
    const auto cellId = static_cast<Form*>(scene::cell(player))->id;
    if (cellId == cell && now < nextScan) return;
    if (cellId != cell) { endScene(); cell = cellId; }
    nextScan = now + 500;
    robot = 0;
    const auto divide = form(call(findDivideED, nullptr));
    const auto actors = call(findActors,nullptr);
    scannedActors = actors.type == 4 && actors.value.array ? arrays->size(actors.value.array) : 0;
    auto best = 210.f;
    if (actors.type == 4 && actors.value.array) {
        for (unsigned i = 0, end = std::min(arrays->size(actors.value.array),512u); i < end; ++i) {
            Element key, value;
            key.type = 1; key.value.number = i;
            if (!arrays->get(actors.value.array,key,value)) continue;
            const auto actor = form(value);
            const auto body = base(actor);
            if (!body || (body->id != 0x10C769 && body->id != 0x1694E2 && body->id != 0x1694E0 && body != divide) || !valid(actor,player)) continue;
            const auto distance = scene::distance(scene::world(scene::root(player)).p,robotPoint(actor,player));
            if (distance >= best || number(call(unsafe, nullptr, actor))) continue;
            robot = actor->id;
            best = distance;
        }
    }
    benches.fill(0);
    auto a = call(findBenches, nullptr);
    scannedBenches = a.type == 4 && a.value.array ? arrays->size(a.value.array) : 0;
    if (a.type != 4 || !a.value.array) return;
    unsigned used = 0;
    for (unsigned i = 0, size = std::min(arrays->size(a.value.array), 512u); i < size && used < benches.size(); ++i) {
        Element key, value;
        key.type = 1; key.value.number = i;
        if (!arrays->get(a.value.array, key, value)) continue;
        auto ref = form(value);
        if (valid(ref, player) && base(ref) && base(ref)->id == Workbench) benches[used++] = ref->id;
    }
    const auto playerPosition = scene::world(scene::root(player)).p;
    std::sort(benches.begin(), benches.begin()+used, [&](u32 a, u32 b) {
        return scene::distance(benchPoint(id(a)).top,playerPosition) < scene::distance(benchPoint(id(b)).top,playerPosition);
    });
}

Location nearest(Form* player, Vec point, bool weapon, bool& toRobot) {
    Location output{};
    float reach = 20.f;
    toRobot = false;
    for (auto key : benches) {
        if (activeBench && activeBench != key) continue;
        auto ref = id(key);
        if (!valid(ref, player)) continue;
        const auto bench = benchPoint(ref);
        const auto distance = scene::distance(bench.top, point);
        if (distance < reach) { output = bench; reach = distance; }
    }
    if (weapon) if (auto actor = id(robot); valid(actor, player)) {
        const auto target = robotPoint(actor,player);
        if (scene::distance(point, target) < reach) { output = {actor->id, target}; toRobot = true; }
    }
    return output;
}

bool startWeapon(Form* ref, u32 receiver, bool toRobot, unsigned now, unsigned hand, Vec p) {
    if (job.weapon.id || !workshop_native::condition(ref)) return false;
    if (!pin(job.weapon, ref)) return false;
    job.receiver = receiver;
    job.robot = toRobot;
    job.started = now;
    job.initial = p;
    job.hand = hand;
    return true;
}

void insertIngredient(Form* ref, Location receiver) {
    const auto model = base(ref);
    if (!model || workshop_native::count(ref) != 1 || (activeBench && activeBench != receiver.id)) return;
    if (model->id == RepairKit && !kit.id && pin(kit, ref)) {
        activeBench = receiver.id;
        setPosition(ref, receiver.top + Vec{16, 0, 1});
        return;
    }
    for (size_t i = 0; i < recipe.size(); ++i) {
        if (ingredients[i].id || model->id != recipe[i]) continue;
        if (pin(ingredients[i], ref)) {
            activeBench = receiver.id;
            setPosition(ref, receiver.top + Vec{float(int(i % 3) - 1) * 9, float(i / 3) * 8 + 5, 1});
        }
        return;
    }
}

void acceptHeld(Form* player, Form* ref, unsigned now) {
    if (!valid(ref, player) || build_adapter::grabbedItem() == ref) return;
    const auto receiver = id(previousReceiver);
    if (!valid(receiver, player)) return;
    if (auto model = base(ref); model && model->type == 0x28) {
        if (startWeapon(ref, receiver->id, previousWasRobot, now, 1, previousOffer) && !previousWasRobot) {
            activeBench = receiver->id;
            setPosition(ref, benchPoint(receiver).top + Vec{0,-5,2});
        }
    } else if (!previousWasRobot) insertIngredient(ref, benchPoint(receiver));
}

Vec blend(Vec a, Vec b, float t) { return a + (b - a) * t; }
float smooth(float t) { t = std::clamp(t, 0.f, 1.f); return t * t * (3.f - 2.f*t); }

Vec unit(Vec v) {
    const float length = scene::distance(v, {});
    return length > .001f ? v * (1.f / length) : Vec{0,1,0};
}

Transform segment(Vec a, Vec b, Vec up, float scale) {
    const auto forward = unit(b-a);
    auto lateral = unit(scene::cross(forward,up));
    if (scene::distance(scene::cross(forward,up),{}) < .1f)
        lateral = unit(scene::cross(forward,{1,0,0}));
    up = scene::cross(lateral,forward);
    return {{{lateral.x,forward.x,up.x},{lateral.y,forward.y,up.y},{lateral.z,forward.z,up.z}},a,scale};
}

#include "workshop-holster-preview.hpp"
#include "workshop-layout-profiles.hpp"

bool poseGrippers(void* root, Vec gun, float close, float extension, Transform orientation) {
    const auto head = scene::find(root, "Bip01 Head");
    if (!head) return false;
    if (head != gripperHost) removeGrippers();
    if (!grippers) {
        grippers = workshop_native::loadModel("Data\\Meshes\\FilthyCasual\\EDEServiceGrippers.nif");
        if (!grippers) return false;
        gripperHost = head;
        scene::retain(head);
        scene::moveParent(grippers, head);
    }
    const auto body = scene::world(head);
    const auto local = [&](Vec p) { return body.p + scene::rotate(body,p) * body.scale; };
    const auto up = scene::rotate(body,{0,0,1});
    for (unsigned side = 0; side < 2; ++side) {
        const auto name = side ? 'R' : 'L';
        const float sign = side ? 1.f : -1.f;
        auto shoulder = local({sign*10,7,-10});
        auto elbow = local({sign*15,16,-16});
        auto contact = gun + scene::rotate(orientation,{0,sign*5,0});
        auto palm = blend(local({sign*13,11,-15}), contact + Vec{0,0,-2.5f}, extension);
        auto restAt = [&](const char* part) {
            char key[12]; sprintf_s(key, "%c%s", name, part);
            return scene::find(grippers,key);
        };
        if (auto node = restAt("Upper")) scene::setWorld(node,segment(shoulder,elbow,up,scene::distance(shoulder,elbow)/13.f));
        if (auto node = restAt("Lower")) scene::setWorld(node,segment(elbow,palm,up,scene::distance(elbow,palm)/13.f));
        if (auto node = restAt("Palm")) scene::setWorld(node,segment(palm,contact,up,body.scale));
        for (const auto jaw : {"JawA","JawB"}) {
            if (auto node = restAt(jaw)) {
                scene::local(node).p = {(jaw[3]=='A' ? -1.f : 1.f) * (2.3f - close*1.15f),1,0};
                scene::update(node);
            }
        }
    }
    return true;
}

bool repair(Form* ref, float amount) {
    auto value = workshop_native::condition(ref);
    const auto maximum = float(number(call(baseHealth, nullptr, ref)));
    if (!value || !std::isfinite(*value) || !std::isfinite(maximum) || maximum <= 0 || *value >= maximum) return false;
    lastBefore = *value;
    *value = std::min(maximum, *value + maximum * amount * .01f);
    lastAfter = *value;
    lastWeapon = ref->id;
    ++sequence;
    call(repairSound, nullptr);
    return true;
}

void tickWeapon(Form* player, unsigned now, unsigned presses) {
    if (!job.weapon.id) return;
    auto ref = id(job.weapon.id);
    auto receiver = id(job.receiver);
    if (!valid(ref, player) || !valid(receiver, player) || build_adapter::grabbedItem() == ref) {
        if (build_adapter::grabbedItem() != ref) unpin(job.weapon);
        removeGrippers(); job = {}; return;
    }
    auto age = now - job.started;
    if (job.robot) {
        auto hold = robotPoint(receiver,player);
        auto initial = blend(job.initial, hold, smooth(age / 430.f));
        auto toPlayer = scene::world(scene::root(player)).p-hold;
        toPlayer.z = 0;
        auto target = hold + unit(toPlayer)*9.f + Vec{0,0,1};
        const auto p = age < 430 ? initial : blend(hold, target, smooth((age - 1050.f) / 400.f));
        // Move the exact world reference. The robot's hands follow visual contact.
        if (age >= 12000) {
            setPosition(ref, target); unpin(job.weapon); removeGrippers(); job = {}; return;
        }
        setPosition(ref, p);
        auto anim = poseGrippers(scene::root(receiver), p, smooth(age / 350.f),
                                 smooth(age/400.f), scene::world(scene::root(ref)));
        if (!job.repaired && age >= 800) {
            auto amount = float(number(call(eligibility, nullptr)));
            if (anim && amount > 0 && repair(ref, amount)) { call(chargeED, nullptr); notify("ED-E repaired your weapon."); }
            else notify("ED-E has no repair available.");
            job.repaired = true;
        }
    } else if (presses && kit.id && !job.repaired && valid(id(kit.id), player)) {
        if (repair(ref, 25)) {
            call(discard, nullptr, id(kit.id)); kit = {};
            notify("Weapon repaired."); job.repaired = true;
        }
    }
}

void craft(Form* player) {
    if (number(call(skill, nullptr)) < 50) { notify("Weapon Repair Kit requires Repair 50."); return; }
    auto ref = id(activeBench);
    if (!valid(ref, player)) return;
    for (size_t i = 0; i < recipe.size(); ++i) {
        auto item = id(ingredients[i].id);
        auto model = base(item);
        if (!valid(item, player) || !model || model->id != recipe[i] || workshop_native::count(item) != 1 ||
            build_adapter::grabbedItem() == item || scene::distance(scene::world(scene::root(item)).p, benchPoint(ref).top) > 32) return;
    }
    auto product = form(call(spawnKit, nullptr, ref));
    if (!product) return;
    for (auto& item : ingredients) { call(discard, nullptr, id(item.id)); item = {}; }
    setPosition(product, benchPoint(ref).top + Vec{10, -8, 3});
    resultsRef = product->id;
    ++sequence;
    call(repairSound, nullptr);
    notify("Weapon Repair Kit crafted.");
}

void retrieve(Form* player, unsigned edges) {
    if (!edges || build_adapter::grabbedItem()) return;
    auto root = scene::root(player);
    auto head = scene::find(root, "Bip01 Head");
    if (!head) return;
    const auto headPos = scene::world(head).p;
    for (unsigned hand = 0; hand < 2; ++hand) {
        if (!(edges & (1u << hand))) continue;
        const auto handPoint = palm(player,hand);
        Held* nearestItem = nullptr;
        float distance = 12.f;
        const auto consider = [&](Held& slot) {
            auto item = id(slot.id);
            if (!valid(item, player)) return;
            const auto delta = scene::distance(handPoint, scene::world(scene::root(item)).p);
            if (delta < distance) { distance = delta; nearestItem = &slot; }
        };
        if (!job.pending && (!job.robot || GetTickCount()-job.started >= 1450)) consider(job.weapon);
        consider(kit);
        for (auto& item : ingredients) consider(item);
        if (nearestItem) {
            auto item = id(nearestItem->id);
            unpin(*nearestItem);
            if (nearestItem == &job.weapon) { removeGrippers(); job = {}; }
            const float headPoint[]{headPos.x,headPos.y,headPos.z};
            // Give the exact reference back at the grip point before testing
            // the stock grab. This avoids a lagging collision pose at the
            // edge of the robot's call-out animation.
            setPosition(item,handPoint);
            receivedBack=build_adapter::grabSelected(player, headPoint, item, hand);
            return;
        }
    }
}

void report(Form* player, unsigned now, Location target) {
    if (!*diagnosticPath || now < nextReport) return;
    nextReport = now + 140;
    FILE* f = nullptr;
    if (fopen_s(&f, diagnosticPath, "w")) return;
    fprintf(f, "{\"pid\":%lu,\"robot\":%u,\"bench\":%u,\"target\":%u,\"grab\":%u,\"job\":%u,\"lastWeapon\":%u,\"before\":%.3f,\"after\":%.3f,\"sequence\":%u,\"product\":%u,\"parts\":[",
            GetCurrentProcessId(), robot, activeBench ? activeBench : benches[0], target.id, previousGrab, job.weapon.id, lastWeapon, lastBefore, lastAfter, sequence, resultsRef);
    for (size_t i = 0; i < recipe.size(); ++i) fprintf(f, "%s%u", i ? "," : "", ingredients[i].id);
    const auto writePoint = [&](const char* label, Vec p) {
        p = pooladapter::tracking(p);
        fprintf(f, ",\"%s\":[%.4f,%.4f,%.4f]",label,p.x,p.y,p.z);
    };
    fprintf(f, "],\"kit\":%u,\"arms\":%d,\"canAccept\":%u,\"jobRobot\":%d,\"jobAge\":%u", kit.id,
            grippers != nullptr, canAccept.load(), job.robot, job.weapon.id ? now-job.started : 0);
    if (auto r = id(robot); valid(r, player)) writePoint("robotAt",robotPoint(r,player));
    if (auto b = id(activeBench ? activeBench : benches[0]); valid(b, player)) writePoint("benchAt",benchPoint(b).top);
    if (auto w = id(job.weapon.id); valid(w, player)) writePoint("weaponAt",scene::world(scene::root(w)).p);
    writePoint("palmR",palm(player,1));
    writePoint("palmL",palm(player,0));
    const auto hand = palm(player,1);
    fprintf(f, ",\"worldPalmR\":[%.4f,%.4f,%.4f]",hand.x,hand.y,hand.z);
    fprintf(f, ",\"scannedActors\":%u,\"scannedBenches\":%u,\"previewSlot\":%d,\"offers\":%u,\"drops\":%u,\"primarySamples\":%u,\"layoutReady\":%d,\"grips\":%u,\"returnResult\":%u,\"markers\":[",
            scannedActors,scannedBenches,workshop_holsters::previewSlot+1,offers,drops,primarySamples.load(),workshop_native_holsters::module()!=nullptr,grips.load(),receivedBack);
    for (const auto& marker : workshop_holsters::markers) {
        const auto p = marker.live ? pooladapter::tracking(marker.position) : Vec{};
        fprintf(f, "%s[%.4f,%.4f,%.4f]", &marker == &workshop_holsters::markers[0] ? "" : ",",p.x,p.y,p.z);
    }
    fputs("]",f);
    workshop_native_holsters::Layout liveSlots{};
    if(workshop_native_holsters::read(liveSlots)){
        unsigned enabled=0;
        for(unsigned slot=0;slot<8;++slot)if(liveSlots.enabled[slot])enabled|=1u<<slot;
        fprintf(f,",\"nativeSlotsEnabled\":%u,\"nativeMarkersShown\":%u",enabled,unsigned(workshop_native_holsters::markersVisible()));
        // TTR 1.1.7 decides stock marker visibility at 2289E4 and publishes
        // each numbered mesh's shown state at 55F3B5. Read only in QA.
        const auto module=workshop_native_holsters::module();
        unsigned dragging=0,numbers=0,spheres=0;
        for(unsigned handIndex=0;handIndex<2;++handIndex){
            const auto record=module+0x1038e4c+0x10*handIndex;
            int slot;std::memcpy(&slot,record,sizeof slot);
            if(record[8]&&slot>=0)dragging|=1u<<handIndex;
        }
        for(unsigned i=0;i<8;++i){
            if(module[0x1a0f68a+i*0x40])numbers|=1u<<i;
            if(module[0x1a0f892+(i+4)*0x24])spheres|=1u<<i;
        }
        ULONGLONG changed=0;std::memcpy(&changed,module+0x1614b20,sizeof changed);
        fprintf(f,",\"nativeDragHands\":%u,\"nativeSlotEditAgeMs\":%lld,\"nativePublishedNumbers\":%u,\"nativePublishedSpheres\":%u",
            dragging,changed?static_cast<long long>(GetTickCount64()-changed):-1LL,numbers,spheres);
    }
    if (workshop_holsters::miniature) {
        const auto pose = scene::world(workshop_holsters::miniature);
        writePoint("previewCenter",pose.p+scene::rotate(pose,workshop_holsters::modelCenter)*pose.scale);
        fprintf(f, ",\"previewWidth\":%.3f,\"previewGlass\":%d",pose.scale*workshop_holsters::modelExtent,workshop_holsters::orb!=nullptr);
    }
    fputs("}\n",f);
    fclose(f);
}

void loop() {
    if (!loaded || !initialize() || !pool_equipment::initialize()) return;
    auto player = id(0x14);
    if (!player || !scene::root(player) || !scene::cell(player)) return;
    auto now = GetTickCount();
    const auto pressed = inputEdges.exchange(0);
    const auto grabbed = gripEdges.exchange(0);
    canAccept = 0;
    if (number(call(menu, nullptr))) {
        workshop_holsters::pendingDraw={};
        workshop_holsters::clear();
        if (job.weapon.id || activeBench) endScene();
        previousGrab = previousReceiver = 0;
        if (*diagnosticPath) report(player,now,{});
        return;
    }
    if(holstersOnly){
        workshop_holsters::update(player,now,grips.load(),grabbed,!workshop_profiles::preview);
        if (*diagnosticPath) report(player,now,{});
        return;
    }
    scan(player, now);
    if (job.weapon.id || activeBench) retrieve(player, grabbed);
    auto item = static_cast<Form*>(build_adapter::grabbedItem());
    if (!valid(item, player)) item = nullptr;
    if (!item && previousGrab && previousReceiver)
        acceptHeld(player, id(previousGrab), now);
    previousReceiver = 0;
    previousGrab = item ? item->id : 0;
    Location target{};
    if (item) {
        bool toRobot = false;
        const auto p = scene::world(scene::root(item)).p;
        auto model = base(item);
        target = nearest(player, p, model && model->type == 0x28, toRobot);
        previousReceiver = target.id; previousWasRobot = toRobot; previousOffer = p;
    } else if (!job.weapon.id && number(call(drawn, nullptr))) {
        const int hand = build_adapter::drawnWeaponHand();
        if (hand == 0 || hand == 1) {
            const auto p = palm(player,hand);
            bool toRobot = false;
            target = nearest(player, p, true, toRobot);
            if (target.id) {
                auto inventoryItem = form(call(equipped, nullptr));
                const auto condition = inventoryItem ? number(call(equippedCondition,nullptr,inventoryItem)) : 1;
                if (condition >= .999 || (toRobot && number(call(eligibility,nullptr)) <= 0)) target = {};
            }
            if (target.id) {
                canAccept |= 1u << hand;
                if (pressed & (1u << hand)) {
                    ++offers;
                    auto inv = form(call(equipped, nullptr));
                    auto exact = inv ? inventoryRef(inv->id) : nullptr;
                    if (exact && exact->type && exact->type->type == 0x28 && exact->container == player && exact->extra &&
                        !(exact->type->flags & 0x400)) {
                        auto dropped = static_cast<Form*>(build_adapter::dropOneAid(player, exact->type, exact->extra));
                        if (dropped) {
                            ++drops;
                            exact->removed = true;
                            job.pending = true; job.weapon.id = dropped->id; job.receiver = target.id;
                            job.robot = toRobot; job.initial = p; job.started = now; job.hand = hand;
                            setPosition(dropped, p);
                        }
                    }
                }
            }
        }
    }
    if (job.pending) {
        auto ref = id(job.weapon.id);
        if (valid(ref, player)) {
            auto pending = job; job = {};
            if (startWeapon(ref, pending.receiver, pending.robot, now, pending.hand, pending.initial) && !pending.robot) {
                activeBench = pending.receiver;
                if (auto bench = id(activeBench); valid(bench, player)) setPosition(ref, benchPoint(bench).top + Vec{0,-5,2});
            }
        } else if (now - job.started > 1000) job = {};
    } else {
        unsigned benchPress = 0;
        if (auto ref = id(activeBench); valid(ref, player)) {
            auto p = benchPoint(ref).top;
            for (unsigned h = 0; h < 2; ++h) {
                if (scene::distance(palm(player,h), p) > 23.f) continue;
                if (job.weapon.id && !job.robot && !job.repaired && kit.id) { canAccept |= 1u << h; benchPress |= pressed & (1u << h); }
                else if (std::all_of(ingredients.begin(), ingredients.end(), [](Held part){ return part.id != 0; })) {
                    canAccept |= 1u << h;
                    if (pressed & (1u << h)) { craft(player); break; }
                }
            }
        }
        tickWeapon(player, now, benchPress);
    }
    workshop_holsters::update(player,now,grips.load(),grabbed,job.weapon.id || activeBench || !workshop_profiles::preview);
    report(player, now, target);
}

void __cdecl message(Message* m) {
    switch (m->type) {
    case PostLoadGame: case NewGame: loaded = true; nextScan = 0; break;
    case PreLoadGame: case ExitMenu: case ExitGame:
        workshop_holsters::pendingDraw={};
        workshop_holsters::hoverSlot=-1;
        workshop_holsters::clear();
        endScene();
        loaded=false;
        break;
    case MainLoop: loop(); break;
    default: break;
    }
}

uint32_t __cdecl input(const char* action, int hand, int kind, float x, float) {
    if (hand < 0 || hand > 1 || !action) return 0;
    const auto bit = 1u << hand;
    if (!std::strcmp(action, "squeeze")) {
        if ((kind & 8) && x > .60f) {
            if (!(grips.fetch_or(bit) & bit)) gripEdges.fetch_or(bit);
        } else if (!(kind & 8) || x < .35f) grips.fetch_and(~bit);
        return 0;
    }
    if (holstersOnly || (kind & 3) != 0 || std::strcmp(action, "primary_click")) return 0;
    const bool down = (kind & 8) && x > .55f;
    if (down) ++primarySamples;
    if (!down) return aHeld[hand].exchange(false) ? 1 : 0;
    if (!aHeld[hand] && !(canAccept.load() & bit)) return 0;
    if (!aHeld[hand].exchange(true)) inputEdges.fetch_or(bit);
    return 1;
}
} // namespace

extern "C" __declspec(dllexport) const fc_interaction::Provider* __cdecl FC_InteractionProviderV1() {
    static const fc_interaction::Provider provider{sizeof(provider), fc_interaction::version, input, nullptr};
    return &provider;
}

extern "C" __declspec(dllexport) bool __cdecl FC_HolsterLayoutView(unsigned profile,fc_equipment::LayoutView* out,unsigned size){
    if(!out || size!=sizeof(*out) || !initialized || !compatible || !pool_equipment::onOwnerThread() ||
       !workshop_profiles::operate(0,profile))return false;
    using namespace workshop_profiles;
    Profile selected=store.profile[profile];
    // The active profile can have unsaved physical marker and hotkey edits.
    if(profile==store.selected){
        if(!workshop_native_holsters::read(selected.layout))return false;
        captureFavorites(selected);
    }else if(!selected.saved){
        if(!seed(profile,selected.layout))return false;
        captureFavorites(selected);
    }
    *out={};
    out->kind=fc_equipment::Kind::Holsters;
    out->count=8;
    for(unsigned i=0;i<8;++i){
        const auto& point=selected.layout.point[i];
        if(!std::isfinite(point.right)||!std::isfinite(point.up))return false;
        out->slots[i]={point.right,point.up,selected.favorite[i]};
    }
    return true;
}

extern "C" __declspec(dllexport) unsigned __cdecl FC_HolsterLayout(unsigned command, unsigned profile) {
    return initialized && compatible && pool_equipment::onOwnerThread() ? workshop_profiles::operate(command,profile) : 0;
}

extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Query(const NVSE* nvse, PluginInfo* info) {
    *info = {1, "FilthyCasualPhysicalWorkshop", 1};
    return !nvse->isEditor && nvse->runtime == 0x040020D0;
}

extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Load(const NVSE* nvse) {
    scripts = static_cast<Scripts*>(nvse->queryInterface(6));
    arrays = static_cast<Arrays*>(nvse->queryInterface(5));
    console = static_cast<Console*>(nvse->queryInterface(1));
    auto data = static_cast<DataInterface*>(nvse->queryInterface(7));
    auto messages = static_cast<Messaging*>(nvse->queryInterface(2));
    if (!scripts || !arrays || !console || !data || !messages) return false;
    inventoryRef = reinterpret_cast<InventoryRef*(__cdecl*)(u32)>(data->function(2));
    char path[MAX_PATH]{};
    HMODULE self=nullptr;
    char modePath[MAX_PATH]{};
    if(GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<const char*>(&holstersOnly),&self)&&GetModuleFileNameA(self,modePath,MAX_PATH)<MAX_PATH){
        if(auto dot=std::strrchr(modePath,'.');dot){
            strcpy_s(dot,5,".ini");
            holstersOnly=GetPrivateProfileIntA("Workshop","HolstersOnly",0,modePath)!=0;
        }
    }
    auto size = GetEnvironmentVariableA("SEATED_TEST_DIR", path, MAX_PATH);
    if (size && size < MAX_PATH) sprintf_s(diagnosticPath, "%s\\physical-workshop.json", path);
    return inventoryRef && messages->listen(nvse->getPluginHandle(), "NVSE", message);
}
