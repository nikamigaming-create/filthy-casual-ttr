#pragma once
#include <array>
#include <cstddef>

namespace fc::wheels {
constexpr int slotCount = 6;
constexpr int profileCount = 3;
constexpr bool validHand(int hand) { return hand == 0 || hand == 1; }
constexpr bool validProfile(int profile) { return profile >= 0 && profile <= profileCount; }
inline constexpr const char* names[2][4] = {
    {"Automatic", "Field kit", "Medicine", "Food and drink"},
    {"Automatic", "Sidearms", "Long guns", "Heavy weapons"}
};
constexpr const char* profileName(int hand, int profile) {
    return validHand(hand) && validProfile(profile) ? names[hand][profile] : "";
}
template<class FormRef> using Slots = std::array<FormRef, slotCount>;
template<class FormRef> void assign(Slots<FormRef>& slots, int index, FormRef item) {
    if (index < 0 || index >= slotCount) return;
    if (item) for (auto& slot : slots) if (slot == item) slot = {};
    slots[index] = item;
}
}
