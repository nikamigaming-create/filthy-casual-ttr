#pragma once
#include <algorithm>
#include <cmath>
#include "../scene_adapter.hpp"
#include "../nvse_api.hpp"

namespace eyebot::flight {
// The caller supplies the game's hull query. Position and aim advance together.
template <class Cast>
bool advance(float (&axis)[4], float lift, float heading, float seconds, scene::Vec origin,
             scene::Vec &p, float &flightYaw, float &flightPitch, Cast cast, Element &hit) {
    for (auto &value : axis)
        value = std::abs(value) <= .16f ? 0.f : std::copysign((std::abs(value) - .16f) / .84f, value);
    if (!axis[0] && !axis[1] && !axis[2] && !axis[3] && !lift)
        return false;
    scene::Vec direction{std::cos(heading) * axis[0] + std::sin(heading) * axis[1],
                         std::cos(heading) * axis[1] - std::sin(heading) * axis[0], lift};
    const auto distance = scene::distance(direction, {});
    if (distance > .001f) {
        const auto toward = direction * (1.f / distance);
        const auto proposed = p + toward * (std::min(distance, 1.f) * seconds * 120.f);
        constexpr float Hull = 30.f;
        const auto from = p + toward * Hull + scene::Vec{0, 0, 22};
        const auto to = proposed + toward * (Hull + 6.f) + scene::Vec{0, 0, 22};
        hit = cast(from, to);
        const auto length = scene::distance(from, to);
        // Interior ray queries sometimes report contact at the exact start.
        // Check back along that span to distinguish a real crossing.
        if (hit.type == 1 && hit.value.number > 0 && hit.value.number <= .05)
            hit = cast(to, from);
        const bool clear = hit.type == 1 && (hit.value.number <= .05 || hit.value.number >= length - .2f);
        if (clear && scene::distance(proposed, origin) < 700.f)
            p = proposed;
    }
    flightYaw = std::remainder(flightYaw + axis[2] * 120.f * seconds, 360.f);
    flightPitch = std::clamp(flightPitch - axis[3] * 80.f * seconds, -50.f, 50.f);
    return true;
}
} // namespace eyebot::flight
