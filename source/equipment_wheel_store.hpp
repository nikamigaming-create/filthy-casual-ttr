#pragma once

// Main-loop access to the player's saved JIP auxiliary variables. The six
// entries are native form references and participate in co-save remapping.
namespace wheelEquipment {
using Layout = fc::wheels::Slots<Form*>;
constexpr const char* keys[2][4] = {
    {"_FC.AidWheel.Active", "_FC.AidWheel.1", "_FC.AidWheel.2", "_FC.AidWheel.3"},
    {"_FC.WeaponWheel.Active", "_FC.WeaponWheel.1", "_FC.WeaponWheel.2", "_FC.WeaponWheel.3"}
};
struct Functions {
    Script* active[2]{};
    Script* activate[2]{};
    Script* size[2][4]{};
    Script* read[2][4]{};
    Script* save{};
};
const Functions& functions() {
    static const Functions native = [] {
        Functions f;
        for (int h=0;h<2;++h) {
            std::string key = std::string("\"") + keys[h][0] + "\"";
            f.active[h] = compile("read wheel selection",("player.AuxVarGetFlt " + key).c_str(),true);
            f.activate[h] = compile("choose wheel profile",("int n\nbegin function {n}\nSetFunctionValue (player.AuxVarSetFlt " + key + " n)\nend").c_str());
            for (int p=1;p<=fc::wheels::profileCount;++p) {
                key = std::string("\"") + keys[h][p] + "\"";
                f.size[h][p] = compile("check wheel profile",("player.AuxVarSize " + key).c_str(),true);
                f.read[h][p] = compile("read wheel slots",("player.AuxVarGetAsArr " + key).c_str(),true);
            }
        }
        f.save=fc::nvse::loadFunction(*scripts,"Player/WheelProfileSave");
        return f;
    }();
    return native;
}
bool available(int hand) {
    return ready && loaded && scripts && arrays && fc::wheels::validHand(hand)
        && feature(hand == 1 ? fc_features::Weapons : fc_features::Consumables);
}
int profileMask(int hand) {
    if (!available(hand)) return 0;
    int mask = 1;
    for (int p = 1; p <= fc::wheels::profileCount; ++p)
        if (number(functions().size[hand][p]) == fc::wheels::slotCount) mask |= 1 << p;
    return mask;
}
int activeProfile(int hand) {
    if (!available(hand)) return -1;
    const int profile = int(number(functions().active[hand]));
    return fc::wheels::validProfile(profile) ? profile : 0;
}
bool useProfile(int hand, int profile) {
    if (!available(hand) || !fc::wheels::validProfile(profile)) return false;
    if (profile && number(functions().size[hand][profile]) != fc::wheels::slotCount) return false;
    if (activeProfile(hand) == profile) return true;
    return number(functions().activate[hand],profile) == 1 && activeProfile(hand) == profile;
}
bool readProfile(int hand, int profile, Layout& result) {
    result = {};
    if (!available(hand) || !fc::wheels::validProfile(profile) || !profile) return false;
    Element array;
    Element values[fc::wheels::slotCount]{};
    const bool ok = call(functions().read[hand][profile],array) && array.type == 4
        && array.value.array && arrays->size(array.value.array) == result.size()
        && arrays->elements(array.value.array,values,nullptr);
    for (size_t i = 0; i < result.size(); ++i) {
        if (ok && values[i].type == 2) result[i] = values[i].value.form;
        release(values[i]);
    }
    release(array);
    return ok;
}
bool writeProfile(int hand, int profile, const Layout& layout) {
    if (!available(hand) || !fc::wheels::validProfile(profile) || !profile) return false;
    const int index = hand * fc::wheels::profileCount + profile;
    if (number(functions().save,index,layout[0],layout[1],layout[2],
            layout[3],layout[4],layout[5]) != layout.size()) return false;
    Layout stored{};
    return readProfile(hand,profile,stored) && stored == layout;
}
bool readWheel(int hand, std::vector<Item>& inventory) {
    const int active = activeProfile(hand);
    if (active <= 0) return readWheelInventory(hand,inventory,true);
    Layout layout{};
    if (!readProfile(hand,active,layout)) return false;
    inventory.resize(layout.size());
    for (size_t i=0;i<layout.size();++i) inventory[i] = wheelItem(hand,layout[i],true);
    return true;
}
}
