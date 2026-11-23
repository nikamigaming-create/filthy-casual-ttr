#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../interaction_registry.hpp"
#include "../eyebot/lift_control.hpp"
#include "../eyebot/wrist_frame.hpp"
#include "../settings_pages.hpp"
#include <cassert>
#include <limits>

namespace {
unsigned calls, rays;
float observed;
uint32_t __cdecl take(const char*, int, int, float amount, float) {
    ++calls;
    observed = amount;
    return 2;
}
uint32_t __cdecl observe(const char*, int, int, float amount, float) {
    ++calls;
    assert(amount == 0);
    return 1;
}
void __cdecl aim(int, const fc_interaction::Ray*) { ++rays; }

void checkProviders() {
    interaction_registry::Providers providers;
    fc_interaction::Provider first{sizeof(first), fc_interaction::version, take, nullptr};
    auto invalid = first;
    invalid.abi = 0;
    providers.add(nullptr);
    providers.add(&invalid);
    providers.add(&first);
    providers.add(&first);
    fc_interaction::Provider remaining[33]{};
    for (auto& entry : remaining) {
        entry = {sizeof(entry), fc_interaction::version, observe, aim};
        providers.add(&entry);
    }
    assert(providers.input("trigger", 1, 9, .75f, 0) == 2);
    assert(calls == 32 && observed == .75f);
    providers.aim(0, {});
    assert(rays == 31);
}

void checkLift() {
    eyebot::flight::LiftControl lift;
    assert(!lift.update(0, 1, false, 100));
    assert(lift.update(1, 1, true, 100));
    assert(lift.axis(100) == -1 && lift.axis(280) == 0);
    assert(lift.update(0, 1, true, 101) && lift.axis(101) == 0);
    assert(lift.update(1, .8f, false, 102));
    assert(lift.update(1, 0, false, 103));
    assert(!lift.update(1, 0, false, 104));
    assert(lift.axis(104) == 1);
    lift.update(0, std::numeric_limits<float>::quiet_NaN(), true, 105);
    assert(lift.axis(105) == 0);
}

void checkPages() {
    using namespace eyebot::wrist_frame;
    assert(nextPage(General, bit(General)) == NoPage);
    for (unsigned mask=0; mask<128; ++mask) {
        for (auto page : {General,Eyebot,Voice,Pool,Workshop,Equipment}) {
            if (!(mask & bit(page))) continue;
            const auto next=nextPage(page,static_cast<uint8_t>(mask));
            const auto previous=nextPage(page,static_cast<uint8_t>(mask),-1);
            if (next==NoPage) assert(previous==NoPage);
            else {
                assert(nextPage(next,static_cast<uint8_t>(mask),-1)==page);
                assert(nextPage(previous,static_cast<uint8_t>(mask))==page);
            }
        }
    }
    assert(nextPage(Workshop, bit(Workshop) | bit(Equipment)) == Equipment);
    assert(nextPage(Equipment, bit(General) | bit(Equipment)) == General);
    assert(nextPage(Equipment, bit(Equipment)) == NoPage);
    for (unsigned i = 0; i < 6; ++i) {
        const float center = (80.f + i * 160.f) / 960.f;
        assert(fullMenuPointer(center, 642.f / 768.f) == GeneralTab + i);
        assert(fullMenuPointer(center, .65f) == PointerMiss);
    }
    const auto twoPages = bit(General) | bit(Equipment);
    assert(fullMenuPointer(.25f, .835f, twoPages) == GeneralTab);
    assert(fullMenuPointer(.75f, .835f, twoPages) == EquipmentTab);
    assert(fullMenuPointer(.50f, .835f, twoPages) == PointerMiss);
    assert(fullMenuPointer(.75f, .22f, twoPages, true) == PointerMiss);
    assert(fullMenuPointer(.75f, .22f, twoPages, false) == Row0);
    assert(fullMenuPointer(.88f, .90f) == CategoryTab);
    assert(fullMenuPointer(800.f / 960.f, 642.f / 768.f) == PointerMiss);
    assert(fullMenuPointer(-1.f, 642.f / 768.f) == PointerMiss);
    assert(nextPage(Pool, bit(Pool) | bit(Workshop)) == Workshop);
    assert(nextPage(Workshop, bit(General) | bit(Workshop)) == General);
    assert(nextPage(General, bit(General) | bit(Voice)) == Voice);
    assert(nextPage(Voice, bit(General) | bit(Voice)) == General);
    assert(nextPage(Eyebot, bit(General) | bit(Eyebot)) == General);
    fc_settings::View view;
    fc_settings::pages::wristHome(view, "CAR", true, true, false, false);
    assert(view.count == 8 && !std::strcmp(view.rows[0].value, "MANUAL"));
    assert(!std::strcmp(view.rows[1].value, "CAR") && view.rows[7].kind == fc_settings::Unavailable);
    const auto mask = fc_features::bit(fc_features::Sprint) | fc_features::bit(fc_features::Book);
    const auto rows = fc_settings::pages::wristFeatureRows(mask, 0);
    assert(rows.pages == 1 && rows.count == 2);
    assert(rows.ids[0] == fc_features::Sprint && rows.ids[1] == fc_features::Book);
}
}
int main() {
    checkProviders();
    checkLift();
    checkPages();
}
